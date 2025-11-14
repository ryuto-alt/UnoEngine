#pragma once

#include "CoreTypes.h"
#include <DirectXMath.h>

namespace UnoEngine::Core::Components
{
    using namespace DirectX;

    // ========================================
    // Transform Component
    // ========================================

    struct TransformComponent
    {
        XMFLOAT3 position{ 0.0f, 0.0f, 0.0f };
        XMFLOAT4 rotation{ 0.0f, 0.0f, 0.0f, 1.0f }; // Quaternion (x, y, z, w)
        XMFLOAT3 scale{ 1.0f, 1.0f, 1.0f };

        // Get world matrix
        [[nodiscard]] auto GetWorldMatrix() const -> XMMATRIX
        {
            XMMATRIX scaleMatrix = XMMatrixScaling(scale.x, scale.y, scale.z);
            XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(XMLoadFloat4(&rotation));
            XMMATRIX translationMatrix = XMMatrixTranslation(position.x, position.y, position.z);

            return scaleMatrix * rotationMatrix * translationMatrix;
        }

        // Set rotation from Euler angles (in radians)
        auto SetRotationFromEuler(float pitch, float yaw, float roll) -> void
        {
            XMVECTOR quaternion = XMQuaternionRotationRollPitchYaw(pitch, yaw, roll);
            XMStoreFloat4(&rotation, quaternion);
        }
    };

    // ========================================
    // Mesh Component
    // ========================================

    struct MeshComponent
    {
        uint32 vertexBufferId{ 0 };
        uint32 indexBufferId{ 0 };
        uint32 indexCount{ 0 };
        uint32 vertexCount{ 0 };

        [[nodiscard]] constexpr auto IsValid() const noexcept -> bool
        {
            return vertexBufferId != 0 && indexBufferId != 0 && indexCount > 0;
        }
    };

    // ========================================
    // Material Component
    // ========================================

    struct MaterialComponent
    {
        XMFLOAT4 albedoColor{ 1.0f, 1.0f, 1.0f, 1.0f };
        float metallic{ 0.0f };
        float roughness{ 0.5f };
        float ambientOcclusion{ 1.0f };

        uint32 albedoTextureId{ 0 };
        uint32 normalTextureId{ 0 };
        uint32 metallicRoughnessTextureId{ 0 };

        [[nodiscard]] constexpr auto HasAlbedoTexture() const noexcept -> bool
        {
            return albedoTextureId != 0;
        }

        [[nodiscard]] constexpr auto HasNormalMap() const noexcept -> bool
        {
            return normalTextureId != 0;
        }

        [[nodiscard]] constexpr auto HasMetallicRoughnessMap() const noexcept -> bool
        {
            return metallicRoughnessTextureId != 0;
        }
    };

    // ========================================
    // Camera Component
    // ========================================

    struct CameraComponent
    {
        float fieldOfView{ 60.0f };  // In degrees
        float aspectRatio{ 16.0f / 9.0f };
        float nearPlane{ 0.1f };
        float farPlane{ 1000.0f };

        [[nodiscard]] auto GetProjectionMatrix() const -> XMMATRIX
        {
            float fovRadians = XMConvertToRadians(fieldOfView);
            return XMMatrixPerspectiveFovLH(fovRadians, aspectRatio, nearPlane, farPlane);
        }

        [[nodiscard]] auto GetOrthographicMatrix(float width, float height) const -> XMMATRIX
        {
            return XMMatrixOrthographicLH(width, height, nearPlane, farPlane);
        }
    };

    // ========================================
    // Light Component
    // ========================================

    enum class LightType : uint8
    {
        Directional,
        Point,
        Spot
    };

    struct LightComponent
    {
        LightType type{ LightType::Directional };
        XMFLOAT3 color{ 1.0f, 1.0f, 1.0f };
        float intensity{ 1.0f };

        // For point and spot lights
        float range{ 10.0f };
        float attenuation{ 1.0f };

        // For spot lights
        float innerConeAngle{ 30.0f }; // In degrees
        float outerConeAngle{ 45.0f }; // In degrees

        [[nodiscard]] constexpr auto IsDirectional() const noexcept -> bool
        {
            return type == LightType::Directional;
        }

        [[nodiscard]] constexpr auto IsPoint() const noexcept -> bool
        {
            return type == LightType::Point;
        }

        [[nodiscard]] constexpr auto IsSpot() const noexcept -> bool
        {
            return type == LightType::Spot;
        }
    };

    // ========================================
    // Tag Component
    // ========================================

    struct TagComponent
    {
        String name{ "Entity" };
        String tag{ "Default" };
        bool isActive{ true };
    };

} // namespace UnoEngine::Core::Components
