#include "../../Include/Scene/Camera.h"
#include <cmath>

namespace UnoEngine::Scene
{
    using namespace DirectX;

    // ========================================
    // Constructor
    // ========================================

    Camera::Camera()
    {
        // Initialize with identity matrices
        XMStoreFloat4x4(&m_viewMatrix, XMMatrixIdentity());
        XMStoreFloat4x4(&m_projectionMatrix, XMMatrixIdentity());

        // Set default perspective projection (60° FOV, 16:9 aspect)
        SetPerspectiveFOV(XMConvertToRadians(60.0f), 16.0f / 9.0f, 0.1f, 1000.0f);
    }

    // ========================================
    // Position and Rotation
    // ========================================

    auto Camera::SetPosition(const XMFLOAT3& position) -> void
    {
        m_position = position;
        m_viewDirty = true;
    }

    auto Camera::SetPosition(float x, float y, float z) -> void
    {
        SetPosition(XMFLOAT3(x, y, z));
    }

    auto Camera::SetRotation(const XMFLOAT3& rotation) -> void
    {
        // Convert Euler angles to quaternion
        XMVECTOR quat = XMQuaternionRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
        XMStoreFloat4(&m_rotation, quat);
        m_viewDirty = true;
    }

    auto Camera::SetRotation(float pitch, float yaw, float roll) -> void
    {
        SetRotation(XMFLOAT3(pitch, yaw, roll));
    }

    auto Camera::SetRotationQuaternion(const XMFLOAT4& quaternion) -> void
    {
        m_rotation = quaternion;
        m_viewDirty = true;
    }

    auto Camera::GetPosition() const noexcept -> XMFLOAT3
    {
        return m_position;
    }

    auto Camera::GetRotationQuaternion() const noexcept -> XMFLOAT4
    {
        return m_rotation;
    }

