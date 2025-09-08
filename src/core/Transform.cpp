#include "core/Transform.hpp"

#include <glm/gtx/matrix_decompose.hpp>

Transform::Transform(const glm::vec3& position, const glm::quat& rotation,
                     const glm::vec3& scale) noexcept
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

Transform::Transform(const Transform& other) noexcept
    : m_pos(other.m_pos),
      m_rot(other.m_rot),
      m_scl(other.m_scl),
      m_matrix(other.m_matrix),
      m_dirty(other.m_dirty) {}

Transform& Transform::operator=(const Transform& other) noexcept {
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

void Transform::SetPosition(const glm::vec3& pos) noexcept {
   if (m_pos != pos) {
      m_pos = pos;
      MarkDirty();
   }
}

void Transform::Translate(const glm::vec3& delta) noexcept {
   if (delta != glm::vec3(0.0f)) {
      m_pos += delta;
      MarkDirty();
   }
}

void Transform::Translate(const float x, const float y, const float z) noexcept {
   Translate(glm::vec3(x, y, z));
}

void Transform::SetPosition(const float x, const float y, const float z) noexcept {
   SetPosition(glm::vec3(x, y, z));
}

void Transform::SetRotation(const glm::quat& rot) noexcept {
   const glm::quat normalizedRot = glm::normalize(rot);
   if (m_rot != normalizedRot) {
      m_rot = normalizedRot;
      MarkDirty();
   }
}

void Transform::SetRotation(const glm::vec3& eulerAngles) noexcept {
   SetRotation(glm::quat(eulerAngles));
}

void Transform::SetRotation(const float pitch, const float yaw, const float roll) noexcept {
   SetRotation(glm::vec3(pitch, yaw, roll));
}

void Transform::Rotate(const glm::quat& deltaRot) noexcept {
   SetRotation(m_rot * glm::normalize(deltaRot));
}

void Transform::Rotate(const glm::vec3& axis, float angle) noexcept {
   if (angle != 0.0f) {
      Rotate(glm::angleAxis(angle, glm::normalize(axis)));
   }
}

void Transform::RotateAround(const glm::vec3& point, const glm::vec3& axis, float angle) noexcept {
   if (angle != 0.0f) {
      glm::vec3 offset = m_pos - point;
      const glm::quat rotation = glm::angleAxis(angle, glm::normalize(axis));
      offset = rotation * offset;
      SetPosition(point + offset);
      Rotate(rotation);
   }
}

void Transform::SetScale(const glm::vec3& scl) noexcept {
   if (m_scl != scl) {
      m_scl = scl;
      MarkDirty();
   }
}

void Transform::SetScale(const float uniform) noexcept { SetScale(glm::vec3(uniform)); }

void Transform::SetScale(const float x, const float y, const float z) noexcept {
   SetScale(glm::vec3(x, y, z));
}

void Transform::Scale(const glm::vec3& factor) noexcept {
   if (factor != glm::vec3(1.0f)) {
      m_scl *= factor;
      MarkDirty();
   }
}

void Transform::Scale(const float uniform) noexcept { Scale(glm::vec3(uniform)); }

const glm::mat4& Transform::GetTransformMatrix() const {
   if (m_dirty) {
      RecalculateMatrix();
      m_dirty = false;
   }
   return m_matrix;
}

glm::vec3 Transform::GetForward() const noexcept {
   return glm::rotate(m_rot, glm::vec3(0.0f, 0.0f, -1.0f));
}

glm::vec3 Transform::GetRight() const noexcept {
   return glm::rotate(m_rot, glm::vec3(1.0f, 0.0f, 0.0f));
}

glm::vec3 Transform::GetUp() const noexcept {
   return glm::rotate(m_rot, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::vec3 Transform::GetEulerAngles() const noexcept { return glm::eulerAngles(m_rot); }

void Transform::MarkDirty() noexcept { m_dirty = true; }

void Transform::RecalculateMatrix() const noexcept {
   const glm::mat4 translation = glm::translate(glm::mat4(1.0f), m_pos);
   const glm::mat4 rotation = glm::toMat4(m_rot);
   const glm::mat4 scaling = glm::scale(glm::mat4(1.0f), m_scl);
   m_matrix = translation * rotation * scaling;
}

bool Transform::DecomposeMatrix(const glm::mat4& matrix, glm::vec3& position, glm::quat& rotation,
                                glm::vec3& scale) const noexcept {
   position = glm::vec3(matrix[3]);
   glm::mat3 rotScale = glm::mat3(matrix);
   scale.x = glm::length(rotScale[0]);
   scale.y = glm::length(rotScale[1]);
   scale.z = glm::length(rotScale[2]);
   if (glm::determinant(rotScale) < 0) {
      scale.x = -scale.x;
   }
   if (scale.x != 0.0f)
      rotScale[0] /= scale.x;
   if (scale.y != 0.0f)
      rotScale[1] /= scale.y;
   if (scale.z != 0.0f)
      rotScale[2] /= scale.z;
   rotation = glm::normalize(glm::quat_cast(rotScale));
   return true;
}
