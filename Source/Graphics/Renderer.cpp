/**
 * @file Renderer.cpp
 * @brief 렌더링 서브시스템 구현
 */

#include "Renderer.h"
#include "Device.h"
#include "CommandQueue.h"
#include "CommandListManager.h"
#include <Utils/Logger.h>

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
            LOG_WARNING(LogCategory::Renderer, L"Renderer already initialized");
            return true;
        }

        LOG_INFO(LogCategory::Renderer, L"Initializing renderer...");

        m_width = width;
        m_height = height;

        // Device 생성 및 초기화
        m_device = std::make_unique<Device>();
        if (!m_device->Initialize(desc.enableDebugLayer))
        {
            LOG_ERROR(LogCategory::Renderer, L"Failed to initialize Device");
            return false;
        }

        // CommandQueue 초기화 (Direct Queue)
        m_commandQueue = std::make_unique<CommandQueue>();
        if (!m_commandQueue->Initialize(m_device->GetDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT))
        {
            LOG_ERROR(LogCategory::Renderer, L"Failed to initialize CommandQueue");
            return false;
        }

        // CommandQueue 동기화 테스트
        uint64_t fenceValue = m_commandQueue->Signal();
        m_commandQueue->WaitForFenceValue(fenceValue);
        LOG_INFO(LogCategory::Renderer, L"CommandQueue fence synchronization test passed (value: {})", fenceValue);

        // CommandListManager 초기화
        m_commandListManager = std::make_unique<CommandListManager>();
        if (!m_commandListManager->Initialize(m_device->GetDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT))
        {
            LOG_ERROR(LogCategory::Renderer, L"Failed to initialize CommandListManager");
            return false;
        }

        // TODO: #9 - SwapChain 초기화 (desc.vsync 사용)
        // TODO: #10 - DescriptorHeapManager 초기화
        // TODO: #11 - RenderTargetView 초기화
        // TODO: #12 - Fence 동기화 시스템 (이미 CommandQueue에 포함)

        m_initialized = true;

        LOG_INFO(LogCategory::Renderer, L"Renderer initialized ({}x{})", m_width, m_height);

        return true;
    }

    void Renderer::BeginFrame()
    {
        // CommandListManager 프레임 시작
        m_commandListManager->BeginFrame(
            m_commandQueue->GetFence(),
            m_commandQueue->GetFenceEvent());

        // TODO: #13 - 프레임 시작 처리
        // - 렌더 타겟 설정
    }

    void Renderer::RenderFrame()
    {
        // TODO: #13 - 실제 렌더링
        // - 렌더 타겟 클리어
        // TODO: #14 - 삼각형 렌더링
    }

    void Renderer::EndFrame()
    {
        // TODO: #13 - 프레임 종료 처리
        // - 커맨드 리스트 제출
        // - Present 호출

        // Fence 시그널 및 CommandListManager 프레임 종료
        uint64_t fenceValue = m_commandQueue->Signal();
        m_commandListManager->EndFrame(fenceValue);
    }

    void Renderer::OnResize(int width, int height)
    {
        if (!m_initialized)
        {
            return;
        }

        m_width = width;
        m_height = height;

        // TODO: #9 - SwapChain 리사이즈 처리

        LOG_INFO(LogCategory::Renderer, L"Renderer resized ({}x{})", m_width, m_height);
    }
}
