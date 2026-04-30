#pragma once
#include <string>
#include <unordered_map>

// Emscripten maps OpenGL ES 3.0 headers to WebGL2
#ifdef __EMSCRIPTEN__
  #include <GLES3/gl3.h>
#else
  #include <GL/gl.h>
#endif

#include "../math/Matrix4.h"
#include "../math/Vector3.h"
#include "../math/Vector2.h"

// ─────────────────────────────────────────────────────────────────────────────
//  renderer/Shader.h
//
//  Owns a compiled and linked OpenGL shader program. Provides type-safe uniform
//  setters so callers never touch raw GL uniform locations.
//
//  Usage:
//      Shader shader(vertexSource, fragmentSource);
//      shader.bind();
//      shader.setUniformMatrix4("modelViewProjection", mvp);
//      // draw calls here
//      shader.unbind();
// ─────────────────────────────────────────────────────────────────────────────

class Shader {
public:
    // Compiles and links the shader program from GLSL source strings.
    // Throws std::runtime_error if compilation or linking fails.
    Shader(const std::string& vertexShaderSource,
           const std::string& fragmentShaderSource);

    ~Shader();

    // Non-copyable: OpenGL resources are owned by this object.
    Shader(const Shader&)            = delete;
    Shader& operator=(const Shader&) = delete;

    // Move semantics transfer ownership of the GL program.
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    // Binds this shader program as the active program for subsequent draw calls.
    void bind()   const;
    void unbind() const;

    GLuint programId() const { return m_programId; }

    // ── Uniform setters ───────────────────────────────────────────────────────
    void setUniformFloat(  const std::string& name, float value)           const;
    void setUniformInt(    const std::string& name, int value)             const;
    void setUniformVector2(const std::string& name, const Vector2& vector) const;
    void setUniformVector3(const std::string& name, const Vector3& vector) const;
    void setUniformMatrix4(const std::string& name, const Matrix4& matrix) const;

private:
    GLuint m_programId = 0;

    // Uniform locations are cached on first lookup to avoid repeated gl calls.
    mutable std::unordered_map<std::string, GLint> m_uniformLocationCache;

    GLint getUniformLocation(const std::string& name) const;

    // Compiles a single shader stage (GL_VERTEX_SHADER or GL_FRAGMENT_SHADER).
    static GLuint compileShaderStage(GLenum shaderType, const std::string& source);
};
