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

#include "math/Vector2f.h"
#include "math/Vector2i.h"
#include "renderers/Renderer.h"
#include "resources/ResourceManager.h"
#include "ThemeData.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <vector>

class TextCache;

#define FONT_SIZE_MINI (static_cast<unsigned int>(0.030f * \
        std::min(Renderer::getScreenHeight(), Renderer::getScreenWidth())))
#define FONT_SIZE_SMALL (static_cast<unsigned int>(0.035f * \
        std::min(Renderer::getScreenHeight(), Renderer::getScreenWidth())))
#define FONT_SIZE_MEDIUM (static_cast<unsigned int>(0.045f * \
        std::min(Renderer::getScreenHeight(), Renderer::getScreenWidth())))
#define FONT_SIZE_LARGE (static_cast<unsigned int>(0.085f * \
        std::min(Renderer::getScreenHeight(), Renderer::getScreenWidth())))

#define FONT_PATH_LIGHT ":/fonts/opensans_hebrew_condensed_light.ttf"
#define FONT_PATH_REGULAR ":/fonts/opensans_hebrew_condensed_regular.ttf"

enum Alignment {
    ALIGN_LEFT,
    ALIGN_CENTER, // Centers both horizontally and vertically.
    ALIGN_RIGHT,
    ALIGN_TOP,
    ALIGN_BOTTOM
};

// A TrueType Font renderer that uses FreeType and OpenGL.
// The library is automatically initialized when it's needed.
class Font : public IReloadable
{
public:
    static void initLibrary();

    static std::shared_ptr<Font> get(int size, const std::string& path = getDefaultPath());

    virtual ~Font();

    // Returns the expected size of a string when rendered. Extra spacing is applied to the Y axis.
    Vector2f sizeText(std::string text, float lineSpacing = 1.5f);
    TextCache* buildTextCache(const std::string& text, float offsetX,
            float offsetY, unsigned int color, float lineSpacing = 1.5f);
    TextCache* buildTextCache(
            const std::string& text,
            Vector2f offset,
            unsigned int color,
            float xLen,
            Alignment alignment = ALIGN_LEFT,
            float lineSpacing = 1.5f);
    void renderTextCache(TextCache* cache);

    // Inserts newlines into text to make it wrap properly.
    std::string wrapText(std::string text, float xLen);
    // Returns the expected size of a string after wrapping is applied.
    Vector2f sizeWrappedText(std::string text, float xLen, float lineSpacing = 1.5f);
    // Returns the position of of the cursor after moving "cursor" characters.
    Vector2f getWrappedTextCursorOffset(std::string text, float xLen, size_t cursor,
            float lineSpacing = 1.5f);

    float getHeight(float lineSpacing = 1.5f) const;
    float getLetterHeight();

    void unload(std::shared_ptr<ResourceManager>& rm) override;
    void reload(std::shared_ptr<ResourceManager>& rm) override;

    int getSize() const;
    inline const std::string& getPath() const { return mPath; }

    inline static std::string getDefaultPath() { return FONT_PATH_REGULAR; }

    static std::shared_ptr<Font> getFromTheme(const ThemeData::ThemeElement* elem,
            unsigned int properties, const std::shared_ptr<Font>& orig);

    // Returns an approximation of VRAM used by this font's texture (in bytes).
    size_t getMemUsage() const;
    // Returns an approximation of total VRAM used by font textures (in bytes).
    static size_t getTotalMemUsage();

private:
    static FT_Library sLibrary;
    static std::map<std::pair<std::string, int>, std::weak_ptr<Font>> sFontMap;

    Font(int size, const std::string& path);

    struct FontTexture {
        unsigned int textureId;
        Vector2i textureSize;

        Vector2i writePos;
        int rowHeight;

        FontTexture(const int mSize);
        ~FontTexture();
        bool findEmpty(const Vector2i& size, Vector2i& cursor_out);

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

    void rebuildTextures();
    void unloadTextures();

    std::vector<FontTexture> mTextures;

    void getTextureForNewGlyph(const Vector2i& glyphSize, FontTexture*& tex_out,
            Vector2i& cursor_out);

    std::map< unsigned int, std::unique_ptr<FontFace> > mFaceCache;
    FT_Face getFaceForChar(unsigned int id);
    void clearFaceCache();

    struct Glyph {
        FontTexture* texture;

        Vector2f texPos;
        Vector2f texSize; // In texels!

        Vector2f advance;
        Vector2f bearing;
    };

    std::map<unsigned int, Glyph> mGlyphMap;

    Glyph* getGlyph(unsigned int id);

    int mMaxGlyphHeight;

    const int mSize;
    const std::string mPath;

    float getNewlineStartOffset(const std::string& text, const unsigned int& charStart,
            const float& xLen, const Alignment& alignment);

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
protected:
    struct VertexList {
        std::vector<Renderer::Vertex> verts;
        // This is a pointer because the texture ID can change during
        // deinit/reinit (when launching a game).
        unsigned int* textureIdPtr;
    };

    std::vector<VertexList> vertexLists;

public:
    struct CacheMetrics {
        Vector2f size;
    } metrics;

    void setColor(unsigned int color);

    friend Font;
};

#endif // ES_CORE_RESOURCES_FONT_H
