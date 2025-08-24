#pragma once

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

class Transform {
public:
   Transform(const glm::vec3& position = glm::vec3(0.0f),
             const glm::quat& rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
             const glm::vec3& scale = glm::vec3(1.0f)
             );
   explicit Transform(const glm::mat4& transformMatrix);

   Transform(const Transform& other);
   Transform& operator=(const Transform& other);
   Transform(Transform&& other) noexcept;
   Transform& operator=(Transform&& other) noexcept;

   // Position
   const glm::vec3& GetPosition() const { return m_pos; }
   void SetPosition(const glm::vec3& pos);
   void SetPosition(float x, float y, float z) { SetPosition(glm::vec3(x, y, z)); }
   void Translate(const glm::vec3& delta);
   void Translate(float x, float y, float z) { Translate(glm::vec3(x, y, z)); }

   // Rotation
   const glm::quat& GetRotation() const { return m_rot; }
   void SetRotation(const glm::quat& rot);
   void SetRotation(const glm::vec3& eulerAngles);
   void SetRotation(float pitch, float yaw, float roll);
   void Rotate(const glm::quat& deltaRot);
   void Rotate(const glm::vec3& axis, float angle);
   void RotateAround(const glm::vec3& point, const glm::vec3& axis, float angle);

   // Scale
   const glm::vec3& GetScale() const { return m_scl; }
   void SetScale(const glm::vec3& scl);
   void SetScale(float uniform) { SetScale(glm::vec3(uniform)); }
   void SetScale(float x, float y, float z) { SetScale(glm::vec3(x, y, z)); }
   void Scale(const glm::vec3& factor);
   void Scale(float uniform) { Scale(glm::vec3(uniform)); }

   // Matrix access (cached and optimized)
   const glm::mat4& GetTransformMatrix();
   const glm::mat4& GetTransformMatrix() const;

   // Utility methods
   glm::vec3 GetForward() const;
   glm::vec3 GetRight() const;
   glm::vec3 GetUp() const;
   glm::vec3 GetEulerAngles() const;
private:
   void MarkDirty() { m_dirty = true; }
   void RecalculateMatrix() const;
   bool DecomposeMatrix(const glm::mat4& matrix, glm::vec3& position,
                        glm::quat& rotation, glm::vec3& scale) const;
private:
   // Transform components
   glm::vec3 m_pos;
   glm::quat m_rot;
   glm::vec3 m_scl;

   // Cached matrices (mutable for lazy evaluation)
   mutable glm::mat4 m_matrix;
   mutable bool m_dirty;
};

