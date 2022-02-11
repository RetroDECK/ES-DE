//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Renderer_GLES20.cpp
//
//  OpenGL ES 2.0 rendering functions.
//

#if defined(USE_OPENGLES_20)

#include "Log.h"
#include "Settings.h"
#include "renderers/Renderer.h"
#include "utils/StringUtil.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengles2.h>

namespace Renderer
{
    static SDL_GLContext sdlContext {nullptr};
    static glm::mat4 projectionMatrix {getIdentity()};
    glm::mat4 worldViewMatrix {getIdentity()};
    static GLuint shaderProgram {0};
    static GLint mvpUniform {0};
    static GLint texAttrib {0};
    static GLint colAttrib {0};
    static GLint posAttrib {0};
    static GLuint vertexBuffer {0};
    static GLuint whiteTexture {0};

    static void setupShaders()
    {
        // Vertex shader.
        const GLchar* vertexSource {"uniform   mat4 u_mvp; \n"
                                    "attribute vec2 a_pos; \n"
                                    "attribute vec2 a_tex; \n"
                                    "attribute vec4 a_col; \n"
                                    "varying   vec2 v_tex; \n"
                                    "varying   vec4 v_col; \n"
                                    "void main(void)                                     \n"
                                    "{                                                   \n"
                                    "    gl_Position = u_mvp * vec4(a_pos.xy, 0.0, 1.0); \n"
                                    "    v_tex       = a_tex;                            \n"
                                    "    v_col       = a_col;                            \n"
                                    "}                                                   \n"};

        const GLuint vertexShader {glCreateShader(GL_VERTEX_SHADER)};
        GL_CHECK_ERROR(glShaderSource(vertexShader, 1, &vertexSource, nullptr));
        GL_CHECK_ERROR(glCompileShader(vertexShader));

        {
            GLint isCompiled {GL_FALSE};
            GLint maxLength {0};

            GL_CHECK_ERROR(glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &isCompiled));
            GL_CHECK_ERROR(glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &maxLength));

