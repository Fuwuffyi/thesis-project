#include "GLRenderPass.hpp"
#include "GLFramebuffer.hpp"
#include "GLShader.hpp"

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
   if (m_isActive) {
      throw std::runtime_error("Render pass is already active");
   }

   // Save current OpenGL state
   glGetIntegerv(GL_VIEWPORT, m_previousState.viewport);
   m_previousState.depthTest = glIsEnabled(GL_DEPTH_TEST);
   glGetBooleanv(GL_DEPTH_WRITEMASK, &m_previousState.depthMask);
   glGetIntegerv(GL_DEPTH_FUNC, reinterpret_cast<GLint*>(&m_previousState.depthFunc));
   m_previousState.cullFace = glIsEnabled(GL_CULL_FACE);
   glGetIntegerv(GL_CULL_FACE_MODE, reinterpret_cast<GLint*>(&m_previousState.cullFaceMode));
   glGetIntegerv(GL_FRONT_FACE, reinterpret_cast<GLint*>(&m_previousState.frontFace));
   m_previousState.blend = glIsEnabled(GL_BLEND);
   glGetIntegerv(GL_BLEND_SRC_RGB, reinterpret_cast<GLint*>(&m_previousState.blendSrc));
   glGetIntegerv(GL_BLEND_DST_RGB, reinterpret_cast<GLint*>(&m_previousState.blendDst));
   glGetIntegerv(GL_BLEND_EQUATION_RGB, reinterpret_cast<GLint*>(&m_previousState.blendEquation));
   glGetFloatv(GL_LINE_WIDTH, &m_previousState.lineWidth);
   glGetFloatv(GL_POINT_SIZE, &m_previousState.pointSize);
   glGetIntegerv(GL_POLYGON_MODE, reinterpret_cast<GLint*>(m_previousState.polygonMode));
   m_previousState.scissorTest = glIsEnabled(GL_SCISSOR_TEST);
   glGetIntegerv(GL_SCISSOR_BOX, m_previousState.scissorBox);

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
   if (!m_isActive) {
      throw std::runtime_error("Render pass is not active");
   }

   // Restore previous OpenGL state
   glViewport(m_previousState.viewport[0], m_previousState.viewport[1], m_previousState.viewport[2],
              m_previousState.viewport[3]);

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
   glScissor(m_previousState.scissorBox[0], m_previousState.scissorBox[1],
             m_previousState.scissorBox[2], m_previousState.scissorBox[3]);

   // Unbind framebuffer
   if (m_framebuffer) {
      m_framebuffer->Unbind();
   }

   // Unbind shader
   if (m_shader) {
      m_shader->Unbind();
   }

   m_isActive = false;
}

void GLRenderPass::SetShader(const GLShader* shader) {
   if (!m_isActive) {
      throw std::runtime_error("Cannot set shader when render pass is not active");
   }
   if (m_shader) {
      m_shader->Unbind();
   }
   m_shader = shader;
   if (m_shader) {
      m_shader->Use();
   }
}

void GLRenderPass::UpdateRenderState(const RenderState& state) {
   m_renderState = state;
   if (m_isActive) {
      ApplyRenderState();
   }
}

uint32_t GLRenderPass::GetPrimitiveType() const { return static_cast<uint32_t>(m_primitiveType); }

uint32_t GLRenderPass::GetViewportWidth() const {
   if (m_renderState.useFramebufferViewport) {
      if (m_framebuffer) {
         return m_framebuffer->GetWidth();
      } else {
         // Get screen width - this would need to be passed in somehow
         GLint viewport[4];
         glGetIntegerv(GL_VIEWPORT, viewport);
         return static_cast<uint32_t>(viewport[2]);
      }
   }
   return m_renderState.viewportWidth;
}

uint32_t GLRenderPass::GetViewportHeight() const {
   if (m_renderState.useFramebufferViewport) {
      if (m_framebuffer) {
         return m_framebuffer->GetHeight();
      } else {
         // Get screen height - this would need to be passed in somehow
         GLint viewport[4];
         glGetIntegerv(GL_VIEWPORT, viewport);
         return static_cast<uint32_t>(viewport[3]);
      }
   }
   return m_renderState.viewportHeight;
}

