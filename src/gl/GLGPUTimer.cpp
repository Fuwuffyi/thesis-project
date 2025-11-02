#include "gl/GLGPUTimer.hpp"

GLGPUTimer::GLGPUTimer() = default;

GLGPUTimer::~GLGPUTimer() {
   for (const auto& [label, query] : m_queries) {
      DeleteQuery(label);
   }
}

void GLGPUTimer::CreateQuery(const std::string& label) {
   auto& query = m_queries[label];
   if (query.beginQuery == 0) {
      glGenQueries(1, &query.beginQuery);
      glGenQueries(1, &query.endQuery);
   }
}

void GLGPUTimer::DeleteQuery(const std::string& label) {
   const auto it = m_queries.find(label);
   if (it != m_queries.end()) {
      if (it->second.beginQuery != 0) {
         glDeleteQueries(1, &it->second.beginQuery);
         glDeleteQueries(1, &it->second.endQuery);
      }
   }
}

void GLGPUTimer::Begin(const std::string& label) {
   CreateQuery(label);
   auto& query = m_queries[label];
   query.active = true;
   query.hasResult = false;
   glQueryCounter(query.beginQuery, GL_TIMESTAMP);
}

void GLGPUTimer::End(const std::string& label) {
   const auto it = m_queries.find(label);
   if (it == m_queries.end() || !it->second.active) {
      return;
   }
   auto& query = it->second;
   glQueryCounter(query.endQuery, GL_TIMESTAMP);
   query.active = false;
   int32_t available = 0;
   glGetQueryObjectiv(query.endQuery, GL_QUERY_RESULT_AVAILABLE, &available);
   if (available) {
      size_t startTime = 0, endTime = 0;
      glGetQueryObjectui64v(query.beginQuery, GL_QUERY_RESULT, &startTime);
      glGetQueryObjectui64v(query.endQuery, GL_QUERY_RESULT, &endTime);
      query.cachedResultMs = static_cast<float>(endTime - startTime) / 1000000.0f;
      query.hasResult = true;
   }
}

float GLGPUTimer::GetElapsedMs(const std::string& label) {
   auto it = m_queries.find(label);
   if (it == m_queries.end()) {
      return 0.0f;
   }
   auto& query = it->second;
   if (!query.hasResult && !query.active) {
      int32_t available = 0;
      glGetQueryObjectiv(query.endQuery, GL_QUERY_RESULT_AVAILABLE, &available);
      if (available) {
         size_t startTime = 0, endTime = 0;
         glGetQueryObjectui64v(query.beginQuery, GL_QUERY_RESULT, &startTime);
         glGetQueryObjectui64v(query.endQuery, GL_QUERY_RESULT, &endTime);
         query.cachedResultMs = static_cast<float>(endTime - startTime) / 1000000.0f;
         query.hasResult = true;
      }
   }
   return query.cachedResultMs;
}

void GLGPUTimer::Reset() {
   for (auto& [label, query] : m_queries) {
      query.hasResult = false;
      query.cachedResultMs = 0.0f;
   }
}

bool GLGPUTimer::IsAvailable(const std::string& label) const {
   const auto it = m_queries.find(label);
   if (it == m_queries.end() || it->second.active) {
      return false;
   }
   if (it->second.hasResult) {
      return true;
   }
   // Check if result is available now
   int32_t available = 0;
   glGetQueryObjectiv(it->second.endQuery, GL_QUERY_RESULT_AVAILABLE, &available);
   return available != 0;
}
