#pragma once

#include "CoreTypes.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <memory>
#include <source_location> // C++20

namespace UnoEngine::Core
{
    /// <summary>
    /// 高速ロギングシステム（spdlog使用）
    /// C++20の source_location を活用して自動的にファイル名・行番号を記録
    /// </summary>
    class Logger
    {
    public:
        /// <summary>
        /// ロガーの初期化
        /// </summary>
        static auto Initialize() -> void
        {
            // コンソール出力用（カラー対応）
            auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            consoleSink->set_level(spdlog::level::trace);
            consoleSink->set_pattern("[%T] [%^%l%$] %v");

            // ファイル出力用
            auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("UnoEngine.log", true);
            fileSink->set_level(spdlog::level::trace);

            // 両方のシンクを持つロガー作成
            std::vector<spdlog::sink_ptr> sinks{ consoleSink, fileSink };
            s_logger = std::make_shared<spdlog::logger>("UnoEngine", sinks.begin(), sinks.end());
            s_logger->set_level(spdlog::level::trace);
            s_logger->flush_on(spdlog::level::err);

            spdlog::register_logger(s_logger);

            Info("Logger initialized successfully");
        }

        /// <summary>
        /// ロガーのシャットダウン
        /// </summary>
        static auto Shutdown() -> void
        {
            if (s_logger)
            {
                Info("Logger shutting down");
                spdlog::shutdown();
            }
        }

        // C++20 source_location を使った自動ロケーション記録
        template<typename... Args>
        static auto Trace(
            std::string_view fmt,
            Args&&... args,
            const std::source_location& loc = std::source_location::current()
        ) -> void
        {
            if (s_logger)
            {
                s_logger->trace("[{}:{}] {}",
                    loc.file_name(),
                    loc.line(),
                    fmt::format(fmt, std::forward<Args>(args)...)
                );
            }
        }

        template<typename... Args>
        static auto Debug(
            std::string_view fmt,
            Args&&... args,
            const std::source_location& loc = std::source_location::current()
        ) -> void
        {
            if (s_logger)
            {
                s_logger->debug("[{}:{}] {}",
                    loc.file_name(),
                    loc.line(),
                    fmt::format(fmt, std::forward<Args>(args)...)
                );
            }
        }

        template<typename... Args>
        static auto Info(
            std::string_view fmt,
            Args&&... args,
            const std::source_location& loc = std::source_location::current()
        ) -> void
        {
            if (s_logger)
            {
                s_logger->info("[{}:{}] {}",
                    loc.file_name(),
                    loc.line(),
                    fmt::format(fmt, std::forward<Args>(args)...)
                );
            }
        }

        template<typename... Args>
        static auto Warn(
            std::string_view fmt,
            Args&&... args,
            const std::source_location& loc = std::source_location::current()
        ) -> void
        {
            if (s_logger)
            {
                s_logger->warn("[{}:{}] {}",
                    loc.file_name(),
                    loc.line(),
                    fmt::format(fmt, std::forward<Args>(args)...)
                );
            }
        }

        template<typename... Args>
        static auto Error(
            std::string_view fmt,
            Args&&... args,
            const std::source_location& loc = std::source_location::current()
        ) -> void
        {
            if (s_logger)
            {
                s_logger->error("[{}:{}] {}",
                    loc.file_name(),
                    loc.line(),
                    fmt::format(fmt, std::forward<Args>(args)...)
                );
            }
        }

        template<typename... Args>
        static auto Critical(
            std::string_view fmt,
            Args&&... args,
            const std::source_location& loc = std::source_location::current()
        ) -> void
        {
            if (s_logger)
            {
                s_logger->critical("[{}:{}] {}",
                    loc.file_name(),
                    loc.line(),
                    fmt::format(fmt, std::forward<Args>(args)...)
                );
            }
        }

        [[nodiscard]] static auto GetLogger() -> std::shared_ptr<spdlog::logger>
        {
            return s_logger;
        }

    private:
        inline static std::shared_ptr<spdlog::logger> s_logger;
    };

} // namespace UnoEngine::Core
