// ========================================
// Common Shader Definitions
// ========================================
// Shared structures and constants used across multiple shaders.

// ========================================
// Constant Buffers
// ========================================

// Scene-level constants (updated per frame)
cbuffer SceneConstants : register(b0)
{
    float4x4 g_view;           // View matrix
    float4x4 g_projection;     // Projection matrix
    float3   g_cameraPosition; // Camera world position (for lighting)
    float    g_padding0;       // Padding for 16-byte alignment
};

// Object-level constants (updated per object)
cbuffer ObjectConstants : register(b1)
{
    float4x4 g_world;              // World matrix
    float4x4 g_worldInvTranspose;  // Inverse transpose of world matrix (for normal transformation)
};

// ========================================
// Vertex Input/Output Structures
// ========================================

// Vertex input from application
struct VertexInput
{
    float3 position : POSITION;  // Local space position
    float4 color    : COLOR;     // Vertex color
};

// Vertex shader output / Pixel shader input
struct VertexOutput
{
    float4 position : SV_POSITION;  // Clip space position
    float4 color    : COLOR;        // Interpolated vertex color
    float2 texCoord : TEXCOORD0;    // Texture coordinates (for future use)
};
