#include "logger.hpp"
#include "tickTimer.hpp"

void Logger::init(const Type logLevel, const size_t reservedMsgSize)
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

void Logger::addTime()
{
    const auto [t, frac_ms]{ThreadX::TickTimer::to_time_t(ThreadX::TickTimer::now())};
    m_message += std::to_string(t) + std::string(".") + std::to_string(frac_ms) + " ";
}

void Logger::addColourControl(const Type logType)
{
    switch (logType)
    {
    case Type::error:
        m_message += RTT_CTRL_TEXT_BRIGHT_RED "ERR : ";
        break;
    case Type::warning:
        m_message += RTT_CTRL_TEXT_BRIGHT_YELLOW "WARN: ";
        break;
    case Type::info:
        m_message += RTT_CTRL_TEXT_BRIGHT_GREEN "INFO: ";
        break;
    case Type::debug:
    default:
        m_message += RTT_CTRL_TEXT_BRIGHT_MAGENTA "DBG : ";
        break;
    }
}

void Logger::addMessage(const Type logType, const std::string_view string)
{
    m_message += string;

    if (logType <= Type::warning)
    {
        m_message += " (%s:%d) '%s'";
    }

    m_message += '\n';
}

void Logger::log(const std::span<const std::byte> buffer)
{
    assert(ThreadX::Kernel::inThread());

    ThreadX::LockGuard lockGuard{m_mutex};
    SEGGER_RTT_Write(0, buffer.data(), buffer.size());
}
