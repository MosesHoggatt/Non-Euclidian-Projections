#include "VertexBuffer.h"

// ─────────────────────────────────────────────────────────────────────────────
//  renderer/VertexBuffer.cpp
// ─────────────────────────────────────────────────────────────────────────────

// ── VertexBuffer ──────────────────────────────────────────────────────────────

VertexBuffer::VertexBuffer(const void* dataPointer, GLsizeiptr dataSizeInBytes) {
    glGenBuffers(1, &m_bufferId);
    glBindBuffer(GL_ARRAY_BUFFER, m_bufferId);
    // GL_DYNAMIC_DRAW: data may be updated (e.g. when switching projections)
    glBufferData(GL_ARRAY_BUFFER, dataSizeInBytes, dataPointer, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

VertexBuffer::~VertexBuffer() {
    if (m_bufferId != 0) glDeleteBuffers(1, &m_bufferId);
}

VertexBuffer::VertexBuffer(VertexBuffer&& other) noexcept : m_bufferId(other.m_bufferId) {
    other.m_bufferId = 0;
}

VertexBuffer& VertexBuffer::operator=(VertexBuffer&& other) noexcept {
    if (this != &other) {
        if (m_bufferId != 0) glDeleteBuffers(1, &m_bufferId);
        m_bufferId = other.m_bufferId;
        other.m_bufferId = 0;
    }
    return *this;
}

void VertexBuffer::bind()   const { glBindBuffer(GL_ARRAY_BUFFER, m_bufferId); }
void VertexBuffer::unbind() const { glBindBuffer(GL_ARRAY_BUFFER, 0); }

void VertexBuffer::updateData(const void* dataPointer, GLsizeiptr dataSizeInBytes,
                               GLintptr offsetInBytes) const {
    glBindBuffer(GL_ARRAY_BUFFER, m_bufferId);
    glBufferSubData(GL_ARRAY_BUFFER, offsetInBytes, dataSizeInBytes, dataPointer);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// ── IndexBuffer ───────────────────────────────────────────────────────────────

IndexBuffer::IndexBuffer(const unsigned int* indices, GLsizei indexCount)
    : m_indexCount(indexCount) {
    glGenBuffers(1, &m_bufferId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufferId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indexCount) * sizeof(unsigned int),
                 indices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

IndexBuffer::~IndexBuffer() {
    if (m_bufferId != 0) glDeleteBuffers(1, &m_bufferId);
}

IndexBuffer::IndexBuffer(IndexBuffer&& other) noexcept
    : m_bufferId(other.m_bufferId), m_indexCount(other.m_indexCount) {
    other.m_bufferId = 0;
    other.m_indexCount = 0;
}

IndexBuffer& IndexBuffer::operator=(IndexBuffer&& other) noexcept {
    if (this != &other) {
        if (m_bufferId != 0) glDeleteBuffers(1, &m_bufferId);
        m_bufferId   = other.m_bufferId;
        m_indexCount = other.m_indexCount;
        other.m_bufferId   = 0;
        other.m_indexCount = 0;
    }
    return *this;
}

void IndexBuffer::bind()   const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufferId); }
void IndexBuffer::unbind() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); }
