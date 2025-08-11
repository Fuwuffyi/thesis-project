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
   Transform(const glm::mat4& transformMatrix);

   const glm::vec3& GetPosition() const;
   void SetPosition(const glm::vec3& pos);

   const glm::quat& GetRotation() const;
   void SetRotation(const glm::quat& rot);

   const glm::vec3& GetScale() const;
   void SetScale(const glm::vec3& scl);

   const glm::mat4& GetPositionMatrix();
   const glm::mat4& GetRotationMatrix();
   const glm::mat4& GetScaleMatrix();
   const glm::mat4& GetTransformMatrix();

private:
   glm::vec3 m_pos;
   glm::quat m_rot;
   glm::vec3 m_scl;

   glm::mat4 m_mPos;
   glm::mat4 m_mRot;
   glm::mat4 m_mScl;
   glm::mat4 m_matrix;
};
