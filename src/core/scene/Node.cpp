#include "Node.hpp"

#include "components/Component.hpp"

Node::~Node() = default;

const std::vector<std::unique_ptr<Node>>& Node::GetChildren() const {
   return m_children;
}

void Node::AddChild(std::unique_ptr<Node> child) {
   m_children.emplace_back(std::move(child));
}

void Node::AddComponent(std::unique_ptr<Component> component) {
   m_components.emplace_back(std::move(component));
}

