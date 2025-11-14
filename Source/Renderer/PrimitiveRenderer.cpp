#include "../../Include/Renderer/PrimitiveRenderer.h"
#include "../../Include/Core/Logger.h"
#include <d3dx12.h>

namespace UnoEngine::Renderer
{
    // ========================================
    // Constructor & Destructor
    // ========================================

    PrimitiveRenderer::PrimitiveRenderer(ID3D12Device* device, BufferManager& bufferManager)
        : m_device(device)
        , m_bufferManager(bufferManager)
    {
        // Reserve space for batch vertices
        m_vertices.reserve(MAX_BATCH_VERTICES);

        Logger::Info("PrimitiveRenderer initialized (max batch: {} vertices)", MAX_BATCH_VERTICES);
    }

    PrimitiveRenderer::~PrimitiveRenderer()
    {
        // Unmap vertex buffer if it was mapped
        if (m_vertexBuffer && m_mappedData)
        {
            m_vertexBuffer->Unmap(0, nullptr);
            m_mappedData = nullptr;
        }

        // ComPtr automatically releases m_vertexBuffer
        // No explicit Release() needed

        Logger::Trace("PrimitiveRenderer destroyed");
    }

    // ========================================
    // Batch Management
    // ========================================

    auto PrimitiveRenderer::Clear() -> void
    {
        m_vertices.clear();
    }

    auto PrimitiveRenderer::IsBatchFull(uint32 vertexCount) const noexcept -> bool
    {
        return (m_vertices.size() + vertexCount) > MAX_BATCH_VERTICES;
    }

    auto PrimitiveRenderer::GetBatchVertexCount() const noexcept -> uint32
    {
        return static_cast<uint32>(m_vertices.size());
    }

    // ========================================
    // Triangle Submission
    // ========================================

    auto PrimitiveRenderer::SubmitTriangle(
        const VertexPositionColor& v0,
        const VertexPositionColor& v1,
        const VertexPositionColor& v2
    ) -> void
    {
        EnsureCapacity(3);

        m_vertices.push_back(v0);
        m_vertices.push_back(v1);
        m_vertices.push_back(v2);
    }

    // ========================================
    // Quad Submission
    // ========================================

    auto PrimitiveRenderer::SubmitQuad(
        const VertexPositionColor& v0,
        const VertexPositionColor& v1,
        const VertexPositionColor& v2,
        const VertexPositionColor& v3
    ) -> void
    {
        EnsureCapacity(6);

        // First triangle: v0, v1, v2
        m_vertices.push_back(v0);
        m_vertices.push_back(v1);
        m_vertices.push_back(v2);

        // Second triangle: v0, v2, v3
        m_vertices.push_back(v0);
        m_vertices.push_back(v2);
        m_vertices.push_back(v3);
    }

    // ========================================
    // Debug Drawing
    // ========================================

    auto PrimitiveRenderer::SubmitLine(
        const VertexPositionColor& v0,
        const VertexPositionColor& v1
    ) -> void
    {
        EnsureCapacity(2);

        m_vertices.push_back(v0);
        m_vertices.push_back(v1);
    }

