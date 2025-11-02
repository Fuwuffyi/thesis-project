#pragma once

#include <string>

class IGPUTimer {
  public:
   virtual ~IGPUTimer() = default;

   virtual void Begin(const std::string& label) = 0;
   virtual void End(const std::string& label) = 0;

   [[nodiscard]] virtual float GetElapsedMs(const std::string& label) = 0;
   virtual void Reset() = 0;
   [[nodiscard]] virtual bool IsAvailable(const std::string& label) const = 0;
};
