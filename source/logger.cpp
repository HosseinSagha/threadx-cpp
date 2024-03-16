#include "logger.hpp"

void Logger::reserveMsgMemory(const size_t msgSize)
{
    m_message.reserve(msgSize);
}

void Logger::clear()
{
    if (not ThreadX::Kernel::inThread())
    {
        return; // If called from a non-thread. mutex cannot wait.
    }

    ThreadX::LockGuard lockGuard(m_mutex);
    SEGGER_RTT_WriteString(0, RTT_CTRL_CLEAR);
}

void Logger::addColourControl(const LogType logType)
{
    switch (logType)
    {
    case LogType::error:
        m_message = RTT_CTRL_TEXT_BRIGHT_RED "ERR: ";
        break;
    case LogType::warning:
        m_message = RTT_CTRL_TEXT_BRIGHT_YELLOW "WRN: ";
        break;
    case LogType::info:
        m_message = RTT_CTRL_TEXT_BRIGHT_GREEN "INF: ";
        break;
    case LogType::debug:
    default:
        m_message = RTT_CTRL_TEXT_BRIGHT_MAGENTA "DBG: ";
        break;
    }
}

void Logger::addMessage(const LogType logType, const std::string_view string)
{
    m_message.append(string);
    if (logType != LogType::info)
    {
        m_message.append(" (%s:%d)");
    }

    m_message.push_back('\n');
}

void Logger::log(const std::span<const std::byte> buffer)
{
    if (not ThreadX::Kernel::inThread())
    {
        return; // If called from a non-thread. mutex cannot wait.
    }

    ThreadX::LockGuard lockGuard{m_mutex};
    SEGGER_RTT_Write(0, buffer.data(), buffer.size());
}
