#include "..\..\Include\Core\Engine.h"
#include <iostream>

namespace UnoEngine::Core
{
    // ========================================
    // Initialization
    // ========================================

    auto Engine::Initialize(const EngineConfig& config) -> bool
    {
        if (m_isInitialized)
        {
            std::cerr << "Engine already initialized!" << std::endl;
            return false;
        }

        std::cout << "Initializing " << config.applicationName << "..." << std::endl;

        // Initialize window
        if (!InitializeWindow(config.windowConfig))
        {
            std::cerr << "Failed to initialize window!" << std::endl;
            return false;
        }

        // Initialize graphics device
        if (!InitializeGraphics(config.graphicsConfig))
        {
            std::cerr << "Failed to initialize graphics device!" << std::endl;
            return false;
        }

        // Initialize ECS
        InitializeECS();

        // Initialize render graph
        InitializeRenderGraph();

        // Call user initialization
        if (!OnInitialize())
        {
            std::cerr << "User initialization failed!" << std::endl;
            return false;
        }

        m_isInitialized = true;
        m_lastFrameTime = Clock::now();

        std::cout << "Engine initialized successfully!" << std::endl;
        return true;
    }

    auto Engine::Run() -> int32
    {
        if (!m_isInitialized)
        {
            std::cerr << "Engine not initialized!" << std::endl;
            return -1;
        }

        m_isRunning = true;

        // Main loop
        while (m_isRunning)
        {
            // Process window events
            if (!ProcessWindowEvents())
            {
                std::cout << "[Engine::Run] Window closed, exiting main loop..." << std::endl;
                break;
            }

            // Update timing
            UpdateTiming();

            // Begin frame
            BeginFrame();

            // Update
            Update();

            // Render
            Render();

            // End frame
            EndFrame();
        }

        std::cout << "[Engine::Run] Main loop exited, returning to main()..." << std::endl;
        return 0;
    }

    auto Engine::Shutdown() -> void
    {
        if (!m_isInitialized)
        {
            return;
        }

        std::cout << "Shutting down engine..." << std::endl;

        // Call user shutdown
        OnShutdown();

        // Shutdown systems in correct order:
        // 1. RenderGraph must be cleared BEFORE GraphicsDevice shutdown
        //    (RenderGraph holds references to BackBuffer from GraphicsDevice)
        // 2. GraphicsDevice shutdown (waits for GPU, releases all D3D12 resources)
        // 3. Window destruction
        m_renderGraph.Reset();
        m_graphicsDevice.Shutdown();
        m_window.Destroy();

        m_isInitialized = false;
        m_isRunning = false;

        std::cout << "Engine shutdown complete." << std::endl;
    }

    // ========================================
    // Internal Initialization Methods
    // ========================================

    auto Engine::InitializeWindow(const WindowConfig& config) -> bool
    {
        // Set up window callbacks
        WindowCallbacks callbacks;

        callbacks.onResize = [this](uint32 width, uint32 height)
        {
            std::cout << "Window resized: " << width << "x" << height << std::endl;
            // Handle resize (recreate swap chain buffers, etc.)
        };

        callbacks.onClose = [this]()
        {
            std::cout << "[Engine::onClose] Window close requested." << std::endl;
            std::cout << "[Engine::onClose] Calling RequestExit()..." << std::endl;
            RequestExit();
            std::cout << "[Engine::onClose] RequestExit() completed" << std::endl;
        };

        callbacks.onFocus = [this](bool focused)
        {
            if (focused)
            {
                std::cout << "Window gained focus." << std::endl;
            }
            else
            {
                std::cout << "Window lost focus." << std::endl;
            }
        };

        m_window.SetCallbacks(callbacks);

        return m_window.Create(config);
    }

    auto Engine::InitializeGraphics(const GraphicsDeviceConfig& config) -> bool
    {
        void* windowHandle = m_window.GetHandle();
        return m_graphicsDevice.Initialize(config, windowHandle);
    }

    auto Engine::InitializeECS() -> void
    {
        m_ecsCoordinator.Initialize();
        std::cout << "ECS initialized." << std::endl;
    }

    auto Engine::InitializeRenderGraph() -> void
    {
        // Render graph will be set up by the user in OnInitialize
        std::cout << "Render graph ready." << std::endl;
    }

    // ========================================
    // Frame Processing
    // ========================================

    auto Engine::UpdateTiming() -> void
    {
        TimePoint currentTime = Clock::now();
        std::chrono::duration<float> deltaTimeDuration = currentTime - m_lastFrameTime;
        m_deltaTime = deltaTimeDuration.count();
        m_lastFrameTime = currentTime;

        // Calculate FPS
        m_frameTimeAccumulator += m_deltaTime;
        ++m_frameCount;

        if (m_frameTimeAccumulator >= 1.0f)
        {
            m_fps = static_cast<float>(m_frameCount) / m_frameTimeAccumulator;
            m_frameCount = 0;
            m_frameTimeAccumulator = 0.0f;

            // Update window title with FPS
            String title = m_window.GetTitle();
            size_t fpsPos = title.find(" - FPS:");
            if (fpsPos != String::npos)
            {
                title = title.substr(0, fpsPos);
            }

            title += " - FPS: " + std::to_string(static_cast<int>(m_fps));
            m_window.SetTitle(title);
        }
    }

    auto Engine::ProcessWindowEvents() -> bool
    {
        return m_window.ProcessMessages();
    }

    auto Engine::BeginFrame() -> void
    {
        m_graphicsDevice.BeginFrame();
    }

    auto Engine::Update() -> void
    {
        OnUpdate(m_deltaTime);
    }

    auto Engine::Render() -> void
    {
        OnRender();
    }

    auto Engine::EndFrame() -> void
    {
        m_graphicsDevice.EndFrame();
        m_graphicsDevice.Present();
    }

} // namespace UnoEngine::Core
