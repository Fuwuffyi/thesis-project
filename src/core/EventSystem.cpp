#include "EventSystem.hpp"

#include <GLFW/glfw3.h>

void EventSystem::OnKeyDown(const uint32_t key, const KeyCallback cb) {
   m_keyDownListeners[key].push_back(std::move(cb));
}

void EventSystem::OnKeyUp(const uint32_t key, const KeyCallback cb) {
   m_keyUpListeners[key].push_back(std::move(cb));
}

void EventSystem::OnKeyHeld(const uint32_t key, const KeyCallback cb) {
   m_keyHeldListeners[key].push_back(std::move(cb));
}

void EventSystem::OnMouseDown(const uint32_t button, const MouseCallback cb) {
   m_mouseDownListeners[button].push_back(std::move(cb));
}

void EventSystem::OnMouseUp(const uint32_t button, const MouseCallback cb) {
   m_mouseUpListeners[button].push_back(std::move(cb));
}

void EventSystem::OnMouseHeld(const uint32_t button, const MouseCallback cb) {
   m_mouseHeldListeners[button].push_back(std::move(cb));
}

void EventSystem::OnCursorPos(const CursorPosCallback cb) {
   m_cursorPosListeners.push_back(std::move(cb));
}

void EventSystem::OnResize(const ResizeCallback cb) { m_resizeListeners.push_back(std::move(cb)); }

void EventSystem::HandleKeyEvent(const uint32_t key, const uint32_t scancode, const uint32_t action,
                                 const uint32_t mods) {
   if (action == GLFW_PRESS) {
      m_pressedKeys.insert(key);
      if (const auto it = m_keyDownListeners.find(key); it != m_keyDownListeners.end()) {
         for (const auto& cb : it->second)
            cb(key, scancode, mods);
      }
   } else if (action == GLFW_RELEASE) {
      m_pressedKeys.erase(key);
      if (const auto it = m_keyUpListeners.find(key); it != m_keyUpListeners.end()) {
         for (const auto& cb : it->second)
            cb(key, scancode, mods);
      }
   }
}

void EventSystem::HandleMouseEvent(const uint32_t button, const uint32_t action,
                                   const uint32_t mods) {
   if (action == GLFW_PRESS) {
      m_pressedMouseButtons.insert(button);
      if (const auto it = m_mouseDownListeners.find(button); it != m_mouseDownListeners.end()) {
         for (const auto& cb : it->second)
            cb(button, mods);
      }
   } else if (action == GLFW_RELEASE) {
      m_pressedMouseButtons.erase(button);
      if (const auto it = m_mouseUpListeners.find(button); it != m_mouseUpListeners.end()) {
         for (const auto& cb : it->second)
            cb(button, mods);
      }
   }
}

void EventSystem::HandleCursorPos(const float xpos, const float ypos) {
   for (const auto& cb : m_cursorPosListeners)
      cb(xpos, ypos);
}

void EventSystem::HandleResize(const uint32_t width, const uint32_t height) {
   for (const auto& cb : m_resizeListeners)
      cb(width, height);
}

void EventSystem::ProcessHeldEvents() {
   for (const uint32_t key : m_pressedKeys) {
      if (const auto it = m_keyHeldListeners.find(key); it != m_keyHeldListeners.end()) {
         for (const auto& cb : it->second)
            cb(key, 0, 0);
      }
   }
   for (const uint32_t button : m_pressedMouseButtons) {
      if (const auto it = m_mouseHeldListeners.find(button); it != m_mouseHeldListeners.end()) {
         for (const auto& cb : it->second)
            cb(button, 0);
      }
   }
}
