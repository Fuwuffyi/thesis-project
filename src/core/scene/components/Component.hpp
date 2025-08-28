#pragma once

#include <concepts>

class Component {
public:
   virtual ~Component() = default;

   Component(const Component&) = delete;
   Component& operator=(const Component&) = delete;
   Component(Component&&) = default;
   Component& operator=(Component&&) = default;

protected:
   Component() = default;
};

template<typename T>
concept ComponentType = std::derived_from<T, Component>;

