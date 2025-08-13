#pragma once

#include <glad/gl.h>
#include <cstdint>
#include <vector>

class GLBuffer;
struct Vertex;

class GLVertexArray {
public:
   GLVertexArray();
   ~GLVertexArray();

   GLVertexArray(const GLVertexArray&) = delete;
   GLVertexArray& operator=(const GLVertexArray&) = delete;
   GLVertexArray(GLVertexArray&& other) noexcept;
   GLVertexArray& operator=(GLVertexArray&& other) noexcept;

   void Bind() const;
   void Unbind() const;

   // Attach a vertex buffer to this VAO
   void AttachVertexBuffer(const GLBuffer& buffer, const GLuint bindingIndex = 0,
                           const GLintptr offset = 0, const GLsizei stride = 0);

   // Attach an element buffer to this VAO
   void AttachElementBuffer(const GLBuffer& buffer);

   // Setup vertex attribute pointers for common vertex formats
   void SetupVertexAttributes();

   // Manual vertex attribute setup
   void EnableAttribute(const GLuint index);
   void DisableAttribute(const GLuint index);
   void SetAttributeFormat(const GLuint index, const GLint size, const GLenum type,
                           const GLboolean normalized, const GLuint relativeOffset);
   void SetAttributeBinding(const GLuint index, const GLuint bindingIndex);

   // Drawing functions
   void DrawArrays(const GLenum mode, const GLint first, const GLsizei count) const;
   void DrawElements(const GLenum mode, const GLsizei count, const GLenum type, const void* indices = nullptr) const;
   void DrawElementsInstanced(const GLenum mode, const GLsizei count, const GLenum type,
                              const void* indices, const GLsizei instanceCount) const;

   GLuint Get() const;
   bool IsValid() const;

private:
   GLuint m_vao = 0;
};

