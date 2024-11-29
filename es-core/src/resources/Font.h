//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  Font.h
//
//  Font management and text shaping and rendering.
//

#ifndef ES_CORE_RESOURCES_FONT_H
#define ES_CORE_RESOURCES_FONT_H

#include "GuiComponent.h"
#include "ThemeData.h"
#include "renderers/Renderer.h"
#include "resources/ResourceManager.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb-ft.h>
#include <vector>

class TextComponent;

#define FONT_SIZE_MINI Font::getMiniFont()
#define FONT_SIZE_SMALL Font::getSmallFont()
#define FONT_SIZE_MEDIUM Font::getMediumFont()
#define FONT_SIZE_MEDIUM_FIXED Font::getMediumFixedFont()
#define FONT_SIZE_LARGE Font::getLargeFont()
#define FONT_SIZE_LARGE_FIXED Font::getLargeFixedFont()

#define FONT_PATH_LIGHT ":/fonts/Akrobat-Regular.ttf"
#define FONT_PATH_REGULAR ":/fonts/Akrobat-SemiBold.ttf"
#define FONT_PATH_BOLD ":/fonts/Akrobat-Bold.ttf"

class Font : public IReloadable
{
public:
    virtual ~Font();
    static std::shared_ptr<Font> get(float size, const std::string& path = getDefaultPath());
    static float getMiniFont(bool forceUpdate = false)
    {
        static float sMiniFont {0.030f *
                                std::min(Renderer::getScreenHeight(), Renderer::getScreenWidth())};
        if (forceUpdate)
            sMiniFont = 0.030f * std::min(Renderer::getScreenHeight(), Renderer::getScreenWidth());
        return sMiniFont;
    }
    static float getSmallFont(bool forceUpdate = false)
    {
        static float sSmallFont {0.035f *
                                 std::min(Renderer::getScreenHeight(), Renderer::getScreenWidth())};
        if (forceUpdate)
            sSmallFont = 0.035f * std::min(Renderer::getScreenHeight(), Renderer::getScreenWidth());
        return sSmallFont;
    }
    static float getMediumFont(bool forceUpdate = false)
    {
        static float sMediumFont {
            (Renderer::getIsVerticalOrientation() ? 0.040f : 0.045f) *
            std::min(Renderer::getScreenHeight(), Renderer::getScreenWidth())};
        if (forceUpdate)
            sMediumFont = (Renderer::getIsVerticalOrientation() ? 0.040f : 0.045f) *
                          std::min(Renderer::getScreenHeight(), Renderer::getScreenWidth());
        return sMediumFont;
    }
    static float getMediumFixedFont(bool forceUpdate = false)
    {
        // Fixed size regardless of screen orientation.
        static float sMediumFixedFont {
            0.045f * std::min(Renderer::getScreenHeight(), Renderer::getScreenWidth())};
        if (forceUpdate)
            sMediumFixedFont =
                0.045f * std::min(Renderer::getScreenHeight(), Renderer::getScreenWidth());
        return sMediumFixedFont;
    }
    static float getLargeFont(bool forceUpdate = false)
    {
        static float sLargeFont {(Renderer::getIsVerticalOrientation() ? 0.080f : 0.085f) *
                                 std::min(Renderer::getScreenHeight(), Renderer::getScreenWidth())};
        if (forceUpdate)
            sLargeFont = (Renderer::getIsVerticalOrientation() ? 0.080f : 0.085f) *
                         std::min(Renderer::getScreenHeight(), Renderer::getScreenWidth());
        return sLargeFont;
    }
    static float getLargeFixedFont(bool forceUpdate = false)
    {
        // Fixed size regardless of screen orientation.
        static float sLargeFixedFont {
            0.085f * std::min(Renderer::getScreenHeight(), Renderer::getScreenWidth())};
        if (forceUpdate)
            sLargeFixedFont =
                0.085f * std::min(Renderer::getScreenHeight(), Renderer::getScreenWidth());
        return sLargeFixedFont;
    }

    // Needed for when the application window has been resized.
    static void updateFontSizes();

    // Returns the size of shaped text without applying any wrapping or abbreviations.
    glm::vec2 sizeText(std::string text, float lineSpacing = 1.5f);

    // This determines mMaxGlyphHeight upfront which is useful for accurate text sizing as
    // the requested font height is not guaranteed and could be exceeded by a few pixels for some
    // glyphs. However in most instances setting mMaxGlyphHeight to the font size is good enough,
    // meaning this somehow expensive operation could be skipped.
    int loadGlyphs(const std::string& text);

    // Return overall height including line spacing.
    const float getHeight(float lineSpacing = 1.5f) const { return mMaxGlyphHeight * lineSpacing; }
    // This uses the letter 'S' as a size reference.
    const float getLetterHeight() { return mLetterHeight; }

    void reload(ResourceManager& rm) override { rebuildTextures(); }
    void unload(ResourceManager& rm) override { unloadTextures(); }

    const float getSize() const { return mFontSize; }
    const std::string& getPath() const { return mPath; }
    static std::string getDefaultPath() { return FONT_PATH_REGULAR; }

    static std::shared_ptr<Font> getFromTheme(const ThemeData::ThemeElement* elem,
                                              unsigned int properties,
                                              const std::shared_ptr<Font>& orig,
                                              const float maxHeight = 0.0f,
                                              const float sizeMultiplier = 1.0f,
                                              const bool fontSizeDimmed = false);

    // Returns an approximation of VRAM used by the glyph atlas textures for this font object.
    size_t getMemUsage() const;
    // Returns an approximation of VRAM used by the glyph atlas textures for all font objects.
    static size_t getTotalMemUsage();

protected:
    TextCache* buildTextCache(const std::string& text,
                              float length,
                              float maxLength,
                              float height,
                              float offsetY,
                              float lineSpacing,
                              Alignment alignment,
                              unsigned int color,
                              bool noTopMargin,
                              bool multiLine,
                              bool needGlyphsPos);

