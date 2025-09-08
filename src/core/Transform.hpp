#pragma once

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

class Transform final {
  public:
   explicit Transform(const glm::vec3& position = glm::vec3(0.0f),
                      const glm::quat& rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
                      const glm::vec3& scale = glm::vec3(1.0f)) noexcept;
   explicit Transform(const glm::mat4& transformMatrix);

   Transform(const Transform& other) noexcept;
   Transform& operator=(const Transform& other) noexcept;
   Transform(Transform&& other) noexcept;
   Transform& operator=(Transform&& other) noexcept;

   // Position
   [[nodiscard]] constexpr const glm::vec3& GetPosition() const noexcept { return m_pos; }
   void SetPosition(const glm::vec3& pos) noexcept;
   void SetPosition(const float x, const float y, const float z) noexcept;
   void Translate(const glm::vec3& delta) noexcept;
   void Translate(const float x, const float y, const float z) noexcept;

   // Rotation
   [[nodiscard]] constexpr const glm::quat& GetRotation() const noexcept { return m_rot; }
   void SetRotation(const glm::quat& rot) noexcept;
   void SetRotation(const glm::vec3& eulerAngles) noexcept;
   void SetRotation(const float pitch, const float yaw, const float roll) noexcept;
   void Rotate(const glm::quat& deltaRot) noexcept;
   void Rotate(const glm::vec3& axis, float angle) noexcept;
   void RotateAround(const glm::vec3& point, const glm::vec3& axis, float angle) noexcept;

   // Scale
   [[nodiscard]] constexpr const glm::vec3& GetScale() const noexcept { return m_scl; }
   void SetScale(const glm::vec3& scl) noexcept;
   void SetScale(const float uniform) noexcept;
   void SetScale(const float x, const float y, const float z) noexcept;
   void Scale(const glm::vec3& factor) noexcept;
   void Scale(const float uniform) noexcept;

   // Matrix access
   [[nodiscard]] const glm::mat4& GetTransformMatrix() const;

   // Utility methods
   [[nodiscard]] glm::vec3 GetForward() const noexcept;
   [[nodiscard]] glm::vec3 GetRight() const noexcept;
   [[nodiscard]] glm::vec3 GetUp() const noexcept;
   [[nodiscard]] glm::vec3 GetEulerAngles() const noexcept;

  private:
   void MarkDirty() noexcept;
   void RecalculateMatrix() const noexcept;
   [[nodiscard]] bool DecomposeMatrix(const glm::mat4& matrix, glm::vec3& position,
                                      glm::quat& rotation, glm::vec3& scale) const noexcept;

  private:
   glm::vec3 m_pos;
   glm::quat m_rot;
   glm::vec3 m_scl;

   mutable glm::mat4 m_matrix;
   mutable bool m_dirty;
};
