//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  Font.h
//
//  Loading, unloading, caching, shaping and rendering of fonts.
//  Also functions for text wrapping and similar.
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

class TextCache;

#define FONT_SIZE_MINI Font::getMiniFont()
#define FONT_SIZE_SMALL Font::getSmallFont()
#define FONT_SIZE_MEDIUM Font::getMediumFont()
#define FONT_SIZE_MEDIUM_FIXED Font::getMediumFixedFont()
#define FONT_SIZE_LARGE Font::getLargeFont()
#define FONT_SIZE_LARGE_FIXED Font::getLargeFixedFont()

#define FONT_PATH_LIGHT ":/fonts/Akrobat-Regular.ttf"
#define FONT_PATH_REGULAR ":/fonts/Akrobat-SemiBold.ttf"
#define FONT_PATH_BOLD ":/fonts/Akrobat-Bold.ttf"

// A TrueType Font renderer that uses FreeType and OpenGL.
// The library is automatically initialized when it's needed.
class Font : public IReloadable
{
public:
    virtual ~Font();
    static std::shared_ptr<Font> get(float size, const std::string& path = getDefaultPath());
    static float getMiniFont()
    {
        static float sMiniFont {0.030f *
                                std::min(Renderer::getScreenHeight(), Renderer::getScreenWidth())};
        return sMiniFont;
    }
    static float getSmallFont()
    {
        static float sSmallFont {0.035f *
                                 std::min(Renderer::getScreenHeight(), Renderer::getScreenWidth())};
        return sSmallFont;
    }
    static float getMediumFont()
    {
        static float sMediumFont {
            (Renderer::getIsVerticalOrientation() ? 0.040f : 0.045f) *
            std::min(Renderer::getScreenHeight(), Renderer::getScreenWidth())};
        return sMediumFont;
    }
    static float getMediumFixedFont()
    {
        // Fixed size regardless of screen orientation.
        static float sMediumFixedFont {
            0.045f * std::min(Renderer::getScreenHeight(), Renderer::getScreenWidth())};
        return sMediumFixedFont;
    }
    static float getLargeFont()
    {
        static float sLargeFont {(Renderer::getIsVerticalOrientation() ? 0.080f : 0.085f) *
                                 std::min(Renderer::getScreenHeight(), Renderer::getScreenWidth())};
        return sLargeFont;
    }
    static float getLargeFixedFont()
    {
        // Fixed size regardless of screen orientation.
        static float sLargeFixedFont {
            0.085f * std::min(Renderer::getScreenHeight(), Renderer::getScreenWidth())};
        return sLargeFixedFont;
    }

    // Returns the expected size of a string when rendered. Extra spacing is applied to the Y axis.
    glm::vec2 sizeText(std::string text, float lineSpacing = 1.5f);

    // Used to determine mMaxGlyphHeight upfront which is needed for accurate text sizing by
    // wrapText and buildTextCache. This is required as the requested font height is not
    // guaranteed and can be exceeded by a few pixels for some glyphs.
    int loadGlyphs(const std::string& text);

    TextCache* buildTextCache(const std::string& text,
                              float offsetX,
                              float offsetY,
                              unsigned int color,
                              float lineSpacing = 1.5f,
                              bool noTopMargin = false);

    TextCache* buildTextCache(const std::string& text,
                              glm::vec2 offset,
                              unsigned int color,
                              float xLen,
                              Alignment alignment = ALIGN_LEFT,
                              float lineSpacing = 1.5f,
                              bool noTopMargin = false);

    void renderTextCache(TextCache* cache);

    // Inserts newlines to make text wrap properly and also abbreviates single-line text.
    std::string wrapText(const std::string& text,
                         const float maxLength,
                         const float maxHeight = 0.0f,
                         const float lineSpacing = 1.5f,
                         const bool multiLine = false);

    // Returns the position of the cursor after moving it to the stop position.
    glm::vec2 getWrappedTextCursorOffset(const std::string& wrappedText,
                                         const size_t stop,
                                         const float lineSpacing = 1.5f);

    // Return overall height including line spacing.
    float getHeight(float lineSpacing = 1.5f) const { return mMaxGlyphHeight * lineSpacing; }
    float getLetterHeight();

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

    // Returns an approximation of VRAM used by this font's texture (in bytes).
    size_t getMemUsage() const;
    // Returns an approximation of total VRAM used by font textures (in bytes).
    static size_t getTotalMemUsage();

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
        // Initializes the OpenGL texture according to this FontTexture's settings,
        // updating textureId.
        void initTexture();

        // Deinitializes any existing OpenGL textures, is automatically called in destructor.
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
        glm::vec2 texSize; // In texels.
        glm::ivec2 advance;
        glm::ivec2 bearing;
        int rows;
    };

    struct FallbackFontCache {
        std::string path;
        std::shared_ptr<FontFace> face;
        hb_font_t* fontHB;
    };

    struct ShapeSegment {
        unsigned int startPos;
        unsigned int length;
        hb_font_t* fontHB;
        bool doShape;
        std::string substring;

        ShapeSegment()
            : startPos {0}
            , length {0}
            , fontHB {nullptr}
            , doShape {false}
        {
        }
    };

    // Completely recreate the texture data for all textures based on mGlyphs information.
    void rebuildTextures();
    void unloadTextures();

    void getTextureForNewGlyph(const glm::ivec2& glyphSize,
                               FontTexture*& texOut,
                               glm::ivec2& cursorOut);

    std::vector<FallbackFontCache> getFallbackFontPaths();
    FT_Face* getFaceForChar(unsigned int id);
    FT_Face* getFaceForGlyphIndex(unsigned int id, hb_font_t* fontArg);
    Glyph* getGlyph(const unsigned int id);
    Glyph* getGlyphByIndex(const unsigned int id, hb_font_t* fontArg);

    float getNewlineStartOffset(const std::string& text,
                                const unsigned int& charStart,
                                const float& xLen,
                                const Alignment& alignment);

    static inline FT_Library sLibrary {nullptr};
    static inline std::map<std::tuple<float, std::string>, std::weak_ptr<Font>> sFontMap;
    static inline std::vector<FallbackFontCache> sFallbackFonts;

    Renderer* mRenderer;
    std::unique_ptr<FontFace> mFontFace;
    std::vector<std::unique_ptr<FontTexture>> mTextures;
    std::map<unsigned int, Glyph> mGlyphMap;
    std::map<std::pair<unsigned int, hb_font_t*>, Glyph> mGlyphMapByIndex;

    const std::string mPath;
    hb_font_t* mFontHB;
    hb_font_t* mLastFontHB;
    hb_buffer_t* mBufHB;

    float mFontSize;
    float mLetterHeight;
    int mMaxGlyphHeight;
};

// Used to store a sort of "pre-rendered" string.
// When a TextCache is constructed (Font::buildTextCache()), the vertices and texture coordinates
// of the string are calculated and stored in the TextCache object. Rendering a previously
// constructed TextCache (Font::renderTextCache) every frame is much faster than rebuilding
// one every frame. Keep in mind you still need the Font object to render a TextCache (as the
// Font holds the OpenGL texture), and if a Font changes your TextCache may become invalid.
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
