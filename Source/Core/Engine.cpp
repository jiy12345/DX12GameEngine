/**
 * @file Engine.cpp
 * @brief 게임 엔진 메인 클래스 구현
 */

#include "Engine.h"
#include <Graphics/Renderer.h>

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
        if (m_initialized)
        {
            OutputDebugStringW(L"[Engine] Already initialized\n");
            return true;
        }

        OutputDebugStringW(L"===========================================\n");
        OutputDebugStringW(L"  DX12 Game Engine - Initializing...\n");
        OutputDebugStringW(L"===========================================\n");

        // 1. 윈도우 생성
        if (!m_window.Create(desc.window))
        {
            OutputDebugStringW(L"[Engine] Error: Failed to create window\n");
            return false;
        }

        // 윈도우 이벤트 핸들러 설정
        SetupWindowCallbacks();

        // 2. Renderer 초기화
        m_renderer = std::make_unique<Renderer>();
        if (!m_renderer->Initialize(m_window.GetHandle(), desc.window.width, desc.window.height, desc.renderer))
        {
            OutputDebugStringW(L"[Engine] Error: Failed to initialize Renderer\n");
            return false;
        }

        m_initialized = true;
        m_running = true;

        OutputDebugStringW(L"[Engine] Successfully initialized\n");
        OutputDebugStringW(L"[Engine] Press ESC to exit\n");
        OutputDebugStringW(L"===========================================\n\n");

        return true;
    }

    int Engine::Run()
    {
        if (!m_initialized)
        {
            OutputDebugStringW(L"[Engine] Error: Engine not initialized\n");
            return -1;
        }

        OutputDebugStringW(L"[Engine] Starting game loop...\n");

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

        OutputDebugStringW(L"[Engine] Game loop ended\n");
        return 0;
    }

    void Engine::Shutdown()
    {
        if (!m_initialized)
        {
            return;
        }

        OutputDebugStringW(L"[Engine] Shutting down...\n");

        // 리소스 정리 (역순으로)
        m_renderer.reset();

        m_initialized = false;
        m_running = false;

        OutputDebugStringW(L"[Engine] Shutdown complete\n");
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
            OutputDebugStringW(L"[Engine] ESC pressed, exiting...\n");
            m_running = false;
        }
    }
}
