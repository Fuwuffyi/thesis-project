#include "GLSampler.hpp"

#include <stdexcept>

GLSampler::GLSampler(const GLSamplerDesc& desc)
:
   m_id(0)
{
   glCreateSamplers(1, &m_id);
   if (m_id == 0) {
      throw std::runtime_error("glCreateSamplers failed.");
   }
   // Filters
   glSamplerParameteri(m_id, GL_TEXTURE_MAG_FILTER, desc.magFilter);
   glSamplerParameteri(m_id, GL_TEXTURE_MIN_FILTER, desc.minFilter);
   // Wraps
   glSamplerParameteri(m_id, GL_TEXTURE_WRAP_S, desc.wrapS);
   glSamplerParameteri(m_id, GL_TEXTURE_WRAP_T, desc.wrapT);
   glSamplerParameteri(m_id, GL_TEXTURE_WRAP_R, desc.wrapR);
   // LOD
   glSamplerParameterf(m_id, GL_TEXTURE_MIN_LOD, desc.minLod);
   glSamplerParameterf(m_id, GL_TEXTURE_MAX_LOD, desc.maxLod);
   glSamplerParameterf(m_id, GL_TEXTURE_LOD_BIAS, desc.lodBias);
   // Compare
   if (desc.compareEnable) {
      glSamplerParameteri(m_id, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
      glSamplerParameteri(m_id, GL_TEXTURE_COMPARE_FUNC, desc.compareFunc);
   } else {
      glSamplerParameteri(m_id, GL_TEXTURE_COMPARE_MODE, GL_NONE);
   }
   // Anisotropy
   if (desc.enableAniso) {
      const GLfloat maxAniso = desc.maxAniso;
      glSamplerParameterf(m_id, GL_TEXTURE_MAX_ANISOTROPY, maxAniso);
   }
}

GLSampler::~GLSampler() {
   if (m_id != 0) {
      glDeleteSamplers(1, &m_id);
      m_id = 0;
   }
}

GLSampler::GLSampler(GLSampler&& other) noexcept
   :
   m_id(other.m_id)
{
   other.m_id = 0;
}

GLSampler& GLSampler::operator=(GLSampler&& other) noexcept {
   if (this != &other) {
      if (m_id != 0) {
         glDeleteSamplers(1, &m_id);
      }
      m_id = other.m_id;
      other.m_id = 0;
   }
   return *this;
}

void GLSampler::BindUnit(const GLuint unit) const {
   glBindSampler(unit, m_id);
}

GLSampler GLSampler::CreateLinear() {
   GLSamplerDesc d;
   d.magFilter = GL_LINEAR;
   d.minFilter = GL_LINEAR_MIPMAP_LINEAR;
   return GLSampler(d);
}

GLSampler GLSampler::CreateNearest() {
   GLSamplerDesc d;
   d.magFilter = GL_NEAREST;
   d.minFilter = GL_NEAREST_MIPMAP_NEAREST;
   return GLSampler(d);
}

GLSampler GLSampler::CreateAnisotropic(const float maxAniso) {
   GLSamplerDesc d;
   d.magFilter = GL_LINEAR;
   d.minFilter = GL_LINEAR_MIPMAP_LINEAR;
   d.enableAniso = true;
   d.maxAniso = maxAniso;
   return GLSampler(d);
}

