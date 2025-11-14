#include "..\..\Include\Platform\Window.h"
#include <stdexcept>

namespace UnoEngine::Platform
{
    // ========================================
    // Static Members
    // ========================================

    static bool s_windowClassRegistered = false;

    // ========================================
    // Constructor / Destructor
    // ========================================

    Window::~Window()
    {
        Destroy();
    }

    Window::Window(Window&& other) noexcept
        : m_hwnd(other.m_hwnd)
        , m_hinstance(other.m_hinstance)
        , m_title(std::move(other.m_title))
        , m_width(other.m_width)
        , m_height(other.m_height)
        , m_isFullscreen(other.m_isFullscreen)
        , m_isFocused(other.m_isFocused)
        , m_isMinimized(other.m_isMinimized)
        , m_isResizable(other.m_isResizable)
        , m_callbacks(std::move(other.m_callbacks))
    {
        other.m_hwnd = nullptr;
        other.m_hinstance = nullptr;
    }

    auto Window::operator=(Window&& other) noexcept -> Window&
    {
        if (this != &other)
        {
            Destroy();

            m_hwnd = other.m_hwnd;
            m_hinstance = other.m_hinstance;
            m_title = std::move(other.m_title);
            m_width = other.m_width;
            m_height = other.m_height;
            m_isFullscreen = other.m_isFullscreen;
            m_isFocused = other.m_isFocused;
            m_isMinimized = other.m_isMinimized;
            m_isResizable = other.m_isResizable;
            m_callbacks = std::move(other.m_callbacks);

            other.m_hwnd = nullptr;
            other.m_hinstance = nullptr;
        }
        return *this;
    }

    // ========================================
    // Window Creation
    // ========================================

    auto Window::Create(const WindowConfig& config) -> bool
    {
        m_hinstance = GetModuleHandle(nullptr);
        m_title = config.title;
        m_width = config.width;
        m_height = config.height;
        m_isFullscreen = config.fullscreen;
        m_isResizable = config.resizable;

        // Register window class if not already registered
        if (!s_windowClassRegistered)
        {
            WNDCLASSEXW wc{};
            wc.cbSize = sizeof(WNDCLASSEXW);
            wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
            wc.lpfnWndProc = WindowProc;
            wc.cbClsExtra = 0;
            wc.cbWndExtra = 0;
            wc.hInstance = m_hinstance;
            wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
            wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
            wc.hbrBackground = nullptr;
            wc.lpszMenuName = nullptr;
            wc.lpszClassName = WindowClassName;
            wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

            if (!RegisterClassExW(&wc))
            {
                return false;
            }

            s_windowClassRegistered = true;
        }

        // Calculate window size
        DWORD style = WS_OVERLAPPEDWINDOW;
        if (!m_isResizable)
        {
            style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
        }

        RECT windowRect = { 0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height) };
        AdjustWindowRect(&windowRect, style, FALSE);

        int windowWidth = windowRect.right - windowRect.left;
        int windowHeight = windowRect.bottom - windowRect.top;

        // Center window on screen
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        int posX = (screenWidth - windowWidth) / 2;
        int posY = (screenHeight - windowHeight) / 2;

        // Convert title to wide string
        int wideStringLength = MultiByteToWideChar(CP_UTF8, 0, m_title.c_str(), -1, nullptr, 0);
        WideString wideTitle(wideStringLength, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, m_title.c_str(), -1, wideTitle.data(), wideStringLength);

        // Create window
        m_hwnd = CreateWindowExW(
            0,
            WindowClassName,
            wideTitle.c_str(),
            style,
            posX,
            posY,
            windowWidth,
            windowHeight,
            nullptr,
            nullptr,
            m_hinstance,
            this // Pass this pointer to WM_CREATE
        );

        if (!m_hwnd)
        {
            return false;
        }

        // Show window
        ShowWindow(m_hwnd, SW_SHOW);
        UpdateWindow(m_hwnd);
        SetForegroundWindow(m_hwnd);
        SetFocus(m_hwnd);

