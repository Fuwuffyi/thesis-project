#include "core/resource/MaterialTemplate.hpp"

#include <stdexcept>
#include <utility>

MaterialTemplate::MaterialTemplate(const std::string name) noexcept : m_name(std::move(name)) {}

void MaterialTemplate::AddParameter(const std::string_view name,
                                    const ParameterDescriptor::Type type,
                                    const MaterialParam& defaultValue) {
   if (m_finalized) {
      throw std::runtime_error("Cannot add parameters to finalized template");
   }
   const ParameterDescriptor desc{
      .type = type,
      .name = std::string(name),
      .defaultValue = defaultValue,
      .offset = 0,
      .size = GetTypeSize(type),
   };
   m_parameters.emplace(desc.name, std::move(desc));
}

void MaterialTemplate::AddTexture(const std::string_view name, const uint32_t bindingSlot,
                                  const std::string_view samplerName,
                                  const TextureHandle defaultTexture) {
   if (m_finalized) {
      throw std::runtime_error("Cannot add textures to finalized template");
   }
   const TextureDescriptor desc{
      .name = std::string(name),
      .bindingSlot = bindingSlot,
      .samplerName = std::string(samplerName),
      .defaultTexture = defaultTexture,
   };
   m_textures.emplace(desc.name, std::move(desc));
}

void MaterialTemplate::Finalize() noexcept {
   if (m_finalized) [[unlikely]]
      return;
   uint32_t currentOffset = 0;
   for (auto& [_, desc] : m_parameters) {
      const auto alignment = GetTypeAlignment(desc.type);
      currentOffset = AlignOffset(currentOffset, alignment);
      desc.offset = currentOffset;
      currentOffset += desc.size;
   }
   m_uboSize = AlignOffset(currentOffset, 16);
   m_finalized = true;
}

constexpr uint32_t MaterialTemplate::GetTypeSize(const ParameterDescriptor::Type type) noexcept {
   using Type = ParameterDescriptor::Type;
   switch (type) {
      case Type::Float:
      case Type::Int:
      case Type::UInt:
         return 4;
      case Type::Vec2:
         return 8;
      case Type::Vec3:
      case Type::Vec4:
         return 16;
      case Type::Mat2:
         return 32;
      case Type::Mat3:
         return 48;
      case Type::Mat4:
         return 64;
   }
   std::unreachable();
}

constexpr uint32_t MaterialTemplate::GetTypeAlignment(
   const ParameterDescriptor::Type type) noexcept {
   using Type = ParameterDescriptor::Type;
   switch (type) {
      case Type::Float:
      case Type::Int:
      case Type::UInt:
         return 4;
      case Type::Vec2:
         return 8;
      case Type::Vec3:
      case Type::Vec4:
      case Type::Mat2:
      case Type::Mat3:
      case Type::Mat4:
         return 16;
   }
   std::unreachable();
}

constexpr uint32_t MaterialTemplate::AlignOffset(const uint32_t offset,
                                                 const uint32_t alignment) noexcept {
   return (offset + alignment - 1) & ~(alignment - 1);
}
