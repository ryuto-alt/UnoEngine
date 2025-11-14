#pragma once

#include <DirectXMath.h>

namespace UnoEngine::Graphics
{
    using namespace DirectX;

    // ========================================
    // Vertex Format Definitions
    // ========================================

    // Basic vertex with position and color (for simple primitives)
    struct VertexPositionColor
    {
        XMFLOAT3 position;  // Local space position
        XMFLOAT4 color;     // RGBA color

        VertexPositionColor() = default;

        VertexPositionColor(const XMFLOAT3& pos, const XMFLOAT4& col)
            : position(pos), color(col)
        {
        }

        VertexPositionColor(float x, float y, float z, float r, float g, float b, float a = 1.0f)
            : position(x, y, z), color(r, g, b, a)
        {
        }
    };

} // namespace UnoEngine::Graphics
