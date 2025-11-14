#pragma once

#include <variant>
#include <stdexcept>
#include <string>
#include <utility>

namespace UnoEngine::Core
{
    // C++23 std::expected の簡易実装（C++23未対応の場合用）
    // std::expectedが使える環境では #include <expected> を使用してください

    /// <summary>
    /// 値またはエラーを保持する型（Result型パターン）
    /// C++23 std::expected の簡易版
    /// </summary>
    template<typename T, typename E>
    class Expected
    {
    public:
        // 値を持つコンストラクタ
        constexpr Expected(const T& value) : m_data(value) {}
        constexpr Expected(T&& value) : m_data(std::move(value)) {}

        // エラーを持つコンストラクタ
        struct Unexpected
        {
            E error;
            constexpr explicit Unexpected(const E& err) : error(err) {}
            constexpr explicit Unexpected(E&& err) : error(std::move(err)) {}
        };

        constexpr Expected(const Unexpected& unexp) : m_data(unexp.error) {}
        constexpr Expected(Unexpected&& unexp) : m_data(std::move(unexp.error)) {}

        // 成功しているか確認
        [[nodiscard]] constexpr auto has_value() const noexcept -> bool
        {
            return std::holds_alternative<T>(m_data);
        }

        [[nodiscard]] constexpr explicit operator bool() const noexcept
        {
            return has_value();
        }

        // 値の取得
        [[nodiscard]] constexpr auto value() & -> T&
        {
            if (!has_value())
            {
                throw std::runtime_error("Expected does not contain a value");
            }
            return std::get<T>(m_data);
        }

        [[nodiscard]] constexpr auto value() const& -> const T&
        {
            if (!has_value())
            {
                throw std::runtime_error("Expected does not contain a value");
            }
            return std::get<T>(m_data);
        }

        [[nodiscard]] constexpr auto value() && -> T&&
        {
            if (!has_value())
            {
                throw std::runtime_error("Expected does not contain a value");
            }
            return std::move(std::get<T>(m_data));
        }

        // エラーの取得
        [[nodiscard]] constexpr auto error() const& -> const E&
        {
            if (has_value())
            {
                throw std::runtime_error("Expected contains a value, not an error");
            }
            return std::get<E>(m_data);
        }

        [[nodiscard]] constexpr auto error() && -> E&&
        {
            if (has_value())
            {
                throw std::runtime_error("Expected contains a value, not an error");
            }
            return std::move(std::get<E>(m_data));
        }

        // デフォルト値を返す
        template<typename U>
        [[nodiscard]] constexpr auto value_or(U&& default_value) const& -> T
        {
            return has_value() ? value() : static_cast<T>(std::forward<U>(default_value));
        }

        // ポインタ風アクセス
        [[nodiscard]] constexpr auto operator->() -> T*
        {
            return &value();
        }

        [[nodiscard]] constexpr auto operator->() const -> const T*
        {
            return &value();
        }

        [[nodiscard]] constexpr auto operator*() & -> T&
        {
            return value();
        }

        [[nodiscard]] constexpr auto operator*() const& -> const T&
        {
            return value();
        }

    private:
        std::variant<T, E> m_data;
    };

    // ヘルパー関数
    template<typename E>
    [[nodiscard]] constexpr auto Unexpected(E&& error) -> typename Expected<void, E>::Unexpected
    {
        return typename Expected<void, E>::Unexpected(std::forward<E>(error));
    }

    // void特殊化（エラーのみ）
    template<typename E>
    class Expected<void, E>
    {
    public:
        struct Unexpected
        {
            E error;
            constexpr explicit Unexpected(const E& err) : error(err) {}
            constexpr explicit Unexpected(E&& err) : error(std::move(err)) {}
        };

        constexpr Expected() : m_hasValue(true), m_error() {}
        constexpr Expected(const Unexpected& unexp) : m_hasValue(false), m_error(unexp.error) {}
        constexpr Expected(Unexpected&& unexp) : m_hasValue(false), m_error(std::move(unexp.error)) {}

        [[nodiscard]] constexpr auto has_value() const noexcept -> bool { return m_hasValue; }
        [[nodiscard]] constexpr explicit operator bool() const noexcept { return has_value(); }

        [[nodiscard]] constexpr auto error() const& -> const E& { return m_error; }
        [[nodiscard]] constexpr auto error() && -> E&& { return std::move(m_error); }

    private:
        bool m_hasValue;
        E m_error;
    };

    // エラー型のエイリアス
    using Error = std::string;

} // namespace UnoEngine::Core