        return true;
    }

    auto Window::Destroy() -> void
    {
        if (m_hwnd)
        {
            DestroyWindow(m_hwnd);
            m_hwnd = nullptr;
        }
    }

    // ========================================
    // Window Operations
    // ========================================

    auto Window::Show() -> void
    {
        if (m_hwnd)
        {
            ShowWindow(m_hwnd, SW_SHOW);
        }
    }

    auto Window::Hide() -> void
    {
        if (m_hwnd)
        {
            ShowWindow(m_hwnd, SW_HIDE);
        }
    }

    auto Window::Minimize() -> void
    {
        if (m_hwnd)
        {
            ShowWindow(m_hwnd, SW_MINIMIZE);
        }
    }

    auto Window::Maximize() -> void
    {
        if (m_hwnd)
        {
            ShowWindow(m_hwnd, SW_MAXIMIZE);
        }
    }

    auto Window::Restore() -> void
    {
        if (m_hwnd)
        {
            ShowWindow(m_hwnd, SW_RESTORE);
        }
    }

    auto Window::ProcessMessages() -> bool
    {
        MSG msg{};

        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                return false;
            }

            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        return true;
    }

    // ========================================
    // Setters
    // ========================================

    auto Window::SetTitle(const String& title) -> void
    {
        m_title = title;

        if (m_hwnd)
        {
            int wideStringLength = MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, nullptr, 0);
            WideString wideTitle(wideStringLength, L'\0');
            MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, wideTitle.data(), wideStringLength);

            SetWindowTextW(m_hwnd, wideTitle.c_str());
        }
    }

    auto Window::SetSize(uint32 width, uint32 height) -> void
    {
        m_width = width;
        m_height = height;

        if (m_hwnd && !m_isFullscreen)
        {
            RECT rect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
            DWORD style = static_cast<DWORD>(GetWindowLongPtr(m_hwnd, GWL_STYLE));
            AdjustWindowRect(&rect, style, FALSE);

            SetWindowPos(
                m_hwnd,
                nullptr,
                0,
                0,
                rect.right - rect.left,
                rect.bottom - rect.top,
                SWP_NOMOVE | SWP_NOZORDER
            );
        }
    }

    auto Window::SetFullscreen(bool fullscreen) -> void
    {
        if (m_isFullscreen == fullscreen)
        {
            return;
        }

        m_isFullscreen = fullscreen;

        if (fullscreen)
        {
            // Store current window placement
            WINDOWPLACEMENT placement{};
            placement.length = sizeof(WINDOWPLACEMENT);
            GetWindowPlacement(m_hwnd, &placement);

            // Get monitor info
            HMONITOR monitor = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);
            MONITORINFO monitorInfo{};
            monitorInfo.cbSize = sizeof(MONITORINFO);
            GetMonitorInfo(monitor, &monitorInfo);

            // Set fullscreen
            SetWindowLongPtr(m_hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
            SetWindowPos(
                m_hwnd,
                HWND_TOP,
                monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.top,
                monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                SWP_FRAMECHANGED | SWP_SHOWWINDOW
            );
        }
        else
        {
            // Restore windowed mode
            DWORD style = WS_OVERLAPPEDWINDOW;
            if (!m_isResizable)
            {
                style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
            }

            SetWindowLongPtr(m_hwnd, GWL_STYLE, style);

            RECT rect = { 0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height) };
            AdjustWindowRect(&rect, style, FALSE);

            int screenWidth = GetSystemMetrics(SM_CXSCREEN);
            int screenHeight = GetSystemMetrics(SM_CYSCREEN);
            int posX = (screenWidth - (rect.right - rect.left)) / 2;
            int posY = (screenHeight - (rect.bottom - rect.top)) / 2;

            SetWindowPos(
                m_hwnd,
                nullptr,
                posX,
                posY,
                rect.right - rect.left,
                rect.bottom - rect.top,
                SWP_FRAMECHANGED | SWP_SHOWWINDOW
            );
        }
    }

    // ========================================
    // Window Procedure
    // ========================================

    auto CALLBACK Window::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT
    {
        Window* window = nullptr;

        if (msg == WM_CREATE)
        {
            CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
            window = reinterpret_cast<Window*>(pCreate->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
        }
        else
        {
            window = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        }

        if (window)
        {
            return window->HandleMessage(msg, wParam, lParam);
        }

        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    auto Window::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT
    {
        switch (msg)
        {
        case WM_CLOSE:
        {
            std::cout << "[Window::WM_CLOSE] Received WM_CLOSE message" << std::endl;

            if (m_callbacks.onClose)
            {
                std::cout << "[Window::WM_CLOSE] Calling onClose callback..." << std::endl;
                m_callbacks.onClose();
                std::cout << "[Window::WM_CLOSE] onClose callback completed" << std::endl;
            }

            std::cout << "[Window::WM_CLOSE] Posting WM_QUIT..." << std::endl;
            // Post quit message directly without calling DestroyWindow
            // This allows the engine to properly shutdown and then destroy the window
            // in Engine::Shutdown() -> m_window.Destroy()
            PostQuitMessage(0);
            std::cout << "[Window::WM_CLOSE] WM_CLOSE processing complete" << std::endl;
            return 0;
        }

        case WM_DESTROY:
        {
            // If DestroyWindow is called explicitly, ensure WM_QUIT is posted
            PostQuitMessage(0);
            return 0;
        }

        case WM_SIZE:
        {
            m_width = LOWORD(lParam);
            m_height = HIWORD(lParam);

            m_isMinimized = (wParam == SIZE_MINIMIZED);

            if (m_callbacks.onResize && wParam != SIZE_MINIMIZED)
            {
                m_callbacks.onResize(m_width, m_height);
            }
            return 0;
        }

        case WM_SETFOCUS:
        {
            m_isFocused = true;
            if (m_callbacks.onFocus)
            {
                m_callbacks.onFocus(true);
            }
            return 0;
        }

        case WM_KILLFOCUS:
        {
            m_isFocused = false;
            if (m_callbacks.onFocus)
            {
                m_callbacks.onFocus(false);
            }
            return 0;
        }

        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        {
            // Input handling will be implemented later
            return 0;
        }

        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_MOUSEWHEEL:
        {
            // Mouse input handling will be implemented later
            return 0;
        }

        default:
            return DefWindowProc(m_hwnd, msg, wParam, lParam);
        }
    }

} // namespace UnoEngine::Platform
