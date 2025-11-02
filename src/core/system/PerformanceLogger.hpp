#pragma once

#include <chrono>
#include <fstream>
#include <string>

#include "core/system/PerformanceMetrics.hpp"

class PerformanceLogger {
  public:
   explicit PerformanceLogger(const std::string& logDirectory = "perf_logs");
   ~PerformanceLogger();
   void StartSession(const std::string& sceneName, const SystemInfo& sysInfo);
   void EndSession();
   void LogFrame(const PerformanceMetrics& metrics);
   [[nodiscard]] const PerformanceStatistics& GetStatistics() const noexcept { return m_stats; }
   void Flush();

  private:
   void WriteSystemInfoCSV(const SystemInfo& info);
   void WriteFrameMetricsHeader();
   void WriteRunSummary();

   std::string m_logDirectory;
   std::string m_sessionName;
   std::ofstream m_frameMetricsFile;
   std::ofstream m_systemInfoFile;
   std::ofstream m_summaryFile;

   SystemInfo m_systemInfo;
   PerformanceStatistics m_stats;
   std::vector<PerformanceMetrics> m_frameBuffer;

   std::chrono::steady_clock::time_point m_sessionStartTime;
   bool m_sessionActive{false};

   static constexpr size_t BUFFER_SIZE = 512;
};
