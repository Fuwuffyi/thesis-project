#include "Node.hpp"

#include "components/Component.hpp"
#include "components/TransformComponent.hpp"
#include <algorithm>
#include <stack>

Node::Node(const std::string& name)
   :
   m_name(name),
   m_active(true),
   m_parent(nullptr),
   m_localTransform(nullptr),
   m_worldTransform(nullptr),
   m_worldTransformDirty(true)
{}

Node::~Node() {
   RemoveAllChildren();
   if (m_parent) {
      m_parent->RemoveChild(this);
   }
}

void Node::AddChild(std::unique_ptr<Node> child) {
   if (!child) return;
   // Remove from previous parent if any
   if (child->m_parent) {
      child->m_parent->RemoveChild(child.get());
   }
   child->SetParent(this);
   m_children.emplace_back(std::move(child));
}

void Node::AddChild(Node* child) {
   if (!child) return;
   // This version assumes external ownership management
   // Just set up the parent-child relationship
   if (child->m_parent && child->m_parent != this) {
      child->m_parent->RemoveChild(child);
   }
   child->SetParent(this);
   // Note: We don't store the child in m_children for external ownership
}

bool Node::RemoveChild(const Node* child) {
   if (!child) return false;
   const auto it = std::find_if(m_children.begin(), m_children.end(),
                                [child](const std::unique_ptr<Node>& ptr) {
                                return ptr.get() == child;
                                });
   if (it != m_children.end()) {
      (*it)->SetParent(nullptr);
      m_children.erase(it);
      return true;
   }
   return false;
}

bool Node::RemoveChild(const std::string& name) {
   const auto it = std::find_if(m_children.begin(), m_children.end(),
                                [&name](const std::unique_ptr<Node>& ptr) {
                                return ptr->GetName() == name;
                                });
   if (it != m_children.end()) {
      (*it)->SetParent(nullptr);
      m_children.erase(it);
      return true;
   }
   return false;
}

void Node::RemoveAllChildren() {
   for (const auto& child : m_children) {
      if (child) {
         child->SetParent(nullptr);
      }
   }
   m_children.clear();
}

Node* Node::GetParent() const {
   return m_parent;
}

const std::vector<std::unique_ptr<Node>>& Node::GetChildren() const {
   return m_children;
}

std::vector<Node*> Node::GetChildrenRaw() const {
   std::vector<Node*> result;
   result.reserve(m_children.size());
   for (const auto& child : m_children) {
      result.push_back(child.get());
   }
   return result;
}

Node* Node::FindChild(const std::string& name, const bool recursive) const {
   // Check direct children first
   for (const auto& child : m_children) {
      if (child->GetName() == name) {
         return child.get();
      }
   }
   // Recursive search if requested
   if (recursive) {
      for (const auto& child : m_children) {
         if (Node* found = child->FindChild(name, true)) {
            return found;
         }
      }
   }
   return nullptr;
}

Node* Node::FindChildByIndex(const size_t index) const {
   if (index < m_children.size()) {
      return m_children[index].get();
   }
   return nullptr;
}

size_t Node::GetChildCount() const {
   return m_children.size();
}

size_t Node::GetDepth() const {
   size_t depth = 0;
   const Node* current = this;
   while (current->m_parent) {
      current = current->m_parent;
      ++depth;
   }
   return depth;
}

bool Node::IsAncestorOf(const Node* node) const {
   if (!node) return false;
   const Node* current = node->m_parent;
   while (current) {
      if (current == this) return true;
      current = current->m_parent;
   }
   return false;
}

bool Node::IsDescendantOf(const Node* node) const {
   return node ? node->IsAncestorOf(this) : false;
}

Node* Node::GetRoot() {
   Node* current = this;
   while (current->m_parent) {
      current = current->m_parent;
   }
   return current;
}

const Node* Node::GetRoot() const {
   const Node* current = this;
   while (current->m_parent) {
      current = current->m_parent;
   }
   return current;
}

