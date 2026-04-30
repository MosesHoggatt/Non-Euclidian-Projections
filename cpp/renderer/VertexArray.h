#pragma once
#include <vector>
#include <cstdint>

#ifdef __EMSCRIPTEN__
  #include <GLES3/gl3.h>
#else
  #include <GL/gl.h>
#endif

// ─────────────────────────────────────────────────────────────────────────────
//  renderer/VertexArray.h
//
//  Wraps a GL Vertex Array Object (VAO). A VAO remembers how vertex attribute
//  data is laid out in a VBO — position at location 0, normal at location 1,
//  UV at location 2, etc. — so we don't repeat that setup on every draw call.
//
//  Vertex layout attribute description:
//      location:       the `layout(location = N)` index in the GLSL vertex shader
//      componentCount: number of floats per vertex for this attribute (1, 2, 3, or 4)
//      strideInBytes:  total bytes per vertex across all attributes
//      offsetInBytes:  byte offset of this attribute within the vertex struct
//
//  Example for a vertex with position (vec3) + normal (vec3) + UV (vec2):
//      stride = (3 + 3 + 2) * sizeof(float) = 32 bytes
//      position offset = 0
//      normal   offset = 12
//      UV       offset = 24
// ─────────────────────────────────────────────────────────────────────────────

struct VertexAttributeDescriptor {
    GLuint   shaderLocation;   // matches layout(location = N) in vertex shader
    GLint    componentCount;   // number of floats: 1, 2, 3, or 4
    GLsizei  strideInBytes;    // total bytes per vertex
    GLsizei  offsetInBytes;    // byte offset of this attribute within the vertex
};

class VertexArray {
public:
    VertexArray();
    ~VertexArray();

    VertexArray(const VertexArray&)            = delete;
    VertexArray& operator=(const VertexArray&) = delete;

    VertexArray(VertexArray&& other) noexcept;
    VertexArray& operator=(VertexArray&& other) noexcept;

    void bind()   const;
    void unbind() const;

    // Defines how the currently-bound VBO's data maps to vertex shader attributes.
    // Must be called while the target VBO is bound.
    void addAttributeDescriptor(const VertexAttributeDescriptor& descriptor);

    GLuint arrayId() const { return m_arrayId; }

private:
    GLuint m_arrayId = 0;
};
