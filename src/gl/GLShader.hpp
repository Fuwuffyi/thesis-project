#pragma once

#include <glad/gl.h>

#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

class GLShader final {
  public:
   enum class Type : uint32_t {
      Vertex = GL_VERTEX_SHADER,
      Fragment = GL_FRAGMENT_SHADER,
      Geometry = GL_GEOMETRY_SHADER,
      Compute = GL_COMPUTE_SHADER,
      TessControl = GL_TESS_CONTROL_SHADER,
      TessEvaluation = GL_TESS_EVALUATION_SHADER
   };

   GLShader();
   ~GLShader();

   GLShader(const GLShader&) = delete;
   GLShader& operator=(const GLShader&) = delete;
   GLShader(GLShader&& other) noexcept;
   GLShader& operator=(GLShader&& other) noexcept;

   void AttachShaderFromFile(const Type type, const std::string_view filepath);
   void AttachShaderFromSource(const Type type, const std::string_view source);
   void Link();

   void Use() const noexcept;
   static void Unbind() noexcept;

   // Uniform setters
   void SetBool(const std::string_view name, const bool value) const noexcept;
   void SetInt(const std::string_view name, const int32_t value) const noexcept;
   void SetUint(const std::string_view name, const uint32_t value) const noexcept;
   void SetFloat(const std::string_view name, const float value) const noexcept;
   void SetVec2(const std::string_view name, const glm::vec2& value) const noexcept;
   void SetVec3(const std::string_view name, const glm::vec3& value) const noexcept;
   void SetVec4(const std::string_view name, const glm::vec4& value) const noexcept;
   void SetMat2(const std::string_view name, const glm::mat2& value) const noexcept;
   void SetMat3(const std::string_view name, const glm::mat3& value) const noexcept;
   void SetMat4(const std::string_view name, const glm::mat4& value) const noexcept;

   void BindUniformBlock(const std::string_view blockName, const uint32_t bindingPoint) const;

   [[nodiscard]] constexpr uint32_t Get() const noexcept { return m_program; }
   [[nodiscard]] constexpr bool IsValid() const noexcept { return m_program != 0; }
   [[nodiscard]] constexpr bool IsLinked() const noexcept { return m_isLinked; }

  private:
   [[nodiscard]] static uint32_t CompileShader(const Type type, const std::string_view source);
   [[nodiscard]] static std::string ReadFile(const std::string_view filepath);
   [[nodiscard]] uint32_t GetUniformLocation(const std::string_view name) const;

  private:
   uint32_t m_program{0};
   bool m_isLinked{false};
   mutable std::unordered_map<std::string, uint32_t> m_uniformLocations;
};
