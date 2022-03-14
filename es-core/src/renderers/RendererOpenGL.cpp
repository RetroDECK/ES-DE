//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  RendererOpenGL.cpp
//
//  OpenGL / OpenGL ES renderering functions.
//

#include "renderers/RendererOpenGL.h"

#include "Settings.h"

#if defined(__APPLE__)
#include <chrono>
#endif

RendererOpenGL::RendererOpenGL() noexcept
    : mShaderFBO1 {0}
    , mShaderFBO2 {0}
    , mVertexBuffer1 {0}
    , mVertexBuffer2 {0}
    , mSDLContext {nullptr}
    , mWhiteTexture {0}
    , mPostProcTexture1 {0}
    , mPostProcTexture2 {0}
    , mCoreShader {nullptr}
    , mBlurHorizontalShader {nullptr}
    , mBlurVerticalShader {nullptr}
    , mScanlinelShader {nullptr}
    , mLastShader {nullptr}
{
}

RendererOpenGL::~RendererOpenGL()
{
    for (auto it = mShaderProgramVector.cbegin(); it != mShaderProgramVector.cend(); ++it)
        delete *it;
}

RendererOpenGL* RendererOpenGL::getInstance()
{
    static RendererOpenGL instance;
    return &instance;
}

ShaderOpenGL* RendererOpenGL::getShaderProgram(unsigned int shaderID)
{
    unsigned int index {0};

    // Find the index in mShaderProgramVector by counting the number
    // of shifts required to reach 0.
    while (shaderID > 0) {
        shaderID = shaderID >> 1;
        ++index;
    }

    if (mShaderProgramVector.size() > index - 1)
        return mShaderProgramVector[index - 1];
    else
        return nullptr;
}

bool RendererOpenGL::loadShaders()
{
    LOG(LogInfo) << "Loading shaders...";

    std::vector<std::string> shaderFiles;
    shaderFiles.push_back(":/shaders/glsl/core.glsl");
    shaderFiles.push_back(":/shaders/glsl/blur_horizontal.glsl");
    shaderFiles.push_back(":/shaders/glsl/blur_vertical.glsl");
    shaderFiles.push_back(":/shaders/glsl/scanlines.glsl");

    for (auto it = shaderFiles.cbegin(); it != shaderFiles.cend(); ++it) {
        ShaderOpenGL* loadShader = new ShaderOpenGL();

        loadShader->loadShaderFile(*it, GL_VERTEX_SHADER);
        loadShader->loadShaderFile(*it, GL_FRAGMENT_SHADER);

        if (!loadShader->createProgram()) {
            LOG(LogError) << "Could not create shader program.";
            return false;
        }

        mShaderProgramVector.push_back(loadShader);
    }

    return true;
}

GLenum RendererOpenGL::convertBlendFactor(const BlendFactor BlendFactorFactor)
{
    // clang-format off
    switch (BlendFactorFactor) {
        case BlendFactor::ZERO:                { return GL_ZERO;                } break;
        case BlendFactor::ONE:                 { return GL_ONE;                 } break;
        case BlendFactor::SRC_COLOR:           { return GL_SRC_COLOR;           } break;
        case BlendFactor::ONE_MINUS_SRC_COLOR: { return GL_ONE_MINUS_SRC_COLOR; } break;
        case BlendFactor::SRC_ALPHA:           { return GL_SRC_ALPHA;           } break;
        case BlendFactor::ONE_MINUS_SRC_ALPHA: { return GL_ONE_MINUS_SRC_ALPHA; } break;
        case BlendFactor::DST_COLOR:           { return GL_DST_COLOR;           } break;
        case BlendFactor::ONE_MINUS_DST_COLOR: { return GL_ONE_MINUS_DST_COLOR; } break;
        case BlendFactor::DST_ALPHA:           { return GL_DST_ALPHA;           } break;
        case BlendFactor::ONE_MINUS_DST_ALPHA: { return GL_ONE_MINUS_DST_ALPHA; } break;
        default:                               { return GL_ZERO;                }
    }
    // clang-format on
}

GLenum RendererOpenGL::convertTextureType(const TextureType type)
{
    // clang-format off
    switch (type) {
        case TextureType::RGBA:  { return GL_RGBA;            } break;
#if defined(USE_OPENGLES)
        case TextureType::BGRA:  { return GL_BGRA_EXT;        } break;
#else
        case TextureType::BGRA:  { return GL_BGRA;            } break;
#endif
        case TextureType::RED:   { return GL_RED;             } break;
        default:                 { return GL_ZERO;            }
    }
    // clang-format on
}

