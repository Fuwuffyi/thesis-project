#include "gl/GLRenderPass.hpp"
#include "gl/GLFramebuffer.hpp"
#include "gl/GLShader.hpp"

#include <stdexcept>

GLRenderPass::GLRenderPass(const CreateInfo& info)
    : m_framebuffer(info.framebuffer),
      m_colorAttachments(info.colorAttachments),
      m_depthStencilAttachment(info.depthStencilAttachment),
      m_renderState(info.renderState),
      m_primitiveType(info.renderState.primitiveType),
      m_shader(info.shader) {
   // Validate color attachments match framebuffer
   if (m_framebuffer) {
      if (m_colorAttachments.size() != m_framebuffer->GetColorAttachmentCount()) {
         throw std::runtime_error(
            "Color attachment count mismatch between render pass and framebuffer");
      }
   }
}

void GLRenderPass::Begin() {
   if (m_isActive) [[unlikely]] {
      throw std::runtime_error("Render pass is already active");
   }
   // Save current OpenGL state efficiently
   glGetIntegerv(GL_VIEWPORT, m_previousState.viewport.data());
   m_previousState.depthTest = glIsEnabled(GL_DEPTH_TEST);
   glGetBooleanv(GL_DEPTH_WRITEMASK, reinterpret_cast<unsigned char*>(&m_previousState.depthMask));
   glGetIntegerv(GL_DEPTH_FUNC, reinterpret_cast<int32_t*>(&m_previousState.depthFunc));
   m_previousState.cullFace = glIsEnabled(GL_CULL_FACE);
   glGetIntegerv(GL_CULL_FACE_MODE, reinterpret_cast<int32_t*>(&m_previousState.cullFaceMode));
   glGetIntegerv(GL_FRONT_FACE, reinterpret_cast<int32_t*>(&m_previousState.frontFace));
   m_previousState.blend = glIsEnabled(GL_BLEND);
   glGetIntegerv(GL_BLEND_SRC_RGB, reinterpret_cast<int32_t*>(&m_previousState.blendSrc));
   glGetIntegerv(GL_BLEND_DST_RGB, reinterpret_cast<int32_t*>(&m_previousState.blendDst));
   glGetIntegerv(GL_BLEND_EQUATION_RGB, reinterpret_cast<int32_t*>(&m_previousState.blendEquation));
   glGetFloatv(GL_LINE_WIDTH, &m_previousState.lineWidth);
   glGetFloatv(GL_POINT_SIZE, &m_previousState.pointSize);
   glGetIntegerv(GL_POLYGON_MODE, reinterpret_cast<int32_t*>(m_previousState.polygonMode.data()));
   m_previousState.scissorTest = glIsEnabled(GL_SCISSOR_TEST);
   glGetIntegerv(GL_SCISSOR_BOX, m_previousState.scissorBox.data());
   // Bind framebuffer
   if (m_framebuffer) {
      m_framebuffer->Bind();
   } else {
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
   }
   // Clear attachments based on load operations
   ClearAttachments();
   // Apply render state
   ApplyRenderState();
   // Bind shader if specified
   if (m_shader) {
      m_shader->Use();
   }
   m_isActive = true;
}

