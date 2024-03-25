#include "logger.hpp"
#include "tickTimer.hpp"

void Logger::init(const LogType logLevel, const size_t reservedMsgSize)
{
    m_message.reserve(reservedMsgSize);
    m_logLevel = logLevel;
    std::setlocale(LC_ALL, "en_AU.UTF-8");
}

void Logger::clear()
{
    assert(ThreadX::Kernel::inThread());

    ThreadX::LockGuard lockGuard(m_mutex);
    SEGGER_RTT_WriteString(0, RTT_CTRL_CLEAR);
}

void Logger::addTime()
{
    auto [t, frac_ms]{ThreadX::TickTimer::to_localtime(ThreadX::TickTimer::now())};
    char tstr[50];
    std::strftime(tstr, 50, "x", std::addressof(t));
    m_message += tstr + std::string(".") + std::to_string(frac_ms);
    // m_message += std::put_time(std::addressof(t), "x");
}

void Logger::addColourControl(const LogType logType)
{
    switch (logType)
    {
    case LogType::error:
        m_message += RTT_CTRL_TEXT_BRIGHT_RED "ERR:  ";
        break;
    case LogType::warning:
        m_message += RTT_CTRL_TEXT_BRIGHT_YELLOW "WARN: ";
        break;
    case LogType::info:
        m_message += RTT_CTRL_TEXT_BRIGHT_GREEN "INFO: ";
        break;
    case LogType::debug:
    default:
        m_message += RTT_CTRL_TEXT_BRIGHT_MAGENTA "DBG:  ";
        break;
    }
}

void Logger::addMessage(const LogType logType, const std::string_view string)
{
    m_message += string;

    if (logType <= LogType::warning)
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
