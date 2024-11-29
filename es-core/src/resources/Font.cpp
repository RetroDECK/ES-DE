//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  Font.cpp
//
//  Font management and text shaping and rendering.
//

#include "resources/Font.h"

#include "Log.h"
#include "renderers/Renderer.h"
#include "utils/FileSystemUtil.h"
#include "utils/PlatformUtil.h"
#include "utils/StringUtil.h"

#define DEBUG_SHAPING false
#define DISABLE_SHAPING false

Font::Font(float size, const std::string& path)
    : mRenderer {Renderer::getInstance()}
    , mPath(path)
    , mFontHB {nullptr}
    , mBufHB {nullptr}
    , mEllipsisGlyph {0, 0, nullptr}
    , mFontSize {size}
    , mLetterHeight {0.0f}
    , mSizeReference {0.0f}
    , mMaxGlyphHeight {static_cast<int>(std::round(size))}
    , mSpaceGlyph {0}
    , mShapeText {true}
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
    hb_face_destroy(faceHB);
    hb_blob_destroy(blobHB);

    mBufHB = hb_buffer_create();

    ResourceData data {ResourceManager::getInstance().getFileData(fontPath)};
    mFontFace = std::make_unique<FontFace>(std::move(data), mFontSize, path, mFontHB);

    // Use the letter 'S' as a size reference.
    mLetterHeight = static_cast<float>(getGlyph('S')->rows);

    // As no faces should contain a newline glyph, requesting this character normally returns
    // the size of the font. However there are instances where this is calculated to a slightly
    // different size than the actual font size, and in this case we want to use this instead
    // of the font size to avoid some minor sizing issues.
    if (getGlyph('\n')->rows > mMaxGlyphHeight)
        mMaxGlyphHeight = getGlyph('\n')->rows;

    // This is used when abbreviating and wrapping text in wrapText().
    std::vector<Font::ShapeSegment> shapedGlyph;
    shapeText("…", shapedGlyph);
    if (!shapedGlyph.empty()) {
        mEllipsisGlyph = std::make_tuple(shapedGlyph.front().glyphIndexes.front().first,
                                         shapedGlyph.front().glyphIndexes.front().second,
                                         shapedGlyph.front().fontHB);
    }
    // This will be zero if there is no space glyph in the font (which hopefully never happens).
    mSpaceGlyph = FT_Get_Char_Index(mFontFace->face, ' ');
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

void Font::updateFontSizes()
{
    getMiniFont(true);
    getSmallFont(true);
    getMediumFont(true);
    getMediumFixedFont(true);
    getLargeFont(true);
    getLargeFixedFont(true);
}

glm::vec2 Font::sizeText(std::string text, float lineSpacing)
{
    if (text == "")
        return glm::vec2 {0.0f, getHeight(lineSpacing)};

    const float lineHeight {getHeight(lineSpacing)};
    float lineWidth {0.0f};
    float highestWidth {0.0f};
    float y {lineHeight};

    std::vector<ShapeSegment> segmentsHB;
    shapeText(text, segmentsHB);

    for (auto& segment : segmentsHB) {
        for (size_t i {0}; i < segment.glyphIndexes.size(); ++i) {
            const unsigned int character {segment.glyphIndexes[i].first};

            // Invalid character.
            if (!segment.doShape && character == 0)
                continue;

            if (!segment.doShape && character == '\n') {
                if (lineWidth > highestWidth)
                    highestWidth = lineWidth;

                lineWidth = 0.0f;
                y += lineHeight;
                continue;
            }

            lineWidth += segment.glyphIndexes[i].second;
        }

        if (lineWidth > highestWidth)
            highestWidth = lineWidth;
    }

    return glm::vec2 {highestWidth, y};
}

