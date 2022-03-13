//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Shader_GL21.cpp
//
//  OpenGL 2.1 GLSL shader functions.
//

#include "Shader_GL21.h"

#include "Log.h"
#include "renderers/Renderer.h"
#include "resources/ResourceManager.h"

namespace Renderer
{
    Renderer::Shader::Shader()
        : mProgramID {0}
        , shaderMVPMatrix {0}
        , shaderPosition {0}
        , shaderTextureCoord {0}
        , shaderColor {0}
        , shaderTextureSize {0}
        , shaderOpacity {0}
        , shaderSaturation {0}
        , shaderDimming {0}
        , shaderBGRAToRGBA {0}
        , shaderFont {0}
        , shaderPostProcessing {0}
    {
    }

    Renderer::Shader::~Shader()
    {
        // Delete the shader program when destroyed.
        deleteProgram(mProgramID);
    }

    void Renderer::Shader::loadShaderFile(const std::string& path, GLenum shaderType)
    {
        std::string preprocessorDefines;
        std::string shaderCode;

        // This will load the entire GLSL source code into the string variable.
        const ResourceData& shaderData {ResourceManager::getInstance().getFileData(path)};
        shaderCode.assign(reinterpret_cast<const char*>(shaderData.ptr.get()), shaderData.length);

        // Define the GLSL version.
#if defined(USE_OPENGLES)
        preprocessorDefines = "#version 300 es\n";
#else
        preprocessorDefines = "#version 330\n";
#endif
        // Define the preprocessor constants that will let the shader compiler know whether
        // the VERTEX or FRAGMENT portion of the code should be used.
        if (shaderType == GL_VERTEX_SHADER)
            preprocessorDefines += "#define VERTEX\n";
        else if (shaderType == GL_FRAGMENT_SHADER)
            preprocessorDefines += "#define FRAGMENT\n";

        shaderVector.push_back(std::make_tuple(path, preprocessorDefines + shaderCode, shaderType));
    }

    bool Renderer::Shader::createProgram()
    {
        GLint programSuccess;

        mProgramID = glCreateProgram();

        // Compile and attach all shaders that have been loaded.
        for (auto it = shaderVector.cbegin(); it != shaderVector.cend(); ++it) {
            GLuint currentShader = glCreateShader(std::get<2>(*it));
            GLchar const* shaderCodePtr = std::get<1>(*it).c_str();

            glShaderSource(currentShader, 1, reinterpret_cast<const GLchar**>(&shaderCodePtr),
                           nullptr);
            glCompileShader(currentShader);

            GLint shaderCompiled;
            glGetShaderiv(currentShader, GL_COMPILE_STATUS, &shaderCompiled);

            if (shaderCompiled != GL_TRUE) {
                LOG(LogError) << "OpenGL error: Unable to compile shader " << currentShader << " ("
                              << std::get<0>(*it) << ").";
                printShaderInfoLog(currentShader, std::get<2>(*it), true);
                return false;
            }
            else {
                printShaderInfoLog(currentShader, std::get<2>(*it), false);
            }

            GL_CHECK_ERROR(glAttachShader(mProgramID, currentShader));
        }

        glLinkProgram(mProgramID);

        glGetProgramiv(mProgramID, GL_LINK_STATUS, &programSuccess);
        if (programSuccess != GL_TRUE) {
            LOG(LogError) << "OpenGL error: Unable to link program " << mProgramID << ".";
            printProgramInfoLog(mProgramID);
            return false;
        }

        getVariableLocations(mProgramID);

        if (shaderPosition != -1)
            GL_CHECK_ERROR(glEnableVertexAttribArray(shaderPosition));

        if (shaderTextureCoord != -1)
            GL_CHECK_ERROR(glEnableVertexAttribArray(shaderTextureCoord));

        if (shaderColor != -1)
            GL_CHECK_ERROR(glEnableVertexAttribArray(shaderColor));

        return true;
    }

    void Renderer::Shader::deleteProgram(GLuint programID)
    {
        GL_CHECK_ERROR(glDeleteProgram(programID));
    }

    void Renderer::Shader::getVariableLocations(GLuint programID)
    {
        // Some of the variable names are chosen to be compatible with the RetroArch GLSL shaders.
        shaderMVPMatrix = glGetUniformLocation(mProgramID, "MVPMatrix");
        shaderPosition = glGetAttribLocation(mProgramID, "positionAttrib");
        shaderTextureCoord = glGetAttribLocation(mProgramID, "TexCoord");
        shaderColor = glGetAttribLocation(mProgramID, "colorAttrib");
        shaderTextureSize = glGetUniformLocation(mProgramID, "TextureSize");
        shaderOpacity = glGetUniformLocation(mProgramID, "opacity");
        shaderSaturation = glGetUniformLocation(mProgramID, "saturation");
        shaderDimming = glGetUniformLocation(mProgramID, "dimming");
        shaderBGRAToRGBA = glGetUniformLocation(mProgramID, "BGRAToRGBA");
        shaderFont = glGetUniformLocation(mProgramID, "font");
        shaderPostProcessing = glGetUniformLocation(mProgramID, "postProcessing");
    }