void RendererOpenGL::setup()
{
#if defined(USE_OPENGLES)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
}

bool RendererOpenGL::createContext()
{
    mSDLContext = SDL_GL_CreateContext(getSDLWindow());

    if (!mSDLContext) {
        LOG(LogError) << "Error creating OpenGL context. " << SDL_GetError();
        return false;
    }

#if defined(_WIN64)
    glewInit();
#endif

    SDL_GL_MakeCurrent(getSDLWindow(), mSDLContext);

    std::string vendor {
        glGetString(GL_VENDOR) ? reinterpret_cast<const char*>(glGetString(GL_VENDOR)) : ""};
    std::string renderer {
        glGetString(GL_RENDERER) ? reinterpret_cast<const char*>(glGetString(GL_RENDERER)) : ""};
    std::string version {
        glGetString(GL_VERSION) ? reinterpret_cast<const char*>(glGetString(GL_VERSION)) : ""};

    LOG(LogInfo) << "GL vendor: " << vendor;
    LOG(LogInfo) << "GL renderer: " << renderer;
    LOG(LogInfo) << "GL version: " << version;
#if defined(_WIN64)
    LOG(LogInfo) << "EmulationStation renderer: OpenGL 3.3 with GLEW";
#else
#if defined(USE_OPENGLES)
    LOG(LogInfo) << "EmulationStation renderer: OpenGL ES 3.0";
#else
    LOG(LogInfo) << "EmulationStation renderer: OpenGL 3.3";
#endif
#endif

#if !defined(USE_OPENGLES)
    // TODO: Fix the issue that causes the first glClearColor function call to fail.
    GL_CHECK_ERROR(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
#endif

    GL_CHECK_ERROR(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
    GL_CHECK_ERROR(glActiveTexture(GL_TEXTURE0));
    GL_CHECK_ERROR(glEnable(GL_BLEND));
    GL_CHECK_ERROR(glPixelStorei(GL_PACK_ALIGNMENT, 1));
    GL_CHECK_ERROR(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));

    // These are used for the shader post processing.
    GL_CHECK_ERROR(glGenFramebuffers(1, &mShaderFBO1));
    GL_CHECK_ERROR(glGenFramebuffers(1, &mShaderFBO2));

    GL_CHECK_ERROR(glGenBuffers(1, &mVertexBuffer1));
    GL_CHECK_ERROR(glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer1));
    GL_CHECK_ERROR(glGenVertexArrays(1, &mVertexBuffer2));
    GL_CHECK_ERROR(glBindVertexArray(mVertexBuffer2));

    uint8_t data[4] = {255, 255, 255, 255};
    mWhiteTexture = createTexture(TextureType::RGBA, false, false, true, 1, 1, data);

    mPostProcTexture1 = createTexture(TextureType::RGBA, false, false, false,
                                      static_cast<unsigned int>(getScreenWidth()),
                                      static_cast<unsigned int>(getScreenHeight()), nullptr);

    mPostProcTexture2 = createTexture(TextureType::RGBA, false, false, false,
                                      static_cast<unsigned int>(getScreenWidth()),
                                      static_cast<unsigned int>(getScreenHeight()), nullptr);

    // Attach textures to the shader framebuffers.
    GL_CHECK_ERROR(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mShaderFBO1));
    GL_CHECK_ERROR(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                          mPostProcTexture1, 0));

    GL_CHECK_ERROR(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mShaderFBO2));
    GL_CHECK_ERROR(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                          mPostProcTexture2, 0));

    GL_CHECK_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));

    return true;
}

void RendererOpenGL::destroyContext()
{
    GL_CHECK_ERROR(glDeleteFramebuffers(1, &mShaderFBO1));
    GL_CHECK_ERROR(glDeleteFramebuffers(1, &mShaderFBO2));
    destroyTexture(mPostProcTexture1);
    destroyTexture(mPostProcTexture2);
    destroyTexture(mWhiteTexture);
    SDL_GL_DeleteContext(mSDLContext);
    mSDLContext = nullptr;
}

void RendererOpenGL::setMatrix(const glm::mat4& matrix)
{
    mTrans = matrix;
    mTrans[3] = glm::round(mTrans[3]);
    mTrans = getProjectionMatrix() * mTrans;
}

void RendererOpenGL::setScissor(const Rect& scissor)
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

void RendererOpenGL::setSwapInterval()
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

