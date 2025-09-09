#include "gl/resource/GLMaterial.hpp"

#include "core/resource/ResourceManager.hpp"

GLMaterial::GLMaterial(const MaterialTemplate& materialTemplate)
    : MaterialInstance(materialTemplate),
      m_ubo(GLBuffer::Type::Uniform, GLBuffer::Usage::DynamicDraw) {}

void GLMaterial::Bind(const uint32_t bindingPoint, const ResourceManager& resourceManager) {
   UpdateUBO();
   m_ubo.BindBase(bindingPoint);
   const auto& textureDescriptors = m_template->GetTextures();
   for (const auto& [textureName, descriptor] : textureDescriptors) {
      ITexture* texture = nullptr;
      const TextureHandle& th = GetTexture(textureName);
      if (th.IsValid()) {
         texture = resourceManager.GetTexture(th);
      }
      if (!texture && descriptor.defaultTexture.IsValid()) {
         texture = resourceManager.GetTexture(descriptor.defaultTexture);
      }
      if (texture && texture->IsValid()) {
         texture->Bind(descriptor.bindingSlot);
      }
   }
}

void GLMaterial::UpdateUBO() {
   if (!IsUBODirty())
      return;
   UpdateUBOData();
   m_ubo.UploadData(GetUBOData(), GetUBOSize());
   ClearDirty();
}

void* GLMaterial::GetNativeHandle() const noexcept {
   return reinterpret_cast<void*>(static_cast<uintptr_t>(m_ubo.Get()));
}
