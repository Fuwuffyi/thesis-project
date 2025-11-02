#include "core/system/CPUTimer.hpp"

void CPUTimer::Begin(const std::string& label) {
   m_timings[label].startTime = std::chrono::high_resolution_clock::now();
   m_timings[label].hasResult = false;
}

void CPUTimer::End(const std::string& label) {
   const auto it = m_timings.find(label);
   if (it == m_timings.end())
      return;
   const auto endTime = std::chrono::high_resolution_clock::now();
   const auto duration = std::chrono::duration<float, std::milli>(endTime - it->second.startTime);
   it->second.elapsedMs = duration.count();
   it->second.hasResult = true;
}

float CPUTimer::GetElapsedMs(const std::string& label) {
   const auto it = m_timings.find(label);
   return it != m_timings.end() ? it->second.elapsedMs : 0.0f;
}

void CPUTimer::Reset() { m_timings.clear(); }

bool CPUTimer::IsAvailable(const std::string& label) const {
   const auto it = m_timings.find(label);
   return it != m_timings.end() && it->second.hasResult;
}
