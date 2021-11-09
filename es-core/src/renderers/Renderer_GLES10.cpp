//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Renderer_GLES10.cpp
//
//  OpenGL ES 1.0 rendering functions.
//

#if defined(USE_OPENGLES_10)

#include "Log.h"
#include "Settings.h"
#include "renderers/Renderer.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengles.h>

namespace Renderer
{
    static SDL_GLContext sdlContext = nullptr;
    static GLuint whiteTexture = 0;

    inline GLenum convertBlendFactor(const Blend::Factor _blendFactor)
    {
        // clang-format off
        switch (_blendFactor) {
            case Blend::ZERO:                { return GL_ZERO;                } break;
            case Blend::ONE:                 { return GL_ONE;                 } break;
            case Blend::SRC_COLOR:           { return GL_SRC_COLOR;           } break;
            case Blend::ONE_MINUS_SRC_COLOR: { return GL_ONE_MINUS_SRC_COLOR; } break;
            case Blend::SRC_ALPHA:           { return GL_SRC_ALPHA;           } break;
            case Blend::ONE_MINUS_SRC_ALPHA: { return GL_ONE_MINUS_SRC_ALPHA; } break;
            case Blend::DST_COLOR:           { return GL_DST_COLOR;           } break;
            case Blend::ONE_MINUS_DST_COLOR: { return GL_ONE_MINUS_DST_COLOR; } break;
            case Blend::DST_ALPHA:           { return GL_DST_ALPHA;           } break;
            case Blend::ONE_MINUS_DST_ALPHA: { return GL_ONE_MINUS_DST_ALPHA; } break;
            default:                         { return GL_ZERO;                }
        }
        // clang-format on
    }

    inline GLenum convertTextureType(const Texture::Type _type)
    {
        // clang-format off
        switch (_type) {
            case Texture::RGBA:  { return GL_RGBA;  } break;
            case Texture::ALPHA: { return GL_ALPHA; } break;
            default:             { return GL_ZERO;  }
        }
        // clang-format on
    }

    void setupWindow()
    {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    }

