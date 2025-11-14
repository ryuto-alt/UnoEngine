// ========================================
// Simple Pixel Shader
// ========================================
// Basic pixel shading with gamma correction and brightness adjustment.

#include "../Common.hlsl"

// ========================================
// Shader Parameters
// ========================================

static const float GAMMA = 2.2f;       // Standard sRGB gamma
static const float BRIGHTNESS = 1.0f;  // Brightness multiplier (adjustable for debugging)

// ========================================
// Pixel Shader Entry Point
// ========================================

float4 PSMain(VertexOutput input) : SV_TARGET
{
    // Start with interpolated vertex color
    float4 color = input.color;

    // Apply brightness adjustment
    color.rgb *= BRIGHTNESS;

    // Apply gamma correction (Linear -> sRGB)
    // Note: In a production engine, this should be handled by the swap chain format (SRGB)
    // but we do it manually here for educational purposes
    color.rgb = pow(abs(color.rgb), 1.0f / GAMMA);

    // Ensure alpha is preserved
    return color;
}
