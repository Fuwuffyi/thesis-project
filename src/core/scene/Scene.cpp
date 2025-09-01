#include "core/scene/Scene.hpp"

#include "core/scene/Node.hpp"

#include "core/scene/components/LightComponent.hpp"
#include "core/scene/components/RendererComponent.hpp"
#include "core/scene/components/TransformComponent.hpp"

#include <imgui.h>

#include <algorithm>
#include <queue>

Scene::Scene(std::string name)
   :
   m_name(std::move(name)),
   m_nodeCounter(0)
{
   m_rootNode = std::make_unique<Node>("Root");
   m_rootNode->AddComponent<TransformComponent>();
   RegisterNode(m_rootNode.get());
}

Scene::~Scene() {
   Clear();
}

void Scene::DrawInspector() {
   const ImGuiViewport* viewport = ImGui::GetMainViewport();
   static Node* selectedNode = nullptr;
   ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - 300, viewport->WorkPos.y)); // 300 px width
   ImGui::SetNextWindowSize(ImVec2(300, viewport->WorkSize.y));
   ImGui::SetNextWindowBgAlpha(0.35f);
   ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
      ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoSavedSettings |
      ImGuiWindowFlags_NoFocusOnAppearing |
      ImGuiWindowFlags_NoNav;
   ImGui::Begin("Scene Graph", nullptr, flags);
   // Recursive function to display hierarchy
   std::function<void(Node*)> displayNodeHierarchy = [&](Node* node) {
      if (!node) return;
      ImGui::PushID(node);
      // Tree node display
      bool hasChildren = node->GetChildCount() > 0;
      ImGuiTreeNodeFlags nodeFlags =
         ImGuiTreeNodeFlags_OpenOnArrow |
         ImGuiTreeNodeFlags_OpenOnDoubleClick |
         ImGuiTreeNodeFlags_SpanAvailWidth |
         (hasChildren ? 0 : ImGuiTreeNodeFlags_Leaf);
      if (selectedNode == node) nodeFlags |= ImGuiTreeNodeFlags_Selected;
      bool open = ImGui::TreeNodeEx("##treenode", nodeFlags, node->GetName().c_str());
      // Select object
      if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::IsItemToggledOpen()) {
         selectedNode = node;
      }
      // Show active checkbox on right side
      ImGui::SameLine(ImGui::GetContentRegionAvail().x - 25);
      bool active = node->IsActive();
      if (ImGui::Checkbox("##active", &active)) {
         node->SetActive(active);
      }
      // Children
      if (open) {
         node->ForEachChild([&](Node* child) {
            displayNodeHierarchy(child);
         }, false);
         ImGui::TreePop();
      }
      ImGui::PopID();
   };
   // Start from root
   if (m_rootNode) {
      displayNodeHierarchy(m_rootNode.get());
   }
   ImGui::End();
   if (selectedNode) {
      ImGui::SetNextWindowBgAlpha(0.35f);
      ImGui::Begin("Inspector");
      if (TransformComponent* comp = selectedNode->GetComponent<TransformComponent>())
         comp->DrawInspector(selectedNode);
      if (RendererComponent* comp = selectedNode->GetComponent<RendererComponent>())
         comp->DrawInspector(selectedNode);
      if (LightComponent* comp = selectedNode->GetComponent<LightComponent>())
         comp->DrawInspector(selectedNode);
      if (ImGui::Button("Add component"))
         ImGui::OpenPopup("add_component_popup");
      if (ImGui::BeginPopup("add_component_popup")) {
         if (!selectedNode->HasComponent<TransformComponent>() && ImGui::Button("Transform")) {
            selectedNode->AddComponent<TransformComponent>();
            ImGui::CloseCurrentPopup();
         }
         if (!selectedNode->HasComponent<RendererComponent>() && ImGui::Button("Renderer")) {
            selectedNode->AddComponent<RendererComponent>();
            ImGui::CloseCurrentPopup();
         }
         if (!selectedNode->HasComponent<LightComponent>() && ImGui::Button("Light")) {
            selectedNode->AddComponent<LightComponent>();
            ImGui::CloseCurrentPopup();
         }
         ImGui::EndPopup();
      }
      static char childName[60] = {};
      if (ImGui::Button("Add child"))
         ImGui::OpenPopup("add_child_popup");
      if (ImGui::BeginPopup("add_child_popup")) {
         ImGui::InputText("Child Name", childName, IM_ARRAYSIZE(childName));
         if (ImGui::Button("Add")) {
            if (strlen(childName) > 0) {
               this->CreateChildNode(selectedNode, std::string_view{childName});
               childName[0] = '\0';
               ImGui::CloseCurrentPopup();
            }
         }
         ImGui::EndPopup();
      }
      ImGui::End();
   }
}

[[nodiscard]] Node* Scene::GetRootNode() const noexcept {
   return m_rootNode.get();
}

Node* Scene::CreateNode(std::string_view name) {
   std::string nodeName{name};
   if (nodeName.empty()) {
      nodeName = "Node_" + std::to_string(m_nodeCounter++);
   }
   auto node = std::make_unique<Node>(nodeName);
   node->AddComponent<TransformComponent>();
   Node* nodePtr = node.get();
   m_rootNode->AddChild(std::move(node));
   RegisterNode(nodePtr);
   return nodePtr;
}