void GLRenderPass::ApplyRenderState() {
   // Viewport
   if (m_renderState.useFramebufferViewport) {
      if (m_framebuffer) {
         glViewport(0, 0, m_framebuffer->GetWidth(), m_framebuffer->GetHeight());
      }
      // For screen rendering, viewport is set by framebuffer bind/unbind
   } else {
      glViewport(static_cast<GLint>(m_renderState.viewportX),
                 static_cast<GLint>(m_renderState.viewportY),
                 static_cast<GLint>(m_renderState.viewportWidth),
                 static_cast<GLint>(m_renderState.viewportHeight));
   }

   // Depth testing
   SetDepthTest(m_renderState.depthTest);
   glDepthMask(m_renderState.depthWrite ? GL_TRUE : GL_FALSE);

   // Face culling
   SetCullMode(m_renderState.cullMode);
   glFrontFace(m_renderState.frontFaceCCW ? GL_CCW : GL_CW);

   // Blending
   SetBlendMode(m_renderState.blendMode);

   // Line width
   glLineWidth(m_renderState.lineWidth);

   // Point size
   glPointSize(m_renderState.pointSize);

   // Polygon mode
   glPolygonMode(GL_FRONT_AND_BACK, m_renderState.polygonMode);

   // Scissor test
   if (m_renderState.enableScissor) {
      glEnable(GL_SCISSOR_TEST);
      glScissor(static_cast<GLint>(m_renderState.scissorX),
                static_cast<GLint>(m_renderState.scissorY),
                static_cast<GLint>(m_renderState.scissorWidth),
                static_cast<GLint>(m_renderState.scissorHeight));
   } else {
      glDisable(GL_SCISSOR_TEST);
   }
}

void GLRenderPass::ClearAttachments() {
   // Clear color attachments
   for (size_t i = 0; i < m_colorAttachments.size(); ++i) {
      const auto& attachment = m_colorAttachments[i];
      if (attachment.loadOp == LoadOp::Clear) {
         if (m_framebuffer) {
            m_framebuffer->ClearColor(static_cast<uint32_t>(i), attachment.clearValue.r,
                                      attachment.clearValue.g, attachment.clearValue.b,
                                      attachment.clearValue.a);
         } else {
            // Clear screen
            glClearColor(attachment.clearValue.r, attachment.clearValue.g, attachment.clearValue.b,
                         attachment.clearValue.a);
            glClear(GL_COLOR_BUFFER_BIT);
         }
      }
   }

   // Clear depth/stencil
   GLbitfield clearMask = 0;

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

void GLRenderPass::SetDepthTest(DepthTest test) {
   if (test == DepthTest::Disabled) {
      glDisable(GL_DEPTH_TEST);
      return;
   }

   glEnable(GL_DEPTH_TEST);

   GLenum depthFunc;
   switch (test) {
      case DepthTest::Less:
         depthFunc = GL_LESS;
         break;
      case DepthTest::LessEqual:
         depthFunc = GL_LEQUAL;
         break;
      case DepthTest::Greater:
         depthFunc = GL_GREATER;
         break;
      case DepthTest::GreaterEqual:
         depthFunc = GL_GEQUAL;
         break;
      case DepthTest::Equal:
         depthFunc = GL_EQUAL;
         break;
      case DepthTest::NotEqual:
         depthFunc = GL_NOTEQUAL;
         break;
      case DepthTest::Always:
         depthFunc = GL_ALWAYS;
         break;
      case DepthTest::Never:
         depthFunc = GL_NEVER;
         break;
      default:
         depthFunc = GL_LESS;
         break;
   }

   glDepthFunc(depthFunc);
}

void GLRenderPass::SetCullMode(CullMode mode) {
   if (mode == CullMode::None) {
      glDisable(GL_CULL_FACE);
      return;
   }

   glEnable(GL_CULL_FACE);

   GLenum cullFace;
   switch (mode) {
      case CullMode::Front:
         cullFace = GL_FRONT;
         break;
      case CullMode::Back:
         cullFace = GL_BACK;
         break;
      case CullMode::FrontAndBack:
         cullFace = GL_FRONT_AND_BACK;
         break;
      default:
         cullFace = GL_BACK;
         break;
   }

   glCullFace(cullFace);
}

void GLRenderPass::SetBlendMode(BlendMode mode) {
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
         glBlendFunc(m_renderState.customSrcFactor, m_renderState.customDstFactor);
         glBlendEquation(m_renderState.customBlendEquation);
         break;
      default:
         break;
   }
}
