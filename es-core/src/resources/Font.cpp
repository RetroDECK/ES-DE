//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Font.h
//
//  Loading, unloading, caching and rendering of fonts.
//  Also functions for text wrapping and similar.
//

#include "resources/Font.h"

#include "Log.h"
#include "renderers/Renderer.h"
#include "utils/FileSystemUtil.h"
#include "utils/PlatformUtil.h"
#include "utils/StringUtil.h"

Font::Font(float size, const std::string& path, const bool linearMagnify)
    : mRenderer {Renderer::getInstance()}
    , mPath(path)
    , mFontSize {size}
    , mLinearMagnify {linearMagnify}
    , mLetterHeight {0.0f}
    , mMaxGlyphHeight {static_cast<int>(std::round(size))}
    , mLegacyMaxGlyphHeight {0}
{
    if (mFontSize < 3.0f) {
        mFontSize = 3.0f;
        LOG(LogWarning) << "Requested font size too small, changing to minimum supported size";
    }
    else if (mFontSize > Renderer::getScreenHeight() * 1.5f) {
        mFontSize = Renderer::getScreenHeight() * 1.5f;
        LOG(LogWarning) << "Requested font size too large, changing to maximum supported size";
    }

    if (!sLibrary)
        initLibrary();

    // Always initialize ASCII characters.
    for (unsigned int i = 32; i < 127; ++i)
        getGlyph(i);

    clearFaceCache();
}

Font::~Font()
{
    unload(ResourceManager::getInstance());

    auto fontEntry =
        sFontMap.find(std::tuple<float, std::string, bool>(mFontSize, mPath, mLinearMagnify));

    if (fontEntry != sFontMap.cend())
        sFontMap.erase(fontEntry);

    if (sFontMap.empty() && sLibrary) {
        FT_Done_FreeType(sLibrary);
        sLibrary = nullptr;
    }
}

std::shared_ptr<Font> Font::get(float size, const std::string& path, const bool linearMagnify)
{
    const std::string canonicalPath {Utils::FileSystem::getCanonicalPath(path)};
    const std::tuple<float, std::string, bool> def {
        size, canonicalPath.empty() ? getDefaultPath() : canonicalPath, linearMagnify};

    auto foundFont = sFontMap.find(def);
    if (foundFont != sFontMap.cend()) {
        if (!foundFont->second.expired())
            return foundFont->second.lock();
    }

    std::shared_ptr<Font> font {new Font(std::get<0>(def), std::get<1>(def), std::get<2>(def))};
    sFontMap[def] = std::weak_ptr<Font>(font);
    ResourceManager::getInstance().addReloadable(font);
    return font;
}

glm::vec2 Font::sizeText(std::string text, float lineSpacing)
{
    const float lineHeight {getHeight(lineSpacing)};
    float lineWidth {0.0f};
    float highestWidth {0.0f};
    float y {lineHeight};

    size_t i {0};
    while (i < text.length()) {
        unsigned int character {Utils::String::chars2Unicode(text, i)}; // Advances i.

        if (character == '\n') {
            if (lineWidth > highestWidth)
                highestWidth = lineWidth;

            lineWidth = 0.0f;
            y += lineHeight;
        }

        Glyph* glyph {getGlyph(character)};
        if (glyph)
            lineWidth += glyph->advance.x;
    }

    if (lineWidth > highestWidth)
        highestWidth = lineWidth;

    return glm::vec2 {highestWidth, y};
}

int Font::loadGlyphs(const std::string& text)
{
    mMaxGlyphHeight = static_cast<int>(std::round(mFontSize));

    for (size_t i = 0; i < text.length();) {
        unsigned int character {Utils::String::chars2Unicode(text, i)}; // Advances i.
        Glyph* glyph {getGlyph(character)};

        if (glyph->rows > mMaxGlyphHeight)
            mMaxGlyphHeight = glyph->rows;
    }
    return mMaxGlyphHeight;
}

TextCache* Font::buildTextCache(const std::string& text,
                                float offsetX,
                                float offsetY,
                                unsigned int color,
                                float lineSpacing,
                                bool noTopMargin)
{
    return buildTextCache(text, glm::vec2 {offsetX, offsetY}, color, 0.0f, ALIGN_LEFT, lineSpacing,
                          noTopMargin);
}