Node* Scene::CreateChildNode(Node* parent, std::string_view name) {
   if (!parent) {
      return CreateNode(name);
   }
   std::string nodeName{name};
   if (nodeName.empty()) {
      nodeName = "Node_" + std::to_string(m_nodeCounter++);
   }
   auto node = std::make_unique<Node>(nodeName);
   node->AddComponent<TransformComponent>();
   Node* nodePtr = node.get();
   parent->AddChild(std::move(node));
   RegisterNode(nodePtr);
   return nodePtr;
}

Node* Scene::CreateChildNode(std::string_view parentName, std::string_view childName) {
   Node* parent = FindNode(parentName);
   return CreateChildNode(parent, childName);
}

bool Scene::AddNode(std::unique_ptr<Node> node, Node* parent) {
   if (!node) return false;
   Node* nodePtr = node.get();
   if (parent) {
      parent->AddChild(std::move(node));
   } else {
      m_rootNode->AddChild(std::move(node));
   }
   RegisterNode(nodePtr);
   return true;
}

bool Scene::RemoveNode(Node* node) {
   if (!node || node == m_rootNode.get()) {
      return false;
   }
   UnregisterNode(node);
   Node* parent = node->GetParent();
   return parent ? parent->RemoveChild(node) : false;
}

bool Scene::RemoveNode(std::string_view name) {
   if (Node* node = FindNode(name)) {
      return RemoveNode(node);
   }
   return false;
}

[[nodiscard]] Node* Scene::FindNode(std::string_view name) const {
   if (auto it = m_nodeRegistry.find(std::string{name}); it != m_nodeRegistry.end()) {
      return it->second;
   }
   return nullptr;
}

[[nodiscard]] std::vector<Node*> Scene::FindNodes(std::string_view name) const {
   std::vector<Node*> result;
   const std::string nameStr{name};
   auto range = m_nodeRegistry.equal_range(nameStr);
   std::ranges::transform(
      std::ranges::subrange(range.first, range.second),
      std::back_inserter(result),
      [](const auto& pair) { return pair.second; }
   );
   return result;
}

void Scene::ForEachNode(const std::function<void(Node*)>& func) {
   if (!func || !m_rootNode) return;
   std::queue<Node*> queue;
   queue.push(m_rootNode.get());
   while (!queue.empty()) {
      Node* current = queue.front();
      queue.pop();
      func(current);
      // Add children to queue
      for (Node* child : current->GetChildrenRaw()) {
         queue.push(child);
      }
   }
}

void Scene::ForEachNode(const std::function<void(const Node*)>& func) const {
   if (!func || !m_rootNode) return;
   std::queue<const Node*> queue;
   queue.push(m_rootNode.get());
   while (!queue.empty()) {
      const Node* current = queue.front();
      queue.pop();
      func(current);
      for (const auto& child : current->GetChildren()) {
         queue.push(child.get());
      }
   }
}

void Scene::UpdateTransforms() {
   if (m_rootNode) {
      m_rootNode->UpdateWorldTransform(true);
   }
}

void Scene::UpdateScene(const float deltaTime) {
   UpdateTransforms();
   // TODO: Add component updates using ranges
   // ForEachNode([deltaTime](Node* node) {
   //     // Update components that need per-frame updates
   //     auto updatableComponents = node->GetComponents<UpdatableComponent>();
   //     std::ranges::for_each(updatableComponents, 
   //         [deltaTime](auto* comp) { comp->Update(deltaTime); });
   // });
}

void Scene::Clear() {
   m_nodeRegistry.clear();
   if (m_rootNode) {
      m_rootNode->RemoveAllChildren();
   }
   m_nodeCounter = 0;
}

[[nodiscard]] size_t Scene::GetNodeCount() const noexcept {
   return m_nodeRegistry.size();
}

[[nodiscard]] const std::string& Scene::GetName() const noexcept {
   return m_name;
}

void Scene::SetName(std::string name) {
   m_name = std::move(name);
}

void Scene::RegisterNode(Node* node) {
   if (node) {
      m_nodeRegistry.emplace(node->GetName(), node);
   }
}

void Scene::UnregisterNode(Node* node) {
   if (!node) return;
   // Remove all entries with this node pointer
   const auto range = m_nodeRegistry.equal_range(node->GetName());
   for (auto it = range.first; it != range.second;) {
      if (it->second == node) {
         it = m_nodeRegistry.erase(it);
      } else {
         ++it;
      }
   }
   // Also unregister all children
   for (Node* child : node->GetChildrenRaw()) {
      UnregisterNode(child);
   }
}

[[nodiscard]] size_t Scene::CalculateMaxDepth(const Node* node, const size_t currentDepth) const {
   if (!node) return currentDepth;
   size_t maxDepth = currentDepth;
   for (const auto& child : node->GetChildren()) {
      const size_t childDepth = CalculateMaxDepth(child.get(), currentDepth + 1);
      maxDepth = std::max(maxDepth, childDepth);
   }
   return maxDepth;
}

