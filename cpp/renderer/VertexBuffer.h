#pragma once
#include <vector>
#include <cstddef>

#ifdef __EMSCRIPTEN__
  #include <GLES3/gl3.h>
#else
  #include <GL/gl.h>
#endif

// ─────────────────────────────────────────────────────────────────────────────
//  renderer/VertexBuffer.h
//
//  A thin RAII wrapper around an OpenGL Vertex Buffer Object (VBO).
//  Uploads vertex data (positions, normals, UVs) to the GPU and manages
//  the lifetime of the underlying GL buffer.
//
//  Usage:
//      VertexBuffer vbo(vertices.data(), vertices.size() * sizeof(float));
//      vbo.bind();
//      // configure vertex attribute pointers
//      vbo.unbind();
// ─────────────────────────────────────────────────────────────────────────────

class VertexBuffer {
public:
    // Uploads the given data to the GPU as a static draw buffer.
    // dataPointer: raw pointer to the vertex data array.
    // dataSizeInBytes: total size of the data in bytes.
    VertexBuffer(const void* dataPointer, GLsizeiptr dataSizeInBytes);

    ~VertexBuffer();

    // Non-copyable.
    VertexBuffer(const VertexBuffer&)            = delete;
    VertexBuffer& operator=(const VertexBuffer&) = delete;

    // Movable.
    VertexBuffer(VertexBuffer&& other) noexcept;
    VertexBuffer& operator=(VertexBuffer&& other) noexcept;

    void bind()   const;
    void unbind() const;

    // Updates the buffer contents without re-allocating. The new data must be
    // the same size or smaller than what was originally uploaded.
    void updateData(const void* dataPointer, GLsizeiptr dataSizeInBytes,
                    GLintptr offsetInBytes = 0) const;

    GLuint bufferId() const { return m_bufferId; }

private:
    GLuint m_bufferId = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
//  renderer/IndexBuffer.h (included here for brevity)
//
//  Wraps a GL Element Array Buffer Object (EBO) — stores triangle indices
//  so vertices can be shared between triangles without duplication.
// ─────────────────────────────────────────────────────────────────────────────

class IndexBuffer {
public:
    // indexCount: number of individual indices (not triangles).
    IndexBuffer(const unsigned int* indices, GLsizei indexCount);

    ~IndexBuffer();

    IndexBuffer(const IndexBuffer&)            = delete;
    IndexBuffer& operator=(const IndexBuffer&) = delete;

    IndexBuffer(IndexBuffer&& other) noexcept;
    IndexBuffer& operator=(IndexBuffer&& other) noexcept;

    void bind()   const;
    void unbind() const;

    GLsizei indexCount() const { return m_indexCount; }
    GLuint  bufferId()   const { return m_bufferId; }

private:
    GLuint  m_bufferId   = 0;
    GLsizei m_indexCount = 0;
};
