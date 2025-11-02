#include "core/system/PerformanceLogger.hpp"

#include <filesystem>
#include <iomanip>
#include <sstream>
#include <ctime>

PerformanceLogger::PerformanceLogger(const std::string& logDirectory)
    : m_logDirectory(logDirectory) {
   std::filesystem::create_directories(m_logDirectory);
}

PerformanceLogger::~PerformanceLogger() {
   if (m_sessionActive) {
      EndSession();
   }
}

void PerformanceLogger::StartSession(const std::string& sceneName, const SystemInfo& sysInfo) {
   if (m_sessionActive) {
      EndSession();
   }
   m_systemInfo = sysInfo;
   m_stats.Reset();
   m_frameBuffer.clear();
   // Generate timestamp for filenames
   const auto now = std::chrono::system_clock::now();
   const std::time_t time_t = std::chrono::system_clock::to_time_t(now);
   std::tm tm;
#ifdef _WIN32
   localtime_s(&tm, &time_t);
#else
   localtime_r(&time_t, &tm);
#endif
   std::ostringstream timestamp;
   timestamp << std::put_time(&tm, "%Y%m%d_%H%M%S");
   m_sessionName = sceneName + "_" + timestamp.str();
   // Open CSV files
   const std::string baseFilename = m_logDirectory + "/" + m_sessionName;
   m_systemInfoFile.open(baseFilename + "_system.csv");
   m_frameMetricsFile.open(baseFilename + "_frames.csv");
   m_summaryFile.open(baseFilename + "_summary.csv");
   if (!m_systemInfoFile.is_open() || !m_frameMetricsFile.is_open() || !m_summaryFile.is_open()) {
      throw std::runtime_error("Failed to open log files");
   }
   // Write headers and initial data
   WriteSystemInfoCSV(sysInfo);
   WriteFrameMetricsHeader();
   m_sessionStartTime = std::chrono::steady_clock::now();
   m_sessionActive = true;
}

void PerformanceLogger::EndSession() {
   if (!m_sessionActive)
      return;
   // Flush any remaining frames
   Flush();
   // Calculate total run time
   const auto endTime = std::chrono::steady_clock::now();
   m_stats.totalRunTimeSeconds = std::chrono::duration<float>(endTime - m_sessionStartTime).count();
   // Write summary
   WriteRunSummary();
   // Close files
   m_frameMetricsFile.close();
   m_systemInfoFile.close();
   m_summaryFile.close();
   m_sessionActive = false;
}

void PerformanceLogger::LogFrame(const PerformanceMetrics& metrics) {
   if (!m_sessionActive)
      return;
   m_frameBuffer.push_back(metrics);
   m_stats.Update(metrics);
   if (m_frameBuffer.size() >= BUFFER_SIZE)
      Flush();
}

void PerformanceLogger::Flush() {
   if (m_frameBuffer.empty() || !m_sessionActive)
      return;
   for (const PerformanceMetrics& frame : m_frameBuffer) {
      const uint64_t frameNum =
         m_stats.totalFrames - m_frameBuffer.size() + (&frame - m_frameBuffer.data()) + 1;
      m_frameMetricsFile << frameNum << "," << frame.frameTimeMs << "," << frame.cpuTimeMs << ","
                         << frame.gpuTimeMs << "," << frame.GetFPS() << "," << frame.geometryPassMs
                         << "," << frame.lightingPassMs << "," << frame.gizmoPassMs << ","
                         << frame.particlePassMs << "," << frame.imguiPassMs << ","
                         << frame.vramUsageMB << "," << frame.systemMemUsageMB << ","
                         << frame.cpuUtilization << "\n";
   }
   m_frameMetricsFile.flush();
   m_frameBuffer.clear();
}

void PerformanceLogger::WriteSystemInfoCSV(const SystemInfo& info) {
   m_systemInfoFile << "Property,Value\n";
   m_systemInfoFile << "CPU Model," << info.cpuModel << "\n";
   m_systemInfoFile << "Thread Count," << info.threadCount << "\n";
   m_systemInfoFile << "Clock Speed (GHz)," << std::fixed << std::setprecision(2)
                    << info.clockSpeedGHz << "\n";
   m_systemInfoFile << "GPU Model," << info.gpuModel << "\n";
   m_systemInfoFile << "VRAM (MB)," << info.vramMB << "\n";
   m_systemInfoFile << "Driver Version," << info.driverVersion << "\n";
   m_systemInfoFile << "API Version," << info.apiVersion << "\n";
   m_systemInfoFile << "Window Width," << info.windowWidth << "\n";
   m_systemInfoFile << "Window Height," << info.windowHeight << "\n";
   m_systemInfoFile.flush();
}

void PerformanceLogger::WriteFrameMetricsHeader() {
   m_frameMetricsFile << "Frame,FrameTime(ms),CPUTime(ms),GPUTime(ms),FPS,"
                      << "GeometryPass(ms),LightingPass(ms),GizmoPass(ms),"
                      << "ParticlePass(ms),ImGuiPass(ms),"
                      << "VRAM(MB),SystemMem(MB),CPUUtil(%)\n";
}

void PerformanceLogger::WriteRunSummary() {
   m_summaryFile << "Metric,Value\n";
   m_summaryFile << "Total Frames," << m_stats.totalFrames << "\n";
   m_summaryFile << "Total Run Time (s)," << std::fixed << std::setprecision(3)
                 << m_stats.totalRunTimeSeconds << "\n";
   m_summaryFile << "Average FPS," << std::fixed << std::setprecision(2) << m_stats.avgFPS << "\n";
   m_summaryFile << "Minimum FPS," << std::fixed << std::setprecision(2) << m_stats.minFPS << "\n";
   m_summaryFile << "Maximum FPS," << std::fixed << std::setprecision(2) << m_stats.maxFPS << "\n";
   m_summaryFile << "Average Frame Time (ms)," << std::fixed << std::setprecision(3)
                 << m_stats.avgFrameTime << "\n";
   m_summaryFile << "Minimum Frame Time (ms)," << std::fixed << std::setprecision(3)
                 << m_stats.minFrameTime << "\n";
   m_summaryFile << "Maximum Frame Time (ms)," << std::fixed << std::setprecision(3)
                 << m_stats.maxFrameTime << "\n";
   m_summaryFile << "\nRender Pass Timings (Average ms)\n";
   m_summaryFile << "Geometry Pass," << std::fixed << std::setprecision(3)
                 << m_stats.avgGeometryPassMs << "\n";
   m_summaryFile << "Lighting Pass," << std::fixed << std::setprecision(3)
                 << m_stats.avgLightingPassMs << "\n";
   m_summaryFile << "Gizmo Pass," << std::fixed << std::setprecision(3) << m_stats.avgGizmoPassMs
                 << "\n";
   m_summaryFile << "Particle Pass," << std::fixed << std::setprecision(3)
                 << m_stats.avgParticlePassMs << "\n";
   m_summaryFile << "ImGui Pass," << std::fixed << std::setprecision(3) << m_stats.avgImguiPassMs
                 << "\n";
   m_summaryFile.flush();
}
