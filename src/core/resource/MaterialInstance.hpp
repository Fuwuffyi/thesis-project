#pragma once

#include "MaterialTemplate.hpp"
#include "IMaterial.hpp"

#include <unordered_map>
#include <vector>
#include <string>
#include <string_view>

class MaterialInstance : public IMaterial {
  public:
   explicit MaterialInstance(const MaterialTemplate& materialTemplate);
   ~MaterialInstance() override = default;

   // IResource implementation
   [[nodiscard]] constexpr ResourceType GetType() const noexcept override {
      return ResourceType::Material;
   }
   [[nodiscard]] size_t GetMemoryUsage() const noexcept override;
   [[nodiscard]] constexpr bool IsValid() const noexcept override { return m_template != nullptr; }

   // IMaterial implementation
   void SetParameter(const std::string_view name, const MaterialParam& value) override;
   [[nodiscard]] MaterialParam GetParameter(const std::string_view name) const override;
   [[nodiscard]] bool HasParameter(const std::string_view name) const noexcept override;

   void SetTexture(const std::string_view name, const TextureHandle texture) override;
   [[nodiscard]] TextureHandle GetTexture(const std::string_view name) const override;
   [[nodiscard]] bool HasTexture(const std::string_view name) const noexcept override;

   [[nodiscard]] std::string_view GetTemplateName() const noexcept override;

   // Get raw UBO data for upload
   [[nodiscard]] constexpr const void* GetUBOData() const noexcept { return m_uboData.data(); }
   [[nodiscard]] uint32_t GetUBOSize() const noexcept;

   // Mark UBO as dirty (needs update)
   constexpr void MarkDirty() noexcept { m_uboDirty = true; }
   [[nodiscard]] constexpr bool IsUBODirty() const noexcept { return m_uboDirty; }
   constexpr void ClearDirty() noexcept { m_uboDirty = false; }

  protected:
   const MaterialTemplate* m_template{};
   std::unordered_map<std::string, MaterialParam> m_parameters;
   std::unordered_map<std::string, TextureHandle> m_textures;
   std::vector<std::byte> m_uboData;
   mutable bool m_uboDirty{true};

   void UpdateUBOData();
   void WriteParamToUBO(const std::string_view name, const MaterialParam& value);
};
