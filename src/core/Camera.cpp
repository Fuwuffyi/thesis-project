#include "Camera.hpp"

#include "Camera.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

Camera::Camera(const Transform& transform, const glm::vec3& up, const float fov, const float aspectRatio, const float near, const float far)
   :
   m_transform(transform),
   m_up(glm::normalize(up)),
   m_fov(fov),
   m_aspectRatio(aspectRatio),
   m_near(near),
   m_far(far),
   m_view(1.0f),
   m_proj(1.0f),
   m_camera(1.0f)
{
   m_proj = glm::perspective(glm::radians(m_fov), m_aspectRatio, m_near, m_far);
}

const glm::vec3& Camera::GetViewDirection() {
   static glm::vec3 forward;
   forward = glm::rotate(m_transform.GetRotation(), glm::vec3(0.0f, 0.0f, -1.0f));
   return forward;
}

const glm::vec3& Camera::GetRightVector() {
   static glm::vec3 right;
   right = glm::rotate(m_transform.GetRotation(), glm::vec3(1.0f, 0.0f, 0.0f));
   return right;
}

const glm::vec3& Camera::GetUpVector() {
   static glm::vec3 upVec;
   upVec = glm::rotate(m_transform.GetRotation(), glm::vec3(0.0f, 1.0f, 0.0f));
   return upVec;
}

float Camera::GetFOV() const {
   return m_fov;
}

void Camera::SetFOV(const float newFov) {
   m_fov = newFov;
   m_proj = glm::perspective(glm::radians(m_fov), m_aspectRatio, m_near, m_far);
}

void Camera::SetAspectRatio(const float newRatio) {
   m_aspectRatio = newRatio;
   m_proj = glm::perspective(glm::radians(m_fov), m_aspectRatio, m_near, m_far);
}

const glm::mat4& Camera::GetViewMatrix() {
   m_view = glm::inverse(m_transform.GetTransformMatrix());
   return m_view;
}

const glm::mat4& Camera::GetProjectionMatrix() {
   return m_proj;
}

const glm::mat4& Camera::GetCameraMatrix() {
   m_camera = GetProjectionMatrix() * GetViewMatrix();
   return m_camera;
}

const Transform& Camera::GetTransform() const {
   return m_transform;
}

Transform& Camera::GetMutableTransform() {
   return m_transform;
}

