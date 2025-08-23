#pragma once

#include "core/resource/IMesh.hpp"
#include "core/resource/ITexture.hpp"

template<typename T>
class ResourceHandle {
public:
   ResourceHandle() : m_id(0) {}
   explicit ResourceHandle(uint64_t id) : m_id(id) {}

   bool IsValid() const { return m_id != 0; }
   uint64_t GetId() const { return m_id; }

   bool operator==(const ResourceHandle& other) const { return m_id == other.m_id; }
   bool operator!=(const ResourceHandle& other) const { return m_id != other.m_id; }

private:
   uint64_t m_id;
};

using TextureHandle = ResourceHandle<ITexture>;
using MeshHandle = ResourceHandle<IMesh>;

