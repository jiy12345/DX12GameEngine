/**
 * @file Logger.cpp
 * @brief 로깅 시스템 구현
 */

#include "Logger.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace DX12GameEngine
{
    Logger& Logger::Get()
    {
        static Logger instance;
        return instance;
    }

    Logger::~Logger()
    {
        Shutdown();
    }

    void Logger::Initialize(LogLevel minLevel, bool logToFile, const std::wstring& logFilePath)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_initialized)
        {
            return;
        }

        m_minLevel = minLevel;
        m_logToFile = logToFile;

        if (logToFile)
        {
            m_logFile.open(logFilePath, std::ios::out | std::ios::trunc);
            if (!m_logFile.is_open())
            {
                OutputDebugStringW(L"[Logger] Warning: Failed to open log file\n");
                m_logToFile = false;
            }
        }

        m_initialized = true;
    }

    void Logger::Shutdown()
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (!m_initialized)
        {
            return;
        }

        if (m_logFile.is_open())
        {
            m_logFile.close();
        }

        m_initialized = false;
    }

    void Logger::Log(LogLevel level, LogCategory category, const std::wstring& message)
    {
        if (level < m_minLevel)
        {
            return;
        }

        std::wstring formattedMessage = FormatLogMessage(level, category, message);

        std::lock_guard<std::mutex> lock(m_mutex);

        // OutputDebugStringW 출력
        OutputDebugStringW(formattedMessage.c_str());

        // 파일 출력
        if (m_logToFile && m_logFile.is_open())
        {
            m_logFile << formattedMessage;
            m_logFile.flush();
        }
    }

    std::wstring Logger::FormatLogMessage(LogLevel level, LogCategory category, const std::wstring& message)
    {
        // 포맷: [2026-01-20 22:30:15.123][INFO ][Engine  ] Message
        std::wstringstream ss;
        ss << L"[" << GetTimestamp() << L"]"
           << L"[" << LogLevelToString(level) << L"]"
           << L"[" << LogCategoryToString(category) << L"] "
           << message << L"\n";
        return ss.str();
    }

    const wchar_t* Logger::LogLevelToString(LogLevel level)
    {
        // 5자 고정 폭
        switch (level)
        {
        case LogLevel::Trace:   return L"TRACE";
        case LogLevel::Debug:   return L"DEBUG";
        case LogLevel::Info:    return L"INFO ";
        case LogLevel::Warning: return L"WARN ";
        case LogLevel::Error:   return L"ERROR";
        case LogLevel::Fatal:   return L"FATAL";
        default:                return L"?????";
        }
    }

    const wchar_t* Logger::LogCategoryToString(LogCategory category)
    {
        // 8자 고정 폭
        switch (category)
        {
        case LogCategory::Engine:   return L"Engine  ";
        case LogCategory::Renderer: return L"Renderer";
        case LogCategory::Device:   return L"Device  ";
        case LogCategory::Window:   return L"Window  ";
        case LogCategory::Input:    return L"Input   ";
        case LogCategory::Resource: return L"Resource";
        case LogCategory::Shader:   return L"Shader  ";
        case LogCategory::Memory:   return L"Memory  ";
        case LogCategory::Core:     return L"Core    ";
        default:                    return L"????????";
        }
    }

    std::wstring Logger::GetTimestamp()
    {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::tm localTime;
        localtime_s(&localTime, &time);

        std::wstringstream ss;
        ss << std::put_time(&localTime, L"%Y-%m-%d %H:%M:%S")
           << L"." << std::setfill(L'0') << std::setw(3) << ms.count();
        return ss.str();
    }
}
