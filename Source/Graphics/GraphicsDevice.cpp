#include "../../Include/Graphics/GraphicsDevice.h"
#include <iostream>
#include <stdexcept>
#include <directx/d3dx12.h> // DirectX 12 Helper Library (DirectX-Headers)

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

namespace UnoEngine::Graphics
{
    // ========================================
    // Initialization
    // ========================================

    auto GraphicsDevice::Initialize(const GraphicsDeviceConfig& config, void* windowHandle) -> bool
    {
        if (m_isInitialized)
        {
            return true;
        }

        m_config = config;

        try
        {
            if (!CreateFactory())       return false;
            if (!SelectAdapter())       return false;
            if (!CreateDevice())        return false;
            if (!CreateCommandQueue())  return false;
            if (!CreateSwapChain(windowHandle)) return false;
            if (!CreateCommandAllocators()) return false;
            if (!CreateCommandList())   return false;
            if (!CreateDescriptorHeaps()) return false;
            if (!CreateRenderTargetViews()) return false;
            if (!CreateDepthStencilBuffer()) return false;
            if (!CreateFence())         return false;

            m_isInitialized = true;
            return true;
        }
        catch (const std::exception&)
        {
            // Log error (implement logging system later)
            return false;
        }
    }

    auto GraphicsDevice::Shutdown() -> void
    {
        if (!m_isInitialized)
        {
            return;
        }

        // Wait for GPU to finish all work
        WaitForGpu();

        // Release resources
        m_commandList.Reset();
        m_commandAllocators.clear();
        m_commandQueue.Reset();
        m_swapChain.Reset();
        m_renderTargets.clear();
        m_depthStencil.Reset();
        m_rtvHeap.Reset();
        m_dsvHeap.Reset();
        m_srvHeap.Reset();
        m_fence.Reset();
        m_device.Reset();
        m_adapter.Reset();
        m_factory.Reset();

        if (m_fenceEvent)
        {
            CloseHandle(m_fenceEvent);
            m_fenceEvent = nullptr;
        }

        m_isInitialized = false;
    }

    // ========================================
    // Factory Creation
    // ========================================

