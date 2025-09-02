#pragma once

#include <glad/gl.h>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

class GLShader {
  public:
   enum class Type : GLenum {
      Vertex = GL_VERTEX_SHADER,
      Fragment = GL_FRAGMENT_SHADER,
      Compute = GL_COMPUTE_SHADER
   };

   GLShader();
   ~GLShader();

   GLShader(const GLShader&) = delete;
   GLShader& operator=(const GLShader&) = delete;
   GLShader(GLShader&& other) noexcept;
   GLShader& operator=(GLShader&& other) noexcept;

   // Shader compilation
   void AttachShaderFromFile(const Type type, const std::string& filepath);
   void AttachShaderFromSource(const Type type, const std::string& source);
   void Link();

   // Shader usage
   void Use() const;
   void Unbind() const;

   // Uniform setters
   void SetBool(const std::string& name, const bool value);
   void SetInt(const std::string& name, const int32_t value);
   void SetUint(const std::string& name, const uint32_t value);
   void SetFloat(const std::string& name, const float value);
   void SetVec2(const std::string& name, const glm::vec2& value);
   void SetVec3(const std::string& name, const glm::vec3& value);
   void SetVec4(const std::string& name, const glm::vec4& value);
   void SetMat2(const std::string& name, const glm::mat2& value);
   void SetMat3(const std::string& name, const glm::mat3& value);
   void SetMat4(const std::string& name, const glm::mat4& value);

   // Uniform buffer binding
   void BindUniformBlock(const std::string& blockName, const GLuint bindingPoint);

   GLuint Get() const;
   bool IsValid() const;
   bool IsLinked() const;

  private:
   GLuint CompileShader(Type type, const std::string& source);
   std::string ReadFile(const std::string& filepath);
   GLint GetUniformLocation(const std::string& name);

  private:
   GLuint m_program = 0;
   bool m_isLinked = false;
   std::unordered_map<std::string, GLint> m_uniformLocations;
};
