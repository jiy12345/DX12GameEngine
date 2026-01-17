/**
 * @file Engine.h
 * @brief 게임 엔진 메인 클래스
 *
 * 윈도우, 렌더러 등 엔진 서브시스템을 관리하고 게임 루프를 실행합니다.
 * 내부 구현은 캡슐화되어 있으며, 외부에는 최소한의 인터페이스만 노출합니다.
 */

#pragma once

#include <Platform/Window.h>
#include <Graphics/Renderer.h>
#include <Core/BuildConfig.h>
#include <memory>
#include <string>

namespace DX12GameEngine
{
    /**
     * @brief 엔진 초기화 설정
     *
     * 모든 서브시스템의 설정을 계층적으로 포함합니다.
     * 설정 파일(JSON, INI 등)에서 로드하거나 외부에서 조정 가능합니다.
     *
     * 기본값은 빌드 구성(Debug/Release/Profile)에 따라 자동 설정되지만,
     * 사용자가 명시적으로 변경하거나 특정 구성의 기본값을 선택할 수 있습니다.
     */
    struct EngineDesc
    {
        WindowDesc window;      // 윈도우 설정
        RendererDesc renderer;  // 렌더러 설정

        /**
         * @brief 기본 생성자 - 현재 빌드 구성에 맞는 기본값 사용
         */
        EngineDesc()
        {
            ApplyCurrentBuildDefaults();
        }

        /**
         * @brief 현재 빌드 구성의 기본값 적용
         */
        void ApplyCurrentBuildDefaults()
        {
            if constexpr (IsDebugBuild()) {
                ApplyDebugDefaults();
            } else if constexpr (IsProfileBuild()) {
                ApplyProfileDefaults();
            } else {
                ApplyReleaseDefaults();
            }
        }

        /**
         * @brief Debug 빌드 기본값 적용
         */
        void ApplyDebugDefaults()
        {
            // 윈도우 기본 설정
            window.title = L"DX12 Game Engine [DEBUG]";
            window.width = 1280;
            window.height = 720;
            window.resizable = true;

            // Debug 렌더러 설정
            renderer.enableDebugLayer = DebugDefaults::EnableD3D12DebugLayer;
            renderer.vsync = DebugDefaults::EnableVSync;
            renderer.msaaSamples = DebugDefaults::DefaultMSAASamples;
        }

        /**
         * @brief Release 빌드 기본값 적용
         */
        void ApplyReleaseDefaults()
        {
            // 윈도우 기본 설정
            window.title = L"DX12 Game Engine";
            window.width = 1280;
            window.height = 720;
            window.resizable = true;

            // Release 렌더러 설정
            renderer.enableDebugLayer = ReleaseDefaults::EnableD3D12DebugLayer;
            renderer.vsync = ReleaseDefaults::EnableVSync;
            renderer.msaaSamples = ReleaseDefaults::DefaultMSAASamples;
        }

        /**
         * @brief Profile 빌드 기본값 적용
         */
        void ApplyProfileDefaults()
        {
            // 윈도우 기본 설정
            window.title = L"DX12 Game Engine [PROFILE]";
            window.width = 1280;
            window.height = 720;
            window.resizable = true;

            // Profile 렌더러 설정
            renderer.enableDebugLayer = ProfileDefaults::EnableD3D12DebugLayer;
            renderer.vsync = ProfileDefaults::EnableVSync;
            renderer.msaaSamples = ProfileDefaults::DefaultMSAASamples;
        }

        /**
         * @brief 정적 팩토리 메서드 - Debug 구성 생성
         */
        static EngineDesc ForDebug()
        {
            EngineDesc desc;
            desc.ApplyDebugDefaults();
            return desc;
        }

        /**
         * @brief 정적 팩토리 메서드 - Release 구성 생성
         */
        static EngineDesc ForRelease()
        {
            EngineDesc desc;
            desc.ApplyReleaseDefaults();
            return desc;
        }

        /**
         * @brief 정적 팩토리 메서드 - Profile 구성 생성
         */
        static EngineDesc ForProfile()
        {
            EngineDesc desc;
            desc.ApplyProfileDefaults();
            return desc;
        }
    };

    /**
     * @brief 게임 엔진 메인 클래스
     *
     * 엔진의 진입점이며, 모든 서브시스템을 초기화하고 게임 루프를 실행합니다.
     * 내부 구현(Device, Window 등)은 완전히 캡슐화되어 외부에 노출되지 않습니다.
     */
    class Engine
    {
    public:
        Engine();
        ~Engine();

        // 복사 및 이동 금지
        Engine(const Engine&) = delete;
        Engine& operator=(const Engine&) = delete;
        Engine(Engine&&) = delete;
        Engine& operator=(Engine&&) = delete;

        /**
         * @brief 엔진 초기화
         * @param desc 엔진 설정
         * @return 성공 시 true
         */
        bool Initialize(const EngineDesc& desc);

        /**
         * @brief 게임 루프 실행
         * @return 종료 코드
         */
        int Run();

        /**
         * @brief 엔진 종료 및 정리
         */
        void Shutdown();

    private:
        /**
         * @brief 윈도우 이벤트 핸들러 설정
         */
        void SetupWindowCallbacks();

        /**
         * @brief 윈도우 리사이즈 콜백
         */
        void OnResize(int width, int height);

        /**
         * @brief 키보드 입력 콜백
         */
        void OnKeyboard(const KeyboardEvent& event);

    private:
        // 모든 서브시스템은 private (외부 노출 없음)
        Window m_window;
        std::unique_ptr<Renderer> m_renderer;

        // 상태
        bool m_initialized;
        bool m_running;
    };
}
