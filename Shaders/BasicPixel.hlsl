// ========================================
// Basic Pixel Shader (PBR-based)
// ========================================

// Constant buffer for material properties
cbuffer MaterialConstants : register(b1)
{
    float4 albedoColor;
    float metallic;
    float roughness;
    float ambientOcclusion;
    float padding;
};

// Constant buffer for lighting
cbuffer LightConstants : register(b2)
{
    float3 lightDirection;
    float lightIntensity;
    float3 lightColor;
    float padding2;
    float3 cameraPosition;
    float padding3;
};

// Textures
Texture2D albedoTexture : register(t0);
Texture2D normalTexture : register(t1);
Texture2D metallicRoughnessTexture : register(t2);

// Sampler
SamplerState defaultSampler : register(s0);

// Input from vertex shader
struct PixelInput
{
    float4 position : SV_POSITION;
    float3 worldPosition : WORLD_POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

// ========================================
// Constants
// ========================================

static const float PI = 3.14159265359f;

// ========================================
// PBR Functions
// ========================================

// Normal Distribution Function (GGX/Trowbridge-Reitz)
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0f);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;

    return nom / denom;
}

// Geometry Function (Schlick-GGX)
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;

    float nom = NdotV;
    float denom = NdotV * (1.0f - k) + k;

    return nom / denom;
}

// Smith's Method
float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// Fresnel-Schlick Approximation
float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

// ========================================
// Main Pixel Shader Entry Point
// ========================================

float4 main(PixelInput input) : SV_TARGET
{
    // Sample textures
    float4 albedo = albedoTexture.Sample(defaultSampler, input.texCoord) * albedoColor;
    float3 sampledNormal = normalTexture.Sample(defaultSampler, input.texCoord).rgb;
    float2 metallicRoughnessSample = metallicRoughnessTexture.Sample(defaultSampler, input.texCoord).rg;

    // Calculate TBN matrix for normal mapping
    float3 N = normalize(input.normal);
    float3 T = normalize(input.tangent);
    float3 B = normalize(input.bitangent);
    float3x3 TBN = float3x3(T, B, N);

    // Transform normal from tangent space to world space
    float3 normal = sampledNormal * 2.0f - 1.0f;
    normal = normalize(mul(normal, TBN));

    // Calculate view direction
    float3 V = normalize(cameraPosition - input.worldPosition);

    // Calculate light direction (directional light)
    float3 L = normalize(-lightDirection);

    // Calculate half vector
    float3 H = normalize(V + L);

    // Base reflectivity (F0)
    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    F0 = lerp(F0, albedo.rgb, metallic);

    // Calculate radiance
    float3 radiance = lightColor * lightIntensity;

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(normal, H, roughness);
    float G = GeometrySmith(normal, V, L, roughness);
    float3 F = FresnelSchlick(max(dot(H, V), 0.0f), F0);

    float3 numerator = NDF * G * F;
    float denominator = 4.0f * max(dot(normal, V), 0.0f) * max(dot(normal, L), 0.0f) + 0.0001f;
    float3 specular = numerator / denominator;

    // Energy conservation
    float3 kS = F;
    float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;
    kD *= 1.0f - metallic;

    // Calculate lighting
    float NdotL = max(dot(normal, L), 0.0f);
    float3 Lo = (kD * albedo.rgb / PI + specular) * radiance * NdotL;

    // Ambient lighting (simple approximation)
    float3 ambient = float3(0.03f, 0.03f, 0.03f) * albedo.rgb * ambientOcclusion;

    // Final color
    float3 color = ambient + Lo;

    // Tone mapping (Reinhard)
    color = color / (color + float3(1.0f, 1.0f, 1.0f));

    // Gamma correction
    color = pow(color, float3(1.0f / 2.2f, 1.0f / 2.2f, 1.0f / 2.2f));

    return float4(color, albedo.a);
}
