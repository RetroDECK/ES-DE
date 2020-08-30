//
//  Shader_GL21.cpp
//
//  OpenGL 2.1 GLSL shader functions.
//

#if defined(USE_OPENGL_21)

#include "Shader_GL21.h"

#include "renderers/Renderer.h"
#include "resources/ResourceManager.h"
#include "Log.h"

namespace Renderer
{
    Renderer::Shader::Shader()
            : mProgramID(-1),
            shaderFloat_0(-1),
            shaderFloat_1(-1),
            shaderFloat_2(-1),
            shaderVec4_0(-1),
            shaderVec4_1(-1),
            shaderVec4_2(-1)
    {
    }

    Renderer::Shader::~Shader()
    {
        deleteProgram(mProgramID);
    }

    void Renderer::Shader::loadShaderFile(const std::string& path, GLenum shaderType)
    {
        std::string preprocessorDefines;
        std::string shaderCode;

        // This will load the entire GLSL source code into the string variable.
        const ResourceData& shaderData = ResourceManager::getInstance()->getFileData(path);
        shaderCode.assign((const char*)shaderData.ptr.get(), shaderData.length);

        // Define the GLSL version (version 120 = OpenGL 2.1).
        preprocessorDefines = "#version 120\n";

        // Define the preprocessor macros that will let the shader compiler know whether
        // the VERTEX or FRAGMENT portion of the code should be used.
        if (shaderType == GL_VERTEX_SHADER)
            preprocessorDefines += "#define VERTEX\n";
        else if (shaderType == GL_FRAGMENT_SHADER)
            preprocessorDefines += "#define FRAGMENT\n";

        shaderVector.push_back(std::make_tuple(
                path, preprocessorDefines + shaderCode, shaderType));
    }

    bool Renderer::Shader::createProgram()
    {
        GLint programSuccess;

        mProgramID = glCreateProgram();

        // Compile and attach all shaders that have been loaded.
        for (auto it = shaderVector.cbegin(); it != shaderVector.cend(); it++) {
            GLuint currentShader = glCreateShader(std::get<2>(*it));
            GLchar const* shaderCodePtr = std::get<1>(*it).c_str();

            glShaderSource(currentShader, 1, (const GLchar**)&shaderCodePtr, nullptr);
            glCompileShader(currentShader);

            GLint shaderCompiled;
            glGetShaderiv(currentShader, GL_COMPILE_STATUS, &shaderCompiled);

            if (shaderCompiled != GL_TRUE) {
                LOG(LogError) << "OpenGL error: Unable to compile shader " <<
                        currentShader << " (" << std::get<0>(*it) << ").";
                printShaderInfoLog(currentShader);
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

        return true;
    }

    void Renderer::Shader::deleteProgram(GLuint programID)
    {
        GL_CHECK_ERROR(glDeleteProgram(programID));
    }

    void Renderer::Shader::getVariableLocations(GLuint programID)
    {
        shaderFloat_0 = glGetUniformLocation(mProgramID, "shaderFloat_0");
        shaderFloat_1 = glGetUniformLocation(mProgramID, "shaderFloat_1");
        shaderFloat_2 = glGetUniformLocation(mProgramID, "shaderFloat_2");
        shaderVec4_0 = glGetUniformLocation(mProgramID, "shaderVec4_0");
        shaderVec4_1 = glGetUniformLocation(mProgramID, "shaderVec4_1");
        shaderVec4_2 = glGetUniformLocation(mProgramID, "shaderVec4_2");
    }

    void Renderer::Shader::setVariable(GLfloat shaderFloat, int index)
    {
        switch (index) {
            case 0:
                GL_CHECK_ERROR(glUniform1f(shaderFloat_0, shaderFloat));
                break;
            case 1:
                GL_CHECK_ERROR(glUniform1f(shaderFloat_1, shaderFloat));
                break;
            case 2:
                GL_CHECK_ERROR(glUniform1f(shaderFloat_2, shaderFloat));
                break;
            default:
                break;
        }
    }

    void Renderer::Shader::setVariable(std::array<GLfloat, 4> shaderVec4, int index)
    {
        switch (index) {
            case 0:
                GL_CHECK_ERROR(glUniform4f(shaderVec4_0, shaderVec4[0],
                        shaderVec4[1], shaderVec4[2], shaderVec4[3]));
                break;
            case 1:
                GL_CHECK_ERROR(glUniform4f(shaderVec4_1, shaderVec4[0],
                        shaderVec4[1], shaderVec4[2], shaderVec4[3]));
            case 2:
                GL_CHECK_ERROR(glUniform4f(shaderVec4_2, shaderVec4[0],
                        shaderVec4[1], shaderVec4[2], shaderVec4[3]));
            default:
                break;
        }
    }

    void Renderer::Shader::activateShaders()
    {
        GL_CHECK_ERROR(glUseProgram(mProgramID));
    }

    void Renderer::Shader::deactivateShaders()
    {
        GL_CHECK_ERROR(glUseProgram(0));
    }

    GLuint Renderer::Shader::getProgramID()
    {
        return mProgramID;
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
                LOG(LogDebug) << "Renderer_GL21::printProgramLog():\n" <<
                        std::string(infoLog.begin(), infoLog.end());
            }
        }
        else {
            LOG(LogError) << "OpenGL error: " << programID << " is not a program.";
        }
    }

    void Renderer::Shader::printShaderInfoLog(GLuint shaderID)
    {
        if (glIsShader(shaderID)) {
            int logLength;
            int maxLength;

            glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &maxLength);
            std::vector<char> infoLog(maxLength);

            glGetShaderInfoLog(shaderID, maxLength, &logLength, &infoLog.front());

            if (logLength > 0) {
                LOG(LogDebug) << "Renderer_GL21::printShaderLog():\n" <<
                        std::string(infoLog.begin(), infoLog.end());
            }
        }
        else {
            LOG(LogError) << "OpenGL error: " << shaderID << " is not a shader.";
        }
    }

} // Renderer

#endif // USE_OPENGL_21
