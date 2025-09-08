#include "gl/GLShader.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

GLShader::GLShader() {
   m_program = glCreateProgram();
   if (m_program == 0) {
      throw std::runtime_error("Failed to create OpenGL shader program");
   }
}

GLShader::~GLShader() {
   if (m_program != 0) {
      glDeleteProgram(m_program);
   }
}

GLShader::GLShader(GLShader&& other) noexcept
    : m_program(std::exchange(other.m_program, 0)),
      m_isLinked(std::exchange(other.m_isLinked, false)),
      m_uniformLocations(std::move(other.m_uniformLocations)) {}

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

void GLShader::AttachShaderFromFile(const Type type, const std::string_view filepath) {
   const std::string source = ReadFile(filepath);
   AttachShaderFromSource(type, source);
}

void GLShader::AttachShaderFromSource(const Type type, const std::string_view source) {
   if (m_program == 0) {
      throw std::runtime_error("Cannot attach shader to invalid program");
   }
   const uint32_t shader = CompileShader(type, source);
   glAttachShader(m_program, shader);
   glDeleteShader(shader);
}

void GLShader::Link() {
   if (m_program == 0) {
      throw std::runtime_error("Cannot link invalid shader program");
   }
   glLinkProgram(m_program);
   int32_t success;
   glGetProgramiv(m_program, GL_LINK_STATUS, &success);
   if (!success) {
      int32_t logLength;
      glGetProgramiv(m_program, GL_INFO_LOG_LENGTH, &logLength);
      std::string infoLog(static_cast<size_t>(logLength), '\0');
      glGetProgramInfoLog(m_program, logLength, nullptr, infoLog.data());
      throw std::runtime_error("Shader program linking failed: " + infoLog);
   }
   m_isLinked = true;
}

void GLShader::Use() const noexcept {
   if (m_program != 0 && m_isLinked) {
      glUseProgram(m_program);
   }
}

void GLShader::Unbind() noexcept { glUseProgram(0); }

void GLShader::SetBool(const std::string_view name, const bool value) const noexcept {
   if (const int32_t location = GetUniformLocation(name); location != -1) {
      glUniform1i(location, static_cast<int>(value));
   }
}

void GLShader::SetInt(const std::string_view name, const int32_t value) const noexcept {
   if (const int32_t location = GetUniformLocation(name); location != -1) {
      glUniform1i(location, value);
   }
}

void GLShader::SetUint(const std::string_view name, const uint32_t value) const noexcept {
   if (const int32_t location = GetUniformLocation(name); location != -1) {
      glUniform1ui(location, value);
   }
}

void GLShader::SetFloat(const std::string_view name, const float value) const noexcept {
   if (const int32_t location = GetUniformLocation(name); location != -1) {
      glUniform1f(location, value);
   }
}

void GLShader::SetVec2(const std::string_view name, const glm::vec2& value) const noexcept {
   if (const int32_t location = GetUniformLocation(name); location != -1) {
      glUniform2fv(location, 1, glm::value_ptr(value));
   }
}

void GLShader::SetVec3(const std::string_view name, const glm::vec3& value) const noexcept {
   if (const int32_t location = GetUniformLocation(name); location != -1) {
      glUniform3fv(location, 1, glm::value_ptr(value));
   }
}

void GLShader::SetVec4(const std::string_view name, const glm::vec4& value) const noexcept {
   if (const int32_t location = GetUniformLocation(name); location != -1) {
      glUniform4fv(location, 1, glm::value_ptr(value));
   }
}

void GLShader::SetMat2(const std::string_view name, const glm::mat2& value) const noexcept {
   if (const int32_t location = GetUniformLocation(name); location != -1) {
      glUniformMatrix2fv(location, 1, GL_FALSE, glm::value_ptr(value));
   }
}

void GLShader::SetMat3(const std::string_view name, const glm::mat3& value) const noexcept {
   if (const int32_t location = GetUniformLocation(name); location != -1) {
      glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value));
   }
}

void GLShader::SetMat4(const std::string_view name, const glm::mat4& value) const noexcept {
   if (const int32_t location = GetUniformLocation(name); location != -1) {
      glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
   }
}

void GLShader::BindUniformBlock(const std::string_view blockName,
                                const uint32_t bindingPoint) const {
   if (m_program == 0) {
      throw std::runtime_error("Cannot bind uniform block on invalid shader program");
   }
   const uint32_t blockIndex = glGetUniformBlockIndex(m_program, blockName.data());
   if (blockIndex == GL_INVALID_INDEX) {
      throw std::runtime_error("Uniform block '" + std::string(blockName) + "' not found");
   }
   glUniformBlockBinding(m_program, blockIndex, bindingPoint);
}

uint32_t GLShader::CompileShader(const Type type, const std::string_view source) {
   const uint32_t shader = glCreateShader(static_cast<GLenum>(type));
   if (shader == 0) {
      throw std::runtime_error("Failed to create shader");
   }
   const char* const sourceCStr = source.data();
   const int32_t sourceLength = static_cast<int32_t>(source.length());
   glShaderSource(shader, 1, &sourceCStr, &sourceLength);
   glCompileShader(shader);
   int32_t success;
   glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
   if (!success) {
      int32_t logLength;
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
      std::string infoLog(static_cast<size_t>(logLength), '\0');
      glGetShaderInfoLog(shader, logLength, nullptr, infoLog.data());
      glDeleteShader(shader);
      const auto getTypeName = [](Type shaderType) -> std::string_view {
         switch (shaderType) {
            case Type::Vertex:
               return "vertex";
            case Type::Fragment:
               return "fragment";
            case Type::Geometry:
               return "geometry";
            case Type::Compute:
               return "compute";
            case Type::TessControl:
               return "tessellation control";
            case Type::TessEvaluation:
               return "tessellation evaluation";
            default:
               return "unknown";
         }
      };
      throw std::runtime_error("Shader compilation failed (" + std::string(getTypeName(type)) +
                               "): " + infoLog);
   }
   return shader;
}

std::string GLShader::ReadFile(const std::string_view filepath) {
   std::ifstream file(filepath.data());
   if (!file.is_open()) {
      throw std::runtime_error("Failed to open shader file: " + std::string(filepath));
   }
   std::ostringstream buffer;
   buffer << file.rdbuf();
   return buffer.str();
}

uint32_t GLShader::GetUniformLocation(const std::string_view name) const {
   const std::string nameStr(name);
   if (const auto it = m_uniformLocations.find(nameStr); it != m_uniformLocations.end()) {
      return it->second;
   }
   const int32_t location = glGetUniformLocation(m_program, nameStr.c_str());
   m_uniformLocations.emplace(nameStr, location);
   return location;
}
