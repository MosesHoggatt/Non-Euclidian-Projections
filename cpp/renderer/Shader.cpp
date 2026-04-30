#include "Shader.h"
#include <stdexcept>
#include <vector>
#include <string>

// ─────────────────────────────────────────────────────────────────────────────
//  renderer/Shader.cpp
// ─────────────────────────────────────────────────────────────────────────────

Shader::Shader(const std::string& vertexShaderSource,
               const std::string& fragmentShaderSource) {

    GLuint vertexShader   = compileShaderStage(GL_VERTEX_SHADER,   vertexShaderSource);
    GLuint fragmentShader = compileShaderStage(GL_FRAGMENT_SHADER, fragmentShaderSource);

    m_programId = glCreateProgram();
    glAttachShader(m_programId, vertexShader);
    glAttachShader(m_programId, fragmentShader);
    glLinkProgram(m_programId);

    // Shader stages are no longer needed once the program is linked.
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Check for link errors.
    GLint linkStatus = 0;
    glGetProgramiv(m_programId, GL_LINK_STATUS, &linkStatus);
    if (linkStatus == GL_FALSE) {
        GLint logLength = 0;
        glGetProgramiv(m_programId, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> log(static_cast<size_t>(logLength));
        glGetProgramInfoLog(m_programId, logLength, nullptr, log.data());
        glDeleteProgram(m_programId);
        throw std::runtime_error("Shader program link failed:\n" + std::string(log.data()));
    }
}

Shader::~Shader() {
    if (m_programId != 0) {
        glDeleteProgram(m_programId);
    }
}

Shader::Shader(Shader&& other) noexcept : m_programId(other.m_programId) {
    other.m_programId = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        if (m_programId != 0) glDeleteProgram(m_programId);
        m_programId = other.m_programId;
        other.m_programId = 0;
    }
    return *this;
}

void Shader::bind()   const { glUseProgram(m_programId); }
void Shader::unbind() const { glUseProgram(0); }

GLint Shader::getUniformLocation(const std::string& name) const {
    auto iterator = m_uniformLocationCache.find(name);
    if (iterator != m_uniformLocationCache.end()) {
        return iterator->second;
    }
    GLint location = glGetUniformLocation(m_programId, name.c_str());
    m_uniformLocationCache[name] = location;
    return location;
}

void Shader::setUniformFloat(const std::string& name, float value) const {
    glUniform1f(getUniformLocation(name), value);
}

void Shader::setUniformInt(const std::string& name, int value) const {
    glUniform1i(getUniformLocation(name), value);
}

void Shader::setUniformVector2(const std::string& name, const Vector2& vector) const {
    glUniform2f(getUniformLocation(name), vector.x, vector.y);
}

void Shader::setUniformVector3(const std::string& name, const Vector3& vector) const {
    glUniform3f(getUniformLocation(name), vector.x, vector.y, vector.z);
}

void Shader::setUniformMatrix4(const std::string& name, const Matrix4& matrix) const {
    // GL_FALSE: we store column-major, which is OpenGL's native format — no transpose needed.
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, matrix.data());
}

// ── Private static ────────────────────────────────────────────────────────────

GLuint Shader::compileShaderStage(GLenum shaderType, const std::string& source) {
    GLuint shaderId = glCreateShader(shaderType);

    const char* sourcePointer = source.c_str();
    glShaderSource(shaderId, 1, &sourcePointer, nullptr);
    glCompileShader(shaderId);

    GLint compileStatus = 0;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &compileStatus);
    if (compileStatus == GL_FALSE) {
        GLint logLength = 0;
        glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> log(static_cast<size_t>(logLength));
        glGetShaderInfoLog(shaderId, logLength, nullptr, log.data());
        glDeleteShader(shaderId);

        std::string stageLabel = (shaderType == GL_VERTEX_SHADER) ? "vertex" : "fragment";
        throw std::runtime_error("Shader compile failed (" + stageLabel + "):\n" + std::string(log.data()));
    }

    return shaderId;
}
