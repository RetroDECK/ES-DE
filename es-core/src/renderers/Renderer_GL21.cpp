//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Renderer_GL21.cpp
//
//  OpenGL 2.1 rendering functions.
//

#if defined(USE_OPENGL_21)

#include "math/Transform4x4f.h"
#include "renderers/Renderer.h"
#include "Settings.h"
#include "Shader_GL21.h"

namespace Renderer
{
    static SDL_GLContext sdlContext = nullptr;
    static GLuint whiteTexture = 0;

    static GLenum convertBlendFactor(const Blend::Factor _blendFactor)
    {
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
    }

    static GLenum convertTextureType(const Texture::Type _type)
    {
        switch (_type) {
            case Texture::RGBA:  { return GL_RGBA;  } break;
            case Texture::ALPHA: { return GL_ALPHA; } break;
            default:             { return GL_ZERO;  }
        }
    }

    unsigned int getWindowFlags()
    {
        return SDL_WINDOW_OPENGL;
    }

    void setupWindow()
    {
        #if defined(__APPLE__)
        // This is required on macOS, as the operating system will otherwise insist on using
        // a newer OpenGL version which completely breaks the application.
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
        #else
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        #endif
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    }

    bool createContext()
    {
        bool missingExtension = false;
        sdlContext = SDL_GL_CreateContext(getSDLWindow());

        if (!sdlContext) {
            LOG(LogError) << "Error creating OpenGL context. " << SDL_GetError();
            return false;
        }

        #if defined(_WIN64)
        glewInit();
        #endif

        SDL_GL_MakeCurrent(getSDLWindow(), sdlContext);

        std::string vendor = glGetString(GL_VENDOR) ?
                reinterpret_cast<const char*>(glGetString(GL_VENDOR)) : "";
        std::string renderer = glGetString(GL_RENDERER) ?
                reinterpret_cast<const char*>(glGetString(GL_RENDERER)) : "";
        std::string version = glGetString(GL_VERSION) ?
                reinterpret_cast<const char*>(glGetString(GL_VERSION)) : "";
        std::string extensions = glGetString(GL_EXTENSIONS) ?
                reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS)) : "";

        LOG(LogInfo) << "GL vendor: " << vendor;
        LOG(LogInfo) << "GL renderer: " << renderer;
        LOG(LogInfo) << "GL version: " << version;
        #if defined(_WIN64)
        LOG(LogInfo) << "EmulationStation renderer: OpenGL 2.1 with GLEW";
        #else
        LOG(LogInfo) << "EmulationStation renderer: OpenGL 2.1";
        #endif
        LOG(LogInfo) << "Checking available OpenGL extensions...";
        std::string glExts = glGetString(GL_EXTENSIONS) ?
                reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS)) : "";
        if (extensions.find("GL_ARB_texture_non_power_of_two") == std::string::npos) {
            LOG(LogError) << "GL_ARB_texture_non_power_of_two: MISSING";
            missingExtension = true;
        }
        else {
            LOG(LogInfo) << "GL_ARB_texture_non_power_of_two: OK";
        }
        if (extensions.find("GL_ARB_vertex_shader") == std::string::npos) {
            LOG(LogError) << "GL_ARB_vertex_shader: MISSING";
            missingExtension = true;
        }
        else {
            LOG(LogInfo) << "GL_ARB_vertex_shader: OK";
        }
        if (extensions.find("GL_ARB_fragment_shader") == std::string::npos) {
            LOG(LogError) << "GL_ARB_fragment_shader: MISSING";
            missingExtension = true;
        }
        else {
            LOG(LogInfo) << "GL_ARB_fragment_shader: OK";
        }
        if (extensions.find("GL_EXT_framebuffer_blit") == std::string::npos) {
            LOG(LogError) << "GL_EXT_framebuffer_blit: MISSING";
            missingExtension = true;
        }
        else {
            LOG(LogInfo) << "GL_EXT_framebuffer_blit: OK";
        }
        if (missingExtension) {
            LOG(LogError) << "Required OpenGL extensions missing.";
            return false;
        }

        uint8_t data[4] = {255, 255, 255, 255};
        whiteTexture = createTexture(Texture::RGBA, false, true, 1, 1, data);

        GL_CHECK_ERROR(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
        GL_CHECK_ERROR(glEnable(GL_TEXTURE_2D));
        GL_CHECK_ERROR(glEnable(GL_BLEND));
        GL_CHECK_ERROR(glPixelStorei(GL_PACK_ALIGNMENT, 1));
        GL_CHECK_ERROR(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
        GL_CHECK_ERROR(glEnableClientState(GL_VERTEX_ARRAY));
        GL_CHECK_ERROR(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
        GL_CHECK_ERROR(glEnableClientState(GL_COLOR_ARRAY));

        // This is the framebuffer that will be used for shader rendering.
        GL_CHECK_ERROR(glGenFramebuffers(1, &shaderFBO));

        return true;
    }

    void destroyContext()
    {
        GL_CHECK_ERROR(glDeleteFramebuffers(1, &shaderFBO));
        SDL_GL_DeleteContext(sdlContext);
        sdlContext = nullptr;
    }

    unsigned int createTexture(
            const Texture::Type _type,
            const bool _linear,
            const bool _repeat,
            const unsigned int _width,
            const unsigned int _height,
            void* _data)
    {
        const GLenum type = convertTextureType(_type);
        unsigned int texture;

        GL_CHECK_ERROR(glGenTextures(1, &texture));
        GL_CHECK_ERROR(glBindTexture(GL_TEXTURE_2D, texture));

        GL_CHECK_ERROR(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, _repeat ?
                static_cast<GLfloat>(GL_REPEAT) : static_cast<GLfloat>(GL_CLAMP_TO_EDGE)));
        GL_CHECK_ERROR(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, _repeat ?
                static_cast<GLfloat>(GL_REPEAT) : static_cast<GLfloat>(GL_CLAMP_TO_EDGE)));
        GL_CHECK_ERROR(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, _linear ?
                static_cast<GLfloat>(GL_LINEAR) : static_cast<GLfloat>(GL_NEAREST)));
        GL_CHECK_ERROR(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));

        GL_CHECK_ERROR(glTexImage2D(GL_TEXTURE_2D, 0, type, _width, _height, 0, type,
                GL_UNSIGNED_BYTE, _data));

        return texture;
    }

    void destroyTexture(const unsigned int _texture)
    {
        GL_CHECK_ERROR(glDeleteTextures(1, &_texture));
    }

    void updateTexture(
            const unsigned int _texture,
            const Texture::Type _type,
            const unsigned int _x,
            const unsigned _y,
            const unsigned int _width,
            const unsigned int _height,
            void* _data)
    {
        const GLenum type = convertTextureType(_type);

        GL_CHECK_ERROR(glBindTexture(GL_TEXTURE_2D, _texture));
        GL_CHECK_ERROR(glTexSubImage2D(GL_TEXTURE_2D, 0, _x, _y, _width, _height,
                type, GL_UNSIGNED_BYTE, _data));
        GL_CHECK_ERROR(glBindTexture(GL_TEXTURE_2D, whiteTexture));
    }

    void bindTexture(const unsigned int _texture)
    {
        if (_texture == 0)
            GL_CHECK_ERROR(glBindTexture(GL_TEXTURE_2D, whiteTexture));
        else
            GL_CHECK_ERROR(glBindTexture(GL_TEXTURE_2D, _texture));
    }

    void drawLines(
            const Vertex* _vertices,
            const unsigned int _numVertices,
            const Blend::Factor _srcBlendFactor,
            const Blend::Factor _dstBlendFactor)
    {
        GL_CHECK_ERROR(glVertexPointer(2, GL_FLOAT, sizeof(Vertex), &_vertices[0].pos));
        GL_CHECK_ERROR(glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), &_vertices[0].tex));
        GL_CHECK_ERROR(glColorPointer( 4, GL_UNSIGNED_BYTE, sizeof(Vertex), &_vertices[0].col));

        GL_CHECK_ERROR(glBlendFunc(convertBlendFactor(_srcBlendFactor),
                convertBlendFactor(_dstBlendFactor)));

        GL_CHECK_ERROR(glDrawArrays(GL_LINES, 0, _numVertices));
    }

    void drawTriangleStrips(
            const Vertex* _vertices,
            const unsigned int _numVertices,
            const Transform4x4f& _trans,
            const Blend::Factor _srcBlendFactor,
            const Blend::Factor _dstBlendFactor,
            const shaderParameters& _parameters)
    {
        float width = _vertices[3].pos[0];
        float height = _vertices[3].pos[1];

        GL_CHECK_ERROR(glVertexPointer(2, GL_FLOAT, sizeof(Vertex), &_vertices[0].pos));
        GL_CHECK_ERROR(glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), &_vertices[0].tex));
        GL_CHECK_ERROR(glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(Vertex), &_vertices[0].col));

        GL_CHECK_ERROR(glBlendFunc(convertBlendFactor(_srcBlendFactor),
                convertBlendFactor(_dstBlendFactor)));

        #if defined(USE_OPENGL_21)
        if (_vertices[0].shaders == 0) {
            GL_CHECK_ERROR(glDrawArrays(GL_TRIANGLE_STRIP, 0, _numVertices));
        }
        else {
            // If saturation is set below the maximum (default) value, run the
            // desaturation shader.
            if (_vertices->saturation < 1.0 || _parameters.fragmentSaturation < 1.0) {
                Shader* runShader = getShaderProgram(SHADER_DESATURATE);
                // Only try to use the shader if it has been loaded properly.
                if (runShader) {
                    runShader->activateShaders();
                    runShader->setModelViewProjectionMatrix(getProjectionMatrix() * _trans);
                    runShader->setSaturation(_vertices->saturation);
                    GL_CHECK_ERROR(glDrawArrays(GL_TRIANGLE_STRIP, 0, _numVertices));
                    runShader->deactivateShaders();
                }
            }

            if (_vertices->shaders & SHADER_OPACITY) {
                Shader* runShader = getShaderProgram(SHADER_OPACITY);
                if (runShader) {
                    runShader->activateShaders();
                    runShader->setModelViewProjectionMatrix(getProjectionMatrix() * _trans);
                    _vertices->opacity < 1.0 ?
                            runShader->setOpacity(_vertices->opacity) :
                            runShader->setOpacity(_parameters.fragmentOpacity);
                    GL_CHECK_ERROR(glDrawArrays(GL_TRIANGLE_STRIP, 0, _numVertices));
                    runShader->deactivateShaders();
                }
            }

            // Check if any other shaders are set to be used and if so, run them.
            if (_vertices->shaders & SHADER_DIM) {
                Shader* runShader = getShaderProgram(SHADER_DIM);
                if (runShader) {
                    runShader->activateShaders();
                    runShader->setModelViewProjectionMatrix(getProjectionMatrix() * _trans);
                    runShader->setDimValue(_parameters.fragmentDimValue);
                    GL_CHECK_ERROR(glDrawArrays(GL_TRIANGLE_STRIP, 0, _numVertices));
                    runShader->deactivateShaders();
                }
            }

            if (_vertices->shaders & SHADER_BLUR_HORIZONTAL) {
                Shader* runShader = getShaderProgram(SHADER_BLUR_HORIZONTAL);
                if (runShader) {
                    runShader->activateShaders();
                    runShader->setModelViewProjectionMatrix(getProjectionMatrix() * _trans);
                    runShader->setTextureSize({width, height});
                    GL_CHECK_ERROR(glDrawArrays(GL_TRIANGLE_STRIP, 0, _numVertices));
                    runShader->deactivateShaders();
                }
            }

            if (_vertices->shaders & SHADER_BLUR_VERTICAL) {
                Shader* runShader = getShaderProgram(SHADER_BLUR_VERTICAL);
                if (runShader) {
                    runShader->activateShaders();
                    runShader->setModelViewProjectionMatrix(getProjectionMatrix() * _trans);
                    runShader->setTextureSize({width, height});
                    GL_CHECK_ERROR(glDrawArrays(GL_TRIANGLE_STRIP, 0, _numVertices));
                    runShader->deactivateShaders();
                }
            }

            if (_vertices->shaders & SHADER_SCANLINES) {
                Shader* runShader = getShaderProgram(SHADER_SCANLINES);
                float shaderWidth = width * 1.2f;
                // Scale the scanlines relative to screen resolution.
                float screenHeightModifier = getScreenHeightModifier();
                float relativeHeight = height / getScreenHeight();
                float shaderHeight = 0.0f;
                if (relativeHeight == 1.0f) {
                    // Full screen.
                    float modifier = 1.30f - (0.1f * screenHeightModifier);
                    shaderHeight = height * modifier;
                }
                else {
                    // Portion of screen, e.g. gamelist view.
                    // Average the relative width and height to avoid applying exaggerated
                    // scanlines to videos with non-standard aspect ratios.
                    float relativeWidth = width / getScreenWidth();
                    float relativeAdjustment = (relativeWidth + relativeHeight) / 2.0f;
                    float modifier = 1.41f + relativeAdjustment / 7.0f -
                            (0.14f * screenHeightModifier);
                    shaderHeight = height * modifier;
                }
                if (runShader) {
                    runShader->activateShaders();
                    runShader->setModelViewProjectionMatrix(getProjectionMatrix() * _trans);
                    runShader->setTextureSize({shaderWidth, shaderHeight});
                    GL_CHECK_ERROR(glDrawArrays(GL_TRIANGLE_STRIP, 0, _numVertices));
                    runShader->deactivateShaders();
                }
            }
        }
        #endif
    }

    void setProjection(const Transform4x4f& _projection)
    {
        GL_CHECK_ERROR(glMatrixMode(GL_PROJECTION));
        GL_CHECK_ERROR(glLoadMatrixf(reinterpret_cast<const GLfloat*>(&_projection)));
    }

    void setMatrix(const Transform4x4f& _matrix)
    {
        Transform4x4f matrix = _matrix;
        matrix.round();

        GL_CHECK_ERROR(glMatrixMode(GL_MODELVIEW));
        GL_CHECK_ERROR(glLoadMatrixf(reinterpret_cast<const GLfloat*>(&matrix)));
    }

    void setViewport(const Rect& _viewport)
    {
        // glViewport starts at the bottom left of the window.
        GL_CHECK_ERROR(glViewport( _viewport.x, getWindowHeight() -
                _viewport.y - _viewport.h, _viewport.w, _viewport.h));
    }

    void setScissor(const Rect& _scissor)
    {
        if ((_scissor.x == 0) && (_scissor.y == 0) && (_scissor.w == 0) && (_scissor.h == 0)) {
            GL_CHECK_ERROR(glDisable(GL_SCISSOR_TEST));
        }
        else {
            // glScissor starts at the bottom left of the window.
            GL_CHECK_ERROR(glScissor(_scissor.x, getWindowHeight() -
                    _scissor.y - _scissor.h, _scissor.w, _scissor.h));
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

    void shaderPostprocessing(unsigned int shaders, const Renderer::shaderParameters& parameters,
            unsigned char* textureRGBA)
    {
        Vertex vertices[4];
        std::vector<unsigned int>shaderList;
        GLuint width = getScreenWidth();
        GLuint height = getScreenHeight();
        float widthf = static_cast<float>(width);
        float heightf = static_cast<float>(height);

        // Set vertex positions and texture coordinates to full screen as all
        // postprocessing is applied to the complete screen area.
        vertices[0] = { { 0, 0 }, { 0, 1 }, 0 };
        vertices[1] = { { 0, heightf }, { 0, 0 }, 0 };
        vertices[2] = { { widthf, 0 }, { 1, 1 }, 0 };
        vertices[3] = { { widthf, heightf }, { 1, 0 }, 0};

        if (shaders & Renderer::SHADER_DESATURATE)
            shaderList.push_back(Renderer::SHADER_DESATURATE);
        if (shaders & Renderer::SHADER_OPACITY)
            shaderList.push_back(Renderer::SHADER_OPACITY);
        if (shaders & Renderer::SHADER_DIM)
            shaderList.push_back(Renderer::SHADER_DIM);
        if (shaders & Renderer::SHADER_BLUR_HORIZONTAL)
            shaderList.push_back(Renderer::SHADER_BLUR_HORIZONTAL);
        if (shaders & Renderer::SHADER_BLUR_VERTICAL)
            shaderList.push_back(Renderer::SHADER_BLUR_VERTICAL);
        if (shaders & Renderer::SHADER_SCANLINES)
            shaderList.push_back(Renderer::SHADER_SCANLINES);

        if (parameters.fragmentSaturation < 1.0)
            vertices[0].saturation = parameters.fragmentSaturation;

        setMatrix(Transform4x4f::Identity());
        GLuint screenTexture = createTexture(Texture::RGBA, false, false, width, height, nullptr);

        GL_CHECK_ERROR(glBindFramebuffer(GL_READ_FRAMEBUFFER, 0));

        for (int i = 0; i < shaderList.size(); i++) {
            vertices[0].shaders = shaderList[i];
            int shaderPasses = 1;
            // For the blur shaders there is an optional variable to set the number of passes
            // to execute, which proportionally affects the blur amount.
            if (shaderList[i] == Renderer::SHADER_BLUR_HORIZONTAL ||
                    shaderList[i] == Renderer::SHADER_BLUR_VERTICAL)
                shaderPasses = parameters.blurPasses;

            for (int p = 0; p < shaderPasses; p++) {
                GL_CHECK_ERROR(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, shaderFBO));

                // Attach the texture to the shader framebuffer.
                GL_CHECK_ERROR(glFramebufferTexture2D(
                        GL_FRAMEBUFFER,
                        GL_COLOR_ATTACHMENT0,
                        GL_TEXTURE_2D,
                        screenTexture,
                        0));

                // Blit the screen contents to screenTexture.
                GL_CHECK_ERROR(glBlitFramebuffer(0, 0, width, height, 0, 0, width, height,
                        GL_COLOR_BUFFER_BIT, GL_NEAREST));

                // Apply/render the shaders.
                drawTriangleStrips(vertices, 4, Transform4x4f::Identity(),
                        Blend::SRC_ALPHA, Blend::ONE_MINUS_SRC_ALPHA, parameters);

                // If textureRGBA has an address, it means that the output should go to this
                // texture rather than to the screen. The glReadPixels() function is slow, but
                // since this will typically only run every now and then to create a cached
                // screen texture, it doesn't really matter.
                if (textureRGBA) {
                    GL_CHECK_ERROR(glBindFramebuffer(GL_READ_FRAMEBUFFER, shaderFBO));
                    GL_CHECK_ERROR(glReadPixels(0, 0, width, height,
                            GL_RGBA, GL_UNSIGNED_BYTE, textureRGBA));
                    GL_CHECK_ERROR(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));
                }
                else {
                    // Blit the resulting postprocessed texture back to the primary framebuffer.
                    GL_CHECK_ERROR(glBindFramebuffer(GL_READ_FRAMEBUFFER, shaderFBO));
                    GL_CHECK_ERROR(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));
                    GL_CHECK_ERROR(glBlitFramebuffer(0, 0, width, height, 0, 0, width, height,
                            GL_COLOR_BUFFER_BIT, GL_NEAREST));
                }
            }
        }

        GL_CHECK_ERROR(glBindFramebuffer(GL_READ_FRAMEBUFFER, 0));
        destroyTexture(screenTexture);
    }

} // Renderer::

#endif // USE_OPENGL_21