void RendererOpenGL::swapBuffers()
{
#if defined(__APPLE__)
    // On macOS when running in the background, the OpenGL driver apparently does not swap
    // the frames which leads to a very fast swap time. This makes ES-DE use a lot of CPU
    // resources which slows down the games significantly on slower machines. By introducing
    // a delay if the swap time is very low we reduce CPU usage while still keeping the
    // application functioning normally.
    const auto beforeSwap = std::chrono::system_clock::now();
    SDL_GL_SwapWindow(getSDLWindow());

    if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() -
                                                              beforeSwap)
            .count() < 3.0)
        SDL_Delay(10);
#else
    SDL_GL_SwapWindow(getSDLWindow());
#endif
    GL_CHECK_ERROR(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

unsigned int RendererOpenGL::createTexture(const TextureType type,
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

    GL_CHECK_ERROR(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                                   repeat ? static_cast<GLfloat>(GL_REPEAT) :
                                            static_cast<GLfloat>(GL_CLAMP_TO_EDGE)));
    GL_CHECK_ERROR(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                                   repeat ? static_cast<GLfloat>(GL_REPEAT) :
                                            static_cast<GLfloat>(GL_CLAMP_TO_EDGE)));
    GL_CHECK_ERROR(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                   linearMinify ? static_cast<GLfloat>(GL_LINEAR) :
                                                  static_cast<GLfloat>(GL_NEAREST)));
    GL_CHECK_ERROR(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                                   linearMagnify ? static_cast<GLfloat>(GL_LINEAR) :
                                                   static_cast<GLfloat>(GL_NEAREST)));

    GL_CHECK_ERROR(glTexImage2D(GL_TEXTURE_2D, 0, textureType, width, height, 0, textureType,
                                GL_UNSIGNED_BYTE, data));
    return texture;
}

void RendererOpenGL::destroyTexture(const unsigned int texture)
{
    GL_CHECK_ERROR(glDeleteTextures(1, &texture));
}

void RendererOpenGL::updateTexture(const unsigned int texture,
                                   const TextureType type,
                                   const unsigned int x,
                                   const unsigned int y,
                                   const unsigned int width,
                                   const unsigned int height,
                                   void* data)
{
    const GLenum textureType {convertTextureType(type)};

    GL_CHECK_ERROR(glBindTexture(GL_TEXTURE_2D, texture));
    GL_CHECK_ERROR(glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, textureType,
                                   GL_UNSIGNED_BYTE, data));

    GL_CHECK_ERROR(glBindTexture(GL_TEXTURE_2D, mWhiteTexture));
}

void RendererOpenGL::bindTexture(const unsigned int texture)
{
    if (texture == 0)
        GL_CHECK_ERROR(glBindTexture(GL_TEXTURE_2D, mWhiteTexture));
    else
        GL_CHECK_ERROR(glBindTexture(GL_TEXTURE_2D, texture));
}