void GLRenderPass::End() {
   if (!m_isActive) [[unlikely]] {
      throw std::runtime_error("Render pass is not active");
   }
   // Restore previous OpenGL state efficiently
   const auto& vp = m_previousState.viewport;
   glViewport(vp[0], vp[1], vp[2], vp[3]);
   if (m_previousState.depthTest) {
      glEnable(GL_DEPTH_TEST);
   } else {
      glDisable(GL_DEPTH_TEST);
   }
   glDepthMask(m_previousState.depthMask);
   glDepthFunc(m_previousState.depthFunc);
   if (m_previousState.cullFace) {
      glEnable(GL_CULL_FACE);
   } else {
      glDisable(GL_CULL_FACE);
   }
   glCullFace(m_previousState.cullFaceMode);
   glFrontFace(m_previousState.frontFace);
   if (m_previousState.blend) {
      glEnable(GL_BLEND);
   } else {
      glDisable(GL_BLEND);
   }
   glBlendFunc(m_previousState.blendSrc, m_previousState.blendDst);
   glBlendEquation(m_previousState.blendEquation);
   glLineWidth(m_previousState.lineWidth);
   glPointSize(m_previousState.pointSize);
   glPolygonMode(GL_FRONT_AND_BACK, m_previousState.polygonMode[0]);
   if (m_previousState.scissorTest) {
      glEnable(GL_SCISSOR_TEST);
   } else {
      glDisable(GL_SCISSOR_TEST);
   }
   const auto& sb = m_previousState.scissorBox;
   glScissor(sb[0], sb[1], sb[2], sb[3]);
   // Unbind framebuffer
   if (m_framebuffer) {
      GLFramebuffer::Unbind();
   }
   // Unbind shader
   if (m_shader) {
      GLShader::Unbind();
   }
   m_isActive = false;
}

void GLRenderPass::SetShader(const GLShader* shader) {
   if (!m_isActive) [[unlikely]] {
      throw std::runtime_error("Cannot set shader when render pass is not active");
   }
   if (m_shader) {
      GLShader::Unbind();
   }
   m_shader = shader;
   if (m_shader) {
      m_shader->Use();
   }
}

void GLRenderPass::UpdateRenderState(const RenderState& state) {
   m_renderState = state;
   m_primitiveType = state.primitiveType;
   if (m_isActive) {
      ApplyRenderState();
   }
}

uint32_t GLRenderPass::GetViewportWidth() const noexcept {
   if (m_renderState.useFramebufferViewport) {
      if (m_framebuffer) {
         return m_framebuffer->GetWidth();
      } else {
         // Get screen width from current viewport
         std::array<int32_t, 4> viewport;
         glGetIntegerv(GL_VIEWPORT, viewport.data());
         return viewport[2];
      }
   }
   return m_renderState.viewportWidth;
}

uint32_t GLRenderPass::GetViewportHeight() const noexcept {
   if (m_renderState.useFramebufferViewport) {
      if (m_framebuffer) {
         return m_framebuffer->GetHeight();
      } else {
         // Get screen height from current viewport
         std::array<int32_t, 4> viewport;
         glGetIntegerv(GL_VIEWPORT, viewport.data());
         return viewport[3];
      }
   }
   return m_renderState.viewportHeight;
}

void GLRenderPass::ApplyRenderState() const {
   // Viewport
   if (m_renderState.useFramebufferViewport) {
      if (m_framebuffer) {
         glViewport(0, 0, m_framebuffer->GetWidth(), m_framebuffer->GetHeight());
      }
   } else {
      glViewport(m_renderState.viewportX, m_renderState.viewportY, m_renderState.viewportWidth,
                 m_renderState.viewportHeight);
   }
   // Apply render state efficiently
   SetDepthTest(m_renderState.depthTest);
   glDepthMask(m_renderState.depthWrite ? GL_TRUE : GL_FALSE);
   SetCullMode(m_renderState.cullMode);
   glFrontFace(m_renderState.frontFaceCCW ? GL_CCW : GL_CW);
   SetBlendMode(m_renderState.blendMode, m_renderState);
   glLineWidth(m_renderState.lineWidth);
   glPointSize(m_renderState.pointSize);
   glPolygonMode(GL_FRONT_AND_BACK, m_renderState.polygonMode);
   // Scissor test
   if (m_renderState.enableScissor) {
      glEnable(GL_SCISSOR_TEST);
      glScissor(m_renderState.scissorX, m_renderState.scissorY, m_renderState.scissorWidth,
                m_renderState.scissorHeight);
   } else {
      glDisable(GL_SCISSOR_TEST);
   }
}

