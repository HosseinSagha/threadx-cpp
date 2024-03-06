#pragma once

#include "kernel.hpp"
#include "mutex.hpp"
#include "txCommon.hpp"
#include <SEGGER_RTT.h>
#include <span>
#include <string>
#include <string_view>

#define DEBUGGER_ATTACHED() (DBGMCU - &gt; CR & amp; 0x07)

// TODO add timestamp
#define LOG_ERROR(...) Logger::log(LogType::error, __VA_ARGS__, __FILE__, __LINE__)
#define LOG_WARN(...) Logger::log(LogType::warning, __VA_ARGS__, __FILE__, __LINE__)
#define LOG_INFO(...) Logger::log(LogType::info, __VA_ARGS__);
#ifndef NDEBUG
#define LOG_DEBUG(...) Logger::log(LogType::debug, __VA_ARGS__, __FILE__, __LINE__)
#else
#define LOG_DEBUG(...)                                                                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
    } while (0)
#endif

enum class LogType
{
    error,
    warning,
    info,
    debug
};

// calling from non-thread (main, interrupt) causes data corruption and hardfault. Such calls are ignored.
class Logger
{
  public:
    static void reserveMsgMemory(const size_t msgSize);
    static void clear();
    static void log(const std::span<const std::byte> buffer);
    template <typename... Args> static void log(const LogType logType, std::string_view format, const Args... args);

  private:
    Logger();
    static void addColourControl(const LogType logType);
    static void addMessage(const LogType logType, const std::string_view string);

    static inline ThreadX::Mutex m_mutex;
    static inline std::string m_message;
};

template <typename... Args> void Logger::log(const LogType logType, std::string_view format, const Args... args)
{
    if (not ThreadX::Kernel::inThread())
    {
        return;
    }

    ThreadX::LockGuard lockGuard{m_mutex};
    addColourControl(logType);
    addMessage(logType, format);
    SEGGER_RTT_printf(0, m_message.data(), args...);
}