void RendererOpenGL::drawTriangleStrips(const Vertex* vertices,
                                        const unsigned int numVertices,
                                        const BlendFactor srcBlendFactorFactor,
                                        const BlendFactor dstBlendFactorFactor)
{
    const float width {vertices[3].position[0]};
    const float height {vertices[3].position[1]};

    GL_CHECK_ERROR(glBlendFunc(convertBlendFactor(srcBlendFactorFactor),
                               convertBlendFactor(dstBlendFactorFactor)));

    if (vertices->shaders == 0 || vertices->shaders & SHADER_CORE) {
        if (mCoreShader == nullptr)
            mCoreShader = getShaderProgram(SHADER_CORE);
        if (mCoreShader) {
            if (mLastShader != mCoreShader)
                mCoreShader->activateShaders();
            mCoreShader->setModelViewProjectionMatrix(mTrans);
            if (mLastShader != mCoreShader)
                mCoreShader->setAttribPointers();
            GL_CHECK_ERROR(glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * numVertices, vertices,
                                        GL_DYNAMIC_DRAW));
            mCoreShader->setOpacity(vertices->opacity);
            mCoreShader->setSaturation(vertices->saturation);
            mCoreShader->setDimming(vertices->dimming);
            mCoreShader->setBGRAToRGBA(vertices->convertBGRAToRGBA);
            mCoreShader->setFont(vertices->font);
            mCoreShader->setPostProcessing(vertices->postProcessing);
            GL_CHECK_ERROR(glDrawArrays(GL_TRIANGLE_STRIP, 0, numVertices));
            mLastShader = mCoreShader;
        }
    }
    else if (vertices->shaders & SHADER_BLUR_HORIZONTAL) {
        if (mBlurHorizontalShader == nullptr)
            mBlurHorizontalShader = getShaderProgram(SHADER_BLUR_HORIZONTAL);
        if (mBlurHorizontalShader) {
            if (mLastShader != mBlurHorizontalShader)
                mBlurHorizontalShader->activateShaders();
            mBlurHorizontalShader->setModelViewProjectionMatrix(mTrans);
            if (mLastShader != mBlurHorizontalShader)
                mBlurHorizontalShader->setAttribPointers();
            GL_CHECK_ERROR(glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * numVertices, vertices,
                                        GL_DYNAMIC_DRAW));
            mBlurHorizontalShader->setTextureSize({width, height});
            GL_CHECK_ERROR(glDrawArrays(GL_TRIANGLE_STRIP, 0, numVertices));
            mLastShader = mBlurHorizontalShader;
        }
        return;
    }
    else if (vertices->shaders & SHADER_BLUR_VERTICAL) {
        if (mBlurVerticalShader == nullptr)
            mBlurVerticalShader = getShaderProgram(SHADER_BLUR_VERTICAL);
        if (mBlurVerticalShader) {
            if (mLastShader != mBlurVerticalShader)
                mBlurVerticalShader->activateShaders();
            mBlurVerticalShader->setModelViewProjectionMatrix(mTrans);
            if (mLastShader != mBlurVerticalShader)
                mBlurVerticalShader->setAttribPointers();
            GL_CHECK_ERROR(glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * numVertices, vertices,
                                        GL_DYNAMIC_DRAW));
            mBlurVerticalShader->setTextureSize({width, height});
            GL_CHECK_ERROR(glDrawArrays(GL_TRIANGLE_STRIP, 0, numVertices));
            mLastShader = mBlurVerticalShader;
        }
        return;
    }
    else if (vertices->shaders & SHADER_SCANLINES) {
        if (mScanlinelShader == nullptr)
            mScanlinelShader = getShaderProgram(SHADER_SCANLINES);
        float shaderWidth {width * 1.2f};
        // Scale the scanlines relative to screen resolution.
        float screenHeightModifier {getScreenHeightModifier()};
        float relativeHeight {height / getScreenHeight()};
        float shaderHeight {0.0f};
        if (relativeHeight == 1.0f) {
            // Full screen.
            float modifier {1.30f - (0.1f * screenHeightModifier)};
            shaderHeight = height * modifier;
        }
        else {
            // Portion of screen, e.g. gamelist view.
            // Average the relative width and height to avoid applying exaggerated
            // scanlines to videos with non-standard aspect ratios.
            float relativeWidth {width / getScreenWidth()};
            float relativeAdjustment {(relativeWidth + relativeHeight) / 2.0f};
            float modifier {1.41f + relativeAdjustment / 7.0f - (0.14f * screenHeightModifier)};
            shaderHeight = height * modifier;
        }
        if (mScanlinelShader) {
            if (mLastShader != mScanlinelShader)
                mScanlinelShader->activateShaders();
            mScanlinelShader->setModelViewProjectionMatrix(mTrans);
            if (mLastShader != mScanlinelShader)
                mScanlinelShader->setAttribPointers();
            GL_CHECK_ERROR(glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * numVertices, vertices,
                                        GL_DYNAMIC_DRAW));
            mScanlinelShader->setOpacity(vertices->opacity);
            mScanlinelShader->setTextureSize({shaderWidth, shaderHeight});
            GL_CHECK_ERROR(glDrawArrays(GL_TRIANGLE_STRIP, 0, numVertices));
            mLastShader = mScanlinelShader;
        }
    }
}

