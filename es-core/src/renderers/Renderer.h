//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Renderer.h
//
//  General rendering functions.
//

#ifndef ES_CORE_RENDERER_RENDERER_H
#define ES_CORE_RENDERER_RENDERER_H

#include "math/Transform4x4f.h"
#include "math/Vector2f.h"
#include "Log.h"
#include "Shader_GL21.h"

#include <string>
#include <vector>

class  Transform4x4f;
class  Vector2i;
struct SDL_Window;

namespace Renderer
{
    const unsigned int SHADER_DESATURATE =          1;
    const unsigned int SHADER_OPACITY =             2;
    const unsigned int SHADER_DIM =                 4;
    const unsigned int SHADER_BLUR_HORIZONTAL =     8;
    const unsigned int SHADER_BLUR_VERTICAL =       16;
    const unsigned int SHADER_SCANLINES =           32;

    struct shaderParameters {
        std::array<GLfloat, 2> textureSize;
        std::array<GLfloat, 4> textureCoordinates;
        float fragmentSaturation;
        float fragmentDimValue;
        float fragmentOpacity;
        unsigned int shaderPasses;

        shaderParameters()
                : textureSize({0.0f, 0.0f}),
                textureCoordinates({0.0f, 0.0f, 0.0f, 0.0f}),
                fragmentSaturation(1.0f),
                fragmentDimValue(0.4f),
                fragmentOpacity(1.0f),
                shaderPasses(1)
                {};
    };

    static std::vector<Shader*> sShaderProgramVector;
    static GLuint shaderFBO;
    static Transform4x4f mProjectionMatrix;

    #if !defined(NDEBUG)
    #define GL_CHECK_ERROR(Function) (Function, _GLCheckError(#Function))

    static void _GLCheckError(const std::string& _funcName)
    {
        const GLenum errorCode = glGetError();

        if (errorCode != GL_NO_ERROR) {
        #if defined(USE_OPENGL_21)
            LOG(LogError) << "OpenGL error: " << _funcName <<
                    " failed with error code: 0x" << std::hex << errorCode;
        #else
            LOG(LogError) << "OpenGLES error: " << _funcName <<
                    " failed with error code: 0x" << std::hex << errorCode;
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
            RGBA  = 0,
            ALPHA = 1
        };
    }

    struct Rect {
        Rect(
                const int _x,
                const int _y,
                const int _w,
                const int _h)
                : x(_x),
                y(_y),
                w(_w),
                h(_h) {}
        int x;
        int y;
        int w;
        int h;
    };

    struct Vertex
    {
        Vertex() {}
        Vertex(
                const Vector2f& _pos,
                const Vector2f& _tex,
                const unsigned int _col)
                : pos(_pos),
                tex(_tex),
                col(_col) { }
        Vector2f pos;
        Vector2f tex;
        unsigned int col;
        float saturation = 1.0;
        float opacity = 1.0;
        unsigned int shaders = 0;
    };

    bool init();
    void deinit();
    void pushClipRect(const Vector2i& _pos, const Vector2i& _size);
    void popClipRect();
    void drawRect(
            const float _x,
            const float _y,
            const float _w,
            const float _h,
            const unsigned int _color,
            const unsigned int _colorEnd,
            bool horizontalGradient = false,
            const float _opacity = 1.0,
            const Transform4x4f& _trans = Transform4x4f::Identity(),
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

    unsigned int convertRGBAToABGR(unsigned int color);
    unsigned int convertABGRToRGBA(unsigned int color);

    Shader* getShaderProgram(unsigned int shaderID);
    const Transform4x4f getProjectionMatrix();
    void shaderPostprocessing(unsigned int shaders,
            const Renderer::shaderParameters& parameters = shaderParameters(),
            unsigned char* textureRGBA = nullptr);

    // API specific.
    unsigned int getWindowFlags();
    void setupWindow();
    bool createContext();
    void destroyContext();
    unsigned int createTexture(
            const Texture::Type _type,
            const bool _linear,
            const bool _repeat,
            const unsigned int _width,
            const unsigned int _height,
            void* _data);
    void destroyTexture(const unsigned int _texture);
    void updateTexture(
            const unsigned int _texture,
            const Texture::Type _type,
            const unsigned int _x,
            const unsigned _y,
            const unsigned int _width,
            const unsigned int _height,
            void* _data);
    void bindTexture(const unsigned int _texture);
    void drawLines(
            const Vertex* _vertices,
            const unsigned int _numVertices,
            const Blend::Factor _srcBlendFactor = Blend::SRC_ALPHA,
            const Blend::Factor _dstBlendFactor = Blend::ONE_MINUS_SRC_ALPHA);
    void drawTriangleStrips(
            const Vertex* _vertices,
            const unsigned int _numVertices,
            const Transform4x4f& _trans = Transform4x4f::Identity(),
            const Blend::Factor _srcBlendFactor = Blend::SRC_ALPHA,
            const Blend::Factor _dstBlendFactor = Blend::ONE_MINUS_SRC_ALPHA,
            const shaderParameters& _parameters = shaderParameters());
    void setProjection(const Transform4x4f& _projection);
    void setMatrix(const Transform4x4f& _matrix);
    void setViewport(const Rect& _viewport);
    void setScissor(const Rect& _scissor);
    void setSwapInterval();
    void swapBuffers();
}

#endif // ES_CORE_RENDERER_RENDERER_H
