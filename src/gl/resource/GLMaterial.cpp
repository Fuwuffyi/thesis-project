#include "gl/resource/GLMaterial.hpp"

#include "core/resource/ResourceManager.hpp"

GLMaterial::GLMaterial(const MaterialTemplate& materialTemplate)
    : MaterialInstance(materialTemplate),
      m_ubo(GLBuffer::Type::Uniform, GLBuffer::Usage::DynamicDraw) {}

void GLMaterial::Bind(const uint32_t bindingPoint, const ResourceManager& resourceManager) {
   UpdateUBO();
   m_ubo.BindBase(bindingPoint);
   // Bind textures
   const auto& textureDescriptors = m_template->GetTextures();
   for (const auto& [textureName, descriptor] : textureDescriptors) {
      const TextureHandle& textureHandle = GetTexture(textureName);
      // Try to get the texture from the handle first
      ITexture* texture = nullptr;
      if (textureHandle.IsValid()) {
         texture = resourceManager.GetTexture(textureHandle);
      }
      // If no texture is set or texture is invalid, try to use default
      if (!texture) {
         if (descriptor.defaultTexture.IsValid()) {
            texture = resourceManager.GetTexture(descriptor.defaultTexture);
         }
      }
      // Bind the texture if we have one
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

void* GLMaterial::GetNativeHandle() const {
   return reinterpret_cast<void*>(static_cast<uintptr_t>(m_ubo.Get()));
}
