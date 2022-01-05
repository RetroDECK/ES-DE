//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Font.h
//
//  Loading, unloading, caching and rendering of fonts.
//  Also functions for word wrapping and similar.
//

#include "resources/Font.h"

#include "Log.h"
#include "renderers/Renderer.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"

FT_Library Font::sLibrary = nullptr;

std::map<std::pair<std::string, int>, std::weak_ptr<Font>> Font::sFontMap;

Font::FontFace::FontFace(ResourceData&& d, int size)
    : data(d)
{
    int err =
        FT_New_Memory_Face(sLibrary, data.ptr.get(), static_cast<FT_Long>(data.length), 0, &face);
    assert(!err);

    if (!err)
        FT_Set_Pixel_Sizes(face, 0, size);
}

Font::FontFace::~FontFace()
{
    if (face)
        FT_Done_Face(face);
}

void Font::initLibrary()
{
    assert(sLibrary == nullptr);

    if (FT_Init_FreeType(&sLibrary)) {
        sLibrary = nullptr;
        LOG(LogError) << "Couldn't initialize FreeType";
    }
}

size_t Font::getMemUsage() const
{
    size_t memUsage = 0;
    for (auto it = mTextures.cbegin(); it != mTextures.cend(); ++it)
        memUsage += it->textureSize.x * it->textureSize.y * 4;

    for (auto it = mFaceCache.cbegin(); it != mFaceCache.cend(); ++it)
        memUsage += it->second->data.length;

    return memUsage;
}

size_t Font::getTotalMemUsage()
{
    size_t total = 0;

    auto it = sFontMap.cbegin();
    while (it != sFontMap.cend()) {
        if (it->second.expired()) {
            it = sFontMap.erase(it);
            continue;
        }

        total += it->second.lock()->getMemUsage();
        ++it;
    }

    return total;
}

Font::Font(int size, const std::string& path)
    : mSize(size)
    , mPath(path)
{
    if (mSize < 9) {
        mSize = 9;
        LOG(LogWarning) << "Requested font size too small, changing to minimum supported size";
    }
    else if (mSize > Renderer::getScreenHeight()) {
        mSize = Renderer::getScreenHeight();
        LOG(LogWarning) << "Requested font size too large, changing to maximum supported size";
    }

    mMaxGlyphHeight = 0;

    if (!sLibrary)
        initLibrary();

    // Always initialize ASCII characters.
    for (unsigned int i = 32; i < 128; ++i)
        getGlyph(i);

    clearFaceCache();
}

Font::~Font()
{
    unload(ResourceManager::getInstance());

    auto fontEntry = sFontMap.find(std::pair<std::string, int>(mPath, mSize));

    if (fontEntry != sFontMap.cend())
        sFontMap.erase(fontEntry);

    if (sFontMap.empty() && sLibrary) {
        FT_Done_FreeType(sLibrary);
        sLibrary = nullptr;
    }
}

std::shared_ptr<Font> Font::get(int size, const std::string& path)
{
    const std::string canonicalPath = Utils::FileSystem::getCanonicalPath(path);
    std::pair<std::string, int> def(canonicalPath.empty() ? getDefaultPath() : canonicalPath, size);

    auto foundFont = sFontMap.find(def);
    if (foundFont != sFontMap.cend()) {
        if (!foundFont->second.expired())
            return foundFont->second.lock();
    }

    std::shared_ptr<Font> font = std::shared_ptr<Font>(new Font(def.second, def.first));
    sFontMap[def] = std::weak_ptr<Font>(font);
    ResourceManager::getInstance().addReloadable(font);
    return font;
}

void Font::unloadTextures()
{
    for (auto it = mTextures.begin(); it != mTextures.end(); ++it)
        it->deinitTexture();
}

