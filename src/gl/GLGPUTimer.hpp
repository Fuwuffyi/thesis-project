#pragma once

#include "core/system/IGPUTimer.hpp"
#include <glad/gl.h>
#include <unordered_map>
#include <string>

class GLGPUTimer : public IGPUTimer {
  public:
   GLGPUTimer();
   ~GLGPUTimer() override;

   void Begin(const std::string& label) override;
   void End(const std::string& label) override;
   [[nodiscard]] float GetElapsedMs(const std::string& label) override;
   void Reset() override;
   [[nodiscard]] bool IsAvailable(const std::string& label) const override;

  private:
   struct QueryPair {
      GLuint beginQuery{0};
      GLuint endQuery{0};
      bool active{false};
      float cachedResultMs{0.0f};
      bool hasResult{false};
   };

   std::unordered_map<std::string, QueryPair> m_queries;
   void CreateQuery(const std::string& label);
   void DeleteQuery(const std::string& label);
};
