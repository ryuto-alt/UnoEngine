#include "Graphics/DescriptorHeapManager.h"
#include "Core/Logger.h"
#include <algorithm>

DescriptorHeapManager::DescriptorHeapManager(
    ID3D12Device* device,
    uint32 cbvSrvUavHeapSize,
    uint32 rtvHeapSize,
    uint32 dsvHeapSize
)
    : m_device(device)
    , m_cbvSrvUavHeapSize(cbvSrvUavHeapSize)
    , m_rtvHeapSize(rtvHeapSize)
    , m_dsvHeapSize(dsvHeapSize)
{
    auto result = InitializeHeaps();
    if (!result) {
        Logger::Critical("Failed to initialize DescriptorHeapManager: {}", result.error());
        // In a real engine, you might want to throw or handle this more gracefully
    }
}

auto DescriptorHeapManager::InitializeHeaps() -> Expected<void, Error> {
    Logger::Info("Initializing DescriptorHeapManager (CBV/SRV/UAV: {}, RTV: {}, DSV: {})",
        m_cbvSrvUavHeapSize, m_rtvHeapSize, m_dsvHeapSize);
    
    // Create CBV/SRV/UAV heap (shader-visible)
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.NumDescriptors = m_cbvSrvUavHeapSize;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        heapDesc.NodeMask = 0;
        
        auto hr = m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_cbvSrvUavHeap));
        if (FAILED(hr)) {
            return Expected<void, Error>::Unexpected(
                Error("Failed to create CBV/SRV/UAV descriptor heap")
            );
        }
        
        m_cbvSrvUavDescriptorSize = m_device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
        );
        
        m_cbvSrvUavHeapStart = m_cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart();
        m_cbvSrvUavHeapStartGPU = m_cbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart();
        
        // Initialize free list with all indices
        m_freeCBVSRVUAVIndices.reserve(m_cbvSrvUavHeapSize);
        for (uint32 i = 0; i < m_cbvSrvUavHeapSize; ++i) {
            m_freeCBVSRVUAVIndices.push_back(i);
        }
        
        Logger::Info("Created CBV/SRV/UAV heap (Size: {}, DescriptorSize: {})",
            m_cbvSrvUavHeapSize, m_cbvSrvUavDescriptorSize);
    }
    
    // Create RTV heap (not shader-visible)
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        heapDesc.NumDescriptors = m_rtvHeapSize;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        heapDesc.NodeMask = 0;
        
        auto hr = m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_rtvHeap));
        if (FAILED(hr)) {
            return Expected<void, Error>::Unexpected(
                Error("Failed to create RTV descriptor heap")
            );
        }
        
        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV
        );
        
        m_rtvHeapStart = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        
        // Initialize free list
        m_freeRTVIndices.reserve(m_rtvHeapSize);
        for (uint32 i = 0; i < m_rtvHeapSize; ++i) {
            m_freeRTVIndices.push_back(i);
        }
        
        Logger::Info("Created RTV heap (Size: {}, DescriptorSize: {})",
            m_rtvHeapSize, m_rtvDescriptorSize);
    }
    
    // Create DSV heap (not shader-visible)
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        heapDesc.NumDescriptors = m_dsvHeapSize;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        heapDesc.NodeMask = 0;
        
        auto hr = m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_dsvHeap));
        if (FAILED(hr)) {
            return Expected<void, Error>::Unexpected(
                Error("Failed to create DSV descriptor heap")
            );
        }
        
        m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_DSV
        );
        
        m_dsvHeapStart = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
        
        // Initialize free list
        m_freeDSVIndices.reserve(m_dsvHeapSize);
        for (uint32 i = 0; i < m_dsvHeapSize; ++i) {
            m_freeDSVIndices.push_back(i);
        }
        
        Logger::Info("Created DSV heap (Size: {}, DescriptorSize: {})",
            m_dsvHeapSize, m_dsvDescriptorSize);
    }
    
    Logger::Info("DescriptorHeapManager initialized successfully");
    return {};
}

auto DescriptorHeapManager::AllocateFromCBVSRVUAVHeap() -> Expected<DescriptorHandle, Error> {
    if (m_freeCBVSRVUAVIndices.empty()) {
        return Expected<DescriptorHandle, Error>::Unexpected(
            Error("CBV/SRV/UAV descriptor heap is full")
        );
    }
    
    // Pop from free list
    uint32 index = m_freeCBVSRVUAVIndices.back();
    m_freeCBVSRVUAVIndices.pop_back();
    
    DescriptorHandle handle{};
    handle.heapIndex = index;
    handle.cpu.ptr = m_cbvSrvUavHeapStart.ptr + index * m_cbvSrvUavDescriptorSize;
    handle.gpu.ptr = m_cbvSrvUavHeapStartGPU.ptr + index * m_cbvSrvUavDescriptorSize;
    
    return handle;
}

auto DescriptorHeapManager::AllocateFromRTVHeap() -> Expected<DescriptorHandle, Error> {
    if (m_freeRTVIndices.empty()) {
        return Expected<DescriptorHandle, Error>::Unexpected(
            Error("RTV descriptor heap is full")
        );
    }
    
    // Pop from free list
    uint32 index = m_freeRTVIndices.back();
    m_freeRTVIndices.pop_back();
    
    DescriptorHandle handle{};
    handle.heapIndex = index;
    handle.cpu.ptr = m_rtvHeapStart.ptr + index * m_rtvDescriptorSize;
    handle.gpu.ptr = 0; // RTV heap is not shader-visible
    
    return handle;
}

