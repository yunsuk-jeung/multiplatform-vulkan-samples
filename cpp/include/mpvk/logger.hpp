#pragma once

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace mpvk {

class Logger {
public:
  // Default location for log files. On desktop this is project-root/logs (when
  // built with MPVK_SOURCE_DIR) or cwd-relative "logs" otherwise. On sandboxed
  // platforms (e.g. iOS) the host app must pass a writable directory to init().
  static std::filesystem::path default_log_dir() {
#ifdef MPVK_SOURCE_DIR
    return std::filesystem::path(MPVK_SOURCE_DIR) / "logs";
#else
    return std::filesystem::path("logs");
#endif
  }

  // Initialize logging. Pass log_dir to write files to a platform-appropriate
  // writable location (required on iOS: e.g. NSDocumentDirectory).
  static void init(bool                         enable_file_logging,
                   const std::filesystem::path& log_dir) {
    if (logger_) {
      return;
    }

    if (enable_file_logging) {
      // Create sinks for both console and file output. If the directory cannot
      // be created (e.g. sandbox/permission issues), fall back to console-only
      // so logging never aborts the application.
      auto
        console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

      std::string                                        log_file;
      std::shared_ptr<spdlog::sinks::basic_file_sink_mt> file_sink;
      try {
        std::filesystem::create_directories(log_dir);

        // Generate filename with current timestamp.
        auto              now  = std::chrono::system_clock::now();
        auto              time = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << "mpvk_" << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S")
           << ".log";
        log_file = (log_dir / ss.str()).string();

        file_sink = std::make_shared<
          spdlog::sinks::basic_file_sink_mt>(log_file, true);
      } catch (const std::exception&) {
        file_sink.reset();
      }

      std::vector<spdlog::sink_ptr> sinks{console_sink};
      if (file_sink) {
        sinks.push_back(file_sink);
      }
      logger_ = std::make_shared<spdlog::logger>("mpvk",
                                                 sinks.begin(),
                                                 sinks.end());
      logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [mpvk] [%^%l%$] %v");

      if (file_sink) {
        logger_->info("Logging to file: {}", log_file);
      }
      else {
        logger_->warn("File logging disabled (could not open log dir: {})",
                      log_dir.string());
      }
    }
    else {
      logger_ = spdlog::default_logger();
    }
    spdlog::set_default_logger(logger_);
  }

  static void init(bool enable_file_logging = true) {
    init(enable_file_logging, default_log_dir());
  }

  // debug/info/warn take a *runtime* format string. spdlog's typed log methods
  // expect a compile-time-checked format string, so the runtime string is
  // wrapped in SPDLOG_FMT_RUNTIME() (spdlog's mode-agnostic macro: fmt::runtime
  // under bundled fmt, a plain pass-through under std::format).
  template <typename... Args>
  static void debug(const char* fmt, Args&&... args) {
    init();
    logger_->debug(SPDLOG_FMT_RUNTIME(fmt), std::forward<Args>(args)...);
  }

  template <typename... Args>
  static void info(const char* fmt, Args&&... args) {
    init();
    logger_->info(SPDLOG_FMT_RUNTIME(fmt), std::forward<Args>(args)...);
  }

  template <typename... Args>
  static void warn(const char* fmt, Args&&... args) {
    init();
    logger_->warn(SPDLOG_FMT_RUNTIME(fmt), std::forward<Args>(args)...);
  }

  template <typename T, typename... Args>
  static void error(const char* file, int line, const T& fmt, Args&&... args) {
    init();
    const std::string_view fmt_view(fmt);
    if constexpr (sizeof...(args) == 0) {
      logger_->error("[{}:{}] {}", file, line, fmt_view);
    }
    else {
      // Format the user message ourselves via spdlog::fmt_lib (= std or fmt
      // depending on the build), then log the finished string through a literal
      // format so no runtime format string reaches spdlog's typed API.
      auto arg_tuple = std::tuple<std::decay_t<Args>...>(
        std::forward<Args>(args)...);
      auto formatted = std::apply(
        [&](auto&... unpacked) {
          return spdlog::fmt_lib::vformat(fmt_view,
                                          spdlog::fmt_lib::make_format_args(
                                            unpacked...));
        },
        arg_tuple);
      logger_->error("[{}:{}] {}", file, line, formatted);
    }
  }

  static const char* extract_file_name(const char* path) {
    if (!path) {
      return "";
    }
    const char* last_slash = path;
    for (const char* p = path; *p != '\0'; ++p) {
      if (*p == '/' || *p == '\\') {
        last_slash = p + 1;
      }
    }
    return last_slash;
  }

private:
  static inline std::shared_ptr<spdlog::logger> logger_;
};

}  // namespace mpvk

// Trailing semicolons are intentionally omitted so call sites read
// `LogI(...);`.
#define LogD(fmt, ...) mpvk::Logger::debug(fmt, ##__VA_ARGS__)
#define LogI(fmt, ...) mpvk::Logger::info(fmt, ##__VA_ARGS__)
#define LogW(fmt, ...) mpvk::Logger::warn(fmt, ##__VA_ARGS__)
#define LogE(fmt, ...)                                                         \
  mpvk::Logger::error(mpvk::Logger::extract_file_name(__FILE__),               \
                      __LINE__,                                                \
                      fmt,                                                     \
                      ##__VA_ARGS__)
#define DEBUG_POINT() LogE("THIS Line is for debugging")
