// ========================================
// Simple Vertex Shader
// ========================================
// Basic vertex transformation with color pass-through.

#include "../Common.hlsl"

// ========================================
// Vertex Shader Entry Point
// ========================================

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;

    // Transform vertex position: Local -> World -> View -> Projection
    float4 worldPos = mul(float4(input.position, 1.0f), g_world);
    float4 viewPos  = mul(worldPos, g_view);
    output.position = mul(viewPos, g_projection);

    // Pass through vertex color
    output.color = input.color;

    // Generate simple UV coordinates (for future texture support)
    // Map position.xy to [0,1] range
    output.texCoord = input.position.xy * 0.5f + 0.5f;

    return output;
}