int Font::loadGlyphs(const std::string& text)
{
    mMaxGlyphHeight = static_cast<int>(std::round(mFontSize));
    if (getGlyph('\n')->rows > mMaxGlyphHeight)
        mMaxGlyphHeight = getGlyph('\n')->rows;

    std::vector<ShapeSegment> segmentsHB;
    shapeText(text, segmentsHB);

    for (auto& segment : segmentsHB) {
        for (size_t i {0}; i < segment.glyphIndexes.size(); ++i) {
            const unsigned int character {segment.glyphIndexes[i].first};
            Glyph* glyph {nullptr};

            // Invalid character.
            if (!segment.doShape && character == 0)
                continue;

            if (segment.doShape)
                glyph = getGlyphByIndex(character, segment.fontHB, segment.glyphIndexes[i].second);
            else
                glyph = getGlyph(character);

            if (glyph && glyph->rows > mMaxGlyphHeight)
                mMaxGlyphHeight = glyph->rows;
        }
    }

    return mMaxGlyphHeight;
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

TextCache* Font::buildTextCache(const std::string& text,
                                float length,
                                float maxLength,
                                float height,
                                float offsetY,
                                float lineSpacing,
                                Alignment alignment,
                                unsigned int color,
                                bool noTopMargin,
                                bool multiLine,
                                bool needGlyphsPos)
{
    if (maxLength == 0.0f)
        maxLength = length;

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

    std::vector<ShapeSegment> segmentsHB;
    shapeText(text, segmentsHB);
    wrapText(segmentsHB, maxLength, height, lineSpacing, multiLine, needGlyphsPos);

    size_t segmentIndex {0};
    float x {0.0f};
    float y {offsetY + ((yBot + yTop) / 2.0f)};
    float lineWidth {0.0f};
    float longestLine {0.0f};
    float accumHeight {getHeight(lineSpacing)};
    bool isNewLine {false};

    // Vertices by texture.
    std::map<FontTexture*, std::vector<Renderer::Vertex>> vertMap;

    std::vector<glm::vec2> glyphPositions;
    if (needGlyphsPos)
        glyphPositions.emplace_back(0.0f, 0.0f);

    for (auto& segment : segmentsHB) {
        if (isNewLine || segmentIndex == 0) {
            isNewLine = false;
            float totalLength {0.0f};
            for (size_t i {segmentIndex}; i < segmentsHB.size(); ++i) {
                if (segmentsHB[i].lineBreak)
                    break;
                totalLength += segmentsHB[i].shapedWidth;
            }
            float lengthTemp {length};
            if (length == 0.0f)
                lengthTemp = totalLength;
            if (alignment == ALIGN_CENTER)
                x = (lengthTemp - totalLength) / 2.0f;
            else if (alignment == ALIGN_RIGHT)
                x = lengthTemp - totalLength;
        }

        for (size_t cursor {0}; cursor < segment.glyphIndexes.size(); ++cursor) {
            const unsigned int character {segment.glyphIndexes[cursor].first};
            Glyph* glyph {nullptr};

            // Invalid character.
            if (!segment.doShape && character == 0) {
                if (needGlyphsPos) {
                    // TODO: This is a temporary workaround for a problem that only seems to be
                    // present on Android, and that is that non-character input from a physical
                    // keyboard generates SDL_TEXTINPUT events even though it shouldn't. This
                    // workaround is not a proper fix, it's only there to prevent ES-DE from
                    // crashing if such input is received when editing text. The issue has been
                    // reported to the SDL developers as it needs to be addressed there.
                    if (glyphPositions.size() > 0)
                        glyphPositions.emplace_back(glyphPositions.back().x,
                                                    glyphPositions.back().y);
                    else
                        glyphPositions.emplace_back(0.0f, 0.0f);
                }
                continue;
            }

            if (!segment.doShape && character == '\n') {
                x = 0.0f;
                y += getHeight(lineSpacing);
                lineWidth = 0.0f;
                accumHeight += getHeight(lineSpacing);

                // This logic changes the position of any space glyph at the end of a row to the
                // beginning of the next row, as that's more intuitive when editing text.
                bool spaceMatch {false};
                if (needGlyphsPos && segmentIndex > 0) {
                    unsigned int spaceChar {0};
#if (DISABLE_SHAPING)
                    if (true)
#else
                    if (!mShapeText)
#endif
                        spaceChar = 32;
                    else if (segmentsHB[segmentIndex - 1].fontHB == mFontHB)
                        spaceChar = mSpaceGlyph;
                    else if (sFallbackSpaceGlyphs.find(segmentsHB[segmentIndex - 1].fontHB) !=
                             sFallbackSpaceGlyphs.cend())
                        spaceChar = sFallbackSpaceGlyphs[segment.fontHB];
                    unsigned int character {segmentsHB[segmentIndex - 1].glyphIndexes.back().first};
                    if (character == spaceChar)
                        spaceMatch = true;
                }

                if (needGlyphsPos && spaceMatch && glyphPositions.size() > 0) {
                    glyphPositions.back().x = 0.0f;
                    glyphPositions.back().y = accumHeight - getHeight(lineSpacing);
                }

                // Only add positions for "real" line breaks that were part of the original text.
                if (needGlyphsPos && !segment.wrapped)
                    glyphPositions.emplace_back(x, accumHeight - getHeight(lineSpacing));

                isNewLine = true;
                continue;
            }
            else if (segment.glyphIndexes[cursor].second == -1) {
                // Special scenario where a space glyph at the end of a segment should be omitted,
                // in which case it's set to -1 advance in wrapText(). We can't set it to 0 as
                // that's actually a valid value for some fonts such as when having an apostrophe
                // followed by a comma.
                continue;
            }

            if (segment.doShape)
                glyph =
                    getGlyphByIndex(character, segment.fontHB, segment.glyphIndexes[cursor].second);
            else
                glyph = getGlyph(character);

            if (glyph == nullptr)
                continue;

            lineWidth += glyph->advance.x;

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

            if (needGlyphsPos)
                glyphPositions.emplace_back(x, accumHeight - getHeight(lineSpacing));

            if (lineWidth > longestLine)
                longestLine = lineWidth;
        }
        ++segmentIndex;
    }

    TextCache* cache {new TextCache()};
    cache->vertexLists.resize(vertMap.size());
    cache->metrics.size = glm::vec2 {longestLine, accumHeight};
    cache->metrics.maxGlyphHeight = mMaxGlyphHeight;
    cache->clipRegion = {0.0f, 0.0f, 0.0f, 0.0f};
    if (needGlyphsPos)
        cache->glyphPositions = std::move(glyphPositions);

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

float Font::getSizeReference()
{
    if (mSizeReference != 0.0f)
        return mSizeReference;

    const std::string includeChars {"ABCDEFGHIJKLMNOPQRSTUVWXYZ"};
    hb_font_t* returnedFont {nullptr};
    bool fontError {false};
    int advance {0};

    FT_Face* face {getFaceForChar('A', &returnedFont)};
    if (!face) {
        // This is completely inaccurate but it should hopefully never happen.
        return static_cast<float>(mMaxGlyphHeight * 16);
    }

    // We don't check the face for each character, we just assume that if the font includes
    // the 'A' character it also includes the other Latin capital letters.
    for (auto character : includeChars) {
        if (!fontError) {
            const FT_GlyphSlot glyphSlot {(*face)->glyph};
            if (FT_Load_Char(*face, character, FT_LOAD_RENDER))
                return static_cast<float>(mMaxGlyphHeight * 16);
            else
                advance += glyphSlot->metrics.horiAdvance >> 6;
        }
    }

    mSizeReference = static_cast<float>(advance);
    return mSizeReference;
}

Font::FontTexture::FontTexture(const int mFontSize)
{
    textureId = 0;
    rowHeight = 0;
    writePos = glm::ivec2 {1, 1};

    // Set the glyph atlas to a reasonable size, if we run out of space for adding glyphs then
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
    // Create a black texture with a zero alpha value so that single-pixel spaces between the
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
    // though as the glyphs will still be much more evenly sized across different resolutions.
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

void Font::shapeText(const std::string& text, std::vector<ShapeSegment>& segmentsHB)
{
    hb_font_t* lastFont {nullptr};
    size_t lastCursor {0};
    size_t byteLength {0};
    size_t textCursor {0};
    size_t lastFlushPos {0};
    bool addSegment {false};
    bool shapeSegment {true};
    bool lastWasNoShaping {false};

    // Step 1, build segments.

    while (textCursor < text.length()) {
        addSegment = false;
        shapeSegment = true;
        lastCursor = textCursor;
        const unsigned int unicode {Utils::String::chars2Unicode(text, textCursor)};
        Glyph* currGlyph {getGlyph(unicode)};
        // Extra precaution in case the font is really broken.
        if (currGlyph == nullptr)
            continue;
        byteLength = textCursor - lastCursor;

        if (unicode == '\n' || currGlyph->fontHB == nullptr) {
            // We need to add a segment if there is a line break, or if no font is set as the
            // latter means there was a missing glyph and the "no glyph" symbol should be shown.
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
            // In case the font changed for the last character.
            if (lastFont != nullptr && lastFont != currGlyph->fontHB && unicode != ' ')
                textCursor -= byteLength;
        }
        else if (lastFont != nullptr && lastFont != currGlyph->fontHB && unicode != ' ') {
            // The font changed, which requires a new segment.
            addSegment = true;
            textCursor -= byteLength;
        }

#if (DISABLE_SHAPING)
        shapeSegment = false;
#else
        if (!mShapeText)
            shapeSegment = false;
#endif

        if (addSegment) {
            ShapeSegment segment;
            segment.startPos = static_cast<unsigned int>(lastFlushPos);
            segment.length = static_cast<unsigned int>(textCursor - lastFlushPos);
            segment.fontHB = (lastFont == nullptr ? currGlyph->fontHB : lastFont);
            segment.doShape = shapeSegment;
#if (DEBUG_SHAPING)
            segment.substring = text.substr(lastFlushPos, textCursor - lastFlushPos);
            if (segment.substring == "\n")
                segment.lineBreak = true;
#else
            if (!shapeSegment) {
                segment.substring = text.substr(lastFlushPos, textCursor - lastFlushPos);
                if (segment.substring == "\n")
                    segment.lineBreak = true;
            }
#endif
            segmentsHB.emplace_back(std::move(segment));

            lastFlushPos = textCursor;
        }
        if (unicode != ' ' || lastFont == nullptr)
            lastFont = currGlyph->fontHB;
    }

    if (segmentsHB.empty())
        return;

    size_t cursor {0};
    size_t length {0};
    hb_glyph_info_t* glyphInfo {nullptr};
    hb_glyph_position_t* glyphPos {nullptr};
    unsigned int glyphCount {0};

    // Step 2, shape text.

    for (auto& segment : segmentsHB) {
        cursor = 0;
        length = 0;
        segment.glyphIndexes.clear();

        if (segment.doShape) {
            hb_buffer_reset(mBufHB);
            hb_buffer_add_utf8(mBufHB, text.c_str(), static_cast<int>(text.length()),
                               segment.startPos, segment.length);
            hb_buffer_guess_segment_properties(mBufHB);
            hb_font_set_scale(segment.fontHB, static_cast<int>(std::round(mFontSize * 256.0f)),
                              static_cast<int>(std::round(mFontSize * 256.0f)));
            hb_shape(segment.fontHB, mBufHB, nullptr, 0);

            if (hb_buffer_get_direction(mBufHB) == HB_DIRECTION_RTL)
                segment.rightToLeft = true;

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
                getGlyphByIndex(character, segment.fontHB == nullptr ? mFontHB : segment.fontHB,
                                glyphPos[cursor].x_advance);
                const int advanceX {static_cast<int>(
                    std::round(static_cast<float>(glyphPos[cursor].x_advance) / 256.0f))};
                segment.shapedWidth += advanceX;
                segment.glyphIndexes.emplace_back(std::make_pair(character, advanceX));
                ++cursor;
            }
            else {
                // This also advances the cursor.
                character = Utils::String::chars2Unicode(segment.substring, cursor);

                Glyph* glyph {getGlyph(character)};
                segment.shapedWidth += glyph->advance.x;
                segment.glyphIndexes.emplace_back(std::make_pair(character, glyph->advance.x));
            }
        }
    }
}

void Font::wrapText(std::vector<ShapeSegment>& segmentsHB,
                    float maxLength,
                    const float maxHeight,
                    const float lineSpacing,
                    const bool multiLine,
                    const bool needGlyphsPos)
{
    std::vector<ShapeSegment> resultSegments;

    // We first need to check whether the text is mixing left-to-right and right-to-left script
    // as such text always needs to be processed in order to get spacing correct between segments.
    bool hasLTR {false};
    bool hasRTL {false};
    for (auto& segment : segmentsHB) {
        if (segment.rightToLeft)
            hasRTL = true;
        else
            hasLTR = true;
        // This is a special case where there is text with mixed script directions but with no
        // length restriction. This most often means it's horizontally scrolling text. In this
        // case we just set the length to a really large number, it's only to correctly get all
        // segments processed below.
        if (hasRTL && hasLTR && maxLength == 0.0f)
            maxLength = 30000.0f;
    }

    if (!(hasLTR && hasRTL)) {
        // This captures all text that is only a single segment and fits within maxLength, or that
        // is not length-restricted.
        if (maxLength == 0.0f ||
            (segmentsHB.size() == 1 && segmentsHB.front().shapedWidth <= maxLength))
            return;

        // Additionally this captures shorter single-line multi-segment text that does not require
        // more involved line breaking or abbreviations.
        float combinedWidth {0.0f};
        bool hasNewline {false};
        for (auto& segment : segmentsHB) {
            combinedWidth += segment.shapedWidth;
            if (segment.lineBreak) {
                hasNewline = true;
                break;
            }
        }
        if (!hasNewline && combinedWidth <= maxLength)
            return;
    }

    // All text that makes it this far requires either abbrevation or wrapping, or both.
    // TODO: Text that mixes left-to-right and right-to-left script may not wrap and
    // abbreviate correctly under all circumstances.

    unsigned int newLength {0};
    unsigned int spaceChar {0};
    int lastSpaceWidth {0};
    const float lineHeight {getHeight(lineSpacing)};
    float totalWidth {0.0f};
    float newShapedWidth {0.0f};
    float accumHeight {lineHeight};
    bool firstGlyphSpace {false};
    bool lastSegmentSpace {false};
    bool addEllipsis {false};

    for (auto& segment : segmentsHB) {
        if (addEllipsis)
            break;

        size_t lastSpace {0};
        size_t spaceAccum {0};

        // The space character glyph differs between fonts, so we need to know the correct
        // index to be able to detect spaces.
        if (segment.doShape == false)
            spaceChar = 32;
        else if (segment.fontHB == mFontHB)
            spaceChar = mSpaceGlyph;
        else if (sFallbackSpaceGlyphs.find(segment.fontHB) != sFallbackSpaceGlyphs.cend())
            spaceChar = sFallbackSpaceGlyphs[segment.fontHB];
        else
            spaceChar = 0;

        newShapedWidth = 0.0f;
        ShapeSegment newSegment;
        newSegment.startPos = newLength;
        newSegment.fontHB = segment.fontHB;
        newSegment.doShape = segment.doShape;
        newSegment.rightToLeft = segment.rightToLeft;
        newSegment.spaceChar = spaceChar;
#if (DEBUG_SHAPING)
        newSegment.substring = segment.substring;
#else
        if (!newSegment.doShape)
            newSegment.substring = segment.substring;
#endif

        // We don't bother to reverse this back later as the segment should only be needed once.
        if (segment.rightToLeft) {
            if (segment.glyphIndexes.front().first == spaceChar)
                std::reverse(segment.glyphIndexes.begin() + 1, segment.glyphIndexes.end());
            else
                std::reverse(segment.glyphIndexes.begin(), segment.glyphIndexes.end());
        }

        for (size_t i {0}; i < segment.glyphIndexes.size(); ++i) {
            if (multiLine) {
                if (segment.lineBreak) {
                    totalWidth = 0.0f;
                    accumHeight += lineHeight;
                    newSegment.lineBreak = true;
                }

                if (segment.glyphIndexes[i].first == spaceChar) {
                    lastSpace = i;
                    lastSpaceWidth = segment.glyphIndexes[i].second;
                    lastSegmentSpace = false;
                    if (i == 0)
                        firstGlyphSpace = true;
                }
            }

            if (totalWidth + segment.glyphIndexes[i].second > maxLength) {
                if (multiLine) {
                    if (maxHeight != 0.0f && accumHeight > maxHeight) {
                        addEllipsis = true;
                        break;
                    }
                    if (maxHeight == 0.0f || accumHeight < maxHeight) {
                        // New row.
                        size_t offset {0};

                        bool shapedSegmentChange {false};
                        if (lastSpace == 0 && resultSegments.size() > 0 &&
                            !resultSegments.back().lineBreak)
                            shapedSegmentChange = true;

                        if (lastSpace == i && !lastSegmentSpace && !shapedSegmentChange) {
                            if (segment.rightToLeft)
                                newSegment.glyphIndexes.insert(newSegment.glyphIndexes.begin(),
                                                               segment.glyphIndexes[i]);
                            else
                                newSegment.glyphIndexes.emplace_back(segment.glyphIndexes[i]);

                            ++i;
                        }
                        else if (lastSpace != 0 || firstGlyphSpace || lastSegmentSpace) {
                            size_t accum {0};
                            if (lastSegmentSpace)
                                ++accum;
                            if (newSegment.rightToLeft &&
                                segment.glyphIndexes.front().first == spaceChar)
                                ++accum;
                            lastSegmentSpace = false;
                            firstGlyphSpace = false;
                            if (lastSpace + spaceAccum - accum != i) {
                                offset = i - (lastSpace + spaceAccum - accum) - 1;
                                newShapedWidth -= lastSpaceWidth;
                                spaceAccum = 0;
                            }
                        }
                        else if (shapedSegmentChange) {
                            offset = i;
                        }
                        else {
                            if (lastSpace == 0)
                                ++spaceAccum;
                        }

                        for (size_t o {0}; o < offset; ++o) {
                            // Remove all glyphs going back to the last space.
                            --i;
                            --newLength;
                            if (newSegment.rightToLeft) {
                                newShapedWidth -= newSegment.glyphIndexes.front().second;
                                newSegment.glyphIndexes.erase(newSegment.glyphIndexes.begin());
                            }
                            else {
                                newShapedWidth -= newSegment.glyphIndexes.back().second;
                                newSegment.glyphIndexes.pop_back();
                            }
                            // If all glyphs were removed and the last character of the previous
                            // segment was a space, then set its advance to -1 so it gets excluded
                            // in buildTextCache(). That is, unless needGlyphPos is true as that
                            // means the text is needed for TextEditComponent and should therefore
                            // not be altered.
                            if (!needGlyphsPos && newSegment.glyphIndexes.empty() &&
                                !resultSegments.empty()) {
                                if (resultSegments.back().glyphIndexes.back().first ==
                                    resultSegments.back().spaceChar) {
                                    resultSegments.back().shapedWidth -=
                                        resultSegments.back().glyphIndexes.back().second;
                                    resultSegments.back().glyphIndexes.back().second = -1;
                                }
                            }
                        }

                        newSegment.length =
                            static_cast<unsigned int>(newSegment.glyphIndexes.size());
                        newSegment.shapedWidth = newShapedWidth;

                        if (newSegment.glyphIndexes.size() != 0)
                            resultSegments.emplace_back(newSegment);

                        ShapeSegment breakSegment;
                        breakSegment.startPos = newLength;
                        breakSegment.length = 1;
                        breakSegment.shapedWidth = 0.0f;
                        breakSegment.fontHB = nullptr;
                        breakSegment.doShape = false;
                        breakSegment.lineBreak = true;
                        breakSegment.wrapped = true;
                        breakSegment.rightToLeft = false;
                        breakSegment.substring = "\n";
                        breakSegment.glyphIndexes.emplace_back(std::make_pair('\n', 0));
                        resultSegments.emplace_back(breakSegment);

                        ++newLength;

                        newSegment.glyphIndexes.clear();
                        newSegment.startPos = newLength;
                        newSegment.length = 0;
                        newSegment.shapedWidth = 0.0f;
                        newShapedWidth = 0.0f;
                        totalWidth = 0.0f;
                        lastSpace = 0;
                        spaceAccum = 0;
                        accumHeight += lineHeight;
                    }
                }
                else {
                    addEllipsis = true;
                    break;
                }
            }

            if (i == segment.glyphIndexes.size())
                continue;

            if (segment.rightToLeft)
                newSegment.glyphIndexes.insert(newSegment.glyphIndexes.begin(),
                                               segment.glyphIndexes[i]);
            else
                newSegment.glyphIndexes.emplace_back(segment.glyphIndexes[i]);

            newShapedWidth += segment.glyphIndexes[i].second;
            if (!segment.lineBreak)
                totalWidth += segment.glyphIndexes[i].second;
            ++newLength;
        }

        // If the last glyph in the segment was a space, then this info may be needed for
        // correct wrapping in the following segment.
        if (lastSpace != 0 && newSegment.glyphIndexes.size() > 0 &&
            newSegment.glyphIndexes.back().first == spaceChar)
            lastSegmentSpace = true;
        else
            lastSegmentSpace = false;

        newSegment.length = static_cast<unsigned int>(newSegment.glyphIndexes.size());
        newSegment.shapedWidth = newShapedWidth;

        if (newSegment.glyphIndexes.size() != 0)
            resultSegments.emplace_back(newSegment);
    }

    if (addEllipsis && resultSegments.size() != 0 &&
        resultSegments.back().glyphIndexes.size() > 0) {
        std::vector<Font::ShapeSegment> shapedGlyph;
        shapeText("…", shapedGlyph);
        if (!shapedGlyph.empty()) {
            mEllipsisGlyph = std::make_tuple(shapedGlyph.front().glyphIndexes.front().first,
                                             shapedGlyph.front().glyphIndexes.front().second,
                                             shapedGlyph.front().fontHB);
        }

        if (resultSegments.back().rightToLeft) {
            std::reverse(resultSegments.back().glyphIndexes.begin(),
                         resultSegments.back().glyphIndexes.end());
        }
        // If the last glyph is a space then remove it.
        if (resultSegments.back().glyphIndexes.back().first == resultSegments.back().spaceChar) {
            totalWidth -= resultSegments.back().glyphIndexes.back().second;
            resultSegments.back().shapedWidth -= resultSegments.back().glyphIndexes.back().second;
            resultSegments.back().glyphIndexes.pop_back();
        }
        // Remove as many glyphs as needed to fit the ellipsis glyph within maxLength.
        while (resultSegments.back().glyphIndexes.size() > 0 &&
               totalWidth + std::get<1>(mEllipsisGlyph) > maxLength) {
            totalWidth -= resultSegments.back().glyphIndexes.back().second;
            resultSegments.back().shapedWidth -= resultSegments.back().glyphIndexes.back().second;
            resultSegments.back().glyphIndexes.pop_back();
        }
        // If the last glyph is a space then remove it before adding the ellipsis. This is
        // however only done for a single space character in case there are repeating spaces.
        if (resultSegments.back().glyphIndexes.size() > 0 &&
            resultSegments.back().glyphIndexes.back().first == resultSegments.back().spaceChar) {
            totalWidth -= resultSegments.back().glyphIndexes.back().second;
            resultSegments.back().shapedWidth -= resultSegments.back().glyphIndexes.back().second;
            resultSegments.back().glyphIndexes.pop_back();
        }
        // This is a special case where the last glyph of the last segment was removed and
        // the last glyph of the previous segment is a space, in this case we want to remove
        // that space glyph as well.
        else if (resultSegments.back().glyphIndexes.empty() && resultSegments.size() > 1 &&
                 resultSegments[resultSegments.size() - 2].glyphIndexes.size() > 0) {
            if (resultSegments[resultSegments.size() - 2].rightToLeft) {
                std::reverse(resultSegments[resultSegments.size() - 2].glyphIndexes.begin(),
                             resultSegments[resultSegments.size() - 2].glyphIndexes.end());
            }
            if (resultSegments[resultSegments.size() - 2].glyphIndexes.back().first ==
                resultSegments[resultSegments.size() - 2].spaceChar) {
                totalWidth -= resultSegments[resultSegments.size() - 2].glyphIndexes.back().second;
                resultSegments[resultSegments.size() - 2].shapedWidth -=
                    resultSegments[resultSegments.size() - 2].glyphIndexes.back().second;
                resultSegments[resultSegments.size() - 2].glyphIndexes.pop_back();
            }
            if (resultSegments[resultSegments.size() - 2].rightToLeft) {
                std::reverse(resultSegments[resultSegments.size() - 2].glyphIndexes.begin(),
                             resultSegments[resultSegments.size() - 2].glyphIndexes.end());
            }
        }
        if (resultSegments.back().rightToLeft) {
            std::reverse(resultSegments.back().glyphIndexes.begin(),
                         resultSegments.back().glyphIndexes.end());
        }

        // Append the ellipsis glyph.
        if (std::get<2>(mEllipsisGlyph) != nullptr) {
            ShapeSegment newSegment;
            newSegment.startPos = 0;
            newSegment.fontHB = std::get<2>(mEllipsisGlyph);
#if (DISABLE_SHAPING)
            newSegment.doShape = false;
#else
            if (mShapeText)
                newSegment.doShape = true;
            else
                newSegment.doShape = false;
#endif
            newSegment.rightToLeft = false;
            newSegment.shapedWidth += std::get<1>(mEllipsisGlyph);
            newSegment.glyphIndexes.emplace_back(
                std::make_pair(std::get<0>(mEllipsisGlyph), std::get<1>(mEllipsisGlyph)));

            if (resultSegments.back().rightToLeft)
                resultSegments.insert(resultSegments.end() - 1, newSegment);
            else
                resultSegments.emplace_back(newSegment);
        }
    }

    std::swap(resultSegments, segmentsHB);
}

void Font::rebuildTextures()
{
    // Recreate all glyph atlas textures.
    for (auto it = mTextures.begin(); it != mTextures.end(); ++it)
        (*it)->initTexture();

    hb_font_t* returnedFont {nullptr};

    // Re-upload the texture data.
    for (auto it = mGlyphMap.cbegin(); it != mGlyphMap.cend(); ++it) {
        FT_Face* face {getFaceForChar(it->first, &returnedFont)};
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
        FT_Face* face {
            getFaceForGlyphIndex(std::get<0>(it->first), std::get<1>(it->first), &returnedFont)};
        FT_GlyphSlot glyphSlot {(*face)->glyph};

        FT_Load_Glyph(*face, std::get<0>(it->first), FT_LOAD_RENDER);

        const glm::ivec2 glyphSize {glyphSlot->bitmap.width, glyphSlot->bitmap.rows};
        const glm::ivec2 cursor {
            static_cast<int>(it->second.texPos.x * it->second.texture->textureSize.x),
            static_cast<int>(it->second.texPos.y * it->second.texture->textureSize.y)};

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
        ":/fonts/NanumSquareNeo-bRg.ttf",
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
        const unsigned int spaceChar {FT_Get_Char_Index(fallbackFont.face->face, ' ')};
        if (spaceChar != 0)
            sFallbackSpaceGlyphs[fontHB] = spaceChar;
        fontPaths.emplace_back(fallbackFont);
    }

    return fontPaths;
}

FT_Face* Font::getFaceForChar(unsigned int id, hb_font_t** returnedFont)
{
    // Look for the glyph in our current font and then in the fallback fonts if needed.
    if (FT_Get_Char_Index(mFontFace->face, id) != 0) {
        *returnedFont = mFontHB;
        return &mFontFace->face;
    }

    for (auto& font : sFallbackFonts) {
        if (FT_Get_Char_Index(font.face->face, id) != 0) {
            // This is most definitely not thread safe.
            FT_Set_Char_Size(font.face->face, static_cast<FT_F26Dot6>(0.0f),
                             static_cast<FT_F26Dot6>(mFontSize * 64.0f), 0, 0);
            *returnedFont = font.fontHB;
            return &font.face->face;
        }
    }

    // Couldn't find a valid glyph, return the current font face so we get a "no glyph" character.
    *returnedFont = nullptr;
    return &mFontFace->face;
}

FT_Face* Font::getFaceForGlyphIndex(unsigned int id, hb_font_t* fontArg, hb_font_t** returnedFont)
{
    if (mFontFace->fontHB == fontArg && FT_Load_Glyph(mFontFace->face, id, FT_LOAD_RENDER) == 0) {
        *returnedFont = mFontHB;
        return &mFontFace->face;
    }

    for (auto& font : sFallbackFonts) {
        if (font.fontHB == fontArg && FT_Load_Glyph(font.face->face, id, FT_LOAD_RENDER) == 0) {
            FT_Set_Char_Size(font.face->face, static_cast<FT_F26Dot6>(0.0f),
                             static_cast<FT_F26Dot6>(mFontSize * 64.0f), 0, 0);
            *returnedFont = font.fontHB;
            return &font.face->face;
        }
    }

    *returnedFont = nullptr;
    return &mFontFace->face;
}

Font::Glyph* Font::getGlyph(const unsigned int id)
{
    // Check if the glyph has already been loaded.
    auto it = mGlyphMap.find(id);
    if (it != mGlyphMap.cend())
        return &it->second;

    hb_font_t* returnedFont {nullptr};

    // We need to create a new entry.
    FT_Face* face {getFaceForChar(id, &returnedFont)};
    if (!face) {
        LOG(LogError) << "Couldn't find appropriate font face for character " << id << " for font "
                      << mPath;
        return nullptr;
    }

    const FT_GlyphSlot glyphSlot {(*face)->glyph};

    if (FT_Load_Char(*face, id, FT_LOAD_RENDER)) {
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

    // Create glyph.
    Glyph& glyph {mGlyphMap[id]};

    glyph.fontHB = returnedFont;
    glyph.texture = tex;
    glyph.texPos = {cursor.x / static_cast<float>(tex->textureSize.x),
                    cursor.y / static_cast<float>(tex->textureSize.y)};
    glyph.texSize = {glyphSize.x / static_cast<float>(tex->textureSize.x),
                     glyphSize.y / static_cast<float>(tex->textureSize.y)};
    glyph.advance = {glyphSlot->metrics.horiAdvance >> 6, glyphSlot->metrics.vertAdvance >> 6};
    glyph.bearing = {glyphSlot->metrics.horiBearingX >> 6, glyphSlot->metrics.horiBearingY >> 6};
    glyph.rows = glyphSize.y;

    // Upload glyph bitmap to glyph atlas texture.
    if (glyphSize.x > 0 && glyphSize.y > 0) {
        mRenderer->updateTexture(tex->textureId, 0, Renderer::TextureType::RED, cursor.x, cursor.y,
                                 glyphSize.x, glyphSize.y, glyphSlot->bitmap.buffer);
    }

    return &glyph;
}

Font::Glyph* Font::getGlyphByIndex(const unsigned int id, hb_font_t* fontArg, int xAdvance)
{
    // Check if the glyph has already been loaded.
    auto it = mGlyphMapByIndex.find(std::make_tuple(id, fontArg, xAdvance));
    if (it != mGlyphMapByIndex.end())
        return &it->second;

    hb_font_t* returnedFont {nullptr};

    // We need to create a new entry.
    FT_Face* face {getFaceForGlyphIndex(id, fontArg, &returnedFont)};
    if (!face) {
        LOG(LogError) << "Couldn't find appropriate font face for glyph index " << id
                      << " for font " << mPath;
        return nullptr;
    }

    const FT_GlyphSlot glyphSlot {(*face)->glyph};

    if (FT_Load_Glyph(*face, id, FT_LOAD_RENDER)) {
        LOG(LogError) << "Couldn't find glyph for glyph index " << id << " for font " << mPath
                      << ", size " << mFontSize;
        return nullptr;
    }

    FontTexture* tex {nullptr};
    glm::ivec2 cursor {0, 0};
    const glm::ivec2 glyphSize {glyphSlot->bitmap.width, glyphSlot->bitmap.rows};

    // Check if there is already a texture entry for the glyph, otherwise create it.
    // This makes sure we don't create multiple identical glyph atlas entries and waste VRAM.
    auto it2 = mGlyphTextureMap.find(std::make_pair(id, fontArg));
    if (it2 != mGlyphTextureMap.end()) {
        tex = (*it2).second.texture;
        cursor = (*it2).second.cursor;
    }
    else {
        getTextureForNewGlyph(glyphSize, tex, cursor);
        GlyphTexture& glyphTexture {mGlyphTextureMap[std::make_pair(id, returnedFont)]};
        glyphTexture.texture = tex;
        glyphTexture.cursor = cursor;
    }

    // This should (hopefully) never occur as size constraints are enforced earlier on.
    if (tex == nullptr) {
        LOG(LogError) << "Couldn't create glyph for glyph index " << id << " for font " << mPath
                      << ", size " << mFontSize << " (no suitable texture found)";
        return nullptr;
    }

    // Create glyph.
    Glyph& glyph {mGlyphMapByIndex[std::make_tuple(id, returnedFont, xAdvance)]};

    glyph.fontHB = returnedFont;
    glyph.texture = tex;
    glyph.texPos = {cursor.x / static_cast<float>(tex->textureSize.x),
                    cursor.y / static_cast<float>(tex->textureSize.y)};
    glyph.texSize = {glyphSize.x / static_cast<float>(tex->textureSize.x),
                     glyphSize.y / static_cast<float>(tex->textureSize.y)};
    glyph.advance = {xAdvance, glyphSlot->metrics.vertAdvance >> 6};
    glyph.bearing = {glyphSlot->metrics.horiBearingX >> 6, glyphSlot->metrics.horiBearingY >> 6};
    glyph.rows = glyphSize.y;

    // Upload glyph bitmap to glyph atlas texture.
    if (glyphSize.x > 0 && glyphSize.y > 0) {
        mRenderer->updateTexture(tex->textureId, 0, Renderer::TextureType::RED, cursor.x, cursor.y,
                                 glyphSize.x, glyphSize.y, glyphSlot->bitmap.buffer);
    }

    return &glyph;
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