TextCache* Font::buildTextCache(const std::string& text,
                                glm::vec2 offset,
                                unsigned int color,
                                float xLen,
                                Alignment alignment,
                                float lineSpacing,
                                bool noTopMargin)
{
    float x {offset[0] + (xLen != 0 ? getNewlineStartOffset(text, 0, xLen, alignment) : 0)};
    float yTop {0.0f};
    float yBot {0.0f};

    if (noTopMargin) {
        yTop = 0;
        yBot = getHeight(1.5);
    }
    else {
        // TODO: This is lacking some precision which is especially visible at higher resolutions
        // like 4K where the text is not always placed entirely correctly vertically. Try to find
        // a way to improve on this.
        yTop = getGlyph('S')->bearing.y;
        yBot = getHeight(lineSpacing);
    }

    float y {offset[1] + (yBot + yTop) / 2.0f};

    // Vertices by texture.
    std::map<FontTexture*, std::vector<Renderer::Vertex>> vertMap;

    size_t cursor {0};
    while (cursor < text.length()) {
        // Also advances cursor.
        unsigned int character {Utils::String::chars2Unicode(text, cursor)};
        Glyph* glyph {nullptr};

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

        std::vector<Renderer::Vertex>& verts {vertMap[glyph->texture]};
        size_t oldVertSize {verts.size()};
        verts.resize(oldVertSize + 6);
        Renderer::Vertex* vertices {verts.data() + oldVertSize};

        const float glyphStartX {x + glyph->bearing.x};
        const glm::ivec2& textureSize {glyph->texture->textureSize};

        vertices[1] = {
            {glyphStartX, y - glyph->bearing.y}, {glyph->texPos.x, glyph->texPos.y}, color};
        vertices[2] = {{glyphStartX, y - glyph->bearing.y + (glyph->texSize.y * textureSize.y)},
                       {glyph->texPos.x, glyph->texPos.y + glyph->texSize.y},
                       color};
        vertices[3] = {{glyphStartX + glyph->texSize.x * textureSize.x, y - glyph->bearing.y},
                       {glyph->texPos.x + glyph->texSize.x, glyph->texPos.y},
                       color};
        vertices[4] = {{glyphStartX + glyph->texSize.x * textureSize.x,
                        y - glyph->bearing.y + (glyph->texSize.y * textureSize.y)},
                       {glyph->texPos.x + glyph->texSize.x, glyph->texPos.y + glyph->texSize.y},
                       color};

        // Round vertices.
        for (int i = 1; i < 5; ++i)
            vertices[i].position = glm::round(vertices[i].position);

        // Make duplicates of first and last vertex so this can be rendered as a triangle strip.
        vertices[0] = vertices[1];
        vertices[5] = vertices[4];

        // Advance.
        x += glyph->advance.x;
    }

    TextCache* cache {new TextCache()};
    cache->vertexLists.resize(vertMap.size());
    cache->metrics.size = {sizeText(text, lineSpacing)};
    cache->metrics.maxGlyphHeight = mMaxGlyphHeight;

    size_t i {0};
    for (auto it = vertMap.cbegin(); it != vertMap.cend(); ++it) {
        TextCache::VertexList& vertList {cache->vertexLists.at(i)};
        vertList.textureIdPtr = &it->first->textureId;
        vertList.verts = it->second;
        ++i;
    }

    clearFaceCache();
    return cache;
}

void Font::renderTextCache(TextCache* cache)
{
    if (cache == nullptr) {
        LOG(LogError) << "Attempted to draw nullptr TextCache";
        return;
    }

    for (auto it = cache->vertexLists.begin(); it != cache->vertexLists.end(); ++it) {
        assert(*it->textureIdPtr != 0);

        auto vertexList = *it;
        it->verts[0].shaderFlags = Renderer::ShaderFlags::FONT_TEXTURE;

        mRenderer->bindTexture(*it->textureIdPtr);
        mRenderer->drawTriangleStrips(
            &it->verts[0], static_cast<const unsigned int>(it->verts.size()),
            Renderer::BlendFactor::SRC_ALPHA, Renderer::BlendFactor::ONE_MINUS_SRC_ALPHA);
    }
}

