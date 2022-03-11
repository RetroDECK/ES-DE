//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Shader_GL21.cpp
//
//  OpenGL 2.1 GLSL shader functions.
//

#if defined(USE_OPENGL_21)

#include "Shader_GL21.h"

#include "Log.h"
#include "renderers/Renderer.h"
#include "resources/ResourceManager.h"

namespace Renderer
{
    Renderer::Shader::Shader()
        : mProgramID {0}
        , shaderMVPMatrix {0}
        , shaderTextureSize {0}
        , shaderTextureCoord {0}
        , shaderOpacity {0}
        , shaderSaturation {0}
        , shaderDimming {0}
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

        // Define the GLSL version (version 120 = OpenGL 2.1).
        preprocessorDefines = "#version 120\n";

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
                printShaderInfoLog(currentShader, std::get<2>(*it));
                return false;
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
        shaderTextureSize = glGetUniformLocation(mProgramID, "TextureSize");
        shaderTextureCoord = glGetAttribLocation(mProgramID, "TexCoord");
        shaderOpacity = glGetUniformLocation(mProgramID, "opacity");
        shaderSaturation = glGetUniformLocation(mProgramID, "saturation");
        shaderDimming = glGetUniformLocation(mProgramID, "dimming");
        shaderBGRAToRGBA = glGetUniformLocation(mProgramID, "BGRAToRGBA");
    }

    void Renderer::Shader::setModelViewProjectionMatrix(glm::mat4 mvpMatrix)
    {
        if (shaderMVPMatrix != GL_INVALID_VALUE && shaderMVPMatrix != GL_INVALID_OPERATION)
            GL_CHECK_ERROR(glUniformMatrix4fv(shaderMVPMatrix, 1, GL_FALSE,
                                              reinterpret_cast<GLfloat*>(&mvpMatrix)));
    }

    void Renderer::Shader::setTextureSize(std::array<GLfloat, 2> shaderVec2)
    {
        if (shaderTextureSize != GL_INVALID_VALUE && shaderTextureSize != GL_INVALID_OPERATION)
            GL_CHECK_ERROR(glUniform2f(shaderTextureSize, shaderVec2[0], shaderVec2[1]));
    }

    void Renderer::Shader::setTextureCoordinates(std::array<GLfloat, 4> shaderVec4)
    {
        if (shaderTextureCoord != GL_INVALID_OPERATION) {
            glVertexAttrib4f(shaderTextureCoord, shaderVec4[0], shaderVec4[1], shaderVec4[2],
                             shaderVec4[3]);
        }
    }

    void Renderer::Shader::setOpacity(GLfloat opacity)
    {
        if (shaderOpacity != GL_INVALID_VALUE && shaderOpacity != GL_INVALID_OPERATION)
            GL_CHECK_ERROR(glUniform1f(shaderOpacity, opacity));
    }

    void Renderer::Shader::setSaturation(GLfloat saturation)
    {
        if (shaderSaturation != GL_INVALID_VALUE && shaderSaturation != GL_INVALID_OPERATION)
            GL_CHECK_ERROR(glUniform1f(shaderSaturation, saturation));
    }

    void Renderer::Shader::setDimming(GLfloat dimming)
    {
        if (shaderDimming != GL_INVALID_VALUE && shaderDimming != GL_INVALID_OPERATION)
            GL_CHECK_ERROR(glUniform1f(shaderDimming, dimming));
    }

    void Renderer::Shader::setBGRAToRGBA(GLboolean BGRAToRGBA)
    {
        if (shaderBGRAToRGBA != GL_INVALID_VALUE && shaderBGRAToRGBA != GL_INVALID_OPERATION)
            GL_CHECK_ERROR(glUniform1i(shaderBGRAToRGBA, BGRAToRGBA ? 1 : 0));
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

    void Renderer::Shader::printShaderInfoLog(GLuint shaderID, GLenum shaderType)
    {
        if (glIsShader(shaderID)) {
            int logLength;
            int maxLength;

            glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &maxLength);
            std::vector<char> infoLog(maxLength);

            glGetShaderInfoLog(shaderID, maxLength, &logLength, &infoLog.front());

            if (logLength > 0) {
                LOG(LogDebug) << "Shader_GL21::printShaderInfoLog(): Error in "
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

#endif // USE_OPENGL_21
