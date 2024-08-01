//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  Font.h
//
//  Loading, unloading, caching, shaping and rendering of fonts.
//  Also functions for text wrapping and similar.
//

#include "resources/Font.h"

#include "Log.h"
#include "renderers/Renderer.h"
#include "utils/FileSystemUtil.h"
#include "utils/PlatformUtil.h"
#include "utils/StringUtil.h"

Font::Font(float size, const std::string& path)
    : mRenderer {Renderer::getInstance()}
    , mPath(path)
    , mFontHB {nullptr}
    , mLastFontHB {nullptr}
    , mBufHB {nullptr}
    , mFontSize {size}
    , mLetterHeight {0.0f}
    , mMaxGlyphHeight {static_cast<int>(std::round(size))}
    , mTextHash {0}
{
    if (mFontSize < 3.0f) {
        mFontSize = 3.0f;
        LOG(LogWarning) << "Requested font size too small, changing to minimum supported size";
    }
    else if (mFontSize > Renderer::getScreenHeight() * 1.5f) {
        mFontSize = Renderer::getScreenHeight() * 1.5f;
        LOG(LogWarning) << "Requested font size too large, changing to maximum supported size";
    }

    if (!sLibrary) {
        initLibrary();
        sFallbackFonts = getFallbackFontPaths();
    }

    const std::string fontPath {ResourceManager::getInstance().getResourcePath(mPath)};
    hb_blob_t* blobHB {hb_blob_create_from_file(fontPath.c_str())};
    hb_face_t* faceHB {hb_face_create(blobHB, 0)};
    mFontHB = hb_font_create(faceHB);
    hb_font_set_ptem(mFontHB, mFontSize);
    hb_face_destroy(faceHB);
    hb_blob_destroy(blobHB);

    mBufHB = hb_buffer_create();

    ResourceData data {ResourceManager::getInstance().getFileData(fontPath)};
    mFontFace = std::make_unique<FontFace>(std::move(data), mFontSize, path, mFontHB);
}

Font::~Font()
{
    mFontFace.reset();
    hb_buffer_destroy(mBufHB);
    hb_font_destroy(mFontHB);

    unload(ResourceManager::getInstance());

    auto fontEntry = sFontMap.find(std::tuple<float, std::string>(mFontSize, mPath));

    if (fontEntry != sFontMap.cend())
        sFontMap.erase(fontEntry);

    if (sFontMap.empty() && sLibrary) {
        for (auto& font : sFallbackFonts)
            hb_font_destroy(font.fontHB);
        sFallbackFonts.clear();
        FT_Done_FreeType(sLibrary);
        sLibrary = nullptr;
    }
}

