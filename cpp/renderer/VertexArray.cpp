#include "VertexArray.h"

// ─────────────────────────────────────────────────────────────────────────────
//  renderer/VertexArray.cpp
// ─────────────────────────────────────────────────────────────────────────────

VertexArray::VertexArray() {
    glGenVertexArrays(1, &m_arrayId);
}

VertexArray::~VertexArray() {
    if (m_arrayId != 0) glDeleteVertexArrays(1, &m_arrayId);
}

VertexArray::VertexArray(VertexArray&& other) noexcept : m_arrayId(other.m_arrayId) {
    other.m_arrayId = 0;
}

VertexArray& VertexArray::operator=(VertexArray&& other) noexcept {
    if (this != &other) {
        if (m_arrayId != 0) glDeleteVertexArrays(1, &m_arrayId);
        m_arrayId = other.m_arrayId;
        other.m_arrayId = 0;
    }
    return *this;
}

void VertexArray::bind()   const { glBindVertexArray(m_arrayId); }
void VertexArray::unbind() const { glBindVertexArray(0); }

void VertexArray::addAttributeDescriptor(const VertexAttributeDescriptor& descriptor) {
    glBindVertexArray(m_arrayId);
    glEnableVertexAttribArray(descriptor.shaderLocation);
    glVertexAttribPointer(
        descriptor.shaderLocation,
        descriptor.componentCount,
        GL_FLOAT,
        GL_FALSE,                               // not normalized
        descriptor.strideInBytes,
        reinterpret_cast<const void*>(static_cast<uintptr_t>(descriptor.offsetInBytes))
    );
    glBindVertexArray(0);
}
