#pragma once

#include <glad/gl.h>
#include <vector>
#include <memory>
#include <cstdint>
#include <glm/glm.hpp>

class GLFramebuffer;
class GLShader;

class GLRenderPass {
  public:
   enum class LoadOp { Load, Clear, DontCare };

   enum class StoreOp { Store, DontCare };

   enum class DepthTest {
      Disabled,
      Less,
      LessEqual,
      Greater,
      GreaterEqual,
      Equal,
      NotEqual,
      Always,
      Never
   };

   enum class CullMode { None, Front, Back, FrontAndBack };

   enum class BlendMode { None, Alpha, Additive, Multiply, Custom };

   enum class PrimitiveType {
      Points = GL_POINTS,
      Lines = GL_LINES,
      LineStrip = GL_LINE_STRIP,
      LineLoop = GL_LINE_LOOP,
      Triangles = GL_TRIANGLES,
      TriangleStrip = GL_TRIANGLE_STRIP,
      TriangleFan = GL_TRIANGLE_FAN
   };

   struct ColorAttachmentDesc {
      LoadOp loadOp = LoadOp::Clear;
      StoreOp storeOp = StoreOp::Store;
      glm::vec4 clearValue = {0.0f, 0.0f, 0.0f, 0.0f};
   };

   struct DepthStencilAttachmentDesc {
      LoadOp depthLoadOp = LoadOp::Clear;
      StoreOp depthStoreOp = StoreOp::Store;
      LoadOp stencilLoadOp = LoadOp::DontCare;
      StoreOp stencilStoreOp = StoreOp::DontCare;
      float depthClearValue = 1.0f;
      int32_t stencilClearValue = 0;
   };

   struct RenderState {
      // Depth testing
      DepthTest depthTest = DepthTest::Less;
      bool depthWrite = true;
      // Face culling
      CullMode cullMode = CullMode::Back;
      bool frontFaceCCW = true;
      // Blending
      BlendMode blendMode = BlendMode::None;
      GLenum customSrcFactor = GL_SRC_ALPHA;
      GLenum customDstFactor = GL_ONE_MINUS_SRC_ALPHA;
      GLenum customBlendEquation = GL_FUNC_ADD;
      // Primitive type
      PrimitiveType primitiveType = PrimitiveType::Triangles;
      // Viewport
      bool useFramebufferViewport = true;
      uint32_t viewportX = 0;
      uint32_t viewportY = 0;
      uint32_t viewportWidth = 0;
      uint32_t viewportHeight = 0;
      // Line width (for line primitives)
      float lineWidth = 1.0f;
      // Point size (for point primitives)
      float pointSize = 1.0f;
      // Polygon mode
      GLenum polygonMode = GL_FILL;
      // Scissor test
      bool enableScissor = false;
      uint32_t scissorX = 0;
      uint32_t scissorY = 0;
      uint32_t scissorWidth = 0;
      uint32_t scissorHeight = 0;
   };

   struct CreateInfo {
      GLFramebuffer* framebuffer = nullptr;
      std::vector<ColorAttachmentDesc> colorAttachments;
      DepthStencilAttachmentDesc depthStencilAttachment;
      RenderState renderState;
      GLShader* shader = nullptr;
   };

   GLRenderPass(const CreateInfo& info);
   ~GLRenderPass() = default;

   GLRenderPass(const GLRenderPass&) = delete;
   GLRenderPass& operator=(const GLRenderPass&) = delete;
   GLRenderPass(GLRenderPass&&) = default;
   GLRenderPass& operator=(GLRenderPass&&) = default;

   // Render pass control
   void Begin();
   void End();

   // State management during render pass
   void SetShader(const GLShader* shader);
   void UpdateRenderState(const RenderState& state);

   // Getters
   GLFramebuffer* GetFramebuffer() const { return m_framebuffer; }
   const RenderState& GetRenderState() const { return m_renderState; }
   const GLShader* GetShader() const { return m_shader; }
   bool IsActive() const { return m_isActive; }

   // Utility methods
   uint32_t GetViewportWidth() const;
   uint32_t GetViewportHeight() const;

  private:
   void ApplyRenderState();
   void ClearAttachments();
   void SetDepthTest(const DepthTest test);
   void SetCullMode(const CullMode mode);
   void SetBlendMode(const BlendMode mode);

  private:
   GLFramebuffer* m_framebuffer;
   std::vector<ColorAttachmentDesc> m_colorAttachments;
   DepthStencilAttachmentDesc m_depthStencilAttachment;
   RenderState m_renderState;
   const GLShader* m_shader;

   bool m_isActive = false;

   struct PreviousState {
      GLint viewport[4];
      GLboolean depthTest;
      GLboolean depthMask;
      GLenum depthFunc;
      GLboolean cullFace;
      GLenum cullFaceMode;
      GLenum frontFace;
      GLboolean blend;
      GLenum blendSrc, blendDst;
      GLenum blendEquation;
      GLfloat lineWidth;
      GLfloat pointSize;
      GLenum polygonMode[2];
      GLboolean scissorTest;
      GLint scissorBox[4];
   } m_previousState;
};
