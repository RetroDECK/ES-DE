//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  ShaderOpenGL.cpp
//
//  OpenGL / OpenGL ES shader functions.
//

#include "ShaderOpenGL.h"

#include "Log.h"
#include "renderers/Renderer.h"
#include "resources/ResourceManager.h"

ShaderOpenGL::ShaderOpenGL()
    : mProgramID {0}
    , mShaderMVPMatrix {0}
    , mShaderPosition {0}
    , mShaderTextureCoord {0}
    , mShaderColor {0}
    , mShaderTextureSize {0}
    , mShaderBrightness {0}
    , mShaderOpacity {0}
    , mShaderSaturation {0}
    , mShaderDimming {0}
    , mShaderReflectionsFalloff {0}
    , mShaderFlags {0}
{
}

ShaderOpenGL::~ShaderOpenGL()
{
    // Delete the shader program when destroyed.
    deleteProgram(mProgramID);
}

void ShaderOpenGL::loadShaderFile(const std::string& path, GLenum shaderType)
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

    mShaderVector.push_back(std::make_tuple(path, preprocessorDefines + shaderCode, shaderType));
}

bool ShaderOpenGL::createProgram()
{
    GLint programSuccess;

    mProgramID = glCreateProgram();

    // Compile and attach all shaders that have been loaded.
    for (auto it = mShaderVector.cbegin(); it != mShaderVector.cend(); ++it) {
        GLuint currentShader {glCreateShader(std::get<2>(*it))};
        GLchar const* shaderCodePtr {std::get<1>(*it).c_str()};

        glShaderSource(currentShader, 1, reinterpret_cast<const GLchar**>(&shaderCodePtr), nullptr);
        glCompileShader(currentShader);

        GLint shaderCompiled;
        glGetShaderiv(currentShader, GL_COMPILE_STATUS, &shaderCompiled);

        if (shaderCompiled != GL_TRUE) {
            LOG(LogError) << "ShaderOpenGL::createProgram(): Unable to compile shader "
                          << currentShader << " (" << std::get<0>(*it) << ")";
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
        LOG(LogError) << "ShaderOpenGL::createProgram(): Unable to link program " << mProgramID;
        printProgramInfoLog(mProgramID);
        return false;
    }

    getVariableLocations(mProgramID);

    if (mShaderPosition != -1)
        GL_CHECK_ERROR(glEnableVertexAttribArray(mShaderPosition));

    if (mShaderTextureCoord != -1)
        GL_CHECK_ERROR(glEnableVertexAttribArray(mShaderTextureCoord));

    if (mShaderColor != -1)
        GL_CHECK_ERROR(glEnableVertexAttribArray(mShaderColor));

    return true;
}

void ShaderOpenGL::deleteProgram(GLuint programID) { GL_CHECK_ERROR(glDeleteProgram(programID)); }

void ShaderOpenGL::getVariableLocations(GLuint programID)
{
    // Some of the variable names are chosen to be compatible with the RetroArch GLSL shaders.
    mShaderMVPMatrix = glGetUniformLocation(mProgramID, "MVPMatrix");
    mShaderPosition = glGetAttribLocation(mProgramID, "positionVertex");
    mShaderTextureCoord = glGetAttribLocation(mProgramID, "texCoordVertex");
    mShaderColor = glGetAttribLocation(mProgramID, "colorVertex");
    mShaderTextureSize = glGetUniformLocation(mProgramID, "textureSize");
    mShaderClipRegion = glGetUniformLocation(mProgramID, "clipRegion");
    mShaderBrightness = glGetUniformLocation(mProgramID, "brightness");
    mShaderOpacity = glGetUniformLocation(mProgramID, "opacity");
    mShaderSaturation = glGetUniformLocation(mProgramID, "saturation");
    mShaderDimming = glGetUniformLocation(mProgramID, "dimming");
    mShaderReflectionsFalloff = glGetUniformLocation(mProgramID, "reflectionsFalloff");
    mShaderFlags = glGetUniformLocation(mProgramID, "shaderFlags");
}

void ShaderOpenGL::setModelViewProjectionMatrix(glm::mat4 mvpMatrix)
{
    if (mShaderMVPMatrix != GL_INVALID_VALUE && mShaderMVPMatrix != GL_INVALID_OPERATION)
        GL_CHECK_ERROR(glUniformMatrix4fv(mShaderMVPMatrix, 1, GL_FALSE,
                                          reinterpret_cast<GLfloat*>(&mvpMatrix)));
}

void ShaderOpenGL::setAttribPointers()
{
    if (mShaderPosition != -1)
        GL_CHECK_ERROR(glVertexAttribPointer(
            mShaderPosition, 2, GL_FLOAT, GL_FALSE, sizeof(Renderer::Vertex),
            reinterpret_cast<const void*>(offsetof(Renderer::Vertex, position))));
    if (mShaderTextureCoord != -1)
        GL_CHECK_ERROR(glVertexAttribPointer(
            mShaderTextureCoord, 2, GL_FLOAT, GL_FALSE, sizeof(Renderer::Vertex),
            reinterpret_cast<const void*>(offsetof(Renderer::Vertex, texcoord))));

    if (mShaderColor != -1)
        GL_CHECK_ERROR(glVertexAttribPointer(
            mShaderColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Renderer::Vertex),
            reinterpret_cast<const void*>(offsetof(Renderer::Vertex, color))));
}

