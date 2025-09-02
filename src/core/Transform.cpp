#include "Transform.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <iostream>

Transform::Transform(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale)
    : m_pos(position),
      m_rot(glm::normalize(rotation)),
      m_scl(scale),
      m_matrix(1.0f),
      m_dirty(true) {}

Transform::Transform(const glm::mat4& transformMatrix) : m_matrix(transformMatrix), m_dirty(false) {
   glm::vec3 skew;
   glm::vec4 perspective;
   glm::decompose(transformMatrix, m_scl, m_rot, m_pos, skew, perspective);
   m_rot = glm::normalize(m_rot);
}

Transform::Transform(const Transform& other)
    : m_pos(other.m_pos),
      m_rot(other.m_rot),
      m_scl(other.m_scl),
      m_matrix(other.m_matrix),
      m_dirty(other.m_dirty) {}

Transform& Transform::operator=(const Transform& other) {
   if (this != &other) {
      m_pos = other.m_pos;
      m_rot = other.m_rot;
      m_scl = other.m_scl;
      m_matrix = other.m_matrix;
      m_dirty = other.m_dirty;
   }
   return *this;
}

Transform::Transform(Transform&& other) noexcept
    : m_pos(std::move(other.m_pos)),
      m_rot(std::move(other.m_rot)),
      m_scl(std::move(other.m_scl)),
      m_matrix(std::move(other.m_matrix)),
      m_dirty(other.m_dirty) {}

Transform& Transform::operator=(Transform&& other) noexcept {
   if (this != &other) {
      m_pos = std::move(other.m_pos);
      m_rot = std::move(other.m_rot);
      m_scl = std::move(other.m_scl);
      m_matrix = std::move(other.m_matrix);
      m_dirty = other.m_dirty;
   }
   return *this;
}

void Transform::SetPosition(const glm::vec3& pos) {
   if (m_pos != pos) {
      m_pos = pos;
      MarkDirty();
   }
}

void Transform::Translate(const glm::vec3& delta) {
   if (delta != glm::vec3(0.0f)) {
      m_pos += delta;
      MarkDirty();
   }
}

void Transform::SetRotation(const glm::quat& rot) {
   const glm::quat normalizedRot = glm::normalize(rot);
   if (m_rot != normalizedRot) {
      m_rot = normalizedRot;
      MarkDirty();
   }
}

void Transform::SetRotation(const glm::vec3& eulerAngles) { SetRotation(glm::quat(eulerAngles)); }

void Transform::SetRotation(float pitch, float yaw, float roll) {
   SetRotation(glm::vec3(pitch, yaw, roll));
}

void Transform::Rotate(const glm::quat& deltaRot) { SetRotation(m_rot * glm::normalize(deltaRot)); }

void Transform::Rotate(const glm::vec3& axis, float angle) {
   if (angle != 0.0f) {
      Rotate(glm::angleAxis(angle, glm::normalize(axis)));
   }
}

void Transform::RotateAround(const glm::vec3& point, const glm::vec3& axis, float angle) {
   if (angle != 0.0f) {
      // Translate to origin, rotate, translate back
      glm::vec3 offset = m_pos - point;
      const glm::quat rotation = glm::angleAxis(angle, glm::normalize(axis));
      offset = rotation * offset;
      SetPosition(point + offset);
      Rotate(rotation);
   }
}

void Transform::SetScale(const glm::vec3& scl) {
   if (m_scl != scl) {
      m_scl = scl;
      MarkDirty();
   }
}

void Transform::Scale(const glm::vec3& factor) {
   if (factor != glm::vec3(1.0f)) {
      m_scl *= factor;
      MarkDirty();
   }
}

const glm::mat4& Transform::GetTransformMatrix() {
   if (m_dirty) {
      RecalculateMatrix();
      m_dirty = false;
   }
   return m_matrix;
}

const glm::mat4& Transform::GetTransformMatrix() const {
   if (m_dirty) {
      RecalculateMatrix();
      m_dirty = false;
   }
   return m_matrix;
}

glm::vec3 Transform::GetForward() const { return glm::rotate(m_rot, glm::vec3(0.0f, 0.0f, -1.0f)); }

glm::vec3 Transform::GetRight() const { return glm::rotate(m_rot, glm::vec3(1.0f, 0.0f, 0.0f)); }

glm::vec3 Transform::GetUp() const { return glm::rotate(m_rot, glm::vec3(0.0f, 1.0f, 0.0f)); }

glm::vec3 Transform::GetEulerAngles() const { return glm::eulerAngles(m_rot); }

void Transform::RecalculateMatrix() const {
   const glm::mat4 translation = glm::translate(glm::mat4(1.0f), m_pos);
   const glm::mat4 rotation = glm::toMat4(m_rot);
   const glm::mat4 scaling = glm::scale(glm::mat4(1.0f), m_scl);
   m_matrix = translation * rotation * scaling;
}

bool Transform::DecomposeMatrix(const glm::mat4& matrix, glm::vec3& position, glm::quat& rotation,
                                glm::vec3& scale) const {
   // Extract position (translation)
   position = glm::vec3(matrix[3]);
   // Extract upper-left 3x3 matrix for rotation and scale
   glm::mat3 rotScale = glm::mat3(matrix);
   // Extract scale
   scale.x = glm::length(rotScale[0]);
   scale.y = glm::length(rotScale[1]);
   scale.z = glm::length(rotScale[2]);
   // Check for negative scale (determinant < 0)
   if (glm::determinant(rotScale) < 0) {
      scale.x = -scale.x;
   }
   // Remove scale to get pure rotation matrix
   if (scale.x != 0.0f)
      rotScale[0] /= scale.x;
   if (scale.y != 0.0f)
      rotScale[1] /= scale.y;
   if (scale.z != 0.0f)
      rotScale[2] /= scale.z;
   // Extract rotation
   rotation = glm::quat_cast(rotScale);
   rotation = glm::normalize(rotation);
   return true;
}
