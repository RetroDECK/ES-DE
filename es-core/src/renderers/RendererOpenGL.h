//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  RendererOpenGL.h
//
//  OpenGL / OpenGL ES renderering functions.
//

#ifndef ES_CORE_RENDERER_RENDERER_OPENGL_H
#define ES_CORE_RENDERER_RENDERER_OPENGL_H

#include "renderers/Renderer.h"
#include "renderers/ShaderOpenGL.h"

#if defined(USE_OPENGLES)
#include <GLES3/gl3.h>
#include <SDL2/SDL_opengles.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#endif

#include <memory>

class RendererOpenGL : public Renderer
{
public:
    static RendererOpenGL* getInstance();

    std::shared_ptr<ShaderOpenGL> getShaderProgram(unsigned int shaderID);
    bool loadShaders() override;

    GLenum convertBlendFactor(const BlendFactor BlendFactor);
    GLenum convertTextureType(const TextureType type);

    void setup() override;
    bool createContext() override;
    void destroyContext() override;

    void setMatrix(const glm::mat4& matrix) override;
    void setViewport(const Rect& viewport) override;
    void setScissor(const Rect& scissor) override;
    void setSwapInterval() override;
    void swapBuffers() override;

    unsigned int createTexture(const TextureType type,
                               const bool linearMinify,
                               const bool linearMagnify,
                               const bool mipmapping,
                               const bool repeat,
                               const unsigned int width,
                               const unsigned int height,
                               void* data) override;
    void destroyTexture(const unsigned int texture) override;
    void updateTexture(const unsigned int texture,
                       const TextureType type,
                       const unsigned int x,
                       const unsigned int y,
                       const unsigned int width,
                       const unsigned int height,
                       void* data) override;
    void bindTexture(const unsigned int texture) override;

    void drawTriangleStrips(
        const Vertex* vertices,
        const unsigned int numVertices,
        const BlendFactor srcBlendFactor = BlendFactor::ONE,
        const BlendFactor dstBlendFactor = BlendFactor::ONE_MINUS_SRC_ALPHA) override;

    void shaderPostprocessing(
        const unsigned int shaders,
        const Renderer::postProcessingParams& parameters = postProcessingParams(),
        unsigned char* textureRGBA = nullptr) override;

private:
    RendererOpenGL() noexcept;

    std::vector<std::shared_ptr<ShaderOpenGL>> mShaderProgramVector;
    GLuint mShaderFBO1;
    GLuint mShaderFBO2;
    GLuint mVertexBuffer1;
    GLuint mVertexBuffer2;

    SDL_GLContext mSDLContext;
    GLuint mWhiteTexture;
    GLuint mPostProcTexture1;
    GLuint mPostProcTexture2;
    std::shared_ptr<ShaderOpenGL> mCoreShader;
    std::shared_ptr<ShaderOpenGL> mBlurHorizontalShader;
    std::shared_ptr<ShaderOpenGL> mBlurVerticalShader;
    std::shared_ptr<ShaderOpenGL> mScanlinelShader;
    std::shared_ptr<ShaderOpenGL> mLastShader;

    int mMajorGLVersion;
    int mMinorGLVersion;

    friend Renderer;
};

#endif // ES_CORE_RENDERER_RENDERER_OPENGL_H
