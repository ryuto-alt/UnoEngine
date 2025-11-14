#pragma once

#pragma once

#include "Core/CoreTypes.h"
#include "Core/Expected.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <vector>
#include <string_view>

using Microsoft::WRL::ComPtr;

/**
 * @brief Descriptor handle containing both CPU and GPU addresses
 */
struct DescriptorHandle {
    D3D12_CPU_DESCRIPTOR_HANDLE cpu{};
    D3D12_GPU_DESCRIPTOR_HANDLE gpu{};
    uint32 heapIndex{0};
    
    [[nodiscard]] constexpr auto IsValid() const noexcept -> bool {
        return cpu.ptr != 0;
    }
};

/**
 * @brief Manages DirectX 12 Descriptor Heaps with dynamic free list allocation
 * 
 * This class provides efficient descriptor allocation and deallocation using a free list approach.
 * - CBV/SRV/UAV share a common heap (as per D3D12 design)
 * - RTV and DSV have separate heaps (required by D3D12)
 * - Fixed-size heaps with dynamic reuse of freed descriptors
 * 
 * Usage:
 * @code
 * auto descriptorMgr = std::make_unique<DescriptorHeapManager>(device);
 * auto cbvHandle = descriptorMgr->AllocateCBV();
 * if (cbvHandle) {
 *     // Use the descriptor
 *     device->CreateConstantBufferView(&cbvDesc, cbvHandle.value().cpu);
 *     // ... later ...
 *     descriptorMgr->FreeCBV(cbvHandle.value());
 * }
 * @endcode
 */
class DescriptorHeapManager {
public:
    /**
     * @brief Constructor
     * @param device DirectX 12 device
     * @param cbvSrvUavHeapSize Size of CBV/SRV/UAV heap (default: 10000)
     * @param rtvHeapSize Size of RTV heap (default: 100)
     * @param dsvHeapSize Size of DSV heap (default: 100)
     */
    explicit DescriptorHeapManager(
        ID3D12Device* device,
        uint32 cbvSrvUavHeapSize = 10000,
        uint32 rtvHeapSize = 100,
        uint32 dsvHeapSize = 100
    );
    
    ~DescriptorHeapManager() = default;
    
    // Non-copyable, movable
    DescriptorHeapManager(const DescriptorHeapManager&) = delete;
    auto operator=(const DescriptorHeapManager&) -> DescriptorHeapManager& = delete;
    DescriptorHeapManager(DescriptorHeapManager&&) = default;
    auto operator=(DescriptorHeapManager&&) -> DescriptorHeapManager& = default;
    
    /**
     * @brief Allocate a CBV (Constant Buffer View) descriptor
     * @return Descriptor handle on success, error message on failure
     */
    [[nodiscard]] auto AllocateCBV() -> Expected<DescriptorHandle, Error>;
    
    /**
     * @brief Allocate an SRV (Shader Resource View) descriptor
     * @return Descriptor handle on success, error message on failure
     */
    [[nodiscard]] auto AllocateSRV() -> Expected<DescriptorHandle, Error>;
    
    /**
     * @brief Allocate a UAV (Unordered Access View) descriptor
     * @return Descriptor handle on success, error message on failure
     */
    [[nodiscard]] auto AllocateUAV() -> Expected<DescriptorHandle, Error>;
    
    /**
     * @brief Allocate an RTV (Render Target View) descriptor
     * @return Descriptor handle on success, error message on failure
     */
    [[nodiscard]] auto AllocateRTV() -> Expected<DescriptorHandle, Error>;
    
    /**
     * @brief Allocate a DSV (Depth Stencil View) descriptor
     * @return Descriptor handle on success, error message on failure
     */
    [[nodiscard]] auto AllocateDSV() -> Expected<DescriptorHandle, Error>;
    
    /**
     * @brief Free a CBV descriptor (returns it to the free list)
     * @param handle Descriptor handle to free
     */
    auto FreeCBV(const DescriptorHandle& handle) -> void;
    