Font::FontTexture::FontTexture(const int mSize)
{
    textureId = 0;

    // This is a hack to add some extra texture size when running at very low resolutions. If not
    // doing this, the use of fallback fonts (such as Japanese characters) could result in the
    // texture not fitting the glyphs which would crash the application.
    int extraTextureSize{0};
    const float screenSizeModifier =
        std::min(Renderer::getScreenWidthModifier(), Renderer::getScreenHeightModifier());

    if (screenSizeModifier < 0.2f)
        extraTextureSize += 6;
    if (screenSizeModifier < 0.45f)
        extraTextureSize += 4;

    // It's not entirely clear if the 18 and 6 constants are correct, but they seem to provide
    // a texture buffer large enough to hold the fonts (otherwise the application would crash).
    // This logic is obviously a hack though and needs to be properly reviewed and improved.
    textureSize = glm::ivec2{mSize * (18 + extraTextureSize), mSize * (6 + extraTextureSize / 2)};
    writePos = glm::ivec2{};
    rowHeight = 0;
}

Font::FontTexture::~FontTexture()
{
    // Deinit the texture when destroyed.
    deinitTexture();
}

bool Font::FontTexture::findEmpty(const glm::ivec2& size, glm::ivec2& cursor_out)
{
    if (size.x >= textureSize.x || size.y >= textureSize.y)
        return false;

    if (writePos.x + size.x >= textureSize.x &&
        writePos.y + rowHeight + size.y + 1 < textureSize.y) {
        // Row full, but it should fit on the next row so move the cursor there.
        // Leave 1px of space between glyphs.
        writePos = glm::ivec2{0, writePos.y + rowHeight + 1};
        rowHeight = 0;
    }

    if (writePos.x + size.x >= textureSize.x || writePos.y + size.y >= textureSize.y) {
        // Nope, still won't fit.
        return false;
    }

    cursor_out = writePos;
    // Leave 1px of space between glyphs.
    writePos.x += size.x + 1;

    if (size.y > rowHeight)
        rowHeight = size.y;

    return true;
}

void Font::FontTexture::initTexture()
{
    assert(textureId == 0);
    textureId = Renderer::createTexture(Renderer::Texture::ALPHA, false, false, false,
                                        textureSize.x, textureSize.y, nullptr);
}

void Font::FontTexture::deinitTexture()
{
    if (textureId != 0) {
        Renderer::destroyTexture(textureId);
        textureId = 0;
    }
}

void Font::getTextureForNewGlyph(const glm::ivec2& glyphSize,
                                 FontTexture*& tex_out,
                                 glm::ivec2& cursor_out)
{
    if (mTextures.size()) {
        // Check if the most recent texture has space.
        tex_out = &mTextures.back();

        // Will this one work?
        if (tex_out->findEmpty(glyphSize, cursor_out))
            return; // Yes.
    }

    // Current textures are full, make a new one.
    mTextures.push_back(FontTexture(mSize));
    tex_out = &mTextures.back();
    tex_out->initTexture();

    bool ok = tex_out->findEmpty(glyphSize, cursor_out);
    if (!ok) {
        LOG(LogError) << "Glyph too big to fit on a new texture (glyph size > "
                      << tex_out->textureSize.x << ", " << tex_out->textureSize.y << ")";
        tex_out = nullptr;
    }
}

std::vector<std::string> getFallbackFontPaths()
{
    std::vector<std::string> fontPaths;

    // Standard fonts, let's include them here for exception handling purposes even though that's
    // not really the correct location. (The application will crash if they are missing.)
    ResourceManager::getInstance().getResourcePath(":/fonts/Akrobat-Regular.ttf");
    ResourceManager::getInstance().getResourcePath(":/fonts/Akrobat-SemiBold.ttf");
    ResourceManager::getInstance().getResourcePath(":/fonts/Akrobat-Bold.ttf");

    // Vera sans Unicode.
    fontPaths.push_back(ResourceManager::getInstance().getResourcePath(":/fonts/DejaVuSans.ttf"));
    // GNU FreeFont monospaced.
    fontPaths.push_back(ResourceManager::getInstance().getResourcePath(":/fonts/FreeMono.ttf"));
    // Various languages, such as Japanese and Chinese.
    fontPaths.push_back(
        ResourceManager::getInstance().getResourcePath(":/fonts/DroidSansFallbackFull.ttf"));
    // Korean.
    fontPaths.push_back(
        ResourceManager::getInstance().getResourcePath(":/fonts/NanumMyeongjo.ttf"));
    // Font Awesome icon glyphs, used for various special symbols like stars, folders etc.
    fontPaths.push_back(
        ResourceManager::getInstance().getResourcePath(":/fonts/fontawesome-webfont.ttf"));
    // This is only needed for some really rare special characters.
    fontPaths.push_back(ResourceManager::getInstance().getResourcePath(":/fonts/Ubuntu-C.ttf"));

    fontPaths.shrink_to_fit();
    return fontPaths;
}

