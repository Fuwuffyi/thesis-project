#include "Camera.hpp"

Camera::Camera(const GraphicsAPI api, const Transform& transform, const glm::vec3& up,
               const float fov, const float aspectRatio, const float near, const float far) noexcept
    : m_api(api),
      m_transform(transform),
      m_up(glm::normalize(up)),
      m_fov(fov),
      m_aspectRatio(aspectRatio),
      m_near(near),
      m_far(far),
      m_view(1.0f),
      m_proj(glm::perspective(glm::radians(fov), aspectRatio, near, far)),
      m_camera(1.0f),
      m_viewDirty(true),
      m_projDirty(true),
      m_cameraDirty(true) {}

glm::vec3 Camera::GetViewDirection() const noexcept { return m_transform.GetForward(); }
glm::vec3 Camera::GetRightVector() const noexcept { return m_transform.GetRight(); }
glm::vec3 Camera::GetUpVector() const noexcept { return m_transform.GetUp(); }

void Camera::SetFOV(const float newFov) noexcept {
   m_fov = newFov;
   m_projDirty = true;
}

void Camera::SetAspectRatio(const float newRatio) noexcept {
   m_aspectRatio = newRatio;
   m_projDirty = true;
}

const glm::mat4& Camera::GetViewMatrix() {
   if (m_viewDirty) {
      RecalculateView();
      m_viewDirty = false;
   }
   return m_view;
}

const glm::mat4& Camera::GetProjectionMatrix() {
   if (m_projDirty) {
      RecalculateProjection();
      m_projDirty = false;
   }
   return m_proj;
}

const glm::mat4& Camera::GetCameraMatrix() {
   if (m_cameraDirty) {
      m_camera = GetProjectionMatrix() * GetViewMatrix();
      m_cameraDirty = false;
   }
   return m_camera;
}

void Camera::RecalculateView() noexcept { m_view = glm::inverse(m_transform.GetTransformMatrix()); }

static const glm::mat4 GL_TO_VK_CLIP = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
                                                 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.5f, 1.0f);

void Camera::RecalculateProjection() noexcept {
   m_proj = glm::perspective(glm::radians(m_fov), m_aspectRatio, m_near, m_far);
   if (m_api == GraphicsAPI::Vulkan) {
      m_proj = GL_TO_VK_CLIP * m_proj;
   }
}
