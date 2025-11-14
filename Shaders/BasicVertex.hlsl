// ========================================
// Basic Vertex Shader
// ========================================

// Constant buffer for per-object data
cbuffer ObjectConstants : register(b0)
{
    float4x4 worldMatrix;
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
};

// Vertex input structure
struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
    float3 tangent : TANGENT;
};

// Vertex output structure
struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 worldPosition : WORLD_POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

// ========================================
// Vertex Shader Main Entry Point
// ========================================

VertexOutput main(VertexInput input)
{
    VertexOutput output;

    // Transform position to world space
    float4 worldPos = mul(float4(input.position, 1.0f), worldMatrix);
    output.worldPosition = worldPos.xyz;

    // Transform position to clip space
    float4 viewPos = mul(worldPos, viewMatrix);
    output.position = mul(viewPos, projectionMatrix);

    // Transform normal to world space
    output.normal = normalize(mul(input.normal, (float3x3)worldMatrix));

    // Transform tangent to world space
    output.tangent = normalize(mul(input.tangent, (float3x3)worldMatrix));

    // Calculate bitangent
    output.bitangent = cross(output.normal, output.tangent);

    // Pass through texture coordinates
    output.texCoord = input.texCoord;

    return output;
}