std::string Font::wrapText(const std::string& text,
                           const float maxLength,
                           const float maxHeight,
                           const float lineSpacing,
                           const bool multiLine)
{
    assert(maxLength > 0.0f);
    const float lineHeight {getHeight(lineSpacing)};
    const float dotsWidth {sizeText("...").x};
    float accumHeight {lineHeight};
    float lineWidth {0.0f};
    float charWidth {0.0f};
    float lastSpacePos {0.0f};
    unsigned int charID {0};
    size_t cursor {0};
    size_t lastSpace {0};
    size_t spaceAccum {0};
    size_t byteCount {0};
    std::string wrappedText;
    std::string charEntry;
    std::vector<std::pair<size_t, float>> dotsSection;
    bool addDots {false};

    for (size_t i = 0; i < text.length(); ++i) {
        if (text[i] == '\n') {
            if (!multiLine) {
                addDots = true;
                break;
            }
            wrappedText.append("\n");
            accumHeight += lineHeight;
            lineWidth = 0.0f;
            lastSpace = 0;
            continue;
        }

        charWidth = 0.0f;
        byteCount = 0;
        cursor = i;

        // Needed to handle multi-byte Unicode characters.
        charID = Utils::String::chars2Unicode(text, cursor);
        charEntry = text.substr(i, cursor - i);

        Glyph* glyph {getGlyph(charID)};
        if (glyph != nullptr) {
            charWidth = glyph->advance.x;
            byteCount = cursor - i;
        }
        else {
            // Missing glyph.
            continue;
        }

        if (multiLine && (charEntry == " " || charEntry == "\t")) {
            lastSpace = i;
            lastSpacePos = lineWidth;
        }

        if (lineWidth + charWidth <= maxLength) {
            if (lineWidth + charWidth + dotsWidth > maxLength)
                dotsSection.emplace_back(std::make_pair(byteCount, charWidth));
            lineWidth += charWidth;
            wrappedText.append(charEntry);
        }
        else if (!multiLine) {
            addDots = true;
            break;
        }
        else {
            if (maxHeight == 0.0f || accumHeight < maxHeight) {
                // New row.
                float spaceOffset {0.0f};
                if (lastSpace == wrappedText.size()) {
                    wrappedText.append("\n");
                }
                else if (lastSpace != 0) {
                    if (lastSpace + spaceAccum == wrappedText.size())
                        wrappedText.append("\n");
                    else
                        wrappedText[lastSpace + spaceAccum] = '\n';
                    spaceOffset = lineWidth - lastSpacePos;
                }
                else {
                    if (lastSpace == 0)
                        ++spaceAccum;
                    wrappedText.append("\n");
                }
                if (charEntry != " " && charEntry != "\t") {
                    wrappedText.append(charEntry);
                    lineWidth = charWidth;
                }
                else {
                    lineWidth = 0.0f;
                }
                accumHeight += lineHeight;
                lineWidth += spaceOffset;
                lastSpacePos = 0.0f;
                lastSpace = 0;
            }
            else {
                if (multiLine)
                    addDots = true;
                break;
            }
        }

        i = cursor - 1;
    }

    if (addDots) {
        if (!wrappedText.empty() && wrappedText.back() == ' ') {
            lineWidth -= sizeText(" ").x;
            wrappedText.pop_back();
        }
        else if (!wrappedText.empty() && wrappedText.back() == '\t') {
            lineWidth -= sizeText("\t").x;
            wrappedText.pop_back();
        }
        while (!wrappedText.empty() && !dotsSection.empty() && lineWidth + dotsWidth > maxLength) {
            lineWidth -= dotsSection.back().second;
            wrappedText.erase(wrappedText.length() - dotsSection.back().first);
            dotsSection.pop_back();
        }
        if (!wrappedText.empty() && wrappedText.back() == ' ')
            wrappedText.pop_back();

        wrappedText.append("...");
    }

    return wrappedText;
}

glm::vec2 Font::getWrappedTextCursorOffset(const std::string& wrappedText,
                                           const size_t stop,
                                           const float lineSpacing)
{
    float lineWidth {0.0f};
    float yPos {0.0f};
    size_t cursor {0};

    while (cursor < stop) {
        unsigned int character {Utils::String::chars2Unicode(wrappedText, cursor)};
        if (character == '\n') {
            lineWidth = 0.0f;
            yPos += getHeight(lineSpacing);
            continue;
        }

        Glyph* glyph {getGlyph(character)};
        if (glyph)
            lineWidth += glyph->advance.x;
    }

    return glm::vec2 {lineWidth, yPos};
}

