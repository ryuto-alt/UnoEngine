#pragma once

#include "CoreTypes.h"
#include "EntityComponentSystem.h"
#include "../Platform/Window.h"
#include "../Graphics/GraphicsDevice.h"
#include "../Renderer/RenderGraph.h"
#include <chrono>

namespace UnoEngine::Core
{
    using namespace UnoEngine::Platform;
    using namespace UnoEngine::Graphics;
    using namespace UnoEngine::Renderer;

    // ========================================
    // Engine Configuration
    // ========================================

    struct EngineConfig
    {
        WindowConfig windowConfig;
        GraphicsDeviceConfig graphicsConfig;
        String applicationName{ "UnoEngine Application" };
    };

    // ========================================
    // Engine Class
    // ========================================

    class Engine
    {
    public:
        Engine() = default;
        virtual ~Engine() = default;

        // Non-copyable, non-movable
        Engine(const Engine&) = delete;
        auto operator=(const Engine&) -> Engine& = delete;
        Engine(Engine&&) = delete;
        auto operator=(Engine&&) -> Engine& = delete;

        // ========================================
        // Lifecycle
        // ========================================

        auto Initialize(const EngineConfig& config) -> bool;
        auto Run() -> int32;
        auto Shutdown() -> void;

        // ========================================
        // Virtual Methods (Override in derived classes)
        // ========================================

        virtual auto OnInitialize() -> bool { return true; }
        virtual auto OnUpdate(float deltaTime) -> void {}
        virtual auto OnRender() -> void {}
        virtual auto OnShutdown() -> void {}

        // ========================================
        // Getters
        // ========================================

        [[nodiscard]] auto GetWindow() noexcept -> Window&
        {
            return m_window;
        }

        [[nodiscard]] auto GetGraphicsDevice() noexcept -> GraphicsDevice&
        {
            return m_graphicsDevice;
        }

        [[nodiscard]] auto GetECSCoordinator() noexcept -> ECS::Coordinator&
        {
            return m_ecsCoordinator;
        }

        [[nodiscard]] auto GetRenderGraph() noexcept -> RenderGraph&
        {
            return m_renderGraph;
        }

        [[nodiscard]] auto GetDeltaTime() const noexcept -> float
        {
            return m_deltaTime;
        }

        [[nodiscard]] auto GetFPS() const noexcept -> float
        {
            return m_fps;
        }

        [[nodiscard]] auto IsRunning() const noexcept -> bool
        {
            return m_isRunning;
        }

        // ========================================
        // Control Methods
        // ========================================

        auto RequestExit() -> void
        {
            m_isRunning = false;
        }

    protected:
        // ========================================
        // Core Systems
        // ========================================

        Window m_window;
        GraphicsDevice m_graphicsDevice;
        ECS::Coordinator m_ecsCoordinator;
        RenderGraph m_renderGraph;

        // ========================================
        // Timing
        // ========================================

        using Clock = std::chrono::high_resolution_clock;
        using TimePoint = std::chrono::time_point<Clock>;

        TimePoint m_lastFrameTime;
        float m_deltaTime{ 0.0f };
        float m_fps{ 0.0f };
        float m_frameTimeAccumulator{ 0.0f };
        uint32 m_frameCount{ 0 };

        // ========================================
        // State
        // ========================================

        bool m_isInitialized{ false };
        bool m_isRunning{ false };

    private:
        // ========================================
        // Internal Methods
        // ========================================

        auto InitializeWindow(const WindowConfig& config) -> bool;
        auto InitializeGraphics(const GraphicsDeviceConfig& config) -> bool;
        auto InitializeECS() -> void;
        auto InitializeRenderGraph() -> void;

        auto UpdateTiming() -> void;
        auto ProcessWindowEvents() -> bool;

        auto BeginFrame() -> void;
        auto Update() -> void;
        auto Render() -> void;
        auto EndFrame() -> void;
    };

} // namespace UnoEngine::Core
