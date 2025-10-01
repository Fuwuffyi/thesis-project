#pragma once

#include "core/scene/components/Component.hpp"

#include <glm/glm.hpp>

#include <atomic>
#include <random>
#include <vector>

struct Particle final {
   glm::vec3 position{0.0f};
   glm::vec3 velocity{0.0f};
   glm::vec3 acceleration{0.0f};
   glm::vec4 color{1.0f};
   float size{1.0f};
   float life{1.0f};
   float maxLife{1.0f};
   float mass{1.0f};
};

struct ParticleInstanceData final {
   glm::mat4 transform{1.0f};
   glm::vec4 color{1.0f};
};

class ParticleSystemComponent final : public Component {
  public:
   struct EmissionSettings final {
      glm::vec3 emissionDirection{0.0f, 1.0f, 0.0f};
      float emissionCone{0.5f};
      float emissionRate{50.0f};
      float initialSpeedMin{0.0f};
      float initialSpeedMax{5.0f};
      float lifeMin{2.0f};
      float lifeMax{5.0f};
      float sizeMin{0.1f};
      float sizeMax{0.3f};
   };

   struct PhysicsSettings final {
      glm::vec3 gravity{0.0f, -9.81f, 0.0f};
      float damping{0.98f};
      bool collisionEnabled{false};
      float groundHeight{0.0f};
      float bounciness{0.5f};
   };

   struct RenderSettings final {
      glm::vec4 startColor{1.0f, 1.0f, 1.0f, 1.0f};
      glm::vec4 endColor{1.0f, 1.0f, 1.0f, 0.0f};
      bool colorOverLifetime{true};
      bool sizeOverLifetime{true};
      float endSizeMultiplier{0.5f};
   };

   explicit ParticleSystemComponent(const uint32_t maxParticles = 25000) noexcept;
   ~ParticleSystemComponent() override = default;

   void DrawInspector(Node* const node) noexcept override;
   void Update(float deltaTime, const glm::vec3& worldPosition) noexcept;

   [[nodiscard]] const std::vector<ParticleInstanceData>& GetInstanceData() const noexcept {
      return m_instanceData;
   }
   [[nodiscard]] const std::vector<Particle>& GetParticles() const noexcept { return m_particles; }

   void SetMaxParticles(const uint32_t count) noexcept {
      m_maxParticles = count;
      ReallocateParticles();
   }
   [[nodiscard]] uint32_t GetActiveParticleCount() const noexcept {
      return m_activeParticles.load(std::memory_order_acquire);
   }
   [[nodiscard]] uint32_t GetMaxParticles() const noexcept { return m_maxParticles; }

   void SetEmissionEnabled(const bool enabled) noexcept {
      m_emissionEnabled.store(enabled, std::memory_order_release);
   }
   [[nodiscard]] bool IsEmissionEnabled() const noexcept {
      return m_emissionEnabled.load(std::memory_order_acquire);
   }

   [[nodiscard]] EmissionSettings& GetEmissionSettings() noexcept { return m_emissionSettings; }
   [[nodiscard]] PhysicsSettings& GetPhysicsSettings() noexcept { return m_physicsSettings; }
   [[nodiscard]] RenderSettings& GetRenderSettings() noexcept { return m_renderSettings; }

  private:
   void EmitParticles(const float deltaTime, const glm::vec3& worldPosition) noexcept;
   void UpdateParticlesCombined(const float deltaTime) noexcept;
   void UpdateInstanceData() noexcept;
   void RemoveDeadParticlesSwap() noexcept;

   [[nodiscard]] glm::vec3 GenerateRandomVelocity() const noexcept;
   [[nodiscard]] glm::vec4 InterpolateColor(const float t) const noexcept;
   [[nodiscard]] float InterpolateSize(const float t) const noexcept;
   void ReallocateParticles() noexcept;

   // Particle data
   std::vector<Particle> m_particles;
   uint32_t m_maxParticles;
   std::atomic<uint32_t> m_activeParticles{0};
   // Emission tracking
   float m_emissionAccumulator{0};
   std::atomic<bool> m_emissionEnabled{true};
   // Settings
   EmissionSettings m_emissionSettings;
   PhysicsSettings m_physicsSettings;
   RenderSettings m_renderSettings;
   // Instance data for rendering
   std::vector<ParticleInstanceData> m_instanceData;
   // Random number generation
   mutable std::random_device m_rd;
   mutable std::mt19937_64 m_gen{m_rd()};
   mutable std::uniform_real_distribution<float> m_dist{0.0f, 1.0f};
   // stable base seed used for thread-local generators
   uint64_t m_baseSeed{0};
};
