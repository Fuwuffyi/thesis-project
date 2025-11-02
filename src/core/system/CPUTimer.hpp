#pragma once

#include "core/system/IGPUTimer.hpp"

#include <string>
#include <chrono>
#include <unordered_map>

// Fallback class for default GPU timer (runs on cpu)
class CPUTimer : public IGPUTimer {
  public:
   void Begin(const std::string& label) override;
   void End(const std::string& label) override;
   [[nodiscard]] float GetElapsedMs(const std::string& label) override;
   void Reset() override;
   [[nodiscard]] bool IsAvailable(const std::string& label) const override;

  private:
   struct TimingData {
      std::chrono::high_resolution_clock::time_point startTime;
      float elapsedMs{0.0f};
      bool hasResult{false};
   };

   std::unordered_map<std::string, TimingData> m_timings;
};
