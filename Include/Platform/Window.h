#pragma once

#include "../Core/CoreTypes.h"
#include <Windows.h>
#include <functional>

namespace UnoEngine::Platform
{
    using namespace UnoEngine::Core;

    // ========================================
    // Window Configuration
    // ========================================

    struct WindowConfig
    {
        String title{ "UnoEngine" };
        uint32 width{ 1280 };
        uint32 height{ 720 };
        bool fullscreen{ false };
        bool resizable{ true };
        bool vsync{ true };
    };

    // ========================================
    // Window Events
    // ========================================

    using WindowResizeCallback = std::function<void(uint32, uint32)>;
    using WindowCloseCallback = std::function<void()>;
    using WindowFocusCallback = std::function<void(bool)>;

    struct WindowCallbacks
    {
        WindowResizeCallback onResize;
        WindowCloseCallback onClose;
        WindowFocusCallback onFocus;
    };

    // ========================================
    // Window Class
    // ========================================

    class Window
    {
    public:
        Window() = default;
        ~Window();

        // Non-copyable, but movable
        Window(const Window&) = delete;
        auto operator=(const Window&) -> Window& = delete;
        Window(Window&&) noexcept;
        auto operator=(Window&&) noexcept -> Window&;

        // ========================================
        // Initialization
        // ========================================

        auto Create(const WindowConfig& config) -> bool;
        auto Destroy() -> void;

        // ========================================
        // Window Operations
        // ========================================

        auto Show() -> void;
        auto Hide() -> void;
        auto Minimize() -> void;
        auto Maximize() -> void;
        auto Restore() -> void;

        [[nodiscard]] auto ProcessMessages() -> bool;

        // ========================================
        // Getters
        // ========================================

        [[nodiscard]] auto GetHandle() const noexcept -> HWND
        {
            return m_hwnd;
        }

        [[nodiscard]] auto GetWidth() const noexcept -> uint32
        {
            return m_width;
        }

        [[nodiscard]] auto GetHeight() const noexcept -> uint32
        {
            return m_height;
        }

        [[nodiscard]] auto GetTitle() const noexcept -> const String&
        {
            return m_title;
        }

        [[nodiscard]] auto IsFullscreen() const noexcept -> bool
        {
            return m_isFullscreen;
        }

        [[nodiscard]] auto IsFocused() const noexcept -> bool
        {
            return m_isFocused;
        }

        [[nodiscard]] auto IsMinimized() const noexcept -> bool
        {
            return m_isMinimized;
        }

        // ========================================
        // Setters
        // ========================================

        auto SetTitle(const String& title) -> void;
        auto SetSize(uint32 width, uint32 height) -> void;
        auto SetFullscreen(bool fullscreen) -> void;

        // ========================================
        // Callbacks
        // ========================================

        auto SetCallbacks(const WindowCallbacks& callbacks) -> void
        {
            m_callbacks = callbacks;
        }

    private:
        // ========================================
        // Window Procedure
        // ========================================

        static auto CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT;

        auto HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT;

        // ========================================
        // Member Variables
        // ========================================

        HWND m_hwnd{ nullptr };
        HINSTANCE m_hinstance{ nullptr };
        String m_title;
        uint32 m_width{ 0 };
        uint32 m_height{ 0 };
        bool m_isFullscreen{ false };
        bool m_isFocused{ true };
        bool m_isMinimized{ false };
        bool m_isResizable{ true };

        WindowCallbacks m_callbacks;

        static constexpr const wchar_t* WindowClassName = L"UnoEngineWindowClass";
    };

} // namespace UnoEngine::Platform
