#pragma once

#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <GLFW/glfw3.h>

class EventSystem {
public:
   using KeyCallback = std::function<void(const uint32_t key, const uint32_t scancode, const uint32_t mods)>;
   using MouseCallback = std::function<void(const uint32_t button, const uint32_t mods)>;
   using CursorPosCallback = std::function<void(const float xpos, const float ypos)>;
   using ResizeCallback = std::function<void(const uint32_t width, const uint32_t height)>;

   EventSystem() = default;
   ~EventSystem() = default;

   void OnKeyDown(const uint32_t key, const KeyCallback cb);
   void OnKeyUp(const uint32_t key, const KeyCallback cb);
   void OnKeyHeld(const uint32_t key, const KeyCallback cb);
   void OnMouseDown(const uint32_t button, const MouseCallback cb);
   void OnMouseUp(const uint32_t button, const MouseCallback cb);
   void OnMouseHeld(const uint32_t button, const MouseCallback cb);

   void OnCursorPos(const CursorPosCallback cb);
   void OnResize(const ResizeCallback cb);

   void HandleKeyEvent(const uint32_t key, const uint32_t scancode, const uint32_t action, const uint32_t mods);
   void HandleMouseEvent(const uint32_t button, const uint32_t action, const uint32_t mods);

   void HandleCursorPos(const float xpos, const float ypos);
   void HandleResize(const uint32_t width, const uint32_t height);
   void ProcessHeldEvents();

private:
   std::unordered_map<uint32_t, std::vector<KeyCallback>> m_keyDownListeners;
   std::unordered_map<uint32_t, std::vector<KeyCallback>> m_keyUpListeners;
   std::unordered_map<uint32_t, std::vector<KeyCallback>> m_keyHeldListeners;
   std::unordered_map<uint32_t, std::vector<MouseCallback>> m_mouseDownListeners;
   std::unordered_map<uint32_t, std::vector<MouseCallback>> m_mouseUpListeners;
   std::unordered_map<uint32_t, std::vector<MouseCallback>> m_mouseHeldListeners;
   std::vector<CursorPosCallback> m_cursorPosListeners;
   std::vector<ResizeCallback> m_resizeListeners;
   std::unordered_set<uint32_t> m_pressedKeys;
   std::unordered_set<uint32_t> m_pressedMouseButtons;
};
