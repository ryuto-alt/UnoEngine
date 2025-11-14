#pragma once

#pragma once

#include "CoreTypes.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <memory>

namespace UnoEngine::Core
{
    /// <summary>
    /// 高速ロギングシステム（spdlog使用）
    /// </summary>
    class Logger
    {
    public:
        static auto Initialize() -> void
        {
            auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            consoleSink->set_level(spdlog::level::trace);
            consoleSink->set_pattern("[%T] [%^%l%$] %v");

            auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("UnoEngine.log", true);
            fileSink->set_level(spdlog::level::trace);

            std::vector<spdlog::sink_ptr> sinks{ consoleSink, fileSink };
            s_logger = std::make_shared<spdlog::logger>("UnoEngine", sinks.begin(), sinks.end());
            s_logger->set_level(spdlog::level::trace);
            s_logger->flush_on(spdlog::level::err);

            spdlog::register_logger(s_logger);

            Info("Logger initialized successfully");
        }

        static auto Shutdown() -> void
        {
            if (s_logger)
            {
                Info("Logger shutting down");
                spdlog::shutdown();
            }
        }

        // 文字列のみ（フォーマットなし）
        static auto Trace(std::string_view msg) -> void
        {
            if (s_logger) s_logger->trace(msg);
        }

        static auto Debug(std::string_view msg) -> void
        {
            if (s_logger) s_logger->debug(msg);
        }

        static auto Info(std::string_view msg) -> void
        {
            if (s_logger) s_logger->info(msg);
        }

        static auto Warn(std::string_view msg) -> void
        {
            if (s_logger) s_logger->warn(msg);
        }

        static auto Error(std::string_view msg) -> void
        {
            if (s_logger) s_logger->error(msg);
        }

        static auto Critical(std::string_view msg) -> void
        {
            if (s_logger) s_logger->critical(msg);
        }

        // フォーマット付き（spdlogのフォーマットを使用）
        template<typename... Args>
        static auto Trace(spdlog::format_string_t<Args...> fmt, Args&&... args) -> void
        {
            if (s_logger) s_logger->trace(fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        static auto Debug(spdlog::format_string_t<Args...> fmt, Args&&... args) -> void
        {
            if (s_logger) s_logger->debug(fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        static auto Info(spdlog::format_string_t<Args...> fmt, Args&&... args) -> void
        {
            if (s_logger) s_logger->info(fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        static auto Warn(spdlog::format_string_t<Args...> fmt, Args&&... args) -> void
        {
            if (s_logger) s_logger->warn(fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        static auto Error(spdlog::format_string_t<Args...> fmt, Args&&... args) -> void
        {
            if (s_logger) s_logger->error(fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        static auto Critical(spdlog::format_string_t<Args...> fmt, Args&&... args) -> void
        {
            if (s_logger) s_logger->critical(fmt, std::forward<Args>(args)...);
        }

        [[nodiscard]] static auto GetLogger() -> std::shared_ptr<spdlog::logger>
        {
            return s_logger;
        }

    private:
        inline static std::shared_ptr<spdlog::logger> s_logger;
    };

} // namespace UnoEngine::Core

// ========================================
// Global Type Alias (for convenience)
// ========================================

using Logger = UnoEngine::Core::Logger;