    /**
     * @brief Free an SRV descriptor (returns it to the free list)
     * @param handle Descriptor handle to free
     */
    auto FreeSRV(const DescriptorHandle& handle) -> void;
    
    /**
     * @brief Free a UAV descriptor (returns it to the free list)
     * @param handle Descriptor handle to free
     */
    auto FreeUAV(const DescriptorHandle& handle) -> void;
    
    /**
     * @brief Free an RTV descriptor (returns it to the free list)
     * @param handle Descriptor handle to free
     */
    auto FreeRTV(const DescriptorHandle& handle) -> void;
    
    /**
     * @brief Free a DSV descriptor (returns it to the free list)
     * @param handle Descriptor handle to free
     */
    auto FreeDSV(const DescriptorHandle& handle) -> void;
    
    /**
     * @brief Get the CBV/SRV/UAV descriptor heap
     * @return Pointer to the descriptor heap
     */
    [[nodiscard]] auto GetCBVSRVUAVHeap() -> ID3D12DescriptorHeap* {
        return m_cbvSrvUavHeap.Get();
    }
    
    /**
     * @brief Get the RTV descriptor heap
     * @return Pointer to the descriptor heap
     */
    [[nodiscard]] auto GetRTVHeap() -> ID3D12DescriptorHeap* {
        return m_rtvHeap.Get();
    }
    
    /**
     * @brief Get the DSV descriptor heap
     * @return Pointer to the descriptor heap
     */
    [[nodiscard]] auto GetDSVHeap() -> ID3D12DescriptorHeap* {
        return m_dsvHeap.Get();
    }
    
    /**
     * @brief Get allocation statistics for debugging
     */
    struct Statistics {
        uint32 cbvSrvUavAllocated;
        uint32 cbvSrvUavFree;
        uint32 rtvAllocated;
        uint32 rtvFree;
        uint32 dsvAllocated;
        uint32 dsvFree;
    };
    
    [[nodiscard]] auto GetStatistics() const -> Statistics;

private:
    /**
     * @brief Initialize descriptor heaps
     */
    auto InitializeHeaps() -> Expected<void, Error>;
    
    /**
     * @brief Allocate from CBV/SRV/UAV heap
     */
    [[nodiscard]] auto AllocateFromCBVSRVUAVHeap() -> Expected<DescriptorHandle, Error>;
    
    /**
     * @brief Allocate from RTV heap
     */
    [[nodiscard]] auto AllocateFromRTVHeap() -> Expected<DescriptorHandle, Error>;
    
    /**
     * @brief Allocate from DSV heap
     */
    [[nodiscard]] auto AllocateFromDSVHeap() -> Expected<DescriptorHandle, Error>;

    ComPtr<ID3D12Device> m_device;
    
    // CBV/SRV/UAV shared heap
    ComPtr<ID3D12DescriptorHeap> m_cbvSrvUavHeap;
    uint32 m_cbvSrvUavDescriptorSize{0};
    uint32 m_cbvSrvUavHeapSize{0};
    std::vector<uint32> m_freeCBVSRVUAVIndices;
    D3D12_CPU_DESCRIPTOR_HANDLE m_cbvSrvUavHeapStart{};
    D3D12_GPU_DESCRIPTOR_HANDLE m_cbvSrvUavHeapStartGPU{};
    
    // RTV heap
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    uint32 m_rtvDescriptorSize{0};
    uint32 m_rtvHeapSize{0};
    std::vector<uint32> m_freeRTVIndices;
    D3D12_CPU_DESCRIPTOR_HANDLE m_rtvHeapStart{};
    
    // DSV heap
    ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
    uint32 m_dsvDescriptorSize{0};
    uint32 m_dsvHeapSize{0};
    std::vector<uint32> m_freeDSVIndices;
    D3D12_CPU_DESCRIPTOR_HANDLE m_dsvHeapStart{};
};