    void renderTextCache(TextCache* cache);
    // This is used to determine the horizontal text scrolling speed.
    float getSizeReference();

    // Enable or disable shaping, used by TextEditComponent.
    void setTextShaping(bool state) { mShapeText = state; }

    friend TextComponent;

private:
    Font(float size, const std::string& path);
    static void initLibrary();

    struct FontTexture {
        unsigned int textureId;
        glm::ivec2 textureSize;
        glm::ivec2 writePos;
        int rowHeight;

        FontTexture(const int mFontSize);
        ~FontTexture();
        bool findEmpty(const glm::ivec2& size, glm::ivec2& cursorOut);

        // You must call initTexture() after creating a FontTexture to get a textureId.
        void initTexture();

        // Deinitializes all glyph atlas textures.
        void deinitTexture();
    };

    struct FontFace {
        const ResourceData data;
        FT_Face face;
        hb_font_t* fontHB;

        FontFace(ResourceData&& d, float size, const std::string& path, hb_font_t* fontArg);
        virtual ~FontFace();
    };

    struct Glyph {
        FontTexture* texture;
        hb_font_t* fontHB;
        glm::vec2 texPos;
        glm::vec2 texSize;
        glm::ivec2 advance;
        glm::ivec2 bearing;
        int rows;
    };

    struct GlyphTexture {
        FontTexture* texture;
        glm::ivec2 cursor;
    };

    struct FallbackFontCache {
        std::string path;
        std::shared_ptr<FontFace> face;
        hb_font_t* fontHB;
        unsigned int spaceChar;
    };

    struct ShapeSegment {
        unsigned int startPos;
        unsigned int length;
        float shapedWidth;
        hb_font_t* fontHB;
        bool doShape;
        bool lineBreak;
        bool wrapped;
        bool rightToLeft;
        unsigned int spaceChar;
        std::string substring;
        std::vector<std::pair<unsigned int, int>> glyphIndexes;

        ShapeSegment()
            : startPos {0}
            , length {0}
            , shapedWidth {0}
            , fontHB {nullptr}
            , doShape {false}
            , lineBreak {false}
            , wrapped {false}
            , rightToLeft {false}
            , spaceChar {0}
        {
        }
    };

    // Shape text using HarfBuzz.
    void shapeText(const std::string& text, std::vector<ShapeSegment>& segmentsHB);

    // Inserts newlines to make text wrap properly and also abbreviates when necessary.
    void wrapText(std::vector<ShapeSegment>& segmentsHB,
                  float maxLength,
                  const float maxHeight,
                  const float lineSpacing,
                  const bool multiLine,
                  const bool needGlyphsPos);

    // Completely recreate the texture data for all glyph atlas entries.
    void rebuildTextures();
    void unloadTextures();

    void getTextureForNewGlyph(const glm::ivec2& glyphSize,
                               FontTexture*& texOut,
                               glm::ivec2& cursorOut);

    std::vector<FallbackFontCache> getFallbackFontPaths();
    FT_Face* getFaceForChar(unsigned int id, hb_font_t** returnedFont);
    FT_Face* getFaceForGlyphIndex(unsigned int id, hb_font_t* fontArg, hb_font_t** returnedFont);
    Glyph* getGlyph(const unsigned int id);
    Glyph* getGlyphByIndex(const unsigned int id, hb_font_t* fontArg, int xAdvance);

    static inline FT_Library sLibrary {nullptr};
    static inline std::map<std::tuple<float, std::string>, std::weak_ptr<Font>> sFontMap;
    static inline std::vector<FallbackFontCache> sFallbackFonts;
    static inline std::map<hb_font_t*, unsigned int> sFallbackSpaceGlyphs;

    Renderer* mRenderer;
    std::unique_ptr<FontFace> mFontFace;
    std::vector<std::unique_ptr<FontTexture>> mTextures;
    std::map<unsigned int, Glyph> mGlyphMap;
    std::map<std::tuple<unsigned int, hb_font_t*, int>, Glyph> mGlyphMapByIndex;
    std::map<std::pair<unsigned int, hb_font_t*>, GlyphTexture> mGlyphTextureMap;

    const std::string mPath;
    hb_font_t* mFontHB;
    hb_buffer_t* mBufHB;
    std::tuple<unsigned int, unsigned int, hb_font_t*> mEllipsisGlyph;

    float mFontSize;
    float mLetterHeight;
    float mSizeReference;
    int mMaxGlyphHeight;
    unsigned int mSpaceGlyph;
    bool mShapeText;
};

// Caching of shaped and rendered text.
class TextCache
{
public:
    struct CacheMetrics {
        glm::vec2 size;
        int maxGlyphHeight;

        CacheMetrics()
            : size {0.0f, 0.0f}
            , maxGlyphHeight {0} {};
    } metrics;

    void setColor(unsigned int color);
    void setOpacity(float opacity);
    void setSaturation(float saturation);
    void setDimming(float dimming);
    void setClipRegion(const glm::vec4& clip) { clipRegion = clip; }
    const glm::vec2& getSize() { return metrics.size; }

    // Used by TextEditComponent to position the cursor and scroll the text box.
    std::vector<glm::vec2> glyphPositions;

    friend Font;

protected:
    struct VertexList {
        std::vector<Renderer::Vertex> verts;
        unsigned int* textureIdPtr;
    };

    std::vector<VertexList> vertexLists;
    glm::vec4 clipRegion;
};

#endif // ES_CORE_RESOURCES_FONT_H