float Font::getLetterHeight()
{
    if (mLetterHeight == 0.0f)
        return mFontSize * 0.737f; // Only needed if face does not contain the letter 'S'.
    else
        return mLetterHeight;
}

std::shared_ptr<Font> Font::getFromTheme(const ThemeData::ThemeElement* elem,
                                         unsigned int properties,
                                         const std::shared_ptr<Font>& orig,
                                         const float maxHeight,
                                         const bool linearMagnify,
                                         const bool legacyTheme,
                                         const float sizeMultiplier)
{
    mLegacyTheme = legacyTheme;

    using namespace ThemeFlags;
    if (!(properties & FONT_PATH) && !(properties & FONT_SIZE))
        return orig;

    float size {static_cast<float>(orig ? orig->mFontSize : FONT_SIZE_MEDIUM_FIXED)};
    std::string path {orig ? orig->mPath : getDefaultPath()};

    const float screenSize {Renderer::getIsVerticalOrientation() ?
                                static_cast<float>(Renderer::getScreenWidth()) :
                                static_cast<float>(Renderer::getScreenHeight())};

    if (properties & FONT_SIZE && elem->has("fontSize")) {
        size = glm::clamp(screenSize * elem->get<float>("fontSize"), screenSize * 0.001f,
                          screenSize * 1.5f);
        // This is used by the carousel where the itemScale property also scales the font size.
        size *= sizeMultiplier;
    }

    if (maxHeight != 0.0f && size > maxHeight)
        size = maxHeight;

    if (properties & FONT_PATH && elem->has("fontPath"))
        path = elem->get<std::string>("fontPath");

    if (!((path[0] == ':') && (path[1] == '/')) && !Utils::FileSystem::exists(path)) {
        LOG(LogError) << "Font file \"" << path
                      << "\" defined by the theme does not exist, "
                         "falling back to \""
                      << getDefaultPath() << "\"";
        path = getDefaultPath();
    }

    if (mLegacyTheme)
        return get(std::floor(size), path, false);
    else
        return get(size, path, linearMagnify);
}

size_t Font::getMemUsage() const
{
    size_t memUsage {0};
    for (auto it = mTextures.cbegin(); it != mTextures.cend(); ++it)
        memUsage += (*it)->textureSize.x * (*it)->textureSize.y * 4;

    for (auto it = mFaceCache.cbegin(); it != mFaceCache.cend(); ++it)
        memUsage += it->second->data.length;

    return memUsage;
}

size_t Font::getTotalMemUsage()
{
    size_t total {0};

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

std::vector<std::string> Font::getFallbackFontPaths()
{
    std::vector<std::string> fontPaths;

    // Default application fonts.
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

    return fontPaths;
}

Font::FontTexture::FontTexture(const int mFontSize, const bool linearMagnifyArg)
{
    textureId = 0;
    rowHeight = 0;
    writePos = glm::ivec2 {0, 0};
    linearMagnify = linearMagnifyArg;

    // Set the texture to a reasonable size, if we run out of space for adding glyphs then
    // more textures will be created dynamically.
    textureSize = glm::ivec2 {mFontSize * 6, mFontSize * 6};
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
        // Row is full, but the glyph should fit on the next row so move the cursor there.
        // Leave 1 pixel of space between glyphs so that pixels from adjacent glyphs will
        // not get sampled during scaling which would lead to edge artifacts.
        writePos = glm::ivec2 {0, writePos.y + rowHeight + 1};
        rowHeight = 0;
    }

    if (writePos.x + size.x >= textureSize.x || writePos.y + size.y >= textureSize.y)
        return false; // No it still won't fit.

    cursor_out = writePos;
    // Leave 1 pixel of space between glyphs.
    writePos.x += size.x + 1;

    if (size.y > rowHeight)
        rowHeight = size.y;

    return true;
}

void Font::FontTexture::initTexture()
{
    assert(textureId == 0);
    // Create a black texture with zero alpha value so that the single-pixel spaces between the
    // glyphs will not be visible. That would otherwise lead to edge artifacts as these pixels
    // would get sampled during scaling.
    std::vector<uint8_t> texture(textureSize.x * textureSize.y * 4, 0);
    textureId = Renderer::getInstance()->createTexture(Renderer::TextureType::RED, true,
                                                       linearMagnify, false, false, textureSize.x,
                                                       textureSize.y, &texture[0]);
}

