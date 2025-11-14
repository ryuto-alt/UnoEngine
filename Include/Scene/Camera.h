#pragma once

#include "../Core/CoreTypes.h"
#include <DirectXMath.h>

namespace UnoEngine::Scene
{
    using namespace UnoEngine::Core;
    using namespace DirectX;

    // ========================================
    // Camera Class
    // ========================================
    // Manages view and projection matrices for rendering.
    // Uses DirectX left-handed coordinate system (+X right, +Y up, +Z forward).

    class Camera
    {
    public:
        Camera();
        ~Camera() = default;

        // Non-copyable, but movable
        Camera(const Camera&) = delete;
        auto operator=(const Camera&) -> Camera& = delete;
        Camera(Camera&&) noexcept = default;
        auto operator=(Camera&&) noexcept -> Camera& = default;

        // ========================================
        // Position and Rotation
        // ========================================

        // Set camera position in world space
        auto SetPosition(const XMFLOAT3& position) -> void;
        auto SetPosition(float x, float y, float z) -> void;

        // Set camera rotation using Euler angles (pitch, yaw, roll in radians)
        auto SetRotation(const XMFLOAT3& rotation) -> void;
        auto SetRotation(float pitch, float yaw, float roll) -> void;

        // Set camera rotation using quaternion
        auto SetRotationQuaternion(const XMFLOAT4& quaternion) -> void;

        // Get camera position
        [[nodiscard]] auto GetPosition() const noexcept -> XMFLOAT3;

        // Get camera rotation as quaternion
        [[nodiscard]] auto GetRotationQuaternion() const noexcept -> XMFLOAT4;

        // Get camera forward/right/up vectors
        [[nodiscard]] auto GetForward() const -> XMFLOAT3;
        [[nodiscard]] auto GetRight() const -> XMFLOAT3;
        [[nodiscard]] auto GetUp() const -> XMFLOAT3;

        // ========================================
        // LookAt
        // ========================================

        // Set camera to look at a target point
        auto SetLookAt(const XMFLOAT3& eye, const XMFLOAT3& target, const XMFLOAT3& up) -> void;

        // ========================================
        // Movement and Rotation
        // ========================================

        // Move camera relative to current orientation
        auto MoveForward(float distance) -> void;
        auto MoveRight(float distance) -> void;
        auto MoveUp(float distance) -> void;

        // Rotate camera (yaw around Y-axis, pitch around X-axis, roll around Z-axis)
        auto RotateYaw(float radians) -> void;
        auto RotatePitch(float radians) -> void;
        auto RotateRoll(float radians) -> void;

        // ========================================
        // Orbit
        // ========================================

        // Orbit around a target point
        auto OrbitAround(const XMFLOAT3& target, float distance, float yaw, float pitch) -> void;

        // ========================================
        // Projection
        // ========================================

        // Set perspective projection using FOV
        auto SetPerspectiveFOV(float fovY, float aspectRatio, float nearZ, float farZ) -> void;

        // Set perspective projection using frustum bounds
        auto SetPerspectiveFrustum(float left, float right, float bottom, float top, float nearZ, float farZ) -> void;

        // Set orthographic projection
        auto SetOrthographic(float left, float right, float bottom, float top, float nearZ, float farZ) -> void;

        // ========================================
        // Matrix Retrieval
        // ========================================

        // Get view matrix (cached with dirty flag)
        [[nodiscard]] auto GetViewMatrix() -> XMMATRIX;

        // Get projection matrix (cached with dirty flag)
        [[nodiscard]] auto GetProjectionMatrix() -> XMMATRIX;

        // Get view-projection matrix
        [[nodiscard]] auto GetViewProjectionMatrix() -> XMMATRIX;

    private:
        // ========================================
        // Internal Update Methods
        // ========================================

        auto UpdateViewMatrix() -> void;
        auto UpdateProjectionMatrix() -> void;

        // ========================================
        // Member Variables
        // ========================================

        // Transform
        XMFLOAT3 m_position{ 0.0f, 0.0f, -5.0f };  // Camera position in world space
        XMFLOAT4 m_rotation{ 0.0f, 0.0f, 0.0f, 1.0f };  // Rotation as quaternion (x, y, z, w)

        // Projection parameters
        float m_left{ -1.0f };
        float m_right{ 1.0f };
        float m_bottom{ -1.0f };
        float m_top{ 1.0f };
        float m_nearZ{ 0.1f };
        float m_farZ{ 1000.0f };
        bool m_isPerspective{ true };

        // Cached matrices (dirty flag pattern)
        XMFLOAT4X4 m_viewMatrix{};
        XMFLOAT4X4 m_projectionMatrix{};
        bool m_viewDirty{ true };
        bool m_projectionDirty{ true };
    };

} // namespace UnoEngine::Scene
