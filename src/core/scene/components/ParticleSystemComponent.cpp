#include "core/scene/components/ParticleSystemComponent.hpp"

#include "core/scene/Node.hpp"

#include <imgui.h>

#include <numbers>
#include <algorithm>
#include <execution>
#include <thread>

ParticleSystemComponent::ParticleSystemComponent(const uint32_t maxParticles) noexcept
    : m_maxParticles(maxParticles), m_gen(m_rd()) {
   m_baseSeed = m_gen();
   ReallocateParticles();
}

void ParticleSystemComponent::DrawInspector(Node* const node) noexcept {
   if (ImGui::CollapsingHeader("Particle System", ImGuiTreeNodeFlags_DefaultOpen |
                                                     ImGuiTreeNodeFlags_NoTreePushOnOpen)) {
      ImGui::Text("Active Particles: %u / %u", GetActiveParticleCount(), m_maxParticles);
      bool em = IsEmissionEnabled();
      ImGui::Checkbox("Emission Enabled", &em);
      SetEmissionEnabled(em);
      int32_t maxParticles = static_cast<int32_t>(m_maxParticles);
      if (ImGui::SliderInt("Max Particles", &maxParticles, 1, 1000000)) {
         SetMaxParticles(static_cast<uint32_t>(maxParticles));
      }
      if (ImGui::CollapsingHeader("Emission")) {
         ImGui::SliderFloat("Emission Rate", &m_emissionSettings.emissionRate, 1.0f, 100000.0f);
         ImGui::SliderAngle("Emission Cone", &m_emissionSettings.emissionCone, 0.0f, 180.0f);
         ImGui::DragFloat3("Direction", &m_emissionSettings.emissionDirection.x, 0.01f);
         ImGui::SliderFloat("Speed Min", &m_emissionSettings.initialSpeedMin, 0.0f, 100.0f);
         ImGui::SliderFloat("Speed Max", &m_emissionSettings.initialSpeedMax, 0.0f, 100.0f);
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

void ParticleSystemComponent::Update(const float deltaTime,
                                     const glm::vec3& worldPosition) noexcept {
   if (IsEmissionEnabled())
      EmitParticles(deltaTime, worldPosition);
   UpdateParticlesCombined(deltaTime);
   RemoveDeadParticlesSwap();
   UpdateInstanceData();
}

void ParticleSystemComponent::EmitParticles(const float deltaTime,
                                            const glm::vec3& worldPosition) noexcept {
   m_emissionAccumulator += m_emissionSettings.emissionRate * deltaTime;
   const uint32_t active = m_activeParticles.load(std::memory_order_relaxed);
   const uint32_t available = m_maxParticles - active;
   const uint32_t toEmit = std::min(static_cast<uint32_t>(m_emissionAccumulator), available);
   m_emissionAccumulator -= toEmit;
   if (toEmit == 0)
      return;
   const auto& es = m_emissionSettings;
   const auto& rs = m_renderSettings;
   for (uint32_t i = 0; i < toEmit; ++i) {
      const uint32_t idx = active + i;
      Particle& p = m_particles[idx];
      p.position = worldPosition;
      p.velocity = GenerateRandomVelocity();
      p.acceleration = glm::vec3(0.0f);
      const float r = m_dist(m_gen);
      p.maxLife = es.lifeMin + r * (es.lifeMax - es.lifeMin);
      p.life = p.maxLife;
      const float r2 = m_dist(m_gen);
      p.size = es.sizeMin + r2 * (es.sizeMax - es.sizeMin);
      p.mass = 1.0f;
      p.color = rs.startColor;
      m_instanceData[idx].color = p.color;
      m_instanceData[idx].transform = glm::translate(glm::mat4(1.0f), p.position) *
                                      glm::scale(glm::mat4(1.0f), glm::vec3(p.size));
   }
   m_activeParticles.store(active + toEmit, std::memory_order_release);
}

void ParticleSystemComponent::UpdateParticlesCombined(const float deltaTime) noexcept {
   const auto ps = m_physicsSettings;
   const auto rs = m_renderSettings;
   const uint32_t active = m_activeParticles.load(std::memory_order_acquire);
   if (active == 0)
      return;
   auto particlesBegin = m_particles.data();
   std::for_each(std::execution::par_unseq, particlesBegin, particlesBegin + active,
                 [ps, rs, deltaTime, this](Particle& p) {
                    // physics
                    p.acceleration = ps.gravity;
                    p.velocity = (p.velocity + p.acceleration * deltaTime) * ps.damping;
                    p.position += p.velocity * deltaTime;
                    if (ps.collisionEnabled && p.position.y <= ps.groundHeight) {
                       p.position.y = ps.groundHeight;
                       p.velocity.y *= -ps.bounciness;
                    }

                    // lifetime + rendering parameters
                    p.life -= deltaTime;
                    const float t = std::clamp(1.0f - p.life / p.maxLife, 0.0f, 1.0f);
                    if (rs.colorOverLifetime)
                       p.color = glm::mix(rs.startColor, rs.endColor, t);
                    if (rs.sizeOverLifetime)
                       p.size *= glm::mix(1.0f, rs.endSizeMultiplier, t);
                 });
}

void ParticleSystemComponent::UpdateInstanceData() noexcept {
   uint32_t active = m_activeParticles.load(std::memory_order_acquire);
   if (active == 0)
      return;
   ParticleInstanceData* instBegin = m_instanceData.data();
   Particle* pBegin = m_particles.data();
   std::for_each(std::execution::par_unseq, instBegin, instBegin + active,
                 [instBegin, pBegin](ParticleInstanceData& inst) {
                    const size_t idx = &inst - instBegin;
                    const Particle& p = pBegin[idx];
                    inst.transform = glm::translate(glm::mat4(1.0f), p.position) *
                                     glm::scale(glm::mat4(1.0f), glm::vec3(p.size));
                    inst.color = p.color;
                 });
}

void ParticleSystemComponent::RemoveDeadParticlesSwap() noexcept {
   uint32_t active = m_activeParticles.load(std::memory_order_acquire);
   uint32_t i = 0;
   while (i < active) {
      if (m_particles[i].life <= 0.0f) {
         --active;
         if (i != active) {
            m_particles[i] = m_particles[active];
            m_instanceData[i] = m_instanceData[active];
         }
      } else {
         ++i;
      }
   }
   m_activeParticles.store(active, std::memory_order_release);
}

glm::vec3 ParticleSystemComponent::GenerateRandomVelocity() const noexcept {
   thread_local std::mt19937_64 localGen(
      static_cast<uint64_t>(std::hash<std::thread::id>{}(std::this_thread::get_id())) ^ m_baseSeed);
   thread_local std::uniform_real_distribution<float> localDist(0.0f, 1.0f);
   const glm::vec3 dir = glm::normalize(m_emissionSettings.emissionDirection);
   const float coneAngle = m_emissionSettings.emissionCone; // radians
   const float u = localDist(localGen);
   const float v = localDist(localGen);
   const float theta = std::acos(std::lerp(std::cos(coneAngle), 1.0f, u));
   const float phi = 2.0f * std::numbers::pi_v<float> * v;
   const float sinTheta = std::sin(theta);
   const glm::vec3 localDir{sinTheta * std::cos(phi), std::cos(theta), sinTheta * std::sin(phi)};
   const glm::quat q = glm::rotation(glm::vec3(0.0f, 1.0f, 0.0f), dir);
   const glm::vec3 worldDir = glm::normalize(q * localDir);
   const float speed = std::lerp(m_emissionSettings.initialSpeedMin,
                                 m_emissionSettings.initialSpeedMax, localDist(localGen));
   return worldDir * speed;
}

glm::vec4 ParticleSystemComponent::InterpolateColor(const float t) const noexcept {
   return glm::mix(m_renderSettings.startColor, m_renderSettings.endColor, t);
}

float ParticleSystemComponent::InterpolateSize(const float t) const noexcept {
   return glm::mix(1.0f, m_renderSettings.endSizeMultiplier, t);
}

void ParticleSystemComponent::ReallocateParticles() noexcept {
   m_particles.assign(m_maxParticles, {});
   m_instanceData.assign(m_maxParticles, {});
   m_activeParticles.store(0, std::memory_order_release);
}