FT_Face Font::getFaceForChar(unsigned int id)
{
    static const std::vector<std::string> fallbackFonts = getFallbackFontPaths();

    // Look through our current font + fallback fonts to see if any have the
    // glyph we're looking for.
    for (unsigned int i = 0; i < fallbackFonts.size() + 1; ++i) {
        auto fit = mFaceCache.find(i);

        // Doesn't exist yet.
        if (fit == mFaceCache.cend()) {
            // i == 0 -> mPath
            // Otherwise, take from fallbackFonts.
            const std::string& path = (i == 0 ? mPath : fallbackFonts.at(i - 1));
            ResourceData data = ResourceManager::getInstance().getFileData(path);
            mFaceCache[i] = std::unique_ptr<FontFace>(new FontFace(std::move(data), mSize));
            fit = mFaceCache.find(i);
        }

        if (FT_Get_Char_Index(fit->second->face, id) != 0)
            return fit->second->face;
    }

    // Nothing has a valid glyph - return the "real" face so we get a "missing" character.
    return mFaceCache.cbegin()->second->face;
}

Font::Glyph* Font::getGlyph(unsigned int id)
{
    // Is it already loaded?
    auto it = mGlyphMap.find(id);
    if (it != mGlyphMap.cend())
        return &it->second;

    // Nope, need to make a glyph.
    FT_Face face = getFaceForChar(id);
    if (!face) {
        LOG(LogError) << "Couldn't find appropriate font face for character " << id << " for font "
                      << mPath;
        return nullptr;
    }

    FT_GlyphSlot g = face->glyph;

    if (FT_Load_Char(face, id, FT_LOAD_RENDER)) {
        LOG(LogError) << "Couldn't find glyph for character " << id << " for font " << mPath
                      << ", size " << mSize;
        return nullptr;
    }

    glm::ivec2 glyphSize{g->bitmap.width, g->bitmap.rows};

    FontTexture* tex = nullptr;
    glm::ivec2 cursor;
    getTextureForNewGlyph(glyphSize, tex, cursor);

    // getTextureForNewGlyph can fail if the glyph is bigger than the max texture
    // size (absurdly large font size).
    if (tex == nullptr) {
        LOG(LogError) << "Couldn't create glyph for character " << id << " for font " << mPath
                      << ", size " << mSize << " (no suitable texture found)";
        return nullptr;
    }

    // Create glyph.
    Glyph& glyph = mGlyphMap[id];

    glyph.texture = tex;
    glyph.texPos = glm::vec2{cursor.x / static_cast<float>(tex->textureSize.x),
                             cursor.y / static_cast<float>(tex->textureSize.y)};
    glyph.texSize = glm::vec2{glyphSize.x / static_cast<float>(tex->textureSize.x),
                              glyphSize.y / static_cast<float>(tex->textureSize.y)};

    glyph.advance = glm::vec2{static_cast<float>(g->metrics.horiAdvance) / 64.0f,
                              static_cast<float>(g->metrics.vertAdvance) / 64.0f};
    glyph.bearing = glm::vec2{static_cast<float>(g->metrics.horiBearingX) / 64.0f,
                              static_cast<float>(g->metrics.horiBearingY) / 64.0f};

    // Upload glyph bitmap to texture.
    Renderer::updateTexture(tex->textureId, Renderer::Texture::ALPHA, cursor.x, cursor.y,
                            glyphSize.x, glyphSize.y, g->bitmap.buffer);

    // Update max glyph height.
    if (glyphSize.y > mMaxGlyphHeight)
        mMaxGlyphHeight = glyphSize.y;

    // Done.
    return &glyph;
}