    auto GraphicsDevice::CreateFactory() -> bool
    {
        std::cout << "\n========================================" << std::endl;
        std::cout << "Initializing DirectX 12 Graphics Device" << std::endl;
        std::cout << "========================================" << std::endl;

        uint32 dxgiFactoryFlags = 0;

#ifdef _DEBUG
        // Enable debug layer
        if (m_config.enableDebugLayer)
        {
            std::cout << "\n[1/8] Enabling D3D12 Debug Layer..." << std::endl;
            ComPtr<ID3D12Debug> debugController;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
            {
                debugController->EnableDebugLayer();
                dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
                std::cout << "  D3D12 Debug Layer enabled" << std::endl;

                if (m_config.enableGpuValidation)
                {
                    ComPtr<ID3D12Debug1> debugController1;
                    if (SUCCEEDED(debugController.As(&debugController1)))
                    {
                        debugController1->SetEnableGPUBasedValidation(true);
                        std::cout << "  GPU-based validation enabled" << std::endl;
                    }
                }
            }
        }
        else
        {
            std::cout << "\n[1/8] Creating DXGI Factory (Debug layer disabled)..." << std::endl;
        }
#else
        std::cout << "\n[1/8] Creating DXGI Factory (Release mode)..." << std::endl;
#endif

        HRESULT hr = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_factory));
        if (SUCCEEDED(hr))
        {
            std::cout << "  DXGI Factory created successfully" << std::endl;
        }
        else
        {
            std::cerr << "  ERROR: Failed to create DXGI Factory! HRESULT: 0x" 
                      << std::hex << hr << std::dec << std::endl;
        }
        return SUCCEEDED(hr);
    }

    // ========================================
    // Adapter Selection
    // ========================================

    auto GraphicsDevice::SelectAdapter() -> bool
    {
        ComPtr<IDXGIAdapter1> adapter1;

        std::cout << "\n[2/8] Enumerating GPU Adapters..." << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "========================================" << std::endl;

        for (uint32 adapterIndex = 0;
             m_factory->EnumAdapterByGpuPreference(
                 adapterIndex,
                 DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                 IID_PPV_ARGS(&adapter1)) != DXGI_ERROR_NOT_FOUND;
             ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter1->GetDesc1(&desc);

            // Convert wide string to narrow string for console output
            char adapterName[128];
            wcstombs_s(nullptr, adapterName, sizeof(adapterName), desc.Description, _TRUNCATE);

            std::cout << "\n[GPU " << adapterIndex << "] " << adapterName << std::endl;
            std::cout << "  Dedicated Video Memory: " 
                      << (desc.DedicatedVideoMemory / 1024 / 1024) << " MB" << std::endl;
            std::cout << "  Dedicated System Memory: " 
                      << (desc.DedicatedSystemMemory / 1024 / 1024) << " MB" << std::endl;
            std::cout << "  Shared System Memory: " 
                      << (desc.SharedSystemMemory / 1024 / 1024) << " MB" << std::endl;
            std::cout << "  Vendor ID: 0x" << std::hex << desc.VendorId << std::dec << std::endl;
            std::cout << "  Device ID: 0x" << std::hex << desc.DeviceId << std::dec << std::endl;

            // Skip software adapters
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                std::cout << "  [SKIPPED] Software adapter" << std::endl;
                continue;
            }

            // Check if adapter supports D3D12
            if (SUCCEEDED(D3D12CreateDevice(adapter1.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
            {
                std::cout << "  [SELECTED] D3D12 Feature Level 11.0 supported!" << std::endl;
                std::cout << "========================================" << std::endl;
                adapter1.As(&m_adapter);
                return true;
            }
            else
            {
                std::cout << "  [REJECTED] D3D12 not supported" << std::endl;
            }
        }

        std::cerr << "\nERROR: No compatible D3D12 adapter found!" << std::endl;
        return false;
    }

    // ========================================
    // Device Creation
    // ========================================

    auto GraphicsDevice::CreateDevice() -> bool
    {
        std::cout << "\n[3/8] Creating D3D12 Device..." << std::endl;
        UNO_ASSERT_NOT_NULL(m_adapter.Get(), "Adapter");

        HRESULT hr = D3D12CreateDevice(
            m_adapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&m_device)
        );

        if (FAILED(hr))
        {
            std::cerr << "Failed to create D3D12 device! HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
            return false;
        }

        std::cout << "  D3D12 Device created successfully" << std::endl;

#ifdef _DEBUG
        // Configure debug device settings
        if (m_config.enableDebugLayer)
        {
            ComPtr<ID3D12InfoQueue> infoQueue;
            if (SUCCEEDED(m_device.As(&infoQueue)))
            {
                infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
                infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
                infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, FALSE);

                UNO_DEBUG_LOG("D3D12 Debug layer configured - Breaking on CORRUPTION and ERROR");
            }
        }
#endif

        // Get descriptor sizes
        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        m_cbvSrvUavDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        return true;
    }

    // ========================================
    // Command Queue Creation
    // ========================================

    auto GraphicsDevice::CreateCommandQueue() -> bool
    {
        std::cout << "\n[4/8] Creating Command Queue..." << std::endl;
        D3D12_COMMAND_QUEUE_DESC queueDesc{};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.NodeMask = 0;

        HRESULT hr = m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue));
        if (SUCCEEDED(hr))
        {
            std::cout << "  Command Queue created successfully" << std::endl;
        }
        else
        {
            std::cerr << "  ERROR: Failed to create Command Queue!" << std::endl;
        }
        return SUCCEEDED(hr);
    }

    // ========================================
    // Command Allocators Creation
    // ========================================

    auto GraphicsDevice::CreateCommandAllocators() -> bool
    {
        std::cout << "\n[6/8] Creating Command Allocators..." << std::endl;
        UNO_ASSERT_NOT_NULL(m_device.Get(), "Device");
        UNO_ASSERT(m_config.backBufferCount > 0, "backBufferCount must be greater than 0");

        m_commandAllocators.resize(m_config.backBufferCount);
        m_fenceValues.resize(m_config.backBufferCount, 0);

        for (uint32 i = 0; i < m_config.backBufferCount; ++i)
        {
            HRESULT hr = m_device->CreateCommandAllocator(
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                IID_PPV_ARGS(&m_commandAllocators[i])
            );

            if (FAILED(hr))
            {
                std::cerr << "Failed to create command allocator " << i << "!" << std::endl;
                return false;
            }

            UNO_ASSERT_NOT_NULL(m_commandAllocators[i].Get(), "Created command allocator");
        }

        std::cout << "  " << m_config.backBufferCount << " Command Allocators created" << std::endl;
        return true;
    }

    // ========================================
    // Command List Creation
    // ========================================

    auto GraphicsDevice::CreateCommandList() -> bool
    {
        std::cout << "\n[7/8] Creating Command List..." << std::endl;
        HRESULT hr = m_device->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            m_commandAllocators[0].Get(),
            nullptr,
            IID_PPV_ARGS(&m_commandList)
        );

        if (FAILED(hr))
        {
            return false;
        }

        // Command lists are created in recording state, close it for now
        m_commandList->Close();

        std::cout << "  Command List created successfully" << std::endl;
        return true;
    }

    // ========================================
    // Descriptor Heaps Creation
    // ========================================

    auto GraphicsDevice::CreateDescriptorHeaps() -> bool
    {
        // RTV descriptor heap
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.NumDescriptors = m_config.backBufferCount;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        rtvHeapDesc.NodeMask = 0;

        HRESULT hr = m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));
        if (FAILED(hr)) return false;

        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        // DSV descriptor heap
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        dsvHeapDesc.NodeMask = 0;

        hr = m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap));
        if (FAILED(hr)) return false;

        m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

        // SRV/CBV/UAV descriptor heap
        D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{};
        srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvHeapDesc.NumDescriptors = 1000; // Allocate enough for various resources
        srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        srvHeapDesc.NodeMask = 0;

        hr = m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap));
        if (FAILED(hr)) return false;

        m_cbvSrvUavDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        return true;
    }

    // ========================================
    // Fence Creation
    // ========================================

    auto GraphicsDevice::CreateFence() -> bool
    {
        std::cout << "\n[8/8] Creating Fence for GPU synchronization..." << std::endl;
        HRESULT hr = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
        if (FAILED(hr))
        {
            return false;
        }

        m_fenceValue = 1;

        // Create event for GPU synchronization
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_fenceEvent == nullptr)
        {
            std::cerr << "  ERROR: Failed to create fence event!" << std::endl;
            return false;
        }

        std::cout << "  Fence and synchronization event created" << std::endl;
        std::cout << "\n========================================" << std::endl;
        std::cout << "Graphics Device Initialized Successfully!" << std::endl;
        std::cout << "========================================\n" << std::endl;
        return true;
    }

    // ========================================
    // Frame Management
    // ========================================

    auto GraphicsDevice::BeginFrame() -> void
    {
        UNO_ASSERT(m_isInitialized, "GraphicsDevice is not initialized");
        UNO_ASSERT_RANGE(m_frameIndex, m_commandAllocators.size(), "m_frameIndex");
        UNO_ASSERT_NOT_NULL(m_commandAllocators[m_frameIndex].Get(), "Command allocator");
        UNO_ASSERT_NOT_NULL(m_commandList.Get(), "Command list");

        // Wait for this frame's GPU work to complete before resetting allocator
        WaitForFence(m_fenceValues[m_frameIndex]);

        // Reset command allocator for current frame
        m_commandAllocators[m_frameIndex]->Reset();

        // Reset command list
        m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), nullptr);

        // Transition back buffer from PRESENT to RENDER_TARGET state
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_renderTargets[m_frameIndex].Get(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET
        );
        m_commandList->ResourceBarrier(1, &barrier);
    }

    auto GraphicsDevice::EndFrame() -> void
    {
        UNO_ASSERT_NOT_NULL(m_commandList.Get(), "Command list");
        UNO_ASSERT_NOT_NULL(m_commandQueue.Get(), "Command queue");

        // Transition back buffer to PRESENT state
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_renderTargets[m_frameIndex].Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT
        );
        m_commandList->ResourceBarrier(1, &barrier);

        // Close command list
        m_commandList->Close();

        // Execute command list
        ID3D12CommandList* commandLists[] = { m_commandList.Get() };
        m_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
    }

    auto GraphicsDevice::Present() -> void
    {
        UNO_ASSERT_NOT_NULL(m_swapChain.Get(), "Swap chain");

        if (m_swapChain)
        {
            m_swapChain->Present(1, 0); // VSync on (1), VSync off (0)
        }

        MoveToNextFrame();
    }

    // ========================================
    // Synchronization
    // ========================================

    auto GraphicsDevice::WaitForGpu() -> void
    {
        UNO_ASSERT_NOT_NULL(m_commandQueue.Get(), "Command queue");
        UNO_ASSERT_NOT_NULL(m_fence.Get(), "Fence");

        // Schedule a Signal command in the queue
        m_commandQueue->Signal(m_fence.Get(), m_fenceValue);

        // Wait until the fence has been processed
        m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent);
        WaitForSingleObject(m_fenceEvent, INFINITE);

        ++m_fenceValue;
    }

    auto GraphicsDevice::WaitForFence(uint64 fenceValue) -> void
    {
        UNO_ASSERT_NOT_NULL(m_fence.Get(), "Fence");

        if (m_fence->GetCompletedValue() < fenceValue)
        {
            m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent);
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }
    }

    auto GraphicsDevice::MoveToNextFrame() -> void
    {
        UNO_ASSERT_NOT_NULL(m_commandQueue.Get(), "Command queue");
        UNO_ASSERT_NOT_NULL(m_swapChain.Get(), "Swap chain");

        // Signal the fence for the current frame
        const uint64 currentFenceValue = m_fenceValue;
        m_fenceValues[m_frameIndex] = currentFenceValue;
        m_commandQueue->Signal(m_fence.Get(), currentFenceValue);
        ++m_fenceValue;

        // Move to next frame
        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

        // Wait if the next frame is not ready yet
        WaitForFence(m_fenceValues[m_frameIndex]);
    }

    // ========================================
    // Create Swap Chain
    // ========================================

    auto GraphicsDevice::CreateSwapChain(void* windowHandle) -> bool
    {
        std::cout << "\n[5/8] Creating Swap Chain..." << std::endl;
        HWND hwnd = static_cast<HWND>(windowHandle);

        // Describe swap chain
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
        swapChainDesc.Width = m_config.width;
        swapChainDesc.Height = m_config.height;
        swapChainDesc.Format = m_config.backBufferFormat;
        swapChainDesc.Stereo = FALSE;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = m_config.backBufferCount;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        swapChainDesc.Flags = 0;

        // Create swap chain
        ComPtr<IDXGISwapChain1> swapChain1;
        HRESULT hr = m_factory->CreateSwapChainForHwnd(
            m_commandQueue.Get(),
            hwnd,
            &swapChainDesc,
            nullptr,
            nullptr,
            &swapChain1
        );

        if (FAILED(hr))
        {
            std::cerr << "Failed to create swap chain!" << std::endl;
            return false;
        }

        // Query IDXGISwapChain4
        hr = swapChain1.As(&m_swapChain);
        if (FAILED(hr))
        {
            std::cerr << "Failed to query IDXGISwapChain4!" << std::endl;
            return false;
        }

        // Disable Alt+Enter fullscreen toggle
        m_factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

        std::cout << "  Swap Chain created successfully (" << m_config.backBufferCount 
                  << " buffers, " << m_config.width << "x" << m_config.height << ")" << std::endl;
        return true;
    }

    // ========================================
    // Render Target Views Creation
    // ========================================

    auto GraphicsDevice::CreateRenderTargetViews() -> bool
    {
        UNO_ASSERT_NOT_NULL(m_device.Get(), "Device");
        UNO_ASSERT_NOT_NULL(m_swapChain.Get(), "SwapChain");
        UNO_ASSERT_NOT_NULL(m_rtvHeap.Get(), "RTV Heap");

        // Create render target views for each back buffer
        m_renderTargets.resize(m_config.backBufferCount);
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

        for (uint32 i = 0; i < m_config.backBufferCount; i++)
        {
            HRESULT hr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i]));
            if (FAILED(hr))
            {
                std::cerr << "Failed to get swap chain buffer " << i << "!" << std::endl;
                return false;
            }

            m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, m_rtvDescriptorSize);
        }

        std::cout << "  Render Target Views created successfully" << std::endl;
        return true;
    }

    // ========================================
    // Depth Stencil Buffer Creation
    // ========================================

    auto GraphicsDevice::CreateDepthStencilBuffer() -> bool
    {
        UNO_ASSERT_NOT_NULL(m_device.Get(), "Device");
        UNO_ASSERT_NOT_NULL(m_dsvHeap.Get(), "DSV Heap");

        // Create depth stencil texture
        D3D12_RESOURCE_DESC depthStencilDesc{};
        depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        depthStencilDesc.Alignment = 0;
        depthStencilDesc.Width = m_config.width;
        depthStencilDesc.Height = m_config.height;
        depthStencilDesc.DepthOrArraySize = 1;
        depthStencilDesc.MipLevels = 1;
        depthStencilDesc.Format = m_config.depthStencilFormat;
        depthStencilDesc.SampleDesc.Count = 1;
        depthStencilDesc.SampleDesc.Quality = 0;
        depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE clearValue{};
        clearValue.Format = m_config.depthStencilFormat;
        clearValue.DepthStencil.Depth = 1.0f;
        clearValue.DepthStencil.Stencil = 0;

        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

        HRESULT hr = m_device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &depthStencilDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clearValue,
            IID_PPV_ARGS(&m_depthStencil)
        );

        if (FAILED(hr))
        {
            std::cerr << "Failed to create depth stencil buffer!" << std::endl;
            return false;
        }

        // Create depth stencil view
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
        dsvDesc.Format = m_config.depthStencilFormat;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
        dsvDesc.Texture2D.MipSlice = 0;

        m_device->CreateDepthStencilView(
            m_depthStencil.Get(),
            &dsvDesc,
            m_dsvHeap->GetCPUDescriptorHandleForHeapStart()
        );

        std::cout << "  Depth Stencil Buffer created successfully" << std::endl;
        return true;
    }

} // namespace UnoEngine::Graphics
