#pragma once

#include "core/resource/MaterialInstance.hpp"

#include "gl/GLBuffer.hpp"

class GLMaterial final : public MaterialInstance {
  public:
   explicit GLMaterial(const MaterialTemplate& materialTemplate);
   ~GLMaterial() override = default;

   void Bind(const uint32_t bindingPoint, const ResourceManager& resourceManager) override;
   void UpdateUBO() override;
   [[nodiscard]] void* GetNativeHandle() const noexcept override;

  private:
   GLBuffer m_ubo;
};
