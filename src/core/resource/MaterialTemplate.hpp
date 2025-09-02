#pragma once

#include "core/resource/IMaterial.hpp"

#include <unordered_map>

struct ParameterDescriptor {
   enum class Type { Float, Int, UInt, Vec2, Vec3, Vec4, Mat3, Mat4 };

   Type type;
   std::string name;
   MaterialParam defaultValue;
   uint32_t offset; // Offset in UBO
   uint32_t size;   // Size in bytes
};

struct TextureDescriptor {
   std::string name;
   uint32_t bindingSlot;
   std::string samplerName;
   TextureHandle defaultTexture;
};

class MaterialTemplate {
  public:
   MaterialTemplate(std::string name);

   // Parameter definitions
   void AddParameter(const std::string& name, ParameterDescriptor::Type type,
                     const MaterialParam& defaultValue);
   void AddTexture(const std::string& name, uint32_t bindingSlot, const std::string& samplerName,
                   const TextureHandle& defaultTexture = {});

   // Getters
   const std::string& GetName() const { return m_name; }
   const std::unordered_map<std::string, ParameterDescriptor>& GetParameters() const {
      return m_parameters;
   }
   const std::unordered_map<std::string, TextureDescriptor>& GetTextures() const {
      return m_textures;
   }

   uint32_t GetUBOSize() const { return m_uboSize; }
   bool IsFinalized() const { return m_finalized; }

   // Finalize the template
   void Finalize();

  private:
   std::string m_name;
   std::unordered_map<std::string, ParameterDescriptor> m_parameters;
   std::unordered_map<std::string, TextureDescriptor> m_textures;
   uint32_t m_uboSize = 0;
   bool m_finalized = false;

   uint32_t GetTypeSize(ParameterDescriptor::Type type) const;
   uint32_t GetTypeAlignment(ParameterDescriptor::Type type) const;
   uint32_t AlignOffset(uint32_t offset, uint32_t alignment) const;
};
