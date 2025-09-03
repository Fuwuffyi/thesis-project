#include "gl/resource/GLMaterial.hpp"

GLMaterial::GLMaterial(const MaterialTemplate& materialTemplate)
    : MaterialInstance(materialTemplate),
      m_ubo(GLBuffer::Type::Uniform, GLBuffer::Usage::DynamicDraw) {}

void GLMaterial::Bind(const uint32_t bindingPoint) {
   UpdateUBO();
   m_ubo.BindBase(bindingPoint);
   // TODO: Bind textures?
}

void GLMaterial::UpdateUBO() {
   if (!IsUBODirty()) return;
   UpdateUBOData();
   m_ubo.UploadData(GetUBOData(), GetUBOSize());
   ClearDirty();
}

void* GLMaterial::GetNativeHandle() const {
   return reinterpret_cast<void*>(static_cast<uintptr_t>(m_ubo.Get()));
}