std::shared_ptr<Font> Font::get(float size, const std::string& path)
{
    const std::string canonicalPath {Utils::FileSystem::getCanonicalPath(path)};
    const std::tuple<float, std::string> def {size, canonicalPath.empty() ? getDefaultPath() :
                                                                            canonicalPath};

    auto foundFont = sFontMap.find(def);
    if (foundFont != sFontMap.cend()) {
        if (!foundFont->second.expired())
            return foundFont->second.lock();
    }

    std::shared_ptr<Font> font {new Font(std::get<0>(def), std::get<1>(def))};
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

    for (size_t i {0}; i < text.length();) {
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
    float x {offset.x + (xLen != 0 ? getNewlineStartOffset(text, 0, xLen, alignment) : 0)};
    int yTop {0};
    float yBot {0.0f};

    if (noTopMargin) {
        yTop = 0;
        yBot = getHeight(1.5);
    }
    else {
        yTop = getGlyph('S')->bearing.y;
        yBot = getHeight(lineSpacing);
    }

    float y {offset.y + ((yBot + yTop) / 2.0f)};

    // Vertices by texture.
    std::map<FontTexture*, std::vector<Renderer::Vertex>> vertMap;

    // Build segments for HarfBuzz.
    buildShapeSegments(text);

    size_t cursor {0};
    size_t length {0};
    hb_glyph_info_t* glyphInfo {nullptr};
    hb_glyph_position_t* glyphPos {nullptr};
    unsigned int glyphCount {0};

    for (auto& segment : mSegmentsHB) {
        cursor = 0;
        length = 0;

        if (segment.doShape) {
            hb_buffer_reset(mBufHB);
            hb_buffer_add_utf8(mBufHB, text.c_str(), text.length(), segment.startPos,
                               segment.length);
            hb_buffer_guess_segment_properties(mBufHB);
            hb_shape(segment.fontHB, mBufHB, nullptr, 0);

            glyphInfo = hb_buffer_get_glyph_infos(mBufHB, &glyphCount);
            glyphPos = hb_buffer_get_glyph_positions(mBufHB, &glyphCount);
            length = glyphCount;
        }
        else {
            length = segment.length;
        }

        while (cursor < length) {
            unsigned int character {0};

            if (segment.doShape) {
                character = glyphInfo[cursor].codepoint;
                ++cursor;
            }
            else {
                // This also advances the cursor.
                character = Utils::String::chars2Unicode(segment.substring, cursor);
            }

            Glyph* glyph {nullptr};

            // Invalid character.
            if (character == 0)
                continue;

            if (character == '\n') {
                y += getHeight(lineSpacing);
                x = offset[0] +
                    (xLen != 0 ? getNewlineStartOffset(text,
                                                       static_cast<const unsigned int>(
                                                           cursor) /* cursor is already advanced */,
                                                       xLen, alignment) :
                                 0);
                continue;
            }

            if (segment.doShape)
                glyph = getGlyphByIndex(character, segment.fontHB);
            else
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
            for (int i {1}; i < 5; ++i)
                vertices[i].position = glm::round(vertices[i].position);

            // Make duplicates of first and last vertex so this can be rendered as a triangle strip.
            vertices[0] = vertices[1];
            vertices[5] = vertices[4];

            // Advance.
            x += glyph->advance.x;
        }
    }

    TextCache* cache {new TextCache()};
    cache->vertexLists.resize(vertMap.size());
    cache->metrics.size = {sizeText(text, lineSpacing)};
    cache->metrics.maxGlyphHeight = mMaxGlyphHeight;
    cache->clipRegion = {0.0f, 0.0f, 0.0f, 0.0f};

    size_t i {0};
    for (auto it = vertMap.cbegin(); it != vertMap.cend(); ++it) {
        TextCache::VertexList& vertList {cache->vertexLists.at(i)};
        vertList.textureIdPtr = &it->first->textureId;
        vertList.verts = it->second;
        ++i;
    }

    return cache;
}

void Font::renderTextCache(TextCache* cache)
{
    if (cache == nullptr) {
        LOG(LogError) << "Attempted to draw nullptr TextCache";
        return;
    }

    const bool clipRegion {cache->clipRegion != glm::vec4 {0.0f, 0.0f, 0.0f, 0.0f}};

    for (auto it = cache->vertexLists.begin(); it != cache->vertexLists.end(); ++it) {
        assert(*it->textureIdPtr != 0);

        it->verts[0].shaderFlags = Renderer::ShaderFlags::FONT_TEXTURE;

        if (clipRegion) {
            it->verts[0].shaderFlags |= Renderer::ShaderFlags::CLIPPING;
            it->verts[0].clipRegion = cache->clipRegion;
        }

        mRenderer->bindTexture(*it->textureIdPtr, 0);
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

    for (size_t i {0}; i < text.length(); ++i) {
        if (text[i] == '\n') {
            if (!multiLine) {
                addDots = true;
                break;
            }
            accumHeight += lineHeight;
            if (maxHeight != 0.0f && accumHeight > maxHeight) {
                addDots = true;
                break;
            }
            wrappedText.append("\n");
            lineWidth = 0.0f;
            lastSpace = 0;
            continue;
        }

        cursor = i;

        // Needed to handle multi-byte Unicode characters.
        charID = Utils::String::chars2Unicode(text, cursor);
        charEntry = text.substr(i, cursor - i);

        Glyph* glyph {getGlyph(charID)};
        if (glyph != nullptr) {
            charWidth = static_cast<float>(glyph->advance.x);
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
                                         const float sizeMultiplier,
                                         const bool fontSizeDimmed)
{
    using namespace ThemeFlags;
    if (!(properties & FONT_PATH) && !(properties & FONT_SIZE))
        return orig;

    float size {static_cast<float>(orig ? orig->mFontSize : FONT_SIZE_MEDIUM_FIXED)};
    std::string path {orig ? orig->mPath : getDefaultPath()};

    const float screenSize {Renderer::getIsVerticalOrientation() ?
                                static_cast<float>(Renderer::getScreenWidth()) :
                                static_cast<float>(Renderer::getScreenHeight())};

    if (fontSizeDimmed && properties & FONT_SIZE && elem->has("fontSizeDimmed")) {
        size = glm::clamp(screenSize * elem->get<float>("fontSizeDimmed"), screenSize * 0.001f,
                          screenSize * 1.5f);
    }
    else if (properties & FONT_SIZE && elem->has("fontSize")) {
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

    return get(size, path);
}

size_t Font::getMemUsage() const
{
    size_t memUsage {0};

    for (auto it = mTextures.cbegin(); it != mTextures.cend(); ++it)
        memUsage += (*it)->textureSize.x * (*it)->textureSize.y * 4;

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

std::vector<Font::FallbackFontCache> Font::getFallbackFontPaths()
{
    std::vector<FallbackFontCache> fontPaths;

    // Default application fonts.
    ResourceManager::getInstance().getResourcePath(":/fonts/Akrobat-Regular.ttf");
    ResourceManager::getInstance().getResourcePath(":/fonts/Akrobat-SemiBold.ttf");
    ResourceManager::getInstance().getResourcePath(":/fonts/Akrobat-Bold.ttf");

    const std::vector<std::string> fallbackFonts {
        // Ubuntu Condensed.
        ":/fonts/Ubuntu-C.ttf",
        // Vera sans Unicode.
        ":/fonts/DejaVuSans.ttf",
        // GNU FreeFont monospaced.
        ":/fonts/FreeMono.ttf",
        // Various languages, such as Japanese and Chinese.
        ":/fonts/DroidSansFallbackFull.ttf",
        // Korean
        ":/fonts/NanumMyeongjo.ttf",
        // Font Awesome icon glyphs, used for various special symbols like stars, folders etc.
        ":/fonts/fontawesome-webfont.ttf",
        // Google Noto Emoji.
        ":/fonts/NotoEmoji.ttf"};

    for (auto& font : fallbackFonts) {
        FallbackFontCache fallbackFont;
        const std::string path {ResourceManager::getInstance().getResourcePath(font)};
        fallbackFont.path = path;
        hb_blob_t* blobHB {hb_blob_create_from_file(path.c_str())};
        hb_face_t* faceHB {hb_face_create(blobHB, 0)};
        hb_font_t* fontHB {hb_font_create(faceHB)};
        fallbackFont.fontHB = fontHB;
        hb_face_destroy(faceHB);
        hb_blob_destroy(blobHB);
        ResourceData data {ResourceManager::getInstance().getFileData(path)};
        fallbackFont.face = std::make_shared<FontFace>(std::move(data), 10.0f, path, fontHB);
        fontPaths.emplace_back(fallbackFont);
    }

    return fontPaths;
}

Font::FontTexture::FontTexture(const int mFontSize)
{
    textureId = 0;
    rowHeight = 0;
    writePos = glm::ivec2 {1, 1};

    // Set the texture atlas to a reasonable size, if we run out of space for adding glyphs then
    // more textures will be created dynamically.
    textureSize = glm::ivec2 {mFontSize * 6, mFontSize * 6};
}

Font::FontTexture::~FontTexture()
{
    // Deinit the texture when destroyed.
    deinitTexture();
}

bool Font::FontTexture::findEmpty(const glm::ivec2& size, glm::ivec2& cursorOut)
{
    if (size.x > textureSize.x || size.y > textureSize.y)
        return false;

    if (writePos.x + size.x + 1 > textureSize.x &&
        writePos.y + rowHeight + size.y + 1 < textureSize.y) {
        // Row is full, but the glyph should fit on the next row so move the cursor there.
        // Leave 1 pixel of space between glyphs so that pixels from adjacent glyphs will not
        // get sampled during scaling and interpolation, which would lead to edge artifacts.
        writePos = glm::ivec2 {1, writePos.y + rowHeight + 1};
        rowHeight = 0;
    }

    if (writePos.x + size.x + 1 > textureSize.x || writePos.y + size.y + 1 > textureSize.y)
        return false; // No it still won't fit.

    cursorOut = writePos;
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
    textureId =
        Renderer::getInstance()->createTexture(0, Renderer::TextureType::RED, true, true, false,
                                               false, textureSize.x, textureSize.y, &texture[0]);
}

void Font::FontTexture::deinitTexture()
{
    if (textureId != 0) {
        Renderer::getInstance()->destroyTexture(textureId);
        textureId = 0;
    }
}

Font::FontFace::FontFace(ResourceData&& d, float size, const std::string& path, hb_font_t* fontArg)
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
    fontHB = fontArg;
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

void Font::buildShapeSegments(const std::string& text)
{
    // Calculate the hash value for the string to make sure we're not building segments
    // repeatedly for the same text.
    const size_t hashValue {std::hash<std::string> {}(text)};
    if (hashValue == mTextHash)
        return;

    mTextHash = hashValue;
    mSegmentsHB.clear();

    hb_font_t* lastFont {nullptr};
    unsigned int lastCursor {0};
    unsigned int byteLength {0};
    bool addSegment {false};
    bool shapeSegment {true};
    bool lastWasNoShaping {false};
    size_t textCursor {0};
    size_t lastFlushPos {0};

    while (textCursor < text.length()) {
        addSegment = false;
        shapeSegment = true;
        lastCursor = textCursor;
        const unsigned int unicode {Utils::String::chars2Unicode(text, textCursor)};
        Glyph* currGlyph {getGlyph(unicode)};
        byteLength = textCursor - lastCursor;

        if (unicode == '\'' || unicode == '\n' || currGlyph->fontHB == nullptr) {
            // HarfBuzz converts ' and newline characters to invalid characters, so we
            // need to exclude these from getting shaped. This means adding a new segment.
            // We also add a segment if there is no font set as it means there was a missing
            // glyph and the "no glyph" symbol should be shown.
            addSegment = true;
            if (!lastWasNoShaping) {
                textCursor -= byteLength;
                if (lastFlushPos == textCursor)
                    addSegment = false;
                lastWasNoShaping = true;
            }
            else {
                shapeSegment = false;
                lastWasNoShaping = false;
            }
        }
        else if (textCursor == text.length()) {
            // Last (and possibly only) segment for this text.
            addSegment = true;
        }
        else if (lastFont != nullptr && lastFont != currGlyph->fontHB) {
            // The font changed, which requires a new segment.
            addSegment = true;
            textCursor -= byteLength;
        }

        if (addSegment) {
            ShapeSegment segment;
            segment.startPos = lastFlushPos;
            segment.length = textCursor - lastFlushPos;
            segment.fontHB = (lastFont == nullptr ? currGlyph->fontHB : lastFont);
            segment.doShape = shapeSegment;
            if (!shapeSegment)
                segment.substring = text.substr(lastFlushPos, textCursor - lastFlushPos);

            mSegmentsHB.emplace_back(std::move(segment));

            lastFlushPos = textCursor;
        }
        lastFont = currGlyph->fontHB;
    }
}

void Font::rebuildTextures()
{
    // Recreate OpenGL textures.
    for (auto it = mTextures.begin(); it != mTextures.end(); ++it)
        (*it)->initTexture();

    // Re-upload the texture data.
    for (auto it = mGlyphMap.cbegin(); it != mGlyphMap.cend(); ++it) {
        FT_Face* face {getFaceForChar(it->first)};
        FT_GlyphSlot glyphSlot {(*face)->glyph};

        // Load the glyph bitmap through FreeType.
        FT_Load_Char(*face, it->first, FT_LOAD_RENDER);

        const glm::ivec2 glyphSize {glyphSlot->bitmap.width, glyphSlot->bitmap.rows};
        const glm::ivec2 cursor {
            static_cast<int>(it->second.texPos.x * it->second.texture->textureSize.x),
            static_cast<int>(it->second.texPos.y * it->second.texture->textureSize.y)};

        // Upload glyph bitmap to texture.
        if (glyphSize.x > 0 && glyphSize.y > 0) {
            mRenderer->updateTexture(it->second.texture->textureId, 0, Renderer::TextureType::RED,
                                     cursor.x, cursor.y, glyphSize.x, glyphSize.y,
                                     glyphSlot->bitmap.buffer);
        }
    }

    for (auto it = mGlyphMapByIndex.cbegin(); it != mGlyphMapByIndex.cend(); ++it) {
        FT_Face* face {getFaceForGlyphIndex(it->first.first, it->first.second)};
        FT_GlyphSlot glyphSlot {(*face)->glyph};

        // Load the glyph bitmap through FreeType.
        FT_Load_Glyph(*face, it->first.first, FT_LOAD_RENDER);

        const glm::ivec2 glyphSize {glyphSlot->bitmap.width, glyphSlot->bitmap.rows};
        const glm::ivec2 cursor {
            static_cast<int>(it->second.texPos.x * it->second.texture->textureSize.x),
            static_cast<int>(it->second.texPos.y * it->second.texture->textureSize.y)};

        // Upload glyph bitmap to texture.
        if (glyphSize.x > 0 && glyphSize.y > 0) {
            mRenderer->updateTexture(it->second.texture->textureId, 0, Renderer::TextureType::RED,
                                     cursor.x, cursor.y, glyphSize.x, glyphSize.y,
                                     glyphSlot->bitmap.buffer);
        }
    }
}

void Font::unloadTextures()
{
    for (auto it = mTextures.begin(); it != mTextures.end(); ++it)
        (*it)->deinitTexture();
}

void Font::getTextureForNewGlyph(const glm::ivec2& glyphSize,
                                 FontTexture*& texOut,
                                 glm::ivec2& cursorOut)
{
    if (mTextures.size()) {
        // Check if the most recent texture has space available for the glyph.
        texOut = mTextures.back().get();

        // Will this one work?
        if (texOut->findEmpty(glyphSize, cursorOut))
            return; // Yes.
    }

    mTextures.emplace_back(std::make_unique<FontTexture>(static_cast<int>(std::round(mFontSize))));
    texOut = mTextures.back().get();
    texOut->initTexture();

    if (!texOut->findEmpty(glyphSize, cursorOut)) {
        LOG(LogError) << "Glyph too big to fit on a new texture (glyph size > "
                      << texOut->textureSize.x << ", " << texOut->textureSize.y << ")";
        texOut = nullptr;
    }
}

FT_Face* Font::getFaceForChar(unsigned int id)
{
    // Look for the glyph in our current font and then in the fallback fonts if needed.
    if (FT_Get_Char_Index(mFontFace->face, id) != 0) {
        mLastFontHB = mFontHB;
        return &mFontFace->face;
    }

    for (auto& font : sFallbackFonts) {
        if (FT_Get_Char_Index(font.face->face, id) != 0) {
            // This is most definitely not thread safe.
            FT_Set_Char_Size(font.face->face, static_cast<FT_F26Dot6>(0.0f),
                             static_cast<FT_F26Dot6>(mFontSize * 64.0f), 0, 0);
            mLastFontHB = font.fontHB;
            return &font.face->face;
        }
    }

    // Couldn't find a valid glyph, return the current font face so we get a "no glyph" character.
    mLastFontHB = nullptr;
    return &mFontFace->face;
}

FT_Face* Font::getFaceForGlyphIndex(unsigned int id, hb_font_t* fontArg)
{
    if (mFontFace->fontHB == fontArg && FT_Load_Glyph(mFontFace->face, id, FT_LOAD_RENDER) == 0) {
        mLastFontHB = mFontHB;
        return &mFontFace->face;
    }

    for (auto& font : sFallbackFonts) {
        if (font.fontHB == fontArg && FT_Load_Glyph(font.face->face, id, FT_LOAD_RENDER) == 0) {
            FT_Set_Char_Size(font.face->face, static_cast<FT_F26Dot6>(0.0f),
                             static_cast<FT_F26Dot6>(mFontSize * 64.0f), 0, 0);
            mLastFontHB = font.fontHB;
            return &font.face->face;
        }
    }

    // Couldn't find a valid glyph, return the current font face so we get a "no glyph" character.
    mLastFontHB = nullptr;
    return &mFontFace->face;
}

Font::Glyph* Font::getGlyph(const unsigned int id)
{
    // Check if the glyph has already been loaded.
    auto it = mGlyphMap.find(id);
    if (it != mGlyphMap.cend())
        return &it->second;

    // We need to create a new entry.
    FT_Face* face {getFaceForChar(id)};
    if (!face) {
        LOG(LogError) << "Couldn't find appropriate font face for character " << id << " for font "
                      << mPath;
        return nullptr;
    }

    const FT_GlyphSlot glyphSlot {(*face)->glyph};

    // If the font does not contain hinting information then force the use of the automatic
    // hinter that is built into FreeType. Note: Using font-supplied hints generally looks worse
    // than using the auto-hinter so it's disabled for now.
    // const bool hasHinting {static_cast<bool>(glyphSlot->face->face_flags & FT_FACE_FLAG_HINTER)};
    const bool hasHinting {true};

    if (FT_Load_Char(*face, id,
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

    glyph.fontHB = mLastFontHB;
    glyph.texture = tex;
    glyph.texPos = {cursor.x / static_cast<float>(tex->textureSize.x),
                    cursor.y / static_cast<float>(tex->textureSize.y)};
    glyph.texSize = {glyphSize.x / static_cast<float>(tex->textureSize.x),
                     glyphSize.y / static_cast<float>(tex->textureSize.y)};
    glyph.advance = {glyphSlot->metrics.horiAdvance >> 6, glyphSlot->metrics.vertAdvance >> 6};
    glyph.bearing = {glyphSlot->metrics.horiBearingX >> 6, glyphSlot->metrics.horiBearingY >> 6};
    glyph.rows = glyphSize.y;

    // Upload glyph bitmap to texture.
    if (glyphSize.x > 0 && glyphSize.y > 0) {
        mRenderer->updateTexture(tex->textureId, 0, Renderer::TextureType::RED, cursor.x, cursor.y,
                                 glyphSize.x, glyphSize.y, glyphSlot->bitmap.buffer);
    }

    return &glyph;
}

Font::Glyph* Font::getGlyphByIndex(const unsigned int id, hb_font_t* fontArg)
{
    // Check if the glyph has already been loaded.
    auto it = mGlyphMapByIndex.find(std::make_pair(id, fontArg));
    if (it != mGlyphMapByIndex.cend())
        return &it->second;

    // We need to create a new entry.
    FT_Face* face {getFaceForGlyphIndex(id, fontArg)};
    if (!face) {
        LOG(LogError) << "Couldn't find appropriate font face for character " << id << " for font "
                      << mPath;
        return nullptr;
    }

    const FT_GlyphSlot glyphSlot {(*face)->glyph};

    // If the font does not contain hinting information then force the use of the automatic
    // hinter that is built into FreeType. Note: Using font-supplied hints generally looks worse
    // than using the auto-hinter so it's disabled for now.
    // const bool hasHinting {static_cast<bool>(glyphSlot->face->face_flags & FT_FACE_FLAG_HINTER)};
    const bool hasHinting {true};

    if (FT_Load_Glyph(*face, id,
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
    Glyph& glyph {mGlyphMapByIndex[std::make_pair(id, mLastFontHB)]};

    glyph.fontHB = mLastFontHB;
    glyph.texture = tex;
    glyph.texPos = {cursor.x / static_cast<float>(tex->textureSize.x),
                    cursor.y / static_cast<float>(tex->textureSize.y)};
    glyph.texSize = {glyphSize.x / static_cast<float>(tex->textureSize.x),
                     glyphSize.y / static_cast<float>(tex->textureSize.y)};
    glyph.advance = {glyphSlot->metrics.horiAdvance >> 6, glyphSlot->metrics.vertAdvance >> 6};
    glyph.bearing = {glyphSlot->metrics.horiBearingX >> 6, glyphSlot->metrics.horiBearingY >> 6};
    glyph.rows = glyphSize.y;

    // Upload glyph bitmap to texture.
    if (glyphSize.x > 0 && glyphSize.y > 0) {
        mRenderer->updateTexture(tex->textureId, 0, Renderer::TextureType::RED, cursor.x, cursor.y,
                                 glyphSize.x, glyphSize.y, glyphSlot->bitmap.buffer);
    }

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
