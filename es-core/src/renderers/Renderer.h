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
    const unsigned int SHADER_DESATURATE {1};
    const unsigned int SHADER_OPACITY {2};
    const unsigned int SHADER_DIM {4};
    const unsigned int SHADER_BLUR_HORIZONTAL {8};
    const unsigned int SHADER_BLUR_VERTICAL {16};
    const unsigned int SHADER_SCANLINES {32};
    const unsigned int SHADER_BGRA_TO_RGBA {64};

    struct shaderParameters {
        std::array<GLfloat, 2> textureSize;
        std::array<GLfloat, 4> textureCoordinates;
        float fragmentSaturation;
        float fragmentDimValue;
        float fragmentOpacity;
        unsigned int blurPasses;

        shaderParameters()
            : textureSize {0.0f, 0.0f}
            , textureCoordinates {0.0f, 0.0f, 0.0f, 0.0f}
            , fragmentSaturation {1.0f}
            , fragmentDimValue {0.4f}
            , fragmentOpacity {1.0f}
            , blurPasses {1}
        {
        }
    };

    static std::vector<Shader*> sShaderProgramVector;
    static GLuint shaderFBO;
    // This is simply to get rid of a GCC false positive -Wunused-variable compiler warning.
    static GLuint shaderFBODummy = shaderFBO;

    static constexpr glm::mat4 getIdentity() { return glm::mat4 {1.0f}; }

#if !defined(NDEBUG)
#define GL_CHECK_ERROR(Function) (Function, _GLCheckError(#Function))

    static inline void _GLCheckError(const std::string& _funcName)
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
            ZERO,
            ONE,
            SRC_COLOR,
            ONE_MINUS_SRC_COLOR,
            SRC_ALPHA,
            ONE_MINUS_SRC_ALPHA,
            DST_COLOR,
            ONE_MINUS_DST_COLOR,
            DST_ALPHA,
            ONE_MINUS_DST_ALPHA
        };
    }

    namespace Texture
    {
        enum Type {
            RGBA, // Replace with AllowShortEnumsOnASingleLine: false (clang-format >=11.0).
            BGRA,
            ALPHA
        };
    }

    struct Rect {
        Rect(const int xValue, const int yValue, const int wValue, const int hValue)
            : x(xValue)
            , y(yValue)
            , w(wValue)
            , h(hValue)
        {
        }
        int x;
        int y;
        int w;
        int h;
    };

    struct Vertex {
        Vertex() {}
        Vertex(const glm::vec2& position, const glm::vec2& textureCoord, const unsigned int color)
            : pos(position)
            , tex(textureCoord)
            , col(color)
        {
        }
        glm::vec2 pos;
        glm::vec2 tex;
        unsigned int col;
        float saturation {1.0};
        float opacity {1.0};
        unsigned int shaders {0};
    };

    bool init();
    void deinit();
    void pushClipRect(const glm::ivec2& pos, const glm::ivec2& size);
    void popClipRect();
    void drawRect(const float x,
                  const float y,
                  const float w,
                  const float h,
                  const unsigned int color,
                  const unsigned int colorEnd,
                  bool horizontalGradient = false,
                  const float opacity = 1.0,
                  const glm::mat4& trans = getIdentity(),
                  const Blend::Factor srcBlendFactor = Blend::SRC_ALPHA,
                  const Blend::Factor dstBlendFactor = Blend::ONE_MINUS_SRC_ALPHA);
    SDL_Window* getSDLWindow();
    const int getWindowWidth();
    const int getWindowHeight();
    const int getScreenWidth();
    const int getScreenHeight();
    const int getScreenOffsetX();
    const int getScreenOffsetY();
    const int getScreenRotate();
    const float getScreenWidthModifier();
    const float getScreenHeightModifier();
    const float getScreenAspectRatio();

    const unsigned int convertRGBAToABGR(const unsigned int color);
    const unsigned int convertABGRToRGBA(const unsigned int color);

    Shader* getShaderProgram(unsigned int shaderID);
    const glm::mat4& getProjectionMatrix();
    void shaderPostprocessing(const unsigned int shaders,
                              const Renderer::shaderParameters& parameters = shaderParameters(),
                              unsigned char* textureRGBA = nullptr);

    void setupWindow();
    bool createContext();
    void destroyContext();
    unsigned int createTexture(const Texture::Type type,
                               const Texture::Type format,
                               const bool linearMinify,
                               const bool linearMagnify,
                               const bool repeat,
                               const unsigned int width,
                               const unsigned int height,
                               void* data);
    void destroyTexture(const unsigned int texture);
    void updateTexture(const unsigned int texture,
                       const Texture::Type type,
                       const unsigned int x,
                       const unsigned int y,
                       const unsigned int width,
                       const unsigned int height,
                       void* data);
    void bindTexture(const unsigned int texture);
    void drawLines(const Vertex* vertices,
                   const unsigned int numVertices,
                   const Blend::Factor srcBlendFactor = Blend::SRC_ALPHA,
                   const Blend::Factor dstBlendFactor = Blend::ONE_MINUS_SRC_ALPHA);
    void drawTriangleStrips(const Vertex* vertices,
                            const unsigned int numVertices,
                            const glm::mat4& trans = getIdentity(),
                            const Blend::Factor srcBlendFactor = Blend::SRC_ALPHA,
                            const Blend::Factor dstBlendFactor = Blend::ONE_MINUS_SRC_ALPHA,
                            const shaderParameters& parameters = shaderParameters());
    void setProjection(const glm::mat4& projection);
    void setMatrix(const glm::mat4& matrix);
    void setViewport(const Rect& viewport);
    void setScissor(const Rect& scissor);
    void setSwapInterval();
    void swapBuffers();

} // namespace Renderer

#endif // ES_CORE_RENDERER_RENDERER_H