auto DescriptorHeapManager::AllocateFromDSVHeap() -> Expected<DescriptorHandle, Error> {
    if (m_freeDSVIndices.empty()) {
        return Expected<DescriptorHandle, Error>::Unexpected(
            Error("DSV descriptor heap is full")
        );
    }
    
    // Pop from free list
    uint32 index = m_freeDSVIndices.back();
    m_freeDSVIndices.pop_back();
    
    DescriptorHandle handle{};
    handle.heapIndex = index;
    handle.cpu.ptr = m_dsvHeapStart.ptr + index * m_dsvDescriptorSize;
    handle.gpu.ptr = 0; // DSV heap is not shader-visible
    
    return handle;
}

auto DescriptorHeapManager::AllocateCBV() -> Expected<DescriptorHandle, Error> {
    auto handle = AllocateFromCBVSRVUAVHeap();
    if (handle) {
        Logger::Trace("Allocated CBV (Index: {}, CPU: {:#x}, GPU: {:#x})",
            handle.value().heapIndex, handle.value().cpu.ptr, handle.value().gpu.ptr);
    }
    return handle;
}

auto DescriptorHeapManager::AllocateSRV() -> Expected<DescriptorHandle, Error> {
    auto handle = AllocateFromCBVSRVUAVHeap();
    if (handle) {
        Logger::Trace("Allocated SRV (Index: {}, CPU: {:#x}, GPU: {:#x})",
            handle.value().heapIndex, handle.value().cpu.ptr, handle.value().gpu.ptr);
    }
    return handle;
}

auto DescriptorHeapManager::AllocateUAV() -> Expected<DescriptorHandle, Error> {
    auto handle = AllocateFromCBVSRVUAVHeap();
    if (handle) {
        Logger::Trace("Allocated UAV (Index: {}, CPU: {:#x}, GPU: {:#x})",
            handle.value().heapIndex, handle.value().cpu.ptr, handle.value().gpu.ptr);
    }
    return handle;
}

auto DescriptorHeapManager::AllocateRTV() -> Expected<DescriptorHandle, Error> {
    auto handle = AllocateFromRTVHeap();
    if (handle) {
        Logger::Trace("Allocated RTV (Index: {}, CPU: {:#x})",
            handle.value().heapIndex, handle.value().cpu.ptr);
    }
    return handle;
}

auto DescriptorHeapManager::AllocateDSV() -> Expected<DescriptorHandle, Error> {
    auto handle = AllocateFromDSVHeap();
    if (handle) {
        Logger::Trace("Allocated DSV (Index: {}, CPU: {:#x})",
            handle.value().heapIndex, handle.value().cpu.ptr);
    }
    return handle;
}

auto DescriptorHeapManager::FreeCBV(const DescriptorHandle& handle) -> void {
    if (!handle.IsValid()) {
        Logger::Warn("Attempted to free invalid CBV descriptor");
        return;
    }
    
    Logger::Trace("Freed CBV (Index: {})", handle.heapIndex);
    m_freeCBVSRVUAVIndices.push_back(handle.heapIndex);
}

auto DescriptorHeapManager::FreeSRV(const DescriptorHandle& handle) -> void {
    if (!handle.IsValid()) {
        Logger::Warn("Attempted to free invalid SRV descriptor");
        return;
    }
    
    Logger::Trace("Freed SRV (Index: {})", handle.heapIndex);
    m_freeCBVSRVUAVIndices.push_back(handle.heapIndex);
}

auto DescriptorHeapManager::FreeUAV(const DescriptorHandle& handle) -> void {
    if (!handle.IsValid()) {
        Logger::Warn("Attempted to free invalid UAV descriptor");
        return;
    }
    
    Logger::Trace("Freed UAV (Index: {})", handle.heapIndex);
    m_freeCBVSRVUAVIndices.push_back(handle.heapIndex);
}

auto DescriptorHeapManager::FreeRTV(const DescriptorHandle& handle) -> void {
    if (!handle.IsValid()) {
        Logger::Warn("Attempted to free invalid RTV descriptor");
        return;
    }
    
    Logger::Trace("Freed RTV (Index: {})", handle.heapIndex);
    m_freeRTVIndices.push_back(handle.heapIndex);
}

auto DescriptorHeapManager::FreeDSV(const DescriptorHandle& handle) -> void {
    if (!handle.IsValid()) {
        Logger::Warn("Attempted to free invalid DSV descriptor");
        return;
    }
    
    Logger::Trace("Freed DSV (Index: {})", handle.heapIndex);
    m_freeDSVIndices.push_back(handle.heapIndex);
}

auto DescriptorHeapManager::GetStatistics() const -> Statistics {
    Statistics stats{};
    stats.cbvSrvUavAllocated = m_cbvSrvUavHeapSize - static_cast<uint32>(m_freeCBVSRVUAVIndices.size());
    stats.cbvSrvUavFree = static_cast<uint32>(m_freeCBVSRVUAVIndices.size());
    stats.rtvAllocated = m_rtvHeapSize - static_cast<uint32>(m_freeRTVIndices.size());
    stats.rtvFree = static_cast<uint32>(m_freeRTVIndices.size());
    stats.dsvAllocated = m_dsvHeapSize - static_cast<uint32>(m_freeDSVIndices.size());
    stats.dsvFree = static_cast<uint32>(m_freeDSVIndices.size());
    return stats;
}
