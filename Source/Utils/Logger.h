/**
 * @file Logger.h
 * @brief 로깅 시스템
 *
 * OutputDebugStringW 기반의 체계적인 로깅 시스템을 제공합니다.
 * 로그 레벨과 카테고리에 따른 필터링, 타임스탬프, 파일 출력을 지원합니다.
 */

#pragma once

#include <string>
#include <format>
#include <mutex>
#include <fstream>

namespace DX12GameEngine
{
    /**
     * @brief 로그 레벨
     */
    enum class LogLevel : uint8_t
    {
        Trace,      // 가장 상세한 디버깅 정보
        Debug,      // 디버깅 정보
        Info,       // 일반 정보
        Warning,    // 경고
        Error,      // 오류
        Fatal       // 치명적 오류
    };

    /**
     * @brief 로그 카테고리
     */
    enum class LogCategory : uint8_t
    {
        Engine,     // 엔진 코어
        Renderer,   // 렌더링
        Device,     // DirectX 디바이스
        Window,     // 윈도우 시스템
        Input,      // 입력 시스템
        Resource,   // 리소스 관리
        Shader,     // 셰이더
        Memory,     // 메모리 관리
        Core        // 기타 코어 시스템
    };

    /**
     * @brief 로깅 시스템 싱글톤
     *
     * 스레드 안전한 로깅을 제공합니다.
     * OutputDebugStringW와 파일 출력을 동시에 지원합니다.
     */
    class Logger
    {
    public:
        /**
         * @brief 싱글톤 인스턴스 반환
         */
        static Logger& Get();

        /**
         * @brief 로거 초기화
         * @param minLevel 최소 로그 레벨 (이 레벨 이상만 출력)
         * @param logToFile 파일 출력 활성화 여부
         * @param logFilePath 로그 파일 경로 (기본값: "Engine.log")
         */
        void Initialize(LogLevel minLevel, bool logToFile = true, const std::wstring& logFilePath = L"Engine.log");

        /**
         * @brief 로거 종료
         */
        void Shutdown();

        /**
         * @brief 로그 메시지 출력
         * @param level 로그 레벨
         * @param category 로그 카테고리
         * @param message 메시지
         */
        void Log(LogLevel level, LogCategory category, const std::wstring& message);

        /**
         * @brief 포맷된 로그 메시지 출력
         * @tparam Args 포맷 인자 타입들
         * @param level 로그 레벨
         * @param category 로그 카테고리
         * @param fmt 포맷 문자열
         * @param args 포맷 인자들
         */
        template<typename... Args>
        void Log(LogLevel level, LogCategory category, std::wformat_string<Args...> fmt, Args&&... args)
        {
            if (level < m_minLevel)
            {
                return;
            }

            std::wstring message = std::format(fmt, std::forward<Args>(args)...);
            Log(level, category, message);
        }

        /**
         * @brief 최소 로그 레벨 설정
         */
        void SetMinLevel(LogLevel level) { m_minLevel = level; }

        /**
         * @brief 현재 최소 로그 레벨 반환
         */
        LogLevel GetMinLevel() const { return m_minLevel; }

    private:
        Logger() = default;
        ~Logger();

        // 복사 및 이동 금지
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;
        Logger(Logger&&) = delete;
        Logger& operator=(Logger&&) = delete;

        /**
         * @brief 로그 메시지 포맷팅
         * @return 포맷된 전체 로그 문자열
         */
        std::wstring FormatLogMessage(LogLevel level, LogCategory category, const std::wstring& message);

        /**
         * @brief 로그 레벨을 문자열로 변환 (5자 고정)
         */
        static const wchar_t* LogLevelToString(LogLevel level);

        /**
         * @brief 로그 카테고리를 문자열로 변환 (8자 고정)
         */
        static const wchar_t* LogCategoryToString(LogCategory category);

        /**
         * @brief 현재 타임스탬프 문자열 반환
         */
        std::wstring GetTimestamp();

    private:
        std::mutex m_mutex;
        std::wofstream m_logFile;
        LogLevel m_minLevel = LogLevel::Trace;
        bool m_initialized = false;
        bool m_logToFile = false;
    };
}

// 로깅 매크로
#define LOG_TRACE(category, ...) \
    ::DX12GameEngine::Logger::Get().Log(::DX12GameEngine::LogLevel::Trace, category, __VA_ARGS__)

#define LOG_DEBUG(category, ...) \
    ::DX12GameEngine::Logger::Get().Log(::DX12GameEngine::LogLevel::Debug, category, __VA_ARGS__)

#define LOG_INFO(category, ...) \
    ::DX12GameEngine::Logger::Get().Log(::DX12GameEngine::LogLevel::Info, category, __VA_ARGS__)

#define LOG_WARNING(category, ...) \
    ::DX12GameEngine::Logger::Get().Log(::DX12GameEngine::LogLevel::Warning, category, __VA_ARGS__)

#define LOG_ERROR(category, ...) \
    ::DX12GameEngine::Logger::Get().Log(::DX12GameEngine::LogLevel::Error, category, __VA_ARGS__)

#define LOG_FATAL(category, ...) \
    ::DX12GameEngine::Logger::Get().Log(::DX12GameEngine::LogLevel::Fatal, category, __VA_ARGS__)
