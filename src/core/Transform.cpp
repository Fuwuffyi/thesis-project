#include "Transform.hpp"

#include <glm/gtc/matrix_transform.hpp>

Transform::Transform(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale)
   :
   m_pos(position),
   m_rot(glm::normalize(rotation)),
   m_scl(scale),
   m_mPos(1.0f),
   m_mRot(1.0f),
   m_mScl(1.0f),
   m_matrix(1.0f),
   m_dirty(true)
{}

Transform::Transform(const glm::mat4& transformMatrix)
   :
   m_mPos(1.0f),
   m_mRot(1.0f),
   m_mScl(1.0f),
   m_matrix(1.0f),
   m_dirty(true)
{
   // Extract position (last column)
   m_pos = glm::vec3(transformMatrix[3]);
   // Extract scale and rotation (upper-left 3x3 matrix)
   glm::mat3 rotationScale = glm::mat3(transformMatrix);
   // Decompose rotationScale into scale and rotation
   m_scl = glm::vec3(glm::length(rotationScale[0]), glm::length(rotationScale[1]), glm::length(rotationScale[2]));
   // Normalize the rotation matrix to get the rotation component
   glm::mat3 rotationMatrix = rotationScale;
   rotationMatrix[0] /= m_scl.x;
   rotationMatrix[1] /= m_scl.y;
   rotationMatrix[2] /= m_scl.z;
   // Extract rotation from quat
   m_rot = glm::normalize(glm::quat_cast(rotationScale));
}

const glm::vec3& Transform::GetPosition() const {
   return m_pos;
}

void Transform::SetPosition(const glm::vec3& pos) {
   m_pos = pos;
   m_dirty = true;
}

const glm::quat& Transform::GetRotation() const {
   return m_rot;
}

void Transform::SetRotation(const glm::quat& rot) {
   m_rot = glm::normalize(rot);
   m_dirty = true;
}

const glm::vec3& Transform::GetScale() const {
   return m_scl;
}

void Transform::SetScale(const glm::vec3& scl) {
   m_scl = scl;
   m_dirty = true;
}

const glm::mat4& Transform::GetTransformMatrix() {
   if (m_dirty) {
      RecalculateMatrix();
      m_dirty = false;
   }
   return m_matrix;
}

void Transform::RecalculateMatrix() {
   m_mPos = glm::translate(glm::mat4(1.0f), m_pos);
   m_mRot = glm::toMat4(m_rot);
   m_mScl = glm::scale(glm::mat4(1.0f), m_scl);
   m_matrix = m_mPos * m_mRot * m_mScl;
}

