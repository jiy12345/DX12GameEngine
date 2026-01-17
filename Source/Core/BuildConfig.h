/**
 * @file BuildConfig.h
 * @brief 빌드 구성 및 기본값 정의
 *
 * 모든 빌드 타입(Debug, Release, Profile)의 기본 동작을 한 곳에서 관리합니다.
 * 이 파일만 보면 각 빌드 구성이 무엇을 활성화/비활성화하는지 명확히 알 수 있습니다.
 */

#pragma once

namespace DX12GameEngine
{
    /**
     * @brief 빌드 구성 타입
     */
    enum class BuildConfiguration
    {
        Debug,      // 개발용 - 모든 디버깅 기능 활성화
        Release,    // 배포용 - 최고 성능
        Profile     // 프로파일링용 - Release + 일부 디버깅
    };

    /**
     * @brief 현재 빌드 구성 반환
     */
    constexpr BuildConfiguration GetCurrentBuildConfig()
    {
#if defined(_DEBUG) || defined(DEBUG)
        return BuildConfiguration::Debug;
#elif defined(PROFILE_BUILD)
        return BuildConfiguration::Profile;
#else
        return BuildConfiguration::Release;
#endif
    }

    /**
     * @brief 빌드 구성 체크 헬퍼
     */
    constexpr bool IsDebugBuild() { return GetCurrentBuildConfig() == BuildConfiguration::Debug; }
    constexpr bool IsReleaseBuild() { return GetCurrentBuildConfig() == BuildConfiguration::Release; }
    constexpr bool IsProfileBuild() { return GetCurrentBuildConfig() == BuildConfiguration::Profile; }

    /**
     * @brief Debug 빌드 기본값
     *
     * 개발 중에 사용. 모든 검증과 디버깅 기능 활성화.
     * 성능보다 오류 검출이 우선.
     */
    struct DebugDefaults
    {
        // DirectX 12 디버깅
        static constexpr bool EnableD3D12DebugLayer = true;     // DX12 Debug Layer
        static constexpr bool EnableGPUValidation = true;       // GPU 기반 검증 (매우 느림)
        static constexpr bool BreakOnError = true;              // 오류 시 중단점

        // 메모리
        static constexpr bool EnableMemoryLeakTracking = true;  // 메모리 누수 추적
        static constexpr bool EnableBoundsChecking = true;      // 배열 범위 검사

        // 로깅
        static constexpr bool EnableVerboseLogging = true;      // 상세 로그
        static constexpr bool LogFrameTime = true;              // 프레임 타임 로그

        // 렌더링
        static constexpr bool EnableVSync = true;               // VSync (프레임 안정성)
        static constexpr int DefaultMSAASamples = 1;            // MSAA 비활성화 (디버깅 쉬움)

        // 어서트
        static constexpr bool EnableAsserts = true;             // assert() 활성화
    };

    /**
     * @brief Release 빌드 기본값
     *
     * 실제 배포용. 최고 성능, 디버깅 기능 전부 제거.
     */
    struct ReleaseDefaults
    {
        // DirectX 12 디버깅
        static constexpr bool EnableD3D12DebugLayer = false;
        static constexpr bool EnableGPUValidation = false;
        static constexpr bool BreakOnError = false;

        // 메모리
        static constexpr bool EnableMemoryLeakTracking = false;
        static constexpr bool EnableBoundsChecking = false;

        // 로깅
        static constexpr bool EnableVerboseLogging = false;
        static constexpr bool LogFrameTime = false;

        // 렌더링
        static constexpr bool EnableVSync = true;               // 기본 VSync 켜기 (화면 찢김 방지)
        static constexpr int DefaultMSAASamples = 1;            // 성능을 위해 MSAA 끔

        // 어서트
        static constexpr bool EnableAsserts = false;            // 성능을 위해 끔
    };

    /**
     * @brief Profile 빌드 기본값
     *
     * 성능 측정용. Release 기반이지만 일부 추적 기능 활성화.
     */
    struct ProfileDefaults
    {
        // DirectX 12 디버깅
        static constexpr bool EnableD3D12DebugLayer = false;    // 성능 영향 큼
        static constexpr bool EnableGPUValidation = false;      // 성능 영향 큼
        static constexpr bool BreakOnError = false;

        // 메모리
        static constexpr bool EnableMemoryLeakTracking = true;  // 메모리는 추적 (성능 영향 작음)
        static constexpr bool EnableBoundsChecking = false;

        // 로깅
        static constexpr bool EnableVerboseLogging = false;
        static constexpr bool LogFrameTime = true;              // 프레임 타임은 측정

        // 렌더링
        static constexpr bool EnableVSync = false;              // 프로파일링 시 VSync 끔 (정확한 측정)
        static constexpr int DefaultMSAASamples = 1;

        // 어서트
        static constexpr bool EnableAsserts = true;             // 로직 오류 검출
    };

    /**
     * @brief 빌드 구성에 따른 기본값 선택 헬퍼
     */
    template<typename DebugValue, typename ReleaseValue, typename ProfileValue>
    constexpr auto SelectByBuildConfig(DebugValue debugVal, ReleaseValue releaseVal, ProfileValue profileVal)
    {
        if constexpr (IsDebugBuild()) {
            return debugVal;
        } else if constexpr (IsProfileBuild()) {
            return profileVal;
        } else {
            return releaseVal;
        }
    }

    /**
     * @brief 현재 빌드 구성의 기본값 가져오기 매크로
     */
#define BUILD_DEFAULT(setting) \
    DX12GameEngine::SelectByBuildConfig( \
        DX12GameEngine::DebugDefaults::setting, \
        DX12GameEngine::ReleaseDefaults::setting, \
        DX12GameEngine::ProfileDefaults::setting)
}
