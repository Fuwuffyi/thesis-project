#include "core/resource/MaterialTemplate.hpp"

#include <algorithm>
#include <stdexcept>
#include <vector>

MaterialTemplate::MaterialTemplate(std::string name) : m_name(std::move(name)) {}

void MaterialTemplate::AddParameter(const std::string& name, ParameterDescriptor::Type type,
                                    const MaterialParam& defaultValue) {
   if (m_finalized) {
      throw std::runtime_error("Cannot add parameters to finalized template");
   }
   ParameterDescriptor desc;
   desc.name = name;
   desc.type = type;
   desc.defaultValue = defaultValue;
   desc.size = GetTypeSize(type);
   desc.offset = 0; // Will be calculated during finalization
   m_parameters[name] = desc;
}

void MaterialTemplate::AddTexture(const std::string& name, uint32_t bindingSlot,
                                  const std::string& samplerName,
                                  const TextureHandle& defaultTexture) {
   if (m_finalized) {
      throw std::runtime_error("Cannot add textures to finalized template");
   }

   TextureDescriptor desc;
   desc.name = name;
   desc.bindingSlot = bindingSlot;
   desc.samplerName = samplerName;
   desc.defaultTexture = defaultTexture;

   m_textures[name] = desc;
}

void MaterialTemplate::Finalize() {
   if (m_finalized)
      return;
   uint32_t currentOffset = 0;
   for (auto& [name, desc] : m_parameters) {
      uint32_t alignment = GetTypeAlignment(desc.type);
      currentOffset = AlignOffset(currentOffset, alignment);
      desc.offset = currentOffset;
      currentOffset += desc.size;
   }
   m_uboSize = AlignOffset(currentOffset, 16);
   m_finalized = true;
}

uint32_t MaterialTemplate::GetTypeSize(ParameterDescriptor::Type type) const {
   switch (type) {
      case ParameterDescriptor::Type::Float:
      case ParameterDescriptor::Type::Int:
      case ParameterDescriptor::Type::UInt:
         return 4;
      case ParameterDescriptor::Type::Vec2:
         return 8;
      case ParameterDescriptor::Type::Vec3:
         return 16; // vec3 is padded to vec4 in std140
      case ParameterDescriptor::Type::Vec4:
         return 16;
      case ParameterDescriptor::Type::Mat2:
         return 32; // 2 columns * 16 bytes each (vec4 alignment)
      case ParameterDescriptor::Type::Mat3:
         return 48; // 3 columns * 16 bytes each (vec4 alignment)
      case ParameterDescriptor::Type::Mat4:
         return 64; // 4 columns * 16 bytes each
      default:
         return 4;
   }
}

uint32_t MaterialTemplate::GetTypeAlignment(ParameterDescriptor::Type type) const {
   switch (type) {
      case ParameterDescriptor::Type::Float:
      case ParameterDescriptor::Type::Int:
      case ParameterDescriptor::Type::UInt:
         return 4;
      case ParameterDescriptor::Type::Vec2:
         return 8;
      case ParameterDescriptor::Type::Vec3:
      case ParameterDescriptor::Type::Vec4:
      case ParameterDescriptor::Type::Mat2:
      case ParameterDescriptor::Type::Mat3:
      case ParameterDescriptor::Type::Mat4:
         return 16;
      default:
         return 4;
   }
}

uint32_t MaterialTemplate::AlignOffset(uint32_t offset, uint32_t alignment) const {
   return (offset + alignment - 1) & ~(alignment - 1);
}
