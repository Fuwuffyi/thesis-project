#include "core/scene/components/ParticleSystemComponent.hpp"

#include "core/scene/Node.hpp"

#include <imgui.h>

#include <algorithm>
#include <execution>

ParticleSystemComponent::ParticleSystemComponent(const uint32_t maxParticles) noexcept
    : m_maxParticles(maxParticles), m_gen(m_rd()) {
   ReallocateParticles();
}

void ParticleSystemComponent::DrawInspector(Node* const node) noexcept {
   if (ImGui::CollapsingHeader("Particle System", ImGuiTreeNodeFlags_DefaultOpen |
                                                     ImGuiTreeNodeFlags_NoTreePushOnOpen)) {
      // Status information
      ImGui::Text("Active Particles: %u / %u", m_activeParticles, m_maxParticles);
      ImGui::Checkbox("Emission Enabled", &m_emissionEnabled);
      // Max particles setting
      int32_t maxParticles = static_cast<int32_t>(m_maxParticles);
      if (ImGui::SliderInt("Max Particles", &maxParticles, 1, 10000000)) {
         SetMaxParticles(static_cast<uint32_t>(maxParticles));
      }
      if (ImGui::CollapsingHeader("Emission")) {
         ImGui::SliderFloat("Emission Rate", &m_emissionSettings.emissionRate, 1.0f, 100000.0f);
         ImGui::SliderAngle("Emission Cone", &m_emissionSettings.emissionCone, 0.0f, 180.0f);
         ImGui::DragFloat3("Direction", &m_emissionSettings.emissionDirection.x, 0.01f);
         ImGui::DragFloat3("Velocity Min", &m_emissionSettings.initialVelocityMin.x, 0.1f);
         ImGui::DragFloat3("Velocity Max", &m_emissionSettings.initialVelocityMax.x, 0.1f);
         ImGui::SliderFloat("Life Min", &m_emissionSettings.lifeMin, 0.1f, 10.0f);
         ImGui::SliderFloat("Life Max", &m_emissionSettings.lifeMax, 0.1f, 10.0f);
         ImGui::SliderFloat("Size Min", &m_emissionSettings.sizeMin, 0.01f, 2.0f);
         ImGui::SliderFloat("Size Max", &m_emissionSettings.sizeMax, 0.01f, 2.0f);
      }
      if (ImGui::CollapsingHeader("Physics")) {
         ImGui::DragFloat3("Gravity", &m_physicsSettings.gravity.x, 0.1f);
         ImGui::SliderFloat("Damping", &m_physicsSettings.damping, 0.9f, 1.0f);
         ImGui::Checkbox("Collision", &m_physicsSettings.collisionEnabled);
         if (m_physicsSettings.collisionEnabled) {
            ImGui::SliderFloat("Ground Height", &m_physicsSettings.groundHeight, -10.0f, 10.0f);
            ImGui::SliderFloat("Bounciness", &m_physicsSettings.bounciness, 0.0f, 1.0f);
         }
      }
      if (ImGui::CollapsingHeader("Rendering")) {
         ImGui::ColorEdit4("Start Color", &m_renderSettings.startColor.x);
         ImGui::ColorEdit4("End Color", &m_renderSettings.endColor.x);
         ImGui::Checkbox("Color Over Lifetime", &m_renderSettings.colorOverLifetime);
         ImGui::Checkbox("Size Over Lifetime", &m_renderSettings.sizeOverLifetime);
         ImGui::SliderFloat("End Size Multiplier", &m_renderSettings.endSizeMultiplier, 0.1f, 2.0f);
      }
   }
}

void ParticleSystemComponent::Update(float deltaTime, const glm::vec3& worldPosition) noexcept {
   // Emit new particles
   if (m_emissionEnabled) [[likely]] {
      EmitParticles(deltaTime, worldPosition);
   }
   // Update existing particles
   UpdateParticlePhysics(deltaTime);
   UpdateParticleLifetime(deltaTime);
   RemoveDeadParticles();
}

