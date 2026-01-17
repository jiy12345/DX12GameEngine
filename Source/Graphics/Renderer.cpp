/**
 * @file Renderer.cpp
 * @brief 렌더링 서브시스템 구현
 */

#include "Renderer.h"
#include "Device.h"
#include <sstream>

namespace DX12GameEngine
{
    Renderer::Renderer()
        : m_initialized(false)
        , m_width(0)
        , m_height(0)
    {
    }

    Renderer::~Renderer()
    {
        // unique_ptr이 자동으로 정리
    }

    bool Renderer::Initialize(HWND hwnd, int width, int height, const RendererDesc& desc)
    {
        if (m_initialized)
        {
            OutputDebugStringW(L"[Renderer] Already initialized\n");
            return true;
        }

        OutputDebugStringW(L"[Renderer] Initializing renderer...\n");

        m_width = width;
        m_height = height;

        // Device 생성 및 초기화
        m_device = std::make_unique<Device>();
        if (!m_device->Initialize(desc.enableDebugLayer))
        {
            OutputDebugStringW(L"[Renderer] Error: Failed to initialize Device\n");
            return false;
        }

        // TODO: #5 - SwapChain 초기화 시 desc.vsync 사용
        // TODO: #6 - MSAA 설정 시 desc.msaaSamples 사용

        // TODO: #4 - CommandQueue 초기화
        // TODO: #5 - SwapChain 초기화
        // TODO: #6 - DescriptorHeapManager 초기화
        // TODO: #7 - RenderTargetView 초기화
        // TODO: #8 - Fence 초기화

        m_initialized = true;

        std::wstringstream ss;
        ss << L"[Renderer] Successfully initialized\n";
        ss << L"  - Resolution: " << m_width << L" x " << m_height << L"\n";
        OutputDebugStringW(ss.str().c_str());

        return true;
    }

    void Renderer::BeginFrame()
    {
        // TODO: #9 - 프레임 시작 처리
        // - 커맨드 리스트 리셋
        // - 렌더 타겟 설정
    }

    void Renderer::RenderFrame()
    {
        // TODO: #9 - 실제 렌더링
        // - 렌더 타겟 클리어
        // TODO: #10 - 삼각형 렌더링
    }

    void Renderer::EndFrame()
    {
        // TODO: #9 - 프레임 종료 처리
        // - 커맨드 리스트 제출
        // - Present 호출
        // - Fence 신호
    }

    void Renderer::OnResize(int width, int height)
    {
        if (!m_initialized)
        {
            return;
        }

        m_width = width;
        m_height = height;

        // TODO: #5 - SwapChain 리사이즈 처리

        std::wstringstream ss;
        ss << L"[Renderer] Resized: " << m_width << L" x " << m_height << L"\n";
        OutputDebugStringW(ss.str().c_str());
    }
}