void RendererOpenGL::shaderPostprocessing(unsigned int shaders,
                                          const Renderer::postProcessingParams& parameters,
                                          unsigned char* textureRGBA)
{
    Vertex vertices[4];
    std::vector<unsigned int> shaderList;
    float widthf {getScreenWidth()};
    float heightf {getScreenHeight()};
    GLuint width {static_cast<GLuint>(widthf)};
    GLuint height {static_cast<GLuint>(heightf)};

    // Set vertex positions and texture coordinates to full screen as all
    // post-processing is applied to the complete screen area.
    // clang-format off
    vertices[0] = {{0.0f,   0.0f   }, {0.0f, 1.0f}, 0xFFFFFFFF};
    vertices[1] = {{0.0f,   heightf}, {0.0f, 0.0f}, 0xFFFFFFFF};
    vertices[2] = {{widthf, 0.0f   }, {1.0f, 1.0f}, 0xFFFFFFFF};
    vertices[3] = {{widthf, heightf}, {1.0f, 0.0f}, 0xFFFFFFFF};
    // clang-format on

    vertices->opacity = parameters.opacity;
    vertices->saturation = parameters.saturation;
    vertices->dimming = parameters.dimming;
    vertices->postProcessing = true;

    if (shaders & SHADER_CORE)
        shaderList.push_back(SHADER_CORE);
    if (shaders & SHADER_BLUR_HORIZONTAL)
        shaderList.push_back(SHADER_BLUR_HORIZONTAL);
    if (shaders & SHADER_BLUR_VERTICAL)
        shaderList.push_back(SHADER_BLUR_VERTICAL);
    if (shaders & SHADER_SCANLINES)
        shaderList.push_back(SHADER_SCANLINES);

    setMatrix(getIdentity());
    bindTexture(mPostProcTexture1);

    GL_CHECK_ERROR(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mShaderFBO1));

    // Blit the screen contents to mPostProcTexture.
    GL_CHECK_ERROR(glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT,
                                     GL_NEAREST));

    GL_CHECK_ERROR(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mShaderFBO2));

    bool firstFBO {true};
    int drawCalls {0};

    for (size_t i = 0; i < shaderList.size(); ++i) {
        vertices->shaders = shaderList[i];
        int shaderPasses = 1;
        // For the blur shaders there is an optional variable to set the number of passes
        // to execute, which proportionally affects the blur amount.
        if (shaderList[i] == Renderer::SHADER_BLUR_HORIZONTAL ||
            shaderList[i] == Renderer::SHADER_BLUR_VERTICAL) {
            shaderPasses = parameters.blurPasses;
        }

        for (int p = 0; p < shaderPasses; ++p) {
            if (textureRGBA == nullptr && i == shaderList.size() - 1 && p == shaderPasses - 1) {
                // If the screen is rotated and we're at an even number of drawcalls, then
                // set the projection to a non-rotated state before making the last drawcall
                // as the image would otherwise get rendered upside down.
                if (getScreenRotated() && drawCalls % 2 == 0) {
                    mTrans = getIdentity();
                    mTrans[3] = glm::round(mTrans[3]);
                    mTrans = getProjectionMatrixNormal() * mTrans;
                }
                // If it's the last shader pass, then render directly to the default framebuffer
                // to avoid having to make an expensive glBlitFramebuffer() call.
                GL_CHECK_ERROR(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));
                drawTriangleStrips(vertices, 4, BlendFactor::SRC_ALPHA,
                                   BlendFactor::ONE_MINUS_SRC_ALPHA);
                break;
            }
            // Apply/render the shaders.
            drawTriangleStrips(vertices, 4, BlendFactor::SRC_ALPHA,
                               BlendFactor::ONE_MINUS_SRC_ALPHA);
            ++drawCalls;
            if (firstFBO) {
                bindTexture(mPostProcTexture2);
                GL_CHECK_ERROR(glBindFramebuffer(GL_READ_FRAMEBUFFER, mShaderFBO2));
                GL_CHECK_ERROR(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mShaderFBO1));
                GL_CHECK_ERROR(glClear(GL_COLOR_BUFFER_BIT));
                firstFBO = false;
            }
            else {
                bindTexture(mPostProcTexture1);
                GL_CHECK_ERROR(glBindFramebuffer(GL_READ_FRAMEBUFFER, mShaderFBO1));
                GL_CHECK_ERROR(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mShaderFBO2));
                GL_CHECK_ERROR(glClear(GL_COLOR_BUFFER_BIT));
                firstFBO = true;
            }
        }
    }

    // If textureRGBA has an address, it means that the output should go to this
    // texture rather than to the screen. The glReadPixels() function is slow, but
    // since this will typically only run every now and then to create a cached
    // screen texture, it doesn't really matter.
    if (textureRGBA) {
        if (firstFBO)
            GL_CHECK_ERROR(glBindFramebuffer(GL_READ_FRAMEBUFFER, mShaderFBO1));
        else
            GL_CHECK_ERROR(glBindFramebuffer(GL_READ_FRAMEBUFFER, mShaderFBO2));

        GL_CHECK_ERROR(glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, textureRGBA));
        GL_CHECK_ERROR(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));
    }

    if (getScreenRotated())
        setMatrix(getIdentity());

    GL_CHECK_ERROR(glBindFramebuffer(GL_READ_FRAMEBUFFER, 0));
}