    void Renderer::Shader::setModelViewProjectionMatrix(glm::mat4 mvpMatrix)
    {
        if (shaderMVPMatrix != GL_INVALID_VALUE && shaderMVPMatrix != GL_INVALID_OPERATION)
            GL_CHECK_ERROR(glUniformMatrix4fv(shaderMVPMatrix, 1, GL_FALSE,
                                              reinterpret_cast<GLfloat*>(&mvpMatrix)));
    }

    void Renderer::Shader::setAttribPointers()
    {
        if (shaderPosition != -1)
            GL_CHECK_ERROR(
                glVertexAttribPointer(shaderPosition, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                      reinterpret_cast<const void*>(offsetof(Vertex, position))));
        if (shaderTextureCoord != -1)
            GL_CHECK_ERROR(
                glVertexAttribPointer(shaderTextureCoord, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                      reinterpret_cast<const void*>(offsetof(Vertex, texture))));

        if (shaderColor != -1)
            GL_CHECK_ERROR(
                glVertexAttribPointer(shaderColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex),
                                      reinterpret_cast<const void*>(offsetof(Vertex, color))));
    }

    void Renderer::Shader::setTextureSize(std::array<GLfloat, 2> shaderVec2)
    {
        if (shaderTextureSize != -1)
            GL_CHECK_ERROR(glUniform2f(shaderTextureSize, shaderVec2[0], shaderVec2[1]));
    }

    void Renderer::Shader::setOpacity(GLfloat opacity)
    {
        if (shaderOpacity != -1)
            GL_CHECK_ERROR(glUniform1f(shaderOpacity, opacity));
    }

    void Renderer::Shader::setSaturation(GLfloat saturation)
    {
        if (shaderSaturation != -1)
            GL_CHECK_ERROR(glUniform1f(shaderSaturation, saturation));
    }

    void Renderer::Shader::setDimming(GLfloat dimming)
    {
        if (shaderDimming != -1)
            GL_CHECK_ERROR(glUniform1f(shaderDimming, dimming));
    }

    void Renderer::Shader::setBGRAToRGBA(GLboolean BGRAToRGBA)
    {
        if (shaderBGRAToRGBA != -1)
            GL_CHECK_ERROR(glUniform1i(shaderBGRAToRGBA, BGRAToRGBA ? 1 : 0));
    }

    void Renderer::Shader::setFont(GLboolean font)
    {
        if (shaderFont != -1)
            GL_CHECK_ERROR(glUniform1i(shaderFont, font ? 1 : 0));
    }

    void Renderer::Shader::setPostProcessing(GLboolean postProcessing)
    {
        if (shaderPostProcessing != -1)
            GL_CHECK_ERROR(glUniform1i(shaderPostProcessing, postProcessing ? 1 : 0));
    }

    void Renderer::Shader::activateShaders()
    {
        // Install the shader program.
        GL_CHECK_ERROR(glUseProgram(mProgramID));
    }

    void Renderer::Shader::deactivateShaders()
    {
        // Remove the shader program.
        GL_CHECK_ERROR(glUseProgram(0));
    }

    void Renderer::Shader::printProgramInfoLog(GLuint programID)
    {
        if (glIsProgram(programID)) {
            int logLength;
            int maxLength;

            glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &maxLength);
            std::vector<char> infoLog(maxLength);

            glGetProgramInfoLog(programID, maxLength, &logLength, &infoLog.front());

            if (logLength > 0) {
                LOG(LogDebug) << "Renderer_GL21::printProgramInfoLog():\n"
                              << std::string(infoLog.begin(), infoLog.end());
            }
        }
        else {
            LOG(LogError) << "OpenGL error: " << programID << " is not a program.";
        }
    }

    void Renderer::Shader::printShaderInfoLog(GLuint shaderID, GLenum shaderType, bool error)
    {
        if (glIsShader(shaderID)) {
            int logLength;
            int maxLength;

            glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &maxLength);
            std::vector<char> infoLog(maxLength);

            if (infoLog.size() == 0)
                return;

            glGetShaderInfoLog(shaderID, maxLength, &logLength, &infoLog.front());

            if (logLength > 0) {
                LOG(LogDebug) << "Shader_GL21::printShaderInfoLog(): "
                              << (error ? "Error" : "Warning") << " in "
                              << (shaderType == GL_VERTEX_SHADER ? "VERTEX section:\n" :
                                                                   "FRAGMENT section:\n")
                              << std::string(infoLog.begin(), infoLog.end());
            }
        }
        else {
            LOG(LogError) << "OpenGL error: " << shaderID << " is not a shader.";
        }
    }

} // namespace Renderer
