#pragma once

#include "core/resource/MaterialInstance.hpp"
#include "gl/GLBuffer.hpp"

class GLMaterial final : public MaterialInstance {
  public:
   GLMaterial(const MaterialTemplate& materialTemplate);
   ~GLMaterial() override = default;

   // IMaterial implementation
   void Bind(const uint32_t bindingPoint) override;
   void UpdateUBO() override;
   void* GetNativeHandle() const override;

  private:
   GLBuffer m_ubo;
};
