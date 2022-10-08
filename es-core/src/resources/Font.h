//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Font.h
//
//  Loading, unloading, caching and rendering of fonts.
//  Also functions for word wrapping and similar.
//

#ifndef ES_CORE_RESOURCES_FONT_H
#define ES_CORE_RESOURCES_FONT_H

#include "GuiComponent.h"
#include "ThemeData.h"
#include "renderers/Renderer.h"
#include "resources/ResourceManager.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <vector>

class TextCache;

// clang-format off
#define FONT_SIZE_MINI (static_cast<unsigned int>(0.030f * \
        std::min(Renderer::getScreenHeight(), Renderer::getScreenWidth())))
#define FONT_SIZE_SMALL (static_cast<unsigned int>(0.035f * \
        std::min(Renderer::getScreenHeight(), Renderer::getScreenWidth())))
#define FONT_SIZE_MEDIUM (static_cast<unsigned int>(0.045f * \
        std::min(Renderer::getScreenHeight(), Renderer::getScreenWidth())))
#define FONT_SIZE_LARGE (static_cast<unsigned int>(0.085f * \
        std::min(Renderer::getScreenHeight(), Renderer::getScreenWidth())))
// clang-format on

#define FONT_PATH_LIGHT ":/fonts/Akrobat-Regular.ttf"
#define FONT_PATH_REGULAR ":/fonts/Akrobat-SemiBold.ttf"
#define FONT_PATH_BOLD ":/fonts/Akrobat-Bold.ttf"

// A TrueType Font renderer that uses FreeType and OpenGL.
// The library is automatically initialized when it's needed.
class Font : public IReloadable
{
public:
    virtual ~Font();
    static void initLibrary();
    std::vector<std::string> getFallbackFontPaths();
    static std::shared_ptr<Font> get(int size, const std::string& path = getDefaultPath());

    // Returns the expected size of a string when rendered. Extra spacing is applied to the Y axis.
    glm::vec2 sizeText(std::string text, float lineSpacing = 1.5f);

    // Returns the portion of a string that fits within the passed argument maxWidth.
    std::string getTextMaxWidth(std::string text, float maxWidth);

    // Returns the size of the overall text area.
    const glm::vec2 getTextSize() { return mTextSize; }

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

    float getHeight(float lineSpacing = 1.5f) const;
    float getLetterHeight();

    void reload(ResourceManager& rm) override { rebuildTextures(); }
    void unload(ResourceManager& rm) override { unloadTextures(); }

    int getSize() const { return mFontSize; }
    const std::string& getPath() const { return mPath; }
    static std::string getDefaultPath() { return FONT_PATH_REGULAR; }

    static std::shared_ptr<Font> getFromTheme(const ThemeData::ThemeElement* elem,
                                              unsigned int properties,
                                              const std::shared_ptr<Font>& orig);

    // Returns an approximation of VRAM used by this font's texture (in bytes).
    size_t getMemUsage() const;
    // Returns an approximation of total VRAM used by font textures (in bytes).
    static size_t getTotalMemUsage();

private:
    Renderer* mRenderer;
    static FT_Library sLibrary;
    static std::map<std::pair<std::string, int>, std::weak_ptr<Font>> sFontMap;

    Font(int size, const std::string& path);

    struct FontTexture {
        unsigned int textureId;
        glm::ivec2 textureSize;

        glm::ivec2 writePos;
        int rowHeight;

        FontTexture(const int mFontSize);
        ~FontTexture();
        bool findEmpty(const glm::ivec2& size, glm::ivec2& cursor_out);

        // You must call initTexture() after creating a FontTexture to get a textureId.
        // Initializes the OpenGL texture according to this FontTexture's settings,
        // updating textureId.
        void initTexture();

        // Deinitializes the OpenGL texture if any exists, is automatically called
        // in the destructor.
        void deinitTexture();
    };

    struct FontFace {
        const ResourceData data;
        FT_Face face;

        FontFace(ResourceData&& d, int size);
        virtual ~FontFace();
    };

    // Completely recreate the texture data for all textures based on mGlyphs information.
    void rebuildTextures();
    void unloadTextures();

    void getTextureForNewGlyph(const glm::ivec2& glyphSize,
                               FontTexture*& tex_out,
                               glm::ivec2& cursor_out);

    std::map<unsigned int, std::unique_ptr<FontFace>> mFaceCache;
    FT_Face getFaceForChar(unsigned int id);
    void clearFaceCache() { mFaceCache.clear(); }

    struct Glyph {
        FontTexture* texture;

        glm::vec2 texPos;
        glm::vec2 texSize; // In texels.

        glm::vec2 advance;
        glm::vec2 bearing;
    };

    std::vector<FontTexture> mTextures;
    std::map<unsigned int, Glyph> mGlyphMap;
    Glyph* getGlyph(const unsigned int id);

    int mFontSize;
    int mMaxGlyphHeight;
    glm::vec2 mTextSize;
    const std::string mPath;

    float getNewlineStartOffset(const std::string& text,
                                const unsigned int& charStart,
                                const float& xLen,
                                const Alignment& alignment);

    friend TextCache;
};

// Used to store a sort of "pre-rendered" string.
// When a TextCache is constructed (Font::buildTextCache()), the vertices and texture coordinates
// of the string are calculated and stored in the TextCache object. Rendering a previously
// constructed TextCache (Font::renderTextCache) every frame is MUCH faster than rebuilding
// one every frame. Keep in mind you still need the Font object to render a TextCache (as the
// Font holds the OpenGL texture), and if a Font changes your TextCache may become invalid.
class TextCache
{
public:
    struct CacheMetrics {
        glm::vec2 size;
    } metrics;

    void setColor(unsigned int color);
    void setOpacity(float opacity);
    void setDimming(float dimming);

    friend Font;

protected:
    struct VertexList {
        std::vector<Renderer::Vertex> verts;
        // This is a pointer because the texture ID can change during
        // deinit/reinit (when launching a game).
        unsigned int* textureIdPtr;
    };

    std::vector<VertexList> vertexLists;
};

#endif // ES_CORE_RESOURCES_FONT_H