void Font::rebuildTextures()
{
    // Recreate OpenGL textures.
    for (auto it = mTextures.begin(); it != mTextures.end(); ++it)
        it->initTexture();

    // Re-upload the texture data.
    for (auto it = mGlyphMap.cbegin(); it != mGlyphMap.cend(); ++it) {
        FT_Face face = getFaceForChar(it->first);
        FT_GlyphSlot glyphSlot = face->glyph;

        // Load the glyph bitmap through FT.
        FT_Load_Char(face, it->first, FT_LOAD_RENDER);

        FontTexture* tex = it->second.texture;

        // Find the position/size.
        glm::ivec2 cursor{static_cast<int>(it->second.texPos.x * tex->textureSize.x),
                          static_cast<int>(it->second.texPos.y * tex->textureSize.y)};
        glm::ivec2 glyphSize{static_cast<int>(it->second.texSize.x * tex->textureSize.x),
                             static_cast<int>(it->second.texSize.y * tex->textureSize.y)};

        // Upload to texture.
        Renderer::updateTexture(tex->textureId, Renderer::Texture::ALPHA, cursor.x, cursor.y,
                                glyphSize.x, glyphSize.y, glyphSlot->bitmap.buffer);
    }
}

void Font::renderTextCache(TextCache* cache)
{
    if (cache == nullptr) {
        LOG(LogError) << "Attempted to draw nullptr TextCache";
        return;
    }

    for (auto it = cache->vertexLists.cbegin(); it != cache->vertexLists.cend(); ++it) {
        assert(*it->textureIdPtr != 0);

        auto vertexList = *it;

        Renderer::bindTexture(*it->textureIdPtr);
        Renderer::drawTriangleStrips(&it->verts[0],
                                     static_cast<const unsigned int>(it->verts.size()));
    }
}

glm::vec2 Font::sizeText(std::string text, float lineSpacing)
{
    float lineWidth = 0.0f;
    float highestWidth = 0.0f;

    const float lineHeight = getHeight(lineSpacing);

    float y = lineHeight;

    size_t i = 0;
    while (i < text.length()) {
        unsigned int character = Utils::String::chars2Unicode(text, i); // Advances i.

        if (character == '\n') {
            if (lineWidth > highestWidth)
                highestWidth = lineWidth;

            lineWidth = 0.0f;
            y += lineHeight;
        }

        Glyph* glyph = getGlyph(character);
        if (glyph)
            lineWidth += glyph->advance.x;
    }

    if (lineWidth > highestWidth)
        highestWidth = lineWidth;

    return glm::vec2{highestWidth, y};
}

std::string Font::getTextMaxWidth(std::string text, float maxWidth)
{
    float width = sizeText(text).x;
    while (width > maxWidth) {
        text.pop_back();
        width = sizeText(text).x;
    }
    return text;
}

float Font::getHeight(float lineSpacing) const
{
    // Return overall height including line spacing.
    return mMaxGlyphHeight * lineSpacing;
}

float Font::getLetterHeight()
{
    Glyph* glyph = getGlyph('S');
    assert(glyph);
    return glyph->texSize.y * glyph->texture->textureSize.y;
}

std::string Font::wrapText(std::string text, float xLen)
{
    std::string out;
    std::string line;
    std::string word;
    std::string abbreviatedWord;
    std::string temp;

    size_t space;
    glm::vec2 textSize;
    float dotsSize = sizeText("...").x;

    // While there's text or we still have text to render.
    while (text.length() > 0) {
        space = text.find_first_of(" \t\n");
        if (space == std::string::npos)
            space = text.length() - 1;

        word = text.substr(0, space + 1);
        text.erase(0, space + 1);

        temp = line + word;

        textSize = sizeText(temp);

        // If the word will fit on the line, add it to our line, and continue.
        if (textSize.x <= xLen) {
            line = temp;
            continue;
        }
        else {
            // The next word won't fit, so break here.

            // If the word is too long to fit within xLen, then abbreviate it.
            if (xLen > 0 && sizeText(word).x > xLen) {
                float length = xLen - dotsSize;
                if (length < 0)
                    length = 0;
                abbreviatedWord = getTextMaxWidth(word, length);
                abbreviatedWord += "...";
                word = abbreviatedWord;
                out += line;
            }
            else {
                out += line + '\n';
            }
            line = word;
        }
    }

    // Whatever's left should fit.
    out += line;

    return out;
}