    auto Camera::GetForward() const -> XMFLOAT3
    {
        XMVECTOR quat = XMLoadFloat4(&m_rotation);
        XMVECTOR forward = XMVector3Rotate(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), quat);
        XMFLOAT3 result;
        XMStoreFloat3(&result, forward);
        return result;
    }

    auto Camera::GetRight() const -> XMFLOAT3
    {
        XMVECTOR quat = XMLoadFloat4(&m_rotation);
        XMVECTOR right = XMVector3Rotate(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), quat);
        XMFLOAT3 result;
        XMStoreFloat3(&result, right);
        return result;
    }

    auto Camera::GetUp() const -> XMFLOAT3
    {
        XMVECTOR quat = XMLoadFloat4(&m_rotation);
        XMVECTOR up = XMVector3Rotate(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), quat);
        XMFLOAT3 result;
        XMStoreFloat3(&result, up);
        return result;
    }

    // ========================================
    // LookAt
    // ========================================

    auto Camera::SetLookAt(const XMFLOAT3& eye, const XMFLOAT3& target, const XMFLOAT3& up) -> void
    {
        // Set position
        m_position = eye;

        // Calculate view matrix using LookAtLH
        XMVECTOR eyeVec = XMLoadFloat3(&eye);
        XMVECTOR targetVec = XMLoadFloat3(&target);
        XMVECTOR upVec = XMLoadFloat3(&up);
        XMMATRIX viewMatrix = XMMatrixLookAtLH(eyeVec, targetVec, upVec);

        // Extract rotation from view matrix
        // View matrix is the inverse of camera's world matrix
        XMMATRIX worldMatrix = XMMatrixInverse(nullptr, viewMatrix);
        XMVECTOR scale, rotation, translation;
        XMMatrixDecompose(&scale, &rotation, &translation, worldMatrix);
        XMStoreFloat4(&m_rotation, rotation);

        m_viewDirty = true;
    }

    // ========================================
    // Movement and Rotation
    // ========================================

    auto Camera::MoveForward(float distance) -> void
    {
        XMFLOAT3 forward = GetForward();
        m_position.x += forward.x * distance;
        m_position.y += forward.y * distance;
        m_position.z += forward.z * distance;
        m_viewDirty = true;
    }

    auto Camera::MoveRight(float distance) -> void
    {
        XMFLOAT3 right = GetRight();
        m_position.x += right.x * distance;
        m_position.y += right.y * distance;
        m_position.z += right.z * distance;
        m_viewDirty = true;
    }

    auto Camera::MoveUp(float distance) -> void
    {
        XMFLOAT3 up = GetUp();
        m_position.x += up.x * distance;
        m_position.y += up.y * distance;
        m_position.z += up.z * distance;
        m_viewDirty = true;
    }

    auto Camera::RotateYaw(float radians) -> void
    {
        XMVECTOR quat = XMLoadFloat4(&m_rotation);
        XMVECTOR yawQuat = XMQuaternionRotationAxis(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), radians);
        quat = XMQuaternionMultiply(quat, yawQuat);
        XMStoreFloat4(&m_rotation, quat);
        m_viewDirty = true;
    }

    auto Camera::RotatePitch(float radians) -> void
    {
        XMVECTOR quat = XMLoadFloat4(&m_rotation);
        XMVECTOR pitchQuat = XMQuaternionRotationAxis(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), radians);
        quat = XMQuaternionMultiply(quat, pitchQuat);
        XMStoreFloat4(&m_rotation, quat);
        m_viewDirty = true;
    }

    auto Camera::RotateRoll(float radians) -> void
    {
        XMVECTOR quat = XMLoadFloat4(&m_rotation);
        XMVECTOR rollQuat = XMQuaternionRotationAxis(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), radians);
        quat = XMQuaternionMultiply(quat, rollQuat);
        XMStoreFloat4(&m_rotation, quat);
        m_viewDirty = true;
    }

    // ========================================
    // Orbit
    // ========================================

    auto Camera::OrbitAround(const XMFLOAT3& target, float distance, float yaw, float pitch) -> void
    {
        // Calculate position based on spherical coordinates
        float x = target.x + distance * cosf(pitch) * sinf(yaw);
        float y = target.y + distance * sinf(pitch);
        float z = target.z + distance * cosf(pitch) * cosf(yaw);

        // Set camera to look at target
        SetLookAt(XMFLOAT3(x, y, z), target, XMFLOAT3(0.0f, 1.0f, 0.0f));
    }

    // ========================================
    // Projection
    // ========================================

    auto Camera::SetPerspectiveFOV(float fovY, float aspectRatio, float nearZ, float farZ) -> void
    {
        // Convert FOV to frustum bounds
        float height = 2.0f * nearZ * tanf(fovY * 0.5f);
        float width = height * aspectRatio;

        m_left = -width * 0.5f;
        m_right = width * 0.5f;
        m_bottom = -height * 0.5f;
        m_top = height * 0.5f;
        m_nearZ = nearZ;
        m_farZ = farZ;
        m_isPerspective = true;
        m_projectionDirty = true;
    }

    auto Camera::SetPerspectiveFrustum(float left, float right, float bottom, float top, float nearZ, float farZ) -> void
    {
        m_left = left;
        m_right = right;
        m_bottom = bottom;
        m_top = top;
        m_nearZ = nearZ;
        m_farZ = farZ;
        m_isPerspective = true;
        m_projectionDirty = true;
    }

    auto Camera::SetOrthographic(float left, float right, float bottom, float top, float nearZ, float farZ) -> void
    {
        m_left = left;
        m_right = right;
        m_bottom = bottom;
        m_top = top;
        m_nearZ = nearZ;
        m_farZ = farZ;
        m_isPerspective = false;
        m_projectionDirty = true;
    }

    // ========================================
    // Matrix Retrieval
    // ========================================

    auto Camera::GetViewMatrix() -> XMMATRIX
    {
        if (m_viewDirty)
        {
            UpdateViewMatrix();
            m_viewDirty = false;
        }

        return XMLoadFloat4x4(&m_viewMatrix);
    }

    auto Camera::GetProjectionMatrix() -> XMMATRIX
    {
        if (m_projectionDirty)
        {
            UpdateProjectionMatrix();
            m_projectionDirty = false;
        }

        return XMLoadFloat4x4(&m_projectionMatrix);
    }

    auto Camera::GetViewProjectionMatrix() -> XMMATRIX
    {
        XMMATRIX view = GetViewMatrix();
        XMMATRIX projection = GetProjectionMatrix();
        return XMMatrixMultiply(view, projection);
    }

    // ========================================
    // Internal Update Methods
    // ========================================

    auto Camera::UpdateViewMatrix() -> void
    {
        // Create view matrix from position and rotation
        XMVECTOR pos = XMLoadFloat3(&m_position);
        XMVECTOR quat = XMLoadFloat4(&m_rotation);

        // Calculate camera axes
        XMVECTOR forward = XMVector3Rotate(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), quat);
        XMVECTOR up = XMVector3Rotate(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), quat);

        // Calculate target point
        XMVECTOR target = XMVectorAdd(pos, forward);

        // Create view matrix
        XMMATRIX viewMatrix = XMMatrixLookAtLH(pos, target, up);
        XMStoreFloat4x4(&m_viewMatrix, viewMatrix);
    }

    auto Camera::UpdateProjectionMatrix() -> void
    {
        XMMATRIX projectionMatrix;

        if (m_isPerspective)
        {
            // Perspective projection using frustum bounds
            projectionMatrix = XMMatrixPerspectiveOffCenterLH(
                m_left, m_right, m_bottom, m_top, m_nearZ, m_farZ
            );
        }
        else
        {
            // Orthographic projection
            projectionMatrix = XMMatrixOrthographicOffCenterLH(
                m_left, m_right, m_bottom, m_top, m_nearZ, m_farZ
            );
        }

        XMStoreFloat4x4(&m_projectionMatrix, projectionMatrix);
    }

} // namespace UnoEngine::Scene
