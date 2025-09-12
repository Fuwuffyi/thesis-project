#pragma once

#include <glad/gl.h>
#include <vector>
#include <cstdint>
#include <array>
#include <glm/glm.hpp>

class GLFramebuffer;
class GLShader;

class GLRenderPass final {
  public:
   enum class LoadOp : uint8_t { Load, Clear, DontCare };
   enum class StoreOp : uint8_t { Store, DontCare };

   enum class DepthTest : uint8_t {
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

   enum class CullMode : uint8_t { None, Front, Back, FrontAndBack };
   enum class BlendMode : uint8_t { None, Alpha, Additive, Multiply, Custom };

   enum class PrimitiveType : uint32_t {
      Points = GL_POINTS,
      Lines = GL_LINES,
      LineStrip = GL_LINE_STRIP,
      LineLoop = GL_LINE_LOOP,
      Triangles = GL_TRIANGLES,
      TriangleStrip = GL_TRIANGLE_STRIP,
      TriangleFan = GL_TRIANGLE_FAN
   };

   struct ColorAttachmentDesc final {
      LoadOp loadOp{LoadOp::Clear};
      StoreOp storeOp{StoreOp::Store};
      glm::vec4 clearValue{0.0f, 0.0f, 0.0f, 0.0f};
   };

   struct DepthStencilAttachmentDesc final {
      LoadOp depthLoadOp{LoadOp::Clear};
      StoreOp depthStoreOp{StoreOp::Store};
      LoadOp stencilLoadOp{LoadOp::DontCare};
      StoreOp stencilStoreOp{StoreOp::DontCare};
      float depthClearValue{1.0f};
      int32_t stencilClearValue{0};
   };

   struct RenderState final {
      // Depth testing
      DepthTest depthTest{DepthTest::Less};
      bool depthWrite{true};
      // Face culling
      CullMode cullMode{CullMode::Back};
      bool frontFaceCCW{true};
      // Blending
      BlendMode blendMode{BlendMode::None};
      uint32_t customSrcFactor{GL_SRC_ALPHA};
      uint32_t customDstFactor{GL_ONE_MINUS_SRC_ALPHA};
      uint32_t customBlendEquation{GL_FUNC_ADD};
      // Primitive type
      PrimitiveType primitiveType{PrimitiveType::Triangles};
      // Viewport
      bool useFramebufferViewport{true};
      uint32_t viewportX{0};
      uint32_t viewportY{0};
      uint32_t viewportWidth{0};
      uint32_t viewportHeight{0};
      // Rendering parameters
      float lineWidth{1.0f};
      float pointSize{1.0f};
      uint32_t polygonMode{GL_FILL};
      // Scissor test
      bool enableScissor{false};
      uint32_t scissorX{0};
      uint32_t scissorY{0};
      uint32_t scissorWidth{0};
      uint32_t scissorHeight{0};
   };

   struct CreateInfo final {
      const GLFramebuffer* framebuffer{nullptr};
      std::vector<ColorAttachmentDesc> colorAttachments;
      DepthStencilAttachmentDesc depthStencilAttachment;
      RenderState renderState;
      const GLShader* shader{nullptr};
   };

   explicit GLRenderPass(const CreateInfo& info);
   ~GLRenderPass() noexcept = default;

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
   [[nodiscard]] constexpr const GLFramebuffer* GetFramebuffer() const noexcept {
      return m_framebuffer;
   }
   [[nodiscard]] constexpr const RenderState& GetRenderState() const noexcept {
      return m_renderState;
   }
   [[nodiscard]] constexpr const GLShader* GetShader() const noexcept { return m_shader; }
   [[nodiscard]] constexpr uint32_t GetPrimitiveType() const noexcept {
      return static_cast<uint32_t>(m_primitiveType);
   }
   [[nodiscard]] constexpr bool IsActive() const noexcept { return m_isActive; }

   // Utility methods
   [[nodiscard]] uint32_t GetViewportWidth() const noexcept;
   [[nodiscard]] uint32_t GetViewportHeight() const noexcept;

  private:
   void ApplyRenderState() const;
   void ClearAttachments() const;

   static void SetDepthTest(const DepthTest test) noexcept;
   static void SetCullMode(const CullMode mode) noexcept;
   static void SetBlendMode(const BlendMode mode, const RenderState& state) noexcept;

   struct PreviousState final {
      std::array<int32_t, 4> viewport;
      bool depthTest;
      bool depthMask;
      uint32_t depthFunc;
      bool cullFace;
      uint32_t cullFaceMode;
      uint32_t frontFace;
      bool blend;
      uint32_t blendSrc, blendDst;
      uint32_t blendEquation;
      float lineWidth;
      float pointSize;
      std::array<uint32_t, 2> polygonMode;
      bool scissorTest;
      std::array<int32_t, 4> scissorBox;
   };

   const GLFramebuffer* m_framebuffer;
   std::vector<ColorAttachmentDesc> m_colorAttachments;
   DepthStencilAttachmentDesc m_depthStencilAttachment;
   RenderState m_renderState;
   PrimitiveType m_primitiveType;
   const GLShader* m_shader;

   bool m_isActive{false};
   PreviousState m_previousState{};
};
