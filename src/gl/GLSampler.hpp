#pragma once

#include <glad/gl.h>

struct GLSamplerDesc {
   GLenum magFilter = GL_LINEAR;
   GLenum minFilter = GL_LINEAR_MIPMAP_LINEAR;
   GLenum wrapS = GL_REPEAT;
   GLenum wrapT = GL_REPEAT;
   GLenum wrapR = GL_REPEAT;
   float minLod = 0.0f;
   float maxLod = 1000.0f;
   float lodBias = 0.0f;
   bool enableAniso = false;
   float maxAniso = 1.0f;
   bool compareEnable = false;
   GLenum compareFunc = GL_LEQUAL;
};

class GLSampler {
public:
   GLSampler(const GLSamplerDesc& desc = {});
   ~GLSampler();

   GLSampler(const GLSampler&) = delete;
   GLSampler& operator=(const GLSampler&) = delete;
   GLSampler(GLSampler&& other) noexcept;
   GLSampler& operator=(GLSampler&& other) noexcept;

   GLuint Get() const;
   void BindUnit(const GLuint unit) const;

   static GLSampler CreateLinear();
   static GLSampler CreateNearest();
   static GLSampler CreateAnisotropic(const float maxAniso);

private:
   GLuint m_id;
};