glm::vec2 Font::sizeWrappedText(std::string text, float xLen, float lineSpacing)
{
    text = wrapText(text, xLen);
    return sizeText(text, lineSpacing);
}

glm::vec2 Font::getWrappedTextCursorOffset(std::string text,
                                           float xLen,
                                           size_t stop,
                                           float lineSpacing)
{
    std::string wrappedText = wrapText(text, xLen);

    float lineWidth = 0.0f;
    float y = 0.0f;

    size_t wrapCursor = 0;
    size_t cursor = 0;
    while (cursor < stop) {
        unsigned int wrappedCharacter = Utils::String::chars2Unicode(wrappedText, wrapCursor);
        unsigned int character = Utils::String::chars2Unicode(text, cursor);

        if (wrappedCharacter == '\n' && character != '\n') {
            // This is where the wordwrap inserted a newline
            // Reset lineWidth and increment y, but don't consume .a cursor character.
            lineWidth = 0.0f;
            y += getHeight(lineSpacing);

            cursor = Utils::String::prevCursor(text, cursor); // Unconsume.
            continue;
        }

        if (character == '\n') {
            lineWidth = 0.0f;
            y += getHeight(lineSpacing);
            continue;
        }

        Glyph* glyph = getGlyph(character);
        if (glyph)
            lineWidth += glyph->advance.x;
    }

    return glm::vec2{lineWidth, y};
}

//
// TextCache.
//

float Font::getNewlineStartOffset(const std::string& text,
                                  const unsigned int& charStart,
                                  const float& xLen,
                                  const Alignment& alignment)
{
    switch (alignment) {
        case ALIGN_LEFT: {
            return 0;
        }
        case ALIGN_CENTER: {
            int endChar = 0;
            endChar = static_cast<int>(text.find('\n', charStart));
            return (xLen - sizeText(text.substr(charStart,
                                                static_cast<size_t>(endChar) != std::string::npos ?
                                                    endChar - charStart :
                                                    endChar))
                               .x) /
                   2.0f;
        }
        case ALIGN_RIGHT: {
            int endChar = static_cast<int>(text.find('\n', charStart));
            return xLen - (sizeText(text.substr(charStart,
                                                static_cast<size_t>(endChar) != std::string::npos ?
                                                    endChar - charStart :
                                                    endChar))
                               .x);
        }
        default:
            return 0;
    }
}