            if (maxLength > 1) {
                std::string infoLog(maxLength + 1, 0);

                GL_CHECK_ERROR(
                    glGetShaderInfoLog(vertexShader, maxLength, &maxLength, &infoLog[0]));
                if (isCompiled == GL_FALSE) {
                    LOG(LogError) << "GLSL Vertex Compile Error\n" << &infoLog[0];
                }
                else {
                    if (Utils::String::toUpper(infoLog).find("WARNING") != std::string::npos) {
                        LOG(LogWarning) << "GLSL Vertex Compile Warning\n" << infoLog;
                    }
                    else {
                        LOG(LogInfo) << "GLSL Vertex Compile Message\n" << infoLog;
                    }
                }
            }
        }

        // Fragment shader.
        const GLchar* fragmentSource {"precision highp float;     \n"
                                      "uniform   sampler2D u_tex; \n"
                                      "varying   vec2      v_tex; \n"
                                      "varying   vec4      v_col; \n"
                                      "void main(void)                                     \n"
                                      "{                                                   \n"
                                      "    gl_FragColor = texture2D(u_tex, v_tex) * v_col; \n"
                                      "}                                                   \n"};

        const GLuint fragmentShader {glCreateShader(GL_FRAGMENT_SHADER)};
        GL_CHECK_ERROR(glShaderSource(fragmentShader, 1, &fragmentSource, nullptr));
        GL_CHECK_ERROR(glCompileShader(fragmentShader));

        {
            GLint isCompiled {GL_FALSE};
            GLint maxLength {0};

            GL_CHECK_ERROR(glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &isCompiled));
            GL_CHECK_ERROR(glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &maxLength));

            if (maxLength > 1) {
                std::string infoLog(maxLength + 1, 0);

                GL_CHECK_ERROR(
                    glGetShaderInfoLog(fragmentShader, maxLength, &maxLength, &infoLog[0]));

                if (isCompiled == GL_FALSE) {
                    LOG(LogError) << "GLSL Fragment Compile Error\n" << infoLog;
                }
                else {
                    if (Utils::String::toUpper(infoLog).find("WARNING") != std::string::npos) {
                        LOG(LogWarning) << "GLSL Fragment Compile Warning\n" << infoLog;
                    }
                    else {
                        LOG(LogInfo) << "GLSL Fragment Compile Message\n" << infoLog;
                    }
                }
            }
        }

        // Shader program.
        shaderProgram = glCreateProgram();
        GL_CHECK_ERROR(glAttachShader(shaderProgram, vertexShader));
        GL_CHECK_ERROR(glAttachShader(shaderProgram, fragmentShader));
        GL_CHECK_ERROR(glLinkProgram(shaderProgram));

        {
            GLint isCompiled {GL_FALSE};
            GLint maxLength {0};

            GL_CHECK_ERROR(glGetProgramiv(shaderProgram, GL_LINK_STATUS, &isCompiled));
            GL_CHECK_ERROR(glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &maxLength));

            if (maxLength > 1) {
                std::string infoLog(maxLength + 1, 0);

                GL_CHECK_ERROR(
                    glGetProgramInfoLog(shaderProgram, maxLength, &maxLength, &infoLog[0]));

                if (isCompiled == GL_FALSE) {
                    LOG(LogError) << "GLSL Link Error\n" << infoLog;
                }
                else {
                    if (Utils::String::toUpper(infoLog).find("WARNING") != std::string::npos) {
                        LOG(LogWarning) << "GLSL Link Warning\n" << infoLog;
                    }
                    else {
                        LOG(LogInfo) << "GLSL Link Message\n" << infoLog;
                    }
                }
            }
        }

        GL_CHECK_ERROR(glUseProgram(shaderProgram));

        mvpUniform = glGetUniformLocation(shaderProgram, "u_mvp");
        posAttrib = glGetAttribLocation(shaderProgram, "a_pos");
        texAttrib = glGetAttribLocation(shaderProgram, "a_tex");
        colAttrib = glGetAttribLocation(shaderProgram, "a_col");
        GLint texUniform = glGetUniformLocation(shaderProgram, "u_tex");
        GL_CHECK_ERROR(glEnableVertexAttribArray(posAttrib));
        GL_CHECK_ERROR(glEnableVertexAttribArray(texAttrib));
        GL_CHECK_ERROR(glEnableVertexAttribArray(colAttrib));
        GL_CHECK_ERROR(glUniform1i(texUniform, 0));
    }

    static void setupVertexBuffer()
    {
        GL_CHECK_ERROR(glGenBuffers(1, &vertexBuffer));
        GL_CHECK_ERROR(glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer));
    }

    static GLenum convertBlendFactor(const Blend::Factor blendFactor)
    {
        // clang-format off
        switch (blendFactor) {
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

    static GLenum convertTextureType(const Texture::Type type)
    {
        // clang-format off
        switch (type) {
            case Texture::RGBA:  { return GL_RGBA;            } break;
            case Texture::BGRA:  { return GL_BGRA_EXT;        } break;
            case Texture::ALPHA: { return GL_LUMINANCE_ALPHA; } break;
            default:             { return GL_ZERO;            }
        }
        // clang-format on
    }

    void setupWindow()
    {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
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
        LOG(LogInfo) << "EmulationStation renderer: OpenGL ES 2.0";
        LOG(LogInfo) << "Checking available OpenGL ES extensions...";
        std::string glExts = glGetString(GL_EXTENSIONS) ?
                                 reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS)) :
                                 "";
        LOG(LogInfo) << "GL_OES_texture_npot: "
                     << (extensions.find("GL_OES_texture_npot") != std::string::npos ? "OK" :
                                                                                       "MISSING");

        setupShaders();
        setupVertexBuffer();

        uint8_t data[4] = {255, 255, 255, 255};
        whiteTexture = createTexture(Texture::RGBA, Texture::RGBA, false, false, true, 1, 1, data);

        GL_CHECK_ERROR(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
        GL_CHECK_ERROR(glActiveTexture(GL_TEXTURE0));
        GL_CHECK_ERROR(glEnable(GL_BLEND));
        GL_CHECK_ERROR(glPixelStorei(GL_PACK_ALIGNMENT, 1));
        GL_CHECK_ERROR(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));

        return true;
    }

    void destroyContext()
    {
        SDL_GL_DeleteContext(sdlContext);
        sdlContext = nullptr;
    }

    unsigned int createTexture(const Texture::Type type,
                               const Texture::Type format,
                               const bool linearMinify,
                               const bool linearMagnify,
                               const bool repeat,
                               const unsigned int width,
                               const unsigned int height,
                               void* data)
    {
        const GLenum textureType {convertTextureType(type)};
        unsigned int texture;

        GL_CHECK_ERROR(glGenTextures(1, &texture));
        GL_CHECK_ERROR(glBindTexture(GL_TEXTURE_2D, texture));

        // Not sure why the corresponding variables are missing in the OpenGL ES include files
        // when specifying the values manually seems to work with all graphics drivers.
        int _GL_TEXTURE_SWIZZLE_R {0x8E42};
        int _GL_TEXTURE_SWIZZLE_B {0x8E44};
        int _GL_RED {0x1903};
        int _GL_BLUE {0x1905};

        // Convert from BGRA to RGBA.
        if (format == Texture::Type::BGRA) {
            glTexParameteri(GL_TEXTURE_2D, _GL_TEXTURE_SWIZZLE_B, _GL_RED);
            glTexParameteri(GL_TEXTURE_2D, _GL_TEXTURE_SWIZZLE_R, _GL_BLUE);
        }
        GL_CHECK_ERROR(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                                       repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE));
        GL_CHECK_ERROR(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                                       repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE));
        GL_CHECK_ERROR(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                       linearMinify ? GL_LINEAR : GL_NEAREST));
        GL_CHECK_ERROR(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                                       linearMagnify ? GL_LINEAR : GL_NEAREST));

        // Regular GL_ALPHA textures are black + alpha when used in shaders, so create a
        // GL_LUMINANCE_ALPHA texture instead so it's white + alpha.
        if (textureType == GL_LUMINANCE_ALPHA) {
            uint8_t* a_data {reinterpret_cast<uint8_t*>(data)};
            uint8_t* la_data {new uint8_t[width * height * 2]};
            for (uint32_t i = 0; i < (width * height); ++i) {
                la_data[(i * 2) + 0] = 255;
                la_data[(i * 2) + 1] = a_data ? a_data[i] : 255;
            }

            GL_CHECK_ERROR(glTexImage2D(GL_TEXTURE_2D, 0, textureType, width, height, 0,
                                        textureType, GL_UNSIGNED_BYTE, la_data));

            delete[] la_data;
        }
        else {
            GL_CHECK_ERROR(glTexImage2D(GL_TEXTURE_2D, 0, textureType, width, height, 0,
                                        textureType, GL_UNSIGNED_BYTE, data));
        }

        return texture;
    }

    void destroyTexture(const unsigned int _texture)
    {
        GL_CHECK_ERROR(glDeleteTextures(1, &_texture));
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

        // Regular GL_ALPHA textures are black + alpha when used in shaders, so create a
        // GL_LUMINANCE_ALPHA texture instead so it's white + alpha.
        if (textureType == GL_LUMINANCE_ALPHA) {
            uint8_t* a_data {reinterpret_cast<uint8_t*>(data)};
            uint8_t* la_data {new uint8_t[width * height * 2]};
            for (uint32_t i = 0; i < (width * height); ++i) {
                la_data[(i * 2) + 0] = 255;
                la_data[(i * 2) + 1] = a_data ? a_data[i] : 255;
            }

            GL_CHECK_ERROR(glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, textureType,
                                           GL_UNSIGNED_BYTE, la_data));

            delete[] la_data;
        }
        else {
            GL_CHECK_ERROR(glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, textureType,
                                           GL_UNSIGNED_BYTE, data));
        }

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
        GL_CHECK_ERROR(glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                             reinterpret_cast<const void*>(offsetof(Vertex, pos))));
        GL_CHECK_ERROR(glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                             reinterpret_cast<const void*>(offsetof(Vertex, tex))));
        GL_CHECK_ERROR(glVertexAttribPointer(colAttrib, 4, GL_UNSIGNED_BYTE, GL_TRUE,
                                             sizeof(Vertex),
                                             reinterpret_cast<const void*>(offsetof(Vertex, col))));

        GL_CHECK_ERROR(
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * numVertices, vertices, GL_DYNAMIC_DRAW));
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
        GL_CHECK_ERROR(glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                             reinterpret_cast<const void*>(offsetof(Vertex, pos))));
        GL_CHECK_ERROR(glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                             reinterpret_cast<const void*>(offsetof(Vertex, tex))));
        GL_CHECK_ERROR(glVertexAttribPointer(colAttrib, 4, GL_UNSIGNED_BYTE, GL_TRUE,
                                             sizeof(Vertex),
                                             reinterpret_cast<const void*>(offsetof(Vertex, col))));

        GL_CHECK_ERROR(
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * numVertices, vertices, GL_DYNAMIC_DRAW));
        GL_CHECK_ERROR(
            glBlendFunc(convertBlendFactor(srcBlendFactor), convertBlendFactor(dstBlendFactor)));

        GL_CHECK_ERROR(glDrawArrays(GL_TRIANGLE_STRIP, 0, numVertices));
    }

    void setProjection(const glm::mat4& projection)
    {
        projectionMatrix = projection;

        glm::mat4 mvpMatrix {projectionMatrix * worldViewMatrix};
        GL_CHECK_ERROR(
            glUniformMatrix4fv(mvpUniform, 1, GL_FALSE, reinterpret_cast<float*>(&mvpMatrix)));
    }

    void setMatrix(const glm::mat4& matrix)
    {
        worldViewMatrix = matrix;
        worldViewMatrix[3] = glm::round(worldViewMatrix[3]);

        glm::mat4 mvpMatrix {projectionMatrix * worldViewMatrix};
        GL_CHECK_ERROR(
            glUniformMatrix4fv(mvpUniform, 1, GL_FALSE, reinterpret_cast<float*>(&mvpMatrix)));
    }

    void setViewport(const Rect& viewport)
    {
        // glViewport starts at the bottom left of the window.
        GL_CHECK_ERROR(glViewport(viewport.x,
                                  static_cast<GLint>(getWindowHeight()) - viewport.y - viewport.h,
                                  viewport.w, viewport.h));
    }

    void setScissor(const Rect& scissor)
    {
        if ((scissor.x == 0) && (scissor.y == 0) && (scissor.w == 0) && (scissor.h == 0)) {
            GL_CHECK_ERROR(glDisable(GL_SCISSOR_TEST));
        }
        else {
            // glScissor starts at the bottom left of the window.
            GL_CHECK_ERROR(glScissor(scissor.x,
                                     static_cast<GLint>(getWindowHeight()) - scissor.y - scissor.h,
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

#endif // USE_OPENGLES_20