void GLRenderPass::ClearAttachments() const {
   // Clear color attachments
   for (size_t i = 0; i < m_colorAttachments.size(); ++i) {
      const auto& attachment = m_colorAttachments[i];
      if (attachment.loadOp == LoadOp::Clear) {
         if (m_framebuffer) {
            const std::array<float, 4> clearColor = {
               attachment.clearValue.r, attachment.clearValue.g, attachment.clearValue.b,
               attachment.clearValue.a};
            m_framebuffer->ClearColor(static_cast<uint32_t>(i), clearColor);
         } else {
            // Clear screen
            const auto& cv = attachment.clearValue;
            glClearColor(cv.r, cv.g, cv.b, cv.a);
            glClear(GL_COLOR_BUFFER_BIT);
         }
      }
   }
   // Clear depth/stencil efficiently
   size_t clearMask = 0;
   if (m_depthStencilAttachment.depthLoadOp == LoadOp::Clear) {
      if (m_framebuffer && m_framebuffer->HasDepthAttachment()) {
         m_framebuffer->ClearDepth(m_depthStencilAttachment.depthClearValue);
      } else {
         clearMask |= GL_DEPTH_BUFFER_BIT;
         glClearDepth(m_depthStencilAttachment.depthClearValue);
      }
   }
   if (m_depthStencilAttachment.stencilLoadOp == LoadOp::Clear) {
      if (m_framebuffer && m_framebuffer->HasStencilAttachment()) {
         m_framebuffer->ClearStencil(m_depthStencilAttachment.stencilClearValue);
      } else {
         clearMask |= GL_STENCIL_BUFFER_BIT;
         glClearStencil(m_depthStencilAttachment.stencilClearValue);
      }
   }
   if (clearMask != 0) {
      glClear(clearMask);
   }
}

void GLRenderPass::SetDepthTest(const DepthTest test) noexcept {
   if (test == DepthTest::Disabled) {
      glDisable(GL_DEPTH_TEST);
      return;
   }
   glEnable(GL_DEPTH_TEST);
   constexpr uint32_t depthFuncLookup[] = {GL_NEVER, GL_LESS,     GL_LEQUAL, GL_GREATER, GL_GEQUAL,
                                           GL_EQUAL, GL_NOTEQUAL, GL_ALWAYS, GL_NEVER};
   const auto index = static_cast<size_t>(test);
   if (index > 0 && index < std::size(depthFuncLookup)) {
      glDepthFunc(depthFuncLookup[index]);
   } else {
      glDepthFunc(GL_LESS);
   }
}

void GLRenderPass::SetCullMode(const CullMode mode) noexcept {
   if (mode == CullMode::None) {
      glDisable(GL_CULL_FACE);
      return;
   }
   glEnable(GL_CULL_FACE);
   constexpr uint32_t cullFaceLookup[] = {GL_BACK, GL_FRONT, GL_BACK, GL_FRONT_AND_BACK};
   const auto index = static_cast<size_t>(mode);
   if (index > 0 && index < std::size(cullFaceLookup)) {
      glCullFace(cullFaceLookup[index]);
   } else {
      glCullFace(GL_BACK);
   }
}

void GLRenderPass::SetBlendMode(const BlendMode mode, const RenderState& state) noexcept {
   if (mode == BlendMode::None) {
      glDisable(GL_BLEND);
      return;
   }
   glEnable(GL_BLEND);
   switch (mode) {
      case BlendMode::Alpha:
         glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
         glBlendEquation(GL_FUNC_ADD);
         break;
      case BlendMode::Additive:
         glBlendFunc(GL_SRC_ALPHA, GL_ONE);
         glBlendEquation(GL_FUNC_ADD);
         break;
      case BlendMode::Multiply:
         glBlendFunc(GL_DST_COLOR, GL_ZERO);
         glBlendEquation(GL_FUNC_ADD);
         break;
      case BlendMode::Custom:
         glBlendFunc(state.customSrcFactor, state.customDstFactor);
         glBlendEquation(state.customBlendEquation);
         break;
      default:
         break;
   }
}
