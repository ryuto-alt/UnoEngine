#pragma once

#include "../Core/CoreTypes.h"
#include "../Graphics/VertexTypes.h"
#include "../Graphics/BufferManager.h"
#include <d3d12.h>
#include <vector>
#include <DirectXMath.h>

namespace UnoEngine::Renderer
{
    using namespace UnoEngine::Core;
    using namespace UnoEngine::Graphics;
    using namespace DirectX;

    // ========================================
    // Primitive Renderer
    // ========================================
    // Batch-mode renderer for drawing simple primitives (triangles, quads, lines).
    // Usage: Submit primitives, then call Flush to record draw commands.

    class PrimitiveRenderer
    {
    public:
        // Maximum batch size (vertices)
        static constexpr uint32 MAX_BATCH_VERTICES = 1024;

        explicit PrimitiveRenderer(ID3D12Device* device, BufferManager& bufferManager);
        ~PrimitiveRenderer();

        // Non-copyable, but movable
        PrimitiveRenderer(const PrimitiveRenderer&) = delete;
        auto operator=(const PrimitiveRenderer&) -> PrimitiveRenderer& = delete;
        PrimitiveRenderer(PrimitiveRenderer&&) noexcept = default;
        auto operator=(PrimitiveRenderer&&) noexcept -> PrimitiveRenderer& = default;

        // ========================================
        // Batch Management
        // ========================================

        // Clear all batched primitives
        auto Clear() -> void;

        // Check if batch is full
        [[nodiscard]] auto IsBatchFull(uint32 vertexCount) const noexcept -> bool;

        // Get current vertex count in batch
        [[nodiscard]] auto GetBatchVertexCount() const noexcept -> uint32;

        // ========================================
        // Triangle Submission
        // ========================================

        // Submit a triangle (3 vertices)
        auto SubmitTriangle(
            const VertexPositionColor& v0,
            const VertexPositionColor& v1,
            const VertexPositionColor& v2
        ) -> void;

        // ========================================
        // Quad Submission
        // ========================================

        // Submit a quad (4 vertices, rendered as 2 triangles)
        // Vertex order: v0 (top-left), v1 (top-right), v2 (bottom-right), v3 (bottom-left)
        auto SubmitQuad(
            const VertexPositionColor& v0,
            const VertexPositionColor& v1,
            const VertexPositionColor& v2,
            const VertexPositionColor& v3
        ) -> void;

        // ========================================
        // Debug Drawing
        // ========================================

        // Submit a line (2 vertices)
        auto SubmitLine(
            const VertexPositionColor& v0,
            const VertexPositionColor& v1
        ) -> void;

        // Submit a wireframe AABB (axis-aligned bounding box)
        auto SubmitWireframeBox(
            const XMFLOAT3& min,
            const XMFLOAT3& max,
            const XMFLOAT4& color
        ) -> void;

        // ========================================
        // Rendering
        // ========================================

        // Flush batched primitives to command list
        // This uploads vertex data and records draw commands
        auto Flush(
            ID3D12GraphicsCommandList* commandList,
            D3D_PRIMITIVE_TOPOLOGY topology
        ) -> void;

    private:
        // ========================================
        // Internal Helpers
        // ========================================

        auto EnsureCapacity(uint32 vertexCount) -> void;

        // ========================================
        // Member Variables
        // ========================================

        ID3D12Device* m_device;
        BufferManager& m_bufferManager;

        // Batched vertices (CPU-side)
        std::vector<VertexPositionColor> m_vertices;

        // Vertex buffer (GPU-side, upload heap)
        ComPtr<ID3D12Resource> m_vertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView{};
        void* m_mappedData{ nullptr };
    };

} // namespace UnoEngine::Renderer
