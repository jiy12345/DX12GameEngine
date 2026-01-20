/**
 * @file Logger.cpp
 * @brief 로깅 시스템 구현
 */

#include "Logger.h"
#include <Core/BuildConfig.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <iostream>

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

    void Logger::Initialize(LogLevel minLevel, bool logToFile, const std::wstring& logFilePrefix)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_initialized)
        {
            return;
        }

        m_minLevel = minLevel;
        m_logToFile = logToFile;

        // 콘솔 로그 활성화 시 콘솔 창 할당
        if constexpr (BUILD_DEFAULT(LogToConsole))
        {
            if (AllocConsole())
            {
                FILE* fp = nullptr;
                freopen_s(&fp, "CONOUT$", "w", stdout);
                freopen_s(&fp, "CONOUT$", "w", stderr);
                std::wcout.clear();
                std::wcerr.clear();
            }
        }

        if (logToFile)
        {
            // 실행 파일 경로에서 Build/Logs 폴더 경로 계산
            wchar_t exePath[MAX_PATH];
            GetModuleFileNameW(nullptr, exePath, MAX_PATH);

            std::filesystem::path logsDir = std::filesystem::path(exePath).parent_path().parent_path().parent_path() / L"Logs";
            std::filesystem::create_directories(logsDir);

            // 타임스탬프 파일명 생성
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            std::tm localTime;
            localtime_s(&localTime, &time);

            std::wstringstream ss;
            ss << std::put_time(&localTime, L"%Y-%m-%d_%H-%M-%S");
            std::wstring timestamp = ss.str();

            std::filesystem::path fullLogPath = logsDir / (logFilePrefix + L"_" + timestamp + L".log");

            m_logFile.open(fullLogPath, std::ios::out | std::ios::trunc);
            if (!m_logFile.is_open())
            {
                std::wcerr << L"[Logger] Warning: Failed to open log file\n";
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

        // 콘솔 출력 (Debug 빌드)
        if constexpr (BUILD_DEFAULT(LogToConsole))
        {
            std::wcout << formattedMessage;
        }
        else
        {
            // Release 빌드: IDE 디버그 출력 창
            OutputDebugStringW(formattedMessage.c_str());
        }

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