void Font::FontTexture::deinitTexture()
{
    if (textureId != 0) {
        Renderer::getInstance()->destroyTexture(textureId);
        textureId = 0;
    }
}

Font::FontFace::FontFace(ResourceData&& d, float size, const std::string& path)
    : data {d}
{
    if (FT_New_Memory_Face(sLibrary, d.ptr.get(), static_cast<FT_Long>(d.length), 0, &face) != 0) {
        LOG(LogError) << "Couldn't load font file \"" << path << "\"";
        Utils::Platform::emergencyShutdown();
    }

    // Even though a fractional font size can be requested, the glyphs will always be rounded
    // to integers. It's not useless to call FT_Set_Char_Size() instead of FT_Set_Pixel_Sizes()
    // though as the glyphs will still be much more evenely sized across different resolutions.
    FT_Set_Char_Size(face, static_cast<FT_F26Dot6>(0.0f), static_cast<FT_F26Dot6>(size * 64.0f), 0,
                     0);
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

void Font::rebuildTextures()
{
    // Recreate OpenGL textures.
    for (auto it = mTextures.begin(); it != mTextures.end(); ++it)
        (*it)->initTexture();

    // Re-upload the texture data.
    for (auto it = mGlyphMap.cbegin(); it != mGlyphMap.cend(); ++it) {
        FT_Face face {getFaceForChar(it->first)};
        FT_GlyphSlot glyphSlot {face->glyph};

        // Load the glyph bitmap through FT.
        FT_Load_Char(face, it->first, FT_LOAD_RENDER);

        FontTexture* tex {it->second.texture};

        // Find the position/size.
        glm::ivec2 cursor {static_cast<int>(it->second.texPos.x * tex->textureSize.x),
                           static_cast<int>(it->second.texPos.y * tex->textureSize.y)};
        glm::ivec2 glyphSize {static_cast<int>(it->second.texSize.x * tex->textureSize.x),
                              static_cast<int>(it->second.texSize.y * tex->textureSize.y)};

        // Upload to texture.
        mRenderer->updateTexture(tex->textureId, Renderer::TextureType::RED, cursor.x, cursor.y,
                                 glyphSize.x, glyphSize.y, glyphSlot->bitmap.buffer);
    }
}

void Font::unloadTextures()
{
    for (auto it = mTextures.begin(); it != mTextures.end(); ++it)
        (*it)->deinitTexture();
}

void Font::getTextureForNewGlyph(const glm::ivec2& glyphSize,
                                 FontTexture*& tex_out,
                                 glm::ivec2& cursor_out)
{
    if (mTextures.size()) {
        // Check if the most recent texture has space available for the glyph.
        tex_out = mTextures.back().get();

        // Will this one work?
        if (tex_out->findEmpty(glyphSize, cursor_out))
            return; // Yes.
    }

    mTextures.emplace_back(
        std::make_unique<FontTexture>(static_cast<int>(std::round(mFontSize)), mLinearMagnify));
    tex_out = mTextures.back().get();
    tex_out->initTexture();

    bool ok {tex_out->findEmpty(glyphSize, cursor_out)};
    if (!ok) {
        LOG(LogError) << "Glyph too big to fit on a new texture (glyph size > "
                      << tex_out->textureSize.x << ", " << tex_out->textureSize.y << ")";
        tex_out = nullptr;
    }
}

FT_Face Font::getFaceForChar(unsigned int id)
{
    static const std::vector<std::string> fallbackFonts {getFallbackFontPaths()};

    // Look for the glyph in our current font and then in the fallback fonts if needed.
    for (unsigned int i = 0; i < fallbackFonts.size() + 1; ++i) {
        auto fit = mFaceCache.find(i);

        if (fit == mFaceCache.cend()) {
            const std::string& path {i == 0 ? mPath : fallbackFonts.at(i - 1)};
            ResourceData data {ResourceManager::getInstance().getFileData(path)};
            mFaceCache[i] =
                std::unique_ptr<FontFace>(new FontFace(std::move(data), mFontSize, mPath));
            fit = mFaceCache.find(i);
        }

        if (FT_Get_Char_Index(fit->second->face, id) != 0)
            return fit->second->face;
    }

    // Couldn't find a valid glyph, return the "real" face so we get a "missing" character.
    return mFaceCache.cbegin()->second->face;
}

Font::Glyph* Font::getGlyph(const unsigned int id)
{
    // Check if the glyph has already been loaded.
    auto it = mGlyphMap.find(id);
    if (it != mGlyphMap.cend())
        return &it->second;

    // We need to create a new entry.
    FT_Face face {getFaceForChar(id)};
    if (!face) {
        LOG(LogError) << "Couldn't find appropriate font face for character " << id << " for font "
                      << mPath;
        return nullptr;
    }

    const FT_GlyphSlot glyphSlot {face->glyph};

    // TODO: Evaluate/test hinting when HarfBuzz has been added.
    // If the font does not contain hinting information then force the use of the automatic
    // hinter that is built into FreeType.
    // const bool hasHinting {static_cast<bool>(glyphSlot->face->face_flags & FT_FACE_FLAG_HINTER)};
    const bool hasHinting {true};

    if (FT_Load_Char(face, id,
                     (hasHinting ?
                          FT_LOAD_RENDER :
                          FT_LOAD_RENDER | FT_LOAD_FORCE_AUTOHINT | FT_LOAD_TARGET_LIGHT))) {
        LOG(LogError) << "Couldn't find glyph for character " << id << " for font " << mPath
                      << ", size " << mFontSize;
        return nullptr;
    }

    FontTexture* tex {nullptr};
    glm::ivec2 cursor {0, 0};
    const glm::ivec2 glyphSize {glyphSlot->bitmap.width, glyphSlot->bitmap.rows};
    getTextureForNewGlyph(glyphSize, tex, cursor);

    // This should (hopefully) never occur as size constraints are enforced earlier on.
    if (tex == nullptr) {
        LOG(LogError) << "Couldn't create glyph for character " << id << " for font " << mPath
                      << ", size " << mFontSize << " (no suitable texture found)";
        return nullptr;
    }

    // Use the letter 'S' as a size reference.
    if (mLetterHeight == 0 && id == 'S')
        mLetterHeight = static_cast<float>(glyphSize.y);

    // Create glyph.
    Glyph& glyph {mGlyphMap[id]};

    glyph.texture = tex;
    glyph.texPos = glm::vec2 {cursor.x / static_cast<float>(tex->textureSize.x),
                              cursor.y / static_cast<float>(tex->textureSize.y)};
    glyph.texSize = glm::vec2 {glyphSize.x / static_cast<float>(tex->textureSize.x),
                               glyphSize.y / static_cast<float>(tex->textureSize.y)};
    glyph.advance = glm::vec2 {static_cast<float>(glyphSlot->metrics.horiAdvance) / 64.0f,
                               static_cast<float>(glyphSlot->metrics.vertAdvance) / 64.0f};
    glyph.bearing = glm::vec2 {static_cast<float>(glyphSlot->metrics.horiBearingX) / 64.0f,
                               static_cast<float>(glyphSlot->metrics.horiBearingY) / 64.0f};
    glyph.rows = glyphSlot->bitmap.rows;

    // Upload glyph bitmap to texture.
    mRenderer->updateTexture(tex->textureId, Renderer::TextureType::RED, cursor.x, cursor.y,
                             glyphSize.x, glyphSize.y, glyphSlot->bitmap.buffer);

    if (glyphSize.y > mLegacyMaxGlyphHeight)
        mLegacyMaxGlyphHeight = glyphSize.y;

    return &glyph;
}

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
            int endChar {0};
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

void TextCache::setColor(unsigned int color)
{
    for (auto it = vertexLists.begin(); it != vertexLists.end(); ++it)
        for (auto it2 = it->verts.begin(); it2 != it->verts.end(); ++it2)
            it2->color = color;
}

void TextCache::setOpacity(float opacity)
{
    for (auto it = vertexLists.begin(); it != vertexLists.end(); ++it) {
        for (auto it2 = it->verts.begin(); it2 != it->verts.end(); ++it2)
            it2->opacity = opacity;
    }
}

void TextCache::setSaturation(float saturation)
{
    for (auto it = vertexLists.begin(); it != vertexLists.end(); ++it) {
        for (auto it2 = it->verts.begin(); it2 != it->verts.end(); ++it2)
            it2->saturation = saturation;
    }
}

void TextCache::setDimming(float dimming)
{
    for (auto it = vertexLists.begin(); it != vertexLists.end(); ++it) {
        for (auto it2 = it->verts.begin(); it2 != it->verts.end(); ++it2)
            it2->dimming = dimming;
    }
}