void ShaderOpenGL::setTextureSize(std::array<GLfloat, 2> shaderVec2)
{
    if (mShaderTextureSize != -1)
        GL_CHECK_ERROR(glUniform2f(mShaderTextureSize, shaderVec2[0], shaderVec2[1]));
}

void ShaderOpenGL::setClipRegion(glm::vec4 clipRegion)
{
    if (mShaderClipRegion != -1)
        GL_CHECK_ERROR(glUniform4f(mShaderClipRegion, clipRegion[0], clipRegion[1], clipRegion[2],
                                   clipRegion[3]));
}

void ShaderOpenGL::setBrightness(GLfloat brightness)
{
    if (mShaderBrightness != -1)
        GL_CHECK_ERROR(glUniform1f(mShaderBrightness, brightness));
}

void ShaderOpenGL::setOpacity(GLfloat opacity)
{
    if (mShaderOpacity != -1)
        GL_CHECK_ERROR(glUniform1f(mShaderOpacity, opacity));
}

void ShaderOpenGL::setSaturation(GLfloat saturation)
{
    if (mShaderSaturation != -1)
        GL_CHECK_ERROR(glUniform1f(mShaderSaturation, saturation));
}

void ShaderOpenGL::setDimming(GLfloat dimming)
{
    if (mShaderDimming != -1)
        GL_CHECK_ERROR(glUniform1f(mShaderDimming, dimming));
}

void ShaderOpenGL::setReflectionsFalloff(GLfloat falloff)
{
    if (mShaderReflectionsFalloff != -1)
        GL_CHECK_ERROR(glUniform1f(mShaderReflectionsFalloff, falloff));
}

void ShaderOpenGL::setFlags(GLuint flags)
{
    if (mShaderFlags != -1)
        GL_CHECK_ERROR(glUniform1ui(mShaderFlags, flags));
}

void ShaderOpenGL::activateShaders()
{
    // Install the shader program.
    GL_CHECK_ERROR(glUseProgram(mProgramID));
}

void ShaderOpenGL::deactivateShaders()
{
    // Remove the shader program.
    GL_CHECK_ERROR(glUseProgram(0));
}

void ShaderOpenGL::printProgramInfoLog(GLuint programID)
{
    if (glIsProgram(programID)) {
        int logLength;
        int maxLength;

        glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &maxLength);
        std::vector<char> infoLog(maxLength);

        glGetProgramInfoLog(programID, maxLength, &logLength, &infoLog.front());

        if (logLength > 0) {
            LOG(LogDebug) << "ShaderOpenGL::createProgram()::\n"
                          << std::string(infoLog.begin(), infoLog.end());
        }
    }
    else {
        LOG(LogError) << "ShaderOpenGL::createProgram(): " << programID << " is not a program";
    }
}

void ShaderOpenGL::printShaderInfoLog(GLuint shaderID, GLenum shaderType, bool error)
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
            LOG(LogDebug) << "ShaderOpenGL::createProgram(): " << (error ? "Error" : "Warning")
                          << " in "
                          << (shaderType == GL_VERTEX_SHADER ? "VERTEX section:\n" :
                                                               "FRAGMENT section:\n")
                          << std::string(infoLog.begin(), infoLog.end());
        }
    }
    else {
        LOG(LogError) << "ShaderOpenGL::createProgram():: " << shaderID << " is not a shader";
    }
}
