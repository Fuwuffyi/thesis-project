#pragma once

#include "core/resource/IResource.hpp"

class IMesh : public IResource {
public:
   virtual ~IMesh() = default;
   virtual void Draw() const = 0;
   virtual size_t GetVertexCount() const = 0;
   virtual size_t GetIndexCount() const = 0;
   virtual void* GetNativeHandle() const = 0;
};

