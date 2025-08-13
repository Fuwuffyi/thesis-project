#include "GLShader.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <iostream>
#include <utility>
#include <stdexcept>

GLShader::GLShader() {
   m_program = glCreateProgram();
   if (m_program == 0) {
      throw std::runtime_error("Failed to create OpenGL shader program.");
   }
}

GLShader::~GLShader() {
   if (m_program != 0) {
      glDeleteProgram(m_program);
   }
}

GLShader::GLShader(GLShader&& other) noexcept
   :
   m_program(std::exchange(other.m_program, 0)),
   m_isLinked(std::exchange(other.m_isLinked, false)),
   m_uniformLocations(std::move(other.m_uniformLocations))
{}

GLShader& GLShader::operator=(GLShader&& other) noexcept {
   if (this != &other) {
      if (m_program != 0) {
         glDeleteProgram(m_program);
      }
      m_program = std::exchange(other.m_program, 0);
      m_isLinked = std::exchange(other.m_isLinked, false);
      m_uniformLocations = std::move(other.m_uniformLocations);
   }
   return *this;
}

void GLShader::AttachShaderFromFile(const Type type, const std::string& filepath) {
   const std::string source = ReadFile(filepath);
   AttachShaderFromSource(type, source);
}

void GLShader::AttachShaderFromSource(const Type type, const std::string& source) {
   if (m_program == 0) {
      throw std::runtime_error("Cannot attach shader to invalid program.");
   }
   const GLuint shader = CompileShader(type, source);
   glAttachShader(m_program, shader);
   glDeleteShader(shader);
}

void GLShader::Link() {
   if (m_program == 0) {
      throw std::runtime_error("Cannot link invalid shader program.");
   }
   glLinkProgram(m_program);
   GLint success;
   glGetProgramiv(m_program, GL_LINK_STATUS, &success);
   if (!success) {
      GLint logLength;
      glGetProgramiv(m_program, GL_INFO_LOG_LENGTH, &logLength);
      std::string infoLog(logLength, '\0');
      glGetProgramInfoLog(m_program, logLength, nullptr, infoLog.data());
      throw std::runtime_error("Shader program linking failed: " + infoLog);
   }
   m_isLinked = true;
}

void GLShader::Use() const {
   if (m_program != 0 && m_isLinked) {
      glUseProgram(m_program);
   }
}

void GLShader::Unbind() const {
   glUseProgram(0);
}

void GLShader::SetBool(const std::string& name, const bool value) {
   glUniform1i(GetUniformLocation(name), static_cast<int>(value));
}

void GLShader::SetInt(const std::string& name, const int32_t value) {
   glUniform1i(GetUniformLocation(name), value);
}

void GLShader::SetUint(const std::string& name, const uint32_t value) {
   glUniform1i(GetUniformLocation(name), value);
}

void GLShader::SetFloat(const std::string& name, const float value) {
   glUniform1f(GetUniformLocation(name), value);
}

void GLShader::SetVec2(const std::string& name, const glm::vec2& value) {
   glUniform2fv(GetUniformLocation(name), 1, glm::value_ptr(value));
}

void GLShader::SetVec3(const std::string& name, const glm::vec3& value) {
   glUniform3fv(GetUniformLocation(name), 1, glm::value_ptr(value));
}

void GLShader::SetVec4(const std::string& name, const glm::vec4& value) {
   glUniform4fv(GetUniformLocation(name), 1, glm::value_ptr(value));
}

void GLShader::SetMat2(const std::string& name, const glm::mat2& value) {
   glUniformMatrix2fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

void GLShader::SetMat3(const std::string& name, const glm::mat3& value) {
   glUniformMatrix3fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

void GLShader::SetMat4(const std::string& name, const glm::mat4& value) {
   glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

void GLShader::BindUniformBlock(const std::string& blockName, const GLuint bindingPoint) {
   if (m_program == 0) {
      throw std::runtime_error("Cannot bind uniform block on invalid shader program.");
   }
   const GLuint blockIndex = glGetUniformBlockIndex(m_program, blockName.c_str());
   if (blockIndex == GL_INVALID_INDEX) {
      throw std::runtime_error("Uniform block '" + blockName + "' not found.");
   }
   glUniformBlockBinding(m_program, blockIndex, bindingPoint);
}

GLuint GLShader::CompileShader(const Type type, const std::string& source) {
   GLuint shader = glCreateShader(static_cast<GLenum>(type));
   if (shader == 0) {
      throw std::runtime_error("Failed to create shader");
   }
   const char* sourceCStr = source.c_str();
   glShaderSource(shader, 1, &sourceCStr, nullptr);
   glCompileShader(shader);
   GLint success;
   glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
   if (!success) {
      GLint logLength;
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
      std::string infoLog(logLength, '\0');
      glGetShaderInfoLog(shader, logLength, nullptr, infoLog.data());
      glDeleteShader(shader);
      std::string shaderTypeName;
      switch (type) {
         case Type::Vertex: shaderTypeName = "vertex"; break;
         case Type::Fragment: shaderTypeName = "fragment"; break;
         case Type::Compute: shaderTypeName = "compute"; break;
      }
      throw std::runtime_error("Shader compilation failed (" + shaderTypeName + "): " + infoLog);
   }
   return shader;
}

std::string GLShader::ReadFile(const std::string& filepath) {
   std::ifstream file(filepath);
   if (!file.is_open()) {
      throw std::runtime_error("Failed to open shader file: " + filepath);
   }
   std::stringstream buffer;
   buffer << file.rdbuf();
   return buffer.str();
}

GLint GLShader::GetUniformLocation(const std::string& name) {
   auto it = m_uniformLocations.find(name);
   if (it != m_uniformLocations.end()) {
      return it->second;
   }
   const GLint location = glGetUniformLocation(m_program, name.c_str());
   m_uniformLocations[name] = location;
   return location;
}

GLuint GLShader::Get() const {
   return m_program;
}

bool GLShader::IsValid() const {
   return m_program != 0;
}

bool GLShader::IsLinked() const {
   return m_isLinked;
}

