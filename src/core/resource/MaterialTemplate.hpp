#pragma once

#include "core/resource/IMaterial.hpp"

#include <unordered_map>
#include <string>
#include <string_view>
#include <cstdint>

struct ParameterDescriptor final {
   enum class Type { Float, Int, UInt, Vec2, Vec3, Vec4, Mat2, Mat3, Mat4 };

   Type type{};
   std::string name;
   MaterialParam defaultValue{};
   uint32_t offset{};
   uint32_t size{};
};

struct TextureDescriptor final {
   std::string name;
   uint32_t bindingSlot{};
   std::string samplerName;
   TextureHandle defaultTexture{};
};

class MaterialTemplate final {
  public:
   explicit MaterialTemplate(const std::string name) noexcept;

   // Parameter definitions
   void AddParameter(const std::string_view name, const ParameterDescriptor::Type type,
                     const MaterialParam& defaultValue);
   void AddTexture(const std::string_view name, const uint32_t bindingSlot,
                   const std::string_view samplerName, const TextureHandle defaultTexture);

   // Getters
   [[nodiscard]] constexpr std::string_view GetName() const noexcept { return m_name; }
   [[nodiscard]] constexpr const auto& GetParameters() const noexcept { return m_parameters; }
   [[nodiscard]] constexpr const auto& GetTextures() const noexcept { return m_textures; }

   [[nodiscard]] constexpr uint32_t GetUBOSize() const noexcept { return m_uboSize; }
   [[nodiscard]] constexpr bool IsFinalized() const noexcept { return m_finalized; }

   // Finalize the template
   void Finalize() noexcept;

  private:
   std::string m_name;
   std::unordered_map<std::string, ParameterDescriptor> m_parameters;
   std::unordered_map<std::string, TextureDescriptor> m_textures;
   uint32_t m_uboSize = 0;
   bool m_finalized = false;

   [[nodiscard]] static constexpr uint32_t GetTypeSize(
      const ParameterDescriptor::Type type) noexcept;
   [[nodiscard]] static constexpr uint32_t GetTypeAlignment(
      ParameterDescriptor::Type type) noexcept;
   [[nodiscard]] static constexpr uint32_t AlignOffset(const uint32_t offset,
                                                       const uint32_t alignment) noexcept;
};