void Node::ForEachChild(const std::function<void(Node*)>& func, const bool recursive) {
   for (const auto& child : m_children) {
      func(child.get());
      if (recursive) {
         child->ForEachChild(func, true);
      }
   }
}

void Node::ForEachChild(const std::function<void(const Node*)>& func, const bool recursive) const {
   for (const auto& child : m_children) {
      func(child.get());
      if (recursive) {
         const Node* constChild = child.get();
         constChild->ForEachChild(func, true);
      }
   }
}

void Node::AddComponent(std::unique_ptr<Component> component) {
   if (!component) return;
   m_components.emplace_back(std::move(component));
   UpdateComponentLookup();
   if (auto* transform = dynamic_cast<TransformComponent*>(m_components.back().get())) {
      m_localTransform = transform->GetTransform();
      MarkTransformDirty();
   }
}

bool Node::RemoveComponent(const Component* component) {
   if (!component) return false;
   const auto it = std::find_if(m_components.begin(), m_components.end(),
                                [component](const std::unique_ptr<Component>& ptr) {
                                return ptr.get() == component;
                                });
   if (it != m_components.end()) {
      // Clear transform cache if removing transform component
      if (m_localTransform && dynamic_cast<TransformComponent*>(it->get())) {
         m_localTransform = nullptr;
         MarkTransformDirty();
      }
      m_components.erase(it);
      UpdateComponentLookup();
      return true;
   }
   return false;
}

Transform* Node::GetTransform() const {
   if (!m_localTransform) {
      for (const auto& component : m_components) {
         if (auto* transformComp = dynamic_cast<TransformComponent*>(component.get())) {
            m_localTransform = transformComp->GetTransform();
            break;
         }
      }
   }
   return m_localTransform;
}

Transform* Node::GetWorldTransform() const {
   if (m_worldTransformDirty) {
      const_cast<Node*>(this)->UpdateWorldTransform();
   }
   return m_worldTransform.get();
}

void Node::UpdateWorldTransform(const bool force) {
   if (!m_worldTransformDirty && !force) return;
   Transform* localTransform = GetTransform();
   if (!localTransform) {
      m_worldTransformDirty = false;
      return;
   }
   if (!m_worldTransform) {
      m_worldTransform = std::make_unique<Transform>();
   }
   if (m_parent) {
      Transform* parentWorldTransform = m_parent->GetWorldTransform();
      if (parentWorldTransform) {
         // Combine parent world transform with local transform
         glm::mat4 parentMatrix = parentWorldTransform->GetTransformMatrix();
         glm::mat4 localMatrix = localTransform->GetTransformMatrix();
         glm::mat4 worldMatrix = parentMatrix * localMatrix;
         *m_worldTransform = Transform(worldMatrix);
      } else {
         *m_worldTransform = *localTransform;
      }
   } else {
      *m_worldTransform = *localTransform;
   }
   m_worldTransformDirty = false;
   // Update children
   UpdateChildrenWorldTransforms();
}

void Node::MarkTransformDirty() {
   InvalidateWorldTransform();
}

const std::string& Node::GetName() const {
   return m_name;
}

void Node::SetName(const std::string& name) {
   m_name = name;
}

bool Node::IsActive() const {
   return m_active;
}

void Node::SetActive(const bool active) {
   if (m_active != active) {
      m_active = active;
   }
}

void Node::SetParent(Node* parent) {
   if (m_parent != parent) {
      m_parent = parent;
      MarkTransformDirty();
   }
}

void Node::UpdateComponentLookup() {
   m_componentLookup.clear();
   for (const auto& component : m_components) {
      const size_t typeId = typeid(*component).hash_code();
      m_componentLookup[typeId] = component.get();
   }
}

void Node::InvalidateWorldTransform() {
   if (m_worldTransformDirty) return;
   m_worldTransformDirty = true;
   // Propagate to children
   for (const auto& child : m_children) {
      child->InvalidateWorldTransform();
   }
}

void Node::UpdateChildrenWorldTransforms() {
   for (const auto& child : m_children) {
      if (child->m_worldTransformDirty) {
         child->UpdateWorldTransform();
      }
   }
}

