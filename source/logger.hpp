#pragma once

#include "kernel.hpp"
#include "mutex.hpp"
#include "txCommon.hpp"
#include <SEGGER_RTT.h>
#include <source_location>
#include <span>
#include <string>
#include <string_view>

enum class LogType
{
    error,
    warning,
    info,
    debug
};

// calling from non-thread (main, interrupt) causes data corruption and hardfault. Such calls are not allowed.
class Logger
{
  public:
    static void init(const LogType = LogType::warning, const size_t reservedMsgSize = 256);
    static void clear();
    template <typename... Args>
    static void log(
        const LogType logType, const std::source_location &location, const std::string_view format, const Args... args);
    static void log(const std::span<const std::byte> buffer);

    Logger() = delete;
    ~Logger() = delete;

  private:
    static void addTime();
    static void addColourControl(const LogType logType);
    static void addMessage(const LogType logType, const std::string_view string);

    static inline ThreadX::Mutex m_mutex;
    static inline std::string m_message;
    static inline LogType m_logLevel;
};

template <typename... Args>
void Logger::log(
    const LogType logType, const std::source_location &location, const std::string_view format, const Args... args)
{
    assert(ThreadX::Kernel::inThread());

    if (logType <= m_logLevel)
    {
        ThreadX::LockGuard lockGuard{m_mutex};
        m_message = RTT_CTRL_RESET;
        addTime();
        addColourControl(logType);
        addMessage(logType, format);
        SEGGER_RTT_printf(
            0, m_message.data(), args..., location.file_name(), location.line(), location.function_name());
    }
}

#define LOG_CLR() Logger::clear()
#define LOG_ERR(...) Logger::log(LogType::error, std::source_location::current(), __VA_ARGS__)
#define LOG_WARN(...) Logger::log(LogType::warning, std::source_location::current(), __VA_ARGS__)
#define LOG_INFO(...) Logger::log(LogType::info, {}, __VA_ARGS__)
#define LOG_DBG(...) Logger::log(LogType::debug, {}, __VA_ARGS__)
