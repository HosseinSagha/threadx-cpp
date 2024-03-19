#include "logger.hpp"

void Logger::init(const LogType logLevel, const size_t reservedMsgSize)
{
    m_message.reserve(reservedMsgSize);
    m_logLevel = logLevel;
}

void Logger::clear()
{
    assert(ThreadX::Kernel::inThread());

    ThreadX::LockGuard lockGuard(m_mutex);
    SEGGER_RTT_WriteString(0, RTT_CTRL_CLEAR);
}

void Logger::addColourControl(const LogType logType)
{
    switch (logType)
    {
    case LogType::error:
        m_message = RTT_CTRL_TEXT_BRIGHT_RED "ERR:  ";
        break;
    case LogType::warning:
        m_message = RTT_CTRL_TEXT_BRIGHT_YELLOW "WARN: ";
        break;
    case LogType::info:
        m_message = RTT_CTRL_TEXT_BRIGHT_GREEN "INFO: ";
        break;
    case LogType::debug:
    default:
        m_message = RTT_CTRL_TEXT_BRIGHT_MAGENTA "DBG:  ";
        break;
    }
}

void Logger::addMessage(const LogType logType, const std::string_view string)
{
    m_message.append(string);
    if (logType != LogType::info)
    {
        m_message.append(" (%s:%d) '%s'");
    }

    m_message.push_back('\n');
}

void Logger::log(const std::span<const std::byte> buffer)
{
    assert(ThreadX::Kernel::inThread());

    ThreadX::LockGuard lockGuard{m_mutex};
    SEGGER_RTT_Write(0, buffer.data(), buffer.size());
}
