#pragma once

#include "../Core/CoreTypes.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <vector>

namespace UnoEngine::Graphics
{
    using namespace UnoEngine::Core;
    using Microsoft::WRL::ComPtr;

    // ========================================
    // Graphics Device Configuration
    // ========================================

    struct GraphicsDeviceConfig
    {
        bool enableDebugLayer{ false };
        bool enableGpuValidation{ false };
        uint32 backBufferCount{ 2 };
        uint32 width{ 1280 };
        uint32 height{ 720 };
        DXGI_FORMAT backBufferFormat{ DXGI_FORMAT_R8G8B8A8_UNORM };
        DXGI_FORMAT depthStencilFormat{ DXGI_FORMAT_D32_FLOAT };
    };

    // ========================================
    // Graphics Device
    // ========================================

    class GraphicsDevice
    {
    public:
        GraphicsDevice() = default;
        ~GraphicsDevice() = default;

        // Non-copyable, but movable
        GraphicsDevice(const GraphicsDevice&) = delete;
        auto operator=(const GraphicsDevice&) -> GraphicsDevice& = delete;
        GraphicsDevice(GraphicsDevice&&) noexcept = default;
        auto operator=(GraphicsDevice&&) noexcept -> GraphicsDevice& = default;

        // ========================================
        // Initialization
        // ========================================

        auto Initialize(const GraphicsDeviceConfig& config, void* windowHandle) -> bool;
        auto Shutdown() -> void;

        // ========================================
        // Frame Management
        // ========================================

        auto BeginFrame() -> void;
        auto EndFrame() -> void;
        auto Present() -> void;

        // ========================================
        // Getters
        // ========================================

        [[nodiscard]] auto GetDevice() const noexcept -> ID3D12Device*
        {
            return m_device.Get();
        }

        [[nodiscard]] auto GetCommandQueue() const noexcept -> ID3D12CommandQueue*
        {
            return m_commandQueue.Get();
        }

        [[nodiscard]] auto GetCommandAllocator() const noexcept -> ID3D12CommandAllocator*
        {
            return m_commandAllocators[m_frameIndex].Get();
        }

        [[nodiscard]] auto GetCommandList() const noexcept -> ID3D12GraphicsCommandList*
        {
            return m_commandList.Get();
        }

        [[nodiscard]] auto GetSwapChain() const noexcept -> IDXGISwapChain4*
        {
            return m_swapChain.Get();
        }

        [[nodiscard]] auto GetCurrentBackBuffer() const noexcept -> ID3D12Resource*
        {
            return m_renderTargets[m_frameIndex].Get();
        }

        [[nodiscard]] auto GetBackBufferFormat() const noexcept -> DXGI_FORMAT
        {
            return m_config.backBufferFormat;
        }

        [[nodiscard]] auto GetDepthStencilFormat() const noexcept -> DXGI_FORMAT
        {
            return m_config.depthStencilFormat;
        }

        [[nodiscard]] auto GetFrameIndex() const noexcept -> uint32
        {
            return m_frameIndex;
        }

        [[nodiscard]] auto GetBackBufferCount() const noexcept -> uint32
        {
            return m_config.backBufferCount;
        }

        [[nodiscard]] auto GetCurrentRenderTargetView() const noexcept -> D3D12_CPU_DESCRIPTOR_HANDLE
        {
            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
            rtvHandle.ptr += static_cast<SIZE_T>(m_frameIndex) * m_rtvDescriptorSize;
            return rtvHandle;
        }

        [[nodiscard]] auto GetDepthStencilView() const noexcept -> D3D12_CPU_DESCRIPTOR_HANDLE
        {
            return m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
        }

    private:
        // ========================================
        // Initialization Helper Methods
        // ========================================

        auto CreateFactory() -> bool;
        auto SelectAdapter() -> bool;
        auto CreateDevice() -> bool;
        auto CreateCommandQueue() -> bool;
        auto CreateSwapChain(void* windowHandle) -> bool;
        auto CreateCommandAllocators() -> bool;
        auto CreateCommandList() -> bool;
        auto CreateRenderTargetViews() -> bool;
        auto CreateDepthStencilBuffer() -> bool;
        auto CreateFence() -> bool;
        auto CreateDescriptorHeaps() -> bool;

        // ========================================
        // Synchronization
        // ========================================

        auto WaitForGpu() -> void;
        auto WaitForFence(uint64 fenceValue) -> void;
        auto MoveToNextFrame() -> void;

        // ========================================
        // Member Variables
        // ========================================

        GraphicsDeviceConfig m_config{};

        // Core DirectX 12 objects
        ComPtr<IDXGIFactory7> m_factory{};
        ComPtr<IDXGIAdapter4> m_adapter{};
        ComPtr<ID3D12Device> m_device{};
        ComPtr<ID3D12CommandQueue> m_commandQueue{};
        ComPtr<IDXGISwapChain4> m_swapChain{};

        // Command recording
        std::vector<ComPtr<ID3D12CommandAllocator>> m_commandAllocators{};
        ComPtr<ID3D12GraphicsCommandList> m_commandList{};

        // Render targets
        std::vector<ComPtr<ID3D12Resource>> m_renderTargets{};
        ComPtr<ID3D12Resource> m_depthStencil{};

        // Descriptor heaps
        ComPtr<ID3D12DescriptorHeap> m_rtvHeap{};
        ComPtr<ID3D12DescriptorHeap> m_dsvHeap{};
        ComPtr<ID3D12DescriptorHeap> m_srvHeap{}; // Shader Resource View heap

        // Descriptor sizes
        uint32 m_rtvDescriptorSize{ 0 };
        uint32 m_dsvDescriptorSize{ 0 };
        uint32 m_cbvSrvUavDescriptorSize{ 0 };

        // Synchronization
        ComPtr<ID3D12Fence> m_fence{};
        uint64 m_fenceValue{ 0 };
        std::vector<uint64> m_fenceValues{};
        void* m_fenceEvent{ nullptr };

        // Frame management
        uint32 m_frameIndex{ 0 };

        // State tracking
        bool m_isInitialized{ false };
    };

} // namespace UnoEngine::Graphics
