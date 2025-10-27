#pragma once

#include <chrono>
#include <fstream>
#include <string>
#include <vector>

class PerformanceLogger {
  public:
   struct SystemInfo {
      std::string cpuModel;
      uint32_t threadCount{0};
      float clockSpeedGHz{0.0f};
      std::string gpuModel;
      size_t vramMB{0};
      std::string driverVersion;
      std::string apiVersion;
      uint32_t windowWidth{0};
      uint32_t windowHeight{0};
   };

   struct FrameMetrics {
      float frameTimeMs{0.0f};
      float cpuTimeMs{0.0f};
      float gpuTimeMs{0.0f};
      size_t vramUsageMB{0};
      size_t systemMemUsageMB{0};
      float gpuUtilization{0.0f};
      float cpuUtilization{0.0f};
   };

   struct RunStatistics {
      float avgFPS{0.0f};
      float minFPS{999999.0f};
      float maxFPS{0.0f};
      float avgFrameTime{0.0f};
      float minFrameTime{999999.0f};
      float maxFrameTime{0.0f};
      uint64_t totalFrames{0};
      float totalRunTimeSeconds{0.0f};
   };

   explicit PerformanceLogger(const std::string& logDirectory = "perf_logs");
   ~PerformanceLogger();

   void StartSession(const std::string& sceneName, const SystemInfo& sysInfo);
   void EndSession();

   void LogFrame(const FrameMetrics& metrics);

   [[nodiscard]] const RunStatistics& GetStatistics() const noexcept { return m_stats; }

   void Flush();

  private:
   void WriteSystemInfoCSV(const SystemInfo& info);
   void WriteFrameMetricsHeader();
   void WriteRunSummary();
   void UpdateStatistics(const FrameMetrics& metrics);

   std::string m_logDirectory;
   std::string m_sessionName;
   std::ofstream m_frameMetricsFile;
   std::ofstream m_systemInfoFile;
   std::ofstream m_summaryFile;

   SystemInfo m_systemInfo;
   RunStatistics m_stats;
   std::vector<FrameMetrics> m_frameBuffer;

   std::chrono::steady_clock::time_point m_sessionStartTime;
   bool m_sessionActive{false};

   static constexpr size_t BUFFER_SIZE = 512;
};
