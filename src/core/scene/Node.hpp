#pragma once

#include <vector>
#include <memory>

class Component;

class Node {
public:
   ~Node();

   const std::vector<std::unique_ptr<Node>>& GetChildren() const;
 
   void AddComponent(std::unique_ptr<Component> component);

   template<typename T>
   T* GetComponent() {
      for (const std::unique_ptr<Component>& c : m_components) {
         if (T* ptr = dynamic_cast<T*>(c.get())) return ptr;
      }
      return nullptr;
   }

private:
   std::vector<std::unique_ptr<Node>> m_children;
   std::vector<std::unique_ptr<Component>> m_components;
};