TextCache* Font::buildTextCache(const std::string& text,
                                glm::vec2 offset,
                                unsigned int color,
                                float xLen,
                                Alignment alignment,
                                float lineSpacing,
                                bool noTopMargin)
{
    float x = offset[0] + (xLen != 0 ? getNewlineStartOffset(text, 0, xLen, alignment) : 0);
    float yTop = 0;
    float yBot = 0;

    if (noTopMargin) {
        yTop = 0;
        yBot = getHeight(1.5);
    }
    else {
        yTop = getGlyph('S')->bearing.y;
        yBot = getHeight(lineSpacing);
    }

    float y = offset[1] + (yBot + yTop) / 2.0f;

    // Vertices by texture.
    std::map<FontTexture*, std::vector<Renderer::Vertex>> vertMap;

    size_t cursor = 0;
    while (cursor < text.length()) {
        // Also advances cursor.
        unsigned int character = Utils::String::chars2Unicode(text, cursor);
        Glyph* glyph;

        // Invalid character.
        if (character == 0)
            continue;

        if (character == '\n') {
            y += getHeight(lineSpacing);
            x = offset[0] + (xLen != 0 ?
                                 getNewlineStartOffset(text,
                                                       static_cast<const unsigned int>(
                                                           cursor) /* cursor is already advanced */,
                                                       xLen, alignment) :
                                 0);
            continue;
        }

        glyph = getGlyph(character);
        if (glyph == nullptr)
            continue;

        std::vector<Renderer::Vertex>& verts = vertMap[glyph->texture];
        size_t oldVertSize = verts.size();
        verts.resize(oldVertSize + 6);
        Renderer::Vertex* vertices = verts.data() + oldVertSize;

        const float glyphStartX{x + glyph->bearing.x};
        const glm::ivec2& textureSize{glyph->texture->textureSize};
        const unsigned int convertedColor = Renderer::convertRGBAToABGR(color);

        vertices[1] = {{glyphStartX, y - glyph->bearing.y},
                       {glyph->texPos.x, glyph->texPos.y},
                       convertedColor};
        vertices[2] = {{glyphStartX, y - glyph->bearing.y + (glyph->texSize.y * textureSize.y)},
                       {glyph->texPos.x, glyph->texPos.y + glyph->texSize.y},
                       convertedColor};
        vertices[3] = {{glyphStartX + glyph->texSize.x * textureSize.x, y - glyph->bearing.y},
                       {glyph->texPos.x + glyph->texSize.x, glyph->texPos.y},
                       convertedColor};
        vertices[4] = {{glyphStartX + glyph->texSize.x * textureSize.x,
                        y - glyph->bearing.y + (glyph->texSize.y * textureSize.y)},
                       {glyph->texPos.x + glyph->texSize.x, glyph->texPos.y + glyph->texSize.y},
                       convertedColor};

        // Round vertices.
        for (int i = 1; i < 5; ++i)
            vertices[i].pos = glm::round(vertices[i].pos);

        // Make duplicates of first and last vertex so this can be rendered as a triangle strip.
        vertices[0] = vertices[1];
        vertices[5] = vertices[4];

        // Advance.
        x += glyph->advance.x;
    }

    TextCache* cache = new TextCache();
    cache->vertexLists.resize(vertMap.size());
    cache->metrics = {sizeText(text, lineSpacing)};

    unsigned int i = 0;
    for (auto it = vertMap.cbegin(); it != vertMap.cend(); ++it) {
        TextCache::VertexList& vertList = cache->vertexLists.at(i);

        vertList.textureIdPtr = &it->first->textureId;
        vertList.verts = it->second;
    }

    clearFaceCache();
    return cache;
}

TextCache* Font::buildTextCache(const std::string& text,
                                float offsetX,
                                float offsetY,
                                unsigned int color,
                                float lineSpacing,
                                bool noTopMargin)
{
    return buildTextCache(text, glm::vec2{offsetX, offsetY}, color, 0.0f, ALIGN_LEFT, lineSpacing,
                          noTopMargin);
}

void TextCache::setColor(unsigned int color)
{
    const unsigned int convertedColor = Renderer::convertRGBAToABGR(color);

    for (auto it = vertexLists.begin(); it != vertexLists.end(); ++it)
        for (auto it2 = it->verts.begin(); it2 != it->verts.end(); ++it2)
            it2->col = convertedColor;
}

std::shared_ptr<Font> Font::getFromTheme(const ThemeData::ThemeElement* elem,
                                         unsigned int properties,
                                         const std::shared_ptr<Font>& orig)
{
    using namespace ThemeFlags;
    if (!(properties & FONT_PATH) && !(properties & FONT_SIZE))
        return orig;

    std::shared_ptr<Font> font;
    int size = (orig ? orig->mSize : FONT_SIZE_MEDIUM);
    std::string path = (orig ? orig->mPath : getDefaultPath());

    float sh = static_cast<float>(Renderer::getScreenHeight());
    if (properties & FONT_SIZE && elem->has("fontSize"))
        size = static_cast<int>(sh * elem->get<float>("fontSize"));
    if (properties & FONT_PATH && elem->has("fontPath"))
        path = elem->get<std::string>("fontPath");

    if (!((path[0] == ':') && (path[1] == '/')) && !Utils::FileSystem::exists(path)) {
        LOG(LogError) << "Font file \"" << path
                      << "\" defined by the theme does not exist, "
                         "falling back to \""
                      << getDefaultPath() << "\"";
        path = getDefaultPath();
    }

    return get(size, path);
}