void ParticleSystemComponent::EmitParticles(float deltaTime, const glm::vec3& worldPosition) noexcept {
   m_emissionAccumulator += m_emissionSettings.emissionRate * deltaTime;
   const uint32_t particlesToEmit = static_cast<uint32_t>(m_emissionAccumulator);
   m_emissionAccumulator -= particlesToEmit;
   for (uint32_t i = 0; i < particlesToEmit && m_activeParticles < m_maxParticles; ++i) {
      Particle& particle = m_particles[m_activeParticles++];
      particle.position = worldPosition;
      particle.velocity = GenerateRandomVelocity();
      particle.acceleration = glm::vec3(0.0f);
      particle.life = particle.maxLife =
         m_emissionSettings.lifeMin +
         m_dist(m_gen) * (m_emissionSettings.lifeMax - m_emissionSettings.lifeMin);
      particle.size = m_emissionSettings.sizeMin +
                      m_dist(m_gen) * (m_emissionSettings.sizeMax - m_emissionSettings.sizeMin);
      particle.mass = 1.0f;
      particle.color = m_renderSettings.startColor;
   }
}

void ParticleSystemComponent::UpdateParticlePhysics(const float deltaTime) noexcept {
   // Use parallel execution to stress CPU more
   std::for_each(std::execution::par_unseq, m_particles.begin(),
                 m_particles.begin() + m_activeParticles, [this, deltaTime](Particle& particle) {
                    // Apply gravity
                    particle.acceleration = m_physicsSettings.gravity;
                    // Update velocity and position using Verlet integration
                    particle.velocity += particle.acceleration * deltaTime;
                    particle.velocity *= m_physicsSettings.damping;
                    particle.position += particle.velocity * deltaTime;
                    // Ground collision
                    if (m_physicsSettings.collisionEnabled &&
                        particle.position.y <= m_physicsSettings.groundHeight) {
                       particle.position.y = m_physicsSettings.groundHeight;
                       particle.velocity.y *= -m_physicsSettings.bounciness;
                    }
                 });
}

void ParticleSystemComponent::UpdateParticleLifetime(const float deltaTime) noexcept {
   // Update particle lifetime and visual properties
   std::for_each(std::execution::par_unseq, m_particles.begin(),
                 m_particles.begin() + m_activeParticles, [this, deltaTime](Particle& particle) {
                    particle.life -= deltaTime;
                    const float t = 1.0f - (particle.life / particle.maxLife);
                    if (m_renderSettings.colorOverLifetime) {
                       particle.color = InterpolateColor(t);
                    }
                    if (m_renderSettings.sizeOverLifetime) {
                       const float originalSize = particle.size;
                       particle.size = originalSize * InterpolateSize(t);
                    }
                 });
}

void ParticleSystemComponent::RemoveDeadParticles() noexcept {
   // Remove dead particles by swapping with alive ones
   uint32_t writeIndex = 0;
   for (uint32_t readIndex = 0; readIndex < m_activeParticles; ++readIndex) {
      if (m_particles[readIndex].life > 0.0f) {
         if (writeIndex != readIndex) {
            m_particles[writeIndex] = std::move(m_particles[readIndex]);
         }
         ++writeIndex;
      }
   }
   m_activeParticles = writeIndex;
}

glm::vec3 ParticleSystemComponent::GenerateRandomVelocity() const noexcept {
   const glm::vec3& min = m_emissionSettings.initialVelocityMin;
   const glm::vec3& max = m_emissionSettings.initialVelocityMax;
   return glm::vec3(min.x + m_dist(m_gen) * (max.x - min.x),
                    min.y + m_dist(m_gen) * (max.y - min.y),
                    min.z + m_dist(m_gen) * (max.z - min.z));
}

glm::vec4 ParticleSystemComponent::InterpolateColor(const float t) const noexcept {
   return glm::mix(m_renderSettings.startColor, m_renderSettings.endColor, t);
}

float ParticleSystemComponent::InterpolateSize(const float t) const noexcept {
   return glm::mix(1.0f, m_renderSettings.endSizeMultiplier, t);
}

void ParticleSystemComponent::ReallocateParticles() noexcept {
   m_particles.clear();
   m_particles.resize(m_maxParticles);
   m_instanceData.clear();
   m_instanceData.resize(m_maxParticles);
   m_activeParticles = 0;
}
