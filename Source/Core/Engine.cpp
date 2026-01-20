/**
 * @file Engine.cpp
 * @brief 게임 엔진 메인 클래스 구현
 */

#include "Engine.h"
#include <Graphics/Renderer.h>
#include <Utils/Logger.h>

namespace DX12GameEngine
{
    Engine::Engine()
        : m_initialized(false)
        , m_running(false)
    {
    }

    Engine::~Engine()
    {
        Shutdown();
    }

    bool Engine::Initialize(const EngineDesc& desc)
    {
        // 0. 로거 초기화 (가장 먼저)
        Logger::Get().Initialize(
            BUILD_DEFAULT(MinLogLevel),
            BUILD_DEFAULT(LogToFile)
        );

        if (m_initialized)
        {
            LOG_WARNING(LogCategory::Engine, L"Already initialized");
            return true;
        }

        LOG_INFO(LogCategory::Engine, L"===========================================");
        LOG_INFO(LogCategory::Engine, L"  DX12 Game Engine - Initializing...");
        LOG_INFO(LogCategory::Engine, L"===========================================");

        // 1. 윈도우 생성
        if (!m_window.Create(desc.window))
        {
            LOG_ERROR(LogCategory::Engine, L"Failed to create window");
            return false;
        }

        // 윈도우 이벤트 핸들러 설정
        SetupWindowCallbacks();

        // 2. Renderer 초기화
        m_renderer = std::make_unique<Renderer>();
        if (!m_renderer->Initialize(m_window.GetHandle(), desc.window.width, desc.window.height, desc.renderer))
        {
            LOG_ERROR(LogCategory::Engine, L"Failed to initialize Renderer");
            return false;
        }

        m_initialized = true;
        m_running = true;

        LOG_INFO(LogCategory::Engine, L"Successfully initialized");
        LOG_INFO(LogCategory::Engine, L"Press ESC to exit");
        LOG_INFO(LogCategory::Engine, L"===========================================");

        return true;
    }

    int Engine::Run()
    {
        if (!m_initialized)
        {
            LOG_ERROR(LogCategory::Engine, L"Engine not initialized");
            return -1;
        }

        LOG_INFO(LogCategory::Engine, L"Starting game loop...");

        // 게임 루프
        while (m_running)
        {
            // 윈도우 메시지 처리 (이벤트 기반, 틱 아님)
            if (!m_window.ProcessMessages())
            {
                m_running = false;
                break;
            }

            // 렌더링 (매 프레임 Update)
            m_renderer->BeginFrame();
            m_renderer->RenderFrame();
            m_renderer->EndFrame();

            // TODO: 나중에 추가
            // m_physics->Update(deltaTime);
            // m_audio->Update();
        }

        LOG_INFO(LogCategory::Engine, L"Game loop ended");
        return 0;
    }

    void Engine::Shutdown()
    {
        if (!m_initialized)
        {
            return;
        }

        LOG_INFO(LogCategory::Engine, L"Shutting down...");

        // 리소스 정리 (역순으로)
        m_renderer.reset();

        m_initialized = false;
        m_running = false;

        LOG_INFO(LogCategory::Engine, L"Shutdown complete");

        // 로거 종료 (가장 마지막)
        Logger::Get().Shutdown();
    }

    void Engine::SetupWindowCallbacks()
    {
        // 키보드 콜백
        m_window.SetKeyboardCallback([this](const KeyboardEvent& event) {
            this->OnKeyboard(event);
        });

        // 리사이즈 콜백
        m_window.SetResizeCallback([this](int width, int height) {
            this->OnResize(width, height);
        });
    }

    void Engine::OnResize(int width, int height)
    {
        if (m_renderer)
        {
            m_renderer->OnResize(width, height);
        }
    }

    void Engine::OnKeyboard(const KeyboardEvent& event)
    {
        // ESC 키로 종료
        if (event.isPressed && event.keyCode == VK_ESCAPE)
        {
            LOG_INFO(LogCategory::Engine, L"ESC pressed, exiting...");
            m_running = false;
        }
    }
}
