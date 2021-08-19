//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Renderer.h
//
//  General rendering functions.
//

#ifndef ES_CORE_RENDERER_RENDERER_H
#define ES_CORE_RENDERER_RENDERER_H

#include "Log.h"
#include "Shader_GL21.h"
#include "utils/MathUtil.h"

#include <string>
#include <vector>

struct SDL_Window;

namespace Renderer
{
    const unsigned int SHADER_DESATURATE = 1;
    const unsigned int SHADER_OPACITY = 2;
    const unsigned int SHADER_DIM = 4;
    const unsigned int SHADER_BLUR_HORIZONTAL = 8;
    const unsigned int SHADER_BLUR_VERTICAL = 16;
    const unsigned int SHADER_SCANLINES = 32;

    struct shaderParameters {
        std::array<GLfloat, 2> textureSize;
        std::array<GLfloat, 4> textureCoordinates;
        float fragmentSaturation;
        float fragmentDimValue;
        float fragmentOpacity;
        unsigned int blurPasses;

        shaderParameters()
            : textureSize({0.0f, 0.0f})
            , textureCoordinates({0.0f, 0.0f, 0.0f, 0.0f})
            , fragmentSaturation(1.0f)
            , fragmentDimValue(0.4f)
            , fragmentOpacity(1.0f)
            , blurPasses(1)
        {
        }
    };

    static std::vector<Shader*> sShaderProgramVector;
    static GLuint shaderFBO;
    static glm::mat4 mProjectionMatrix;
    static constexpr glm::mat4 getIdentity() { return glm::mat4{1.0f}; }

#if !defined(NDEBUG)
#define GL_CHECK_ERROR(Function) (Function, _GLCheckError(#Function))

    static void _GLCheckError(const std::string& _funcName)
    {
        const GLenum errorCode = glGetError();

        if (errorCode != GL_NO_ERROR) {
#if defined(USE_OPENGL_21)
            LOG(LogError) << "OpenGL error: " << _funcName << " failed with error code: 0x"
                          << std::hex << errorCode;
#else
            LOG(LogError) << "OpenGLES error: " << _funcName << " failed with error code: 0x"
                          << std::hex << errorCode;
#endif
        }
    }
#else
#define GL_CHECK_ERROR(Function) (Function)
#endif

    namespace Blend
    {
        enum Factor {
            ZERO = 0,
            ONE = 1,
            SRC_COLOR = 2,
            ONE_MINUS_SRC_COLOR = 3,
            SRC_ALPHA = 4,
            ONE_MINUS_SRC_ALPHA = 5,
            DST_COLOR = 6,
            ONE_MINUS_DST_COLOR = 7,
            DST_ALPHA = 8,
            ONE_MINUS_DST_ALPHA = 9
        };
    }

    namespace Texture
    {
        enum Type {
            RGBA = 0, // Replace with AllowShortEnumsOnASingleLine: false (clang-format >=11.0).
            ALPHA = 1
        };
    }

    struct Rect {
        Rect(const int _x, const int _y, const int _w, const int _h)
            : x(_x)
            , y(_y)
            , w(_w)
            , h(_h)
        {
        }
        int x;
        int y;
        int w;
        int h;
    };

    struct Vertex {
        Vertex() {}
        Vertex(const glm::vec2& _pos, const glm::vec2& _tex, const unsigned int _col)
            : pos(_pos)
            , tex(_tex)
            , col(_col)
        {
        }
        glm::vec2 pos;
        glm::vec2 tex;
        unsigned int col;
        float saturation = 1.0;
        float opacity = 1.0;
        unsigned int shaders = 0;
    };

    bool init();
    void deinit();
    void pushClipRect(const glm::ivec2& _pos, const glm::ivec2& _size);
    void popClipRect();
    void drawRect(const float _x,
                  const float _y,
                  const float _w,
                  const float _h,
                  const unsigned int _color,
                  const unsigned int _colorEnd,
                  bool horizontalGradient = false,
                  const float _opacity = 1.0,
                  const glm::mat4& _trans = getIdentity(),
                  const Blend::Factor _srcBlendFactor = Blend::SRC_ALPHA,
                  const Blend::Factor _dstBlendFactor = Blend::ONE_MINUS_SRC_ALPHA);
    SDL_Window* getSDLWindow();
    int getWindowWidth();
    int getWindowHeight();
    int getScreenWidth();
    int getScreenHeight();
    int getScreenOffsetX();
    int getScreenOffsetY();
    int getScreenRotate();
    float getScreenWidthModifier();
    float getScreenHeightModifier();
    float getScreenAspectRatio();

    unsigned int convertRGBAToABGR(unsigned int color);
    unsigned int convertABGRToRGBA(unsigned int color);

    Shader* getShaderProgram(unsigned int shaderID);
    const glm::mat4 getProjectionMatrix();
    void shaderPostprocessing(unsigned int shaders,
                              const Renderer::shaderParameters& parameters = shaderParameters(),
                              unsigned char* textureRGBA = nullptr);

    static unsigned int getWindowFlags() { return SDL_WINDOW_OPENGL; }

    void setupWindow();
    bool createContext();
    void destroyContext();
    unsigned int createTexture(const Texture::Type type,
                               const bool linearMinify,
                               const bool linearMagnify,
                               const bool repeat,
                               const unsigned int width,
                               const unsigned int height,
                               void* data);
    void destroyTexture(const unsigned int _texture);
    void updateTexture(const unsigned int _texture,
                       const Texture::Type _type,
                       const unsigned int _x,
                       const unsigned _y,
                       const unsigned int _width,
                       const unsigned int _height,
                       void* _data);
    void bindTexture(const unsigned int _texture);
    void drawLines(const Vertex* _vertices,
                   const unsigned int _numVertices,
                   const Blend::Factor _srcBlendFactor = Blend::SRC_ALPHA,
                   const Blend::Factor _dstBlendFactor = Blend::ONE_MINUS_SRC_ALPHA);
    void drawTriangleStrips(const Vertex* _vertices,
                            const unsigned int _numVertices,
                            const glm::mat4& _trans = getIdentity(),
                            const Blend::Factor _srcBlendFactor = Blend::SRC_ALPHA,
                            const Blend::Factor _dstBlendFactor = Blend::ONE_MINUS_SRC_ALPHA,
                            const shaderParameters& _parameters = shaderParameters());
    void setProjection(const glm::mat4& _projection);
    void setMatrix(const glm::mat4& _matrix);
    void setViewport(const Rect& _viewport);
    void setScissor(const Rect& _scissor);
    void setSwapInterval();
    void swapBuffers();

} // namespace Renderer

#endif // ES_CORE_RENDERER_RENDERER_H
