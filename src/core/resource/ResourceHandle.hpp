#pragma once

#include <cstdint>

template <typename T>
class ResourceHandle final {
  public:
   ResourceHandle() : m_id(0) {}

   explicit ResourceHandle(const uint64_t id) : m_id(id) {}

   bool IsValid() const { return m_id != 0; }

   uint64_t GetId() const { return m_id; }

   bool operator==(const ResourceHandle& other) const { return m_id == other.m_id; }

   bool operator!=(const ResourceHandle& other) const { return m_id != other.m_id; }

  private:
   uint64_t m_id;
};
