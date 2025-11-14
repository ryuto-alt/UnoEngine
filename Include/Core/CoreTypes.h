#pragma once

#include <cstdint>
#include <concepts>
#include <memory>
#include <string>
#include <string_view>
#include <cassert>
#include <iostream>

namespace UnoEngine::Core
{
    // ========================================
    // Basic Type Definitions
    // ========================================

    using int8    = std::int8_t;
    using int16   = std::int16_t;
    using int32   = std::int32_t;
    using int64   = std::int64_t;

    using uint8   = std::uint8_t;
    using uint16  = std::uint16_t;
    using uint32  = std::uint32_t;
    using uint64  = std::uint64_t;

    using float32 = float;
    using float64 = double;

    // ========================================
    // Smart Pointer Type Aliases
    // ========================================

    template<typename T>
    using UniquePtr = std::unique_ptr<T>;

    template<typename T>
    using SharedPtr = std::shared_ptr<T>;

    template<typename T>
    using WeakPtr = std::weak_ptr<T>;

    // ========================================
    // String Type Aliases
    // ========================================

    using String     = std::string;
    using StringView = std::string_view;
    using WideString = std::wstring;

    // ========================================
    // C++20 Concepts for Type Safety
    // ========================================

    // Numeric type concept
    template<typename T>
    concept Numeric = std::integral<T> || std::floating_point<T>;

    // Pointer-like concept
    template<typename T>
    concept PointerLike = std::is_pointer_v<T> ||
                          requires(T ptr) {
                              { ptr.get() } -> std::convertible_to<typename T::element_type*>;
                          };

    // Resource concept (must be movable but not copyable)
    template<typename T>
    concept Resource = std::movable<T> && !std::copyable<T>;

    // Hashable concept
    template<typename T>
    concept Hashable = requires(T value) {
        { std::hash<T>{}(value) } -> std::convertible_to<std::size_t>;
    };

    // ========================================
    // Utility Functions
    // ========================================

    // Create unique pointer with custom deleter
    template<typename T, typename... Args>
    [[nodiscard]] constexpr auto MakeUnique(Args&&... args) -> UniquePtr<T>
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    // Create shared pointer
    template<typename T, typename... Args>
    [[nodiscard]] constexpr auto MakeShared(Args&&... args) -> SharedPtr<T>
    {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

} // namespace UnoEngine::Core

// ========================================
// Global Type Aliases (for convenience)
// ========================================

using int8    = UnoEngine::Core::int8;
using int16   = UnoEngine::Core::int16;
using int32   = UnoEngine::Core::int32;
using int64   = UnoEngine::Core::int64;

using uint8   = UnoEngine::Core::uint8;
using uint16  = UnoEngine::Core::uint16;
using uint32  = UnoEngine::Core::uint32;
using uint64  = UnoEngine::Core::uint64;

using float32 = UnoEngine::Core::float32;
using float64 = UnoEngine::Core::float64;

// ========================================
// Debug Macros
// ========================================

#ifdef _DEBUG
    #define UNO_ASSERT(condition, message) \
        do { \
            if (!(condition)) { \
                std::cerr << "ASSERTION FAILED: " << #condition << std::endl; \
                std::cerr << "Message: " << message << std::endl; \
                std::cerr << "File: " << __FILE__ << ", Line: " << __LINE__ << std::endl; \
                assert(condition); \
            } \
        } while(false)

    #define UNO_ASSERT_NOT_NULL(ptr, name) \
        UNO_ASSERT((ptr) != nullptr, name " is nullptr")

    #define UNO_ASSERT_RANGE(index, size, name) \
        UNO_ASSERT((index) < (size), name " index out of range")

    #define UNO_DEBUG_LOG(message) \
        std::cout << "[DEBUG] " << message << std::endl
#else
    #define UNO_ASSERT(condition, message) ((void)0)
    #define UNO_ASSERT_NOT_NULL(ptr, name) ((void)0)
    #define UNO_ASSERT_RANGE(index, size, name) ((void)0)
    #define UNO_DEBUG_LOG(message) ((void)0)
#endif