    bool createContext()
    {
        sdlContext = SDL_GL_CreateContext(getSDLWindow());
        SDL_GL_MakeCurrent(getSDLWindow(), sdlContext);

        std::string vendor =
            glGetString(GL_VENDOR) ? reinterpret_cast<const char*>(glGetString(GL_VENDOR)) : "";
        std::string renderer =
            glGetString(GL_RENDERER) ? reinterpret_cast<const char*>(glGetString(GL_RENDERER)) : "";
        std::string version =
            glGetString(GL_VERSION) ? reinterpret_cast<const char*>(glGetString(GL_VERSION)) : "";
        std::string extensions = glGetString(GL_EXTENSIONS) ?
                                     reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS)) :
                                     "";

        LOG(LogInfo) << "GL vendor: " << vendor;
        LOG(LogInfo) << "GL renderer: " << renderer;
        LOG(LogInfo) << "GL version: " << version;
        LOG(LogInfo) << "EmulationStation renderer: OpenGL ES 1.0";
        LOG(LogInfo) << "Checking available OpenGL ES extensions...";
        std::string glExts = glGetString(GL_EXTENSIONS) ?
                                 reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS)) :
                                 "";
        LOG(LogInfo) << "GL_OES_texture_npot: "
                     << (extensions.find("GL_OES_texture_npot") != std::string::npos ? "OK" :
                                                                                       "MISSING");

        uint8_t data[4] = {255, 255, 255, 255};
        whiteTexture = createTexture(Texture::RGBA, false, false, true, 1, 1, data);

        GL_CHECK_ERROR(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
        GL_CHECK_ERROR(glEnable(GL_TEXTURE_2D));
        GL_CHECK_ERROR(glEnable(GL_BLEND));
        GL_CHECK_ERROR(glPixelStorei(GL_PACK_ALIGNMENT, 1));
        GL_CHECK_ERROR(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
        GL_CHECK_ERROR(glEnableClientState(GL_VERTEX_ARRAY));
        GL_CHECK_ERROR(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
        GL_CHECK_ERROR(glEnableClientState(GL_COLOR_ARRAY));

        return true;
    }

    void destroyContext()
    {
        SDL_GL_DeleteContext(sdlContext);
        sdlContext = nullptr;
    }

    unsigned int createTexture(const Texture::Type type,
                               const bool linearMinify,
                               const bool linearMagnify,
                               const bool repeat,
                               const unsigned int width,
                               const unsigned int height,
                               void* data)
    {
        const GLenum textureType = convertTextureType(type);
        unsigned int texture;

        GL_CHECK_ERROR(glGenTextures(1, &texture));
        GL_CHECK_ERROR(glBindTexture(GL_TEXTURE_2D, texture));

        GL_CHECK_ERROR(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                                       repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE));
        GL_CHECK_ERROR(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                                       repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE));
        GL_CHECK_ERROR(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                       linearMinify ? GL_LINEAR : GL_NEAREST));
        GL_CHECK_ERROR(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                                       linearMagnify ? GL_LINEAR : GL_NEAREST));

        GL_CHECK_ERROR(glTexImage2D(GL_TEXTURE_2D, 0, textureType, width, height, 0, textureType,
                                    GL_UNSIGNED_BYTE, data));

        return texture;
    }

    void destroyTexture(const unsigned int texture)
    {
        GL_CHECK_ERROR(glDeleteTextures(1, &texture));
    }

    void updateTexture(const unsigned int texture,
                       const Texture::Type type,
                       const unsigned int x,
                       const unsigned int y,
                       const unsigned int width,
                       const unsigned int height,
                       void* data)
    {
        const GLenum textureType = convertTextureType(type);

        GL_CHECK_ERROR(glBindTexture(GL_TEXTURE_2D, texture));
        GL_CHECK_ERROR(glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, textureType,
                                       GL_UNSIGNED_BYTE, data));
        GL_CHECK_ERROR(glBindTexture(GL_TEXTURE_2D, whiteTexture));
    }

    void bindTexture(const unsigned int texture)
    {
        if (texture == 0)
            GL_CHECK_ERROR(glBindTexture(GL_TEXTURE_2D, whiteTexture));
        else
            GL_CHECK_ERROR(glBindTexture(GL_TEXTURE_2D, texture));
    }

    void drawLines(const Vertex* vertices,
                   const unsigned int numVertices,
                   const Blend::Factor srcBlendFactor,
                   const Blend::Factor dstBlendFactor)
    {
        GL_CHECK_ERROR(glVertexPointer(2, GL_FLOAT, sizeof(Vertex), &vertices[0].pos));
        GL_CHECK_ERROR(glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), &vertices[0].tex));
        GL_CHECK_ERROR(glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(Vertex), &vertices[0].col));

        GL_CHECK_ERROR(
            glBlendFunc(convertBlendFactor(srcBlendFactor), convertBlendFactor(dstBlendFactor)));

        GL_CHECK_ERROR(glDrawArrays(GL_LINES, 0, numVertices));
    }

    void drawTriangleStrips(const Vertex* vertices,
                            const unsigned int numVertices,
                            const glm::mat4& trans,
                            const Blend::Factor srcBlendFactor,
                            const Blend::Factor dstBlendFactor,
                            const shaderParameters& parameters)
    {
        GL_CHECK_ERROR(glVertexPointer(2, GL_FLOAT, sizeof(Vertex), &vertices[0].pos));
        GL_CHECK_ERROR(glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), &vertices[0].tex));
        GL_CHECK_ERROR(glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(Vertex), &vertices[0].col));

        GL_CHECK_ERROR(
            glBlendFunc(convertBlendFactor(srcBlendFactor), convertBlendFactor(dstBlendFactor)));

        GL_CHECK_ERROR(glDrawArrays(GL_TRIANGLE_STRIP, 0, numVertices));
    }

    void setProjection(const glm::mat4& projection)
    {
        GL_CHECK_ERROR(glMatrixMode(GL_PROJECTION));
        GL_CHECK_ERROR(glLoadMatrixf((GLfloat*)&projection));
    }

    void setMatrix(const glm::mat4& matrix)
    {
        glm::mat4 newMatrix{matrix};
        newMatrix[3] = glm::round(newMatrix[3]);

        GL_CHECK_ERROR(glMatrixMode(GL_MODELVIEW));
        GL_CHECK_ERROR(glLoadMatrixf((GLfloat*)&newMatrix));
    }

    void setViewport(const Rect& viewport)
    {
        // glViewport starts at the bottom left of the window.
        GL_CHECK_ERROR(glViewport(viewport.x, getWindowHeight() - viewport.y - viewport.h,
                                  viewport.w, viewport.h));
    }

    void setScissor(const Rect& scissor)
    {
        if ((scissor.x == 0) && (scissor.y == 0) && (scissor.w == 0) && (scissor.h == 0)) {
            GL_CHECK_ERROR(glDisable(GL_SCISSOR_TEST));
        }
        else {
            // glScissor starts at the bottom left of the window.
            GL_CHECK_ERROR(glScissor(scissor.x, getWindowHeight() - scissor.y - scissor.h,
                                     scissor.w, scissor.h));
            GL_CHECK_ERROR(glEnable(GL_SCISSOR_TEST));
        }
    }

    void setSwapInterval()
    {
        if (Settings::getInstance()->getBool("VSync")) {
            // Adaptive VSync seems to be nonfunctional or having issues on some hardware
            // and drivers, so only attempt to apply regular VSync.
            if (SDL_GL_SetSwapInterval(1) == 0) {
                LOG(LogInfo) << "Enabling VSync...";
            }
            else {
                LOG(LogWarning) << "Could not enable VSync: " << SDL_GetError();
            }
        }
        else {
            SDL_GL_SetSwapInterval(0);
            LOG(LogInfo) << "Disabling VSync...";
        }
    }

    void swapBuffers()
    {
        SDL_GL_SwapWindow(getSDLWindow());
        GL_CHECK_ERROR(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
    }

} // namespace Renderer

#endif // USE_OPENGLES_10