    auto PrimitiveRenderer::SubmitWireframeBox(
        const XMFLOAT3& min,
        const XMFLOAT3& max,
        const XMFLOAT4& color
    ) -> void
    {
        // AABB has 12 edges (24 vertices for line list)
        EnsureCapacity(24);

        // Define 8 corners
        XMFLOAT3 corners[8] = {
            { min.x, min.y, min.z },  // 0: left-bottom-back
            { max.x, min.y, min.z },  // 1: right-bottom-back
            { max.x, max.y, min.z },  // 2: right-top-back
            { min.x, max.y, min.z },  // 3: left-top-back
            { min.x, min.y, max.z },  // 4: left-bottom-front
            { max.x, min.y, max.z },  // 5: right-bottom-front
            { max.x, max.y, max.z },  // 6: right-top-front
            { min.x, max.y, max.z },  // 7: left-top-front
        };

        // Bottom face (4 edges)
        SubmitLine(VertexPositionColor(corners[0], color), VertexPositionColor(corners[1], color));
        SubmitLine(VertexPositionColor(corners[1], color), VertexPositionColor(corners[5], color));
        SubmitLine(VertexPositionColor(corners[5], color), VertexPositionColor(corners[4], color));
        SubmitLine(VertexPositionColor(corners[4], color), VertexPositionColor(corners[0], color));

        // Top face (4 edges)
        SubmitLine(VertexPositionColor(corners[3], color), VertexPositionColor(corners[2], color));
        SubmitLine(VertexPositionColor(corners[2], color), VertexPositionColor(corners[6], color));
        SubmitLine(VertexPositionColor(corners[6], color), VertexPositionColor(corners[7], color));
        SubmitLine(VertexPositionColor(corners[7], color), VertexPositionColor(corners[3], color));

        // Vertical edges (4 edges)
        SubmitLine(VertexPositionColor(corners[0], color), VertexPositionColor(corners[3], color));
        SubmitLine(VertexPositionColor(corners[1], color), VertexPositionColor(corners[2], color));
        SubmitLine(VertexPositionColor(corners[5], color), VertexPositionColor(corners[6], color));
        SubmitLine(VertexPositionColor(corners[4], color), VertexPositionColor(corners[7], color));
    }

    // ========================================
    // Rendering
    // ========================================

    auto PrimitiveRenderer::Flush(
        ID3D12GraphicsCommandList* commandList,
        D3D_PRIMITIVE_TOPOLOGY topology
    ) -> void
    {
        if (m_vertices.empty())
        {
            return;  // Nothing to draw
        }

        uint32 vertexCount = static_cast<uint32>(m_vertices.size());
        uint32 bufferSize = vertexCount * sizeof(VertexPositionColor);

        // Create or resize vertex buffer if needed
        if (!m_vertexBuffer || m_vertexBuffer->GetDesc().Width < bufferSize)
        {
            // Unmap old buffer if exists
            if (m_mappedData)
            {
                m_vertexBuffer->Unmap(0, nullptr);
                m_mappedData = nullptr;
            }

            // Create new vertex buffer (upload heap) manually using D3D12 API
            CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
            CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

            HRESULT hr = m_device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &bufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_vertexBuffer)
            );

            if (FAILED(hr))
            {
                Logger::Error("Failed to create vertex buffer: {:#x}", static_cast<uint32>(hr));
                return;
            }

            // Map buffer for CPU writes
            CD3DX12_RANGE readRange(0, 0);  // We don't intend to read from this resource
            hr = m_vertexBuffer->Map(0, &readRange, &m_mappedData);
            if (FAILED(hr))
            {
                Logger::Error("Failed to map vertex buffer: {:#x}", static_cast<uint32>(hr));
                return;
            }

            // Setup vertex buffer view
            m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
            m_vertexBufferView.SizeInBytes = bufferSize;
            m_vertexBufferView.StrideInBytes = sizeof(VertexPositionColor);

            Logger::Trace("Created vertex buffer: {} bytes", bufferSize);
        }

        // Copy vertex data to GPU
        memcpy(m_mappedData, m_vertices.data(), bufferSize);

        // Set topology and vertex buffer
        commandList->IASetPrimitiveTopology(topology);
        commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);

        // Draw
        commandList->DrawInstanced(vertexCount, 1, 0, 0);

        Logger::Trace("PrimitiveRenderer flushed {} vertices (topology: {})", vertexCount, static_cast<int>(topology));

        // Clear batch for next frame
        m_vertices.clear();
    }

    // ========================================
    // Internal Helpers
    // ========================================

    auto PrimitiveRenderer::EnsureCapacity(uint32 vertexCount) -> void
    {
        if (IsBatchFull(vertexCount))
        {
            Logger::Warn("Primitive batch full! Consider flushing before submitting more primitives.");
            // Note: In a production engine, you might want to auto-flush here
        }
    }

} // namespace UnoEngine::Renderer
