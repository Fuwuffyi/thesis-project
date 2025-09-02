#pragma once

#include "MaterialTemplate.hpp"
#include "IMaterial.hpp"

#include <vector>

class MaterialInstance : public IMaterial {
  public:
   MaterialInstance(const MaterialTemplate& materialTemplate);
   virtual ~MaterialInstance() = default;

   // IResource implementation
   ResourceType GetType() const override { return ResourceType::Material; }
   size_t GetMemoryUsage() const override;
   bool IsValid() const override { return m_template != nullptr; }

   // IMaterial implementation
   void SetParameter(const std::string& name, const MaterialParam& value) override;
   MaterialParam GetParameter(const std::string& name) const override;
   bool HasParameter(const std::string& name) const override;

   void SetTexture(const std::string& name, const TextureHandle& texture) override;
   TextureHandle GetTexture(const std::string& name) const override;
   bool HasTexture(const std::string& name) const override;

   const std::string& GetTemplateName() const override;

   // Get raw UBO data for upload
   const void* GetUBOData() const { return m_uboData.data(); }
   uint32_t GetUBOSize() const;

   // Mark UBO as dirty (needs update)
   void MarkDirty() { m_uboDirty = true; }
   bool IsUBODirty() const { return m_uboDirty; }
   void ClearDirty() { m_uboDirty = false; }

  protected:
   const MaterialTemplate* m_template;
   std::unordered_map<std::string, MaterialParam> m_parameters;
   std::unordered_map<std::string, TextureHandle> m_textures;
   std::vector<uint8_t> m_uboData;
   mutable bool m_uboDirty = true;

   void UpdateUBOData();
   void WriteParamToUBO(const std::string& name, const MaterialParam& value);
};
