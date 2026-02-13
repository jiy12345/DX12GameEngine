/**
 * @file Renderer.cpp
 * @brief 렌더링 서브시스템 구현
 */

#include "Renderer.h"
#include "Device.h"
#include "CommandQueue.h"
#include "CommandListManager.h"
#include "SwapChain.h"
#include "DescriptorHeapManager.h"
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
        ReleaseRenderTargetViews();
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

        // SwapChain 초기화
        SwapChainDesc swapChainDesc;
        swapChainDesc.hwnd = hwnd;
        swapChainDesc.width = static_cast<uint32_t>(width);
        swapChainDesc.height = static_cast<uint32_t>(height);
        swapChainDesc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.vsync = desc.vsync;
        swapChainDesc.allowTearing = !desc.vsync;  // VSync OFF일 때 Tearing 허용

        m_swapChain = std::make_unique<SwapChain>();
        if (!m_swapChain->Initialize(m_device->GetFactory(), m_commandQueue->GetQueue(), swapChainDesc))
        {
            LOG_ERROR(LogCategory::Renderer, L"Failed to initialize SwapChain");
            return false;
        }

        // DescriptorHeapManager 초기화
        m_descriptorHeapManager = std::make_unique<DescriptorHeapManager>();
        if (!m_descriptorHeapManager->Initialize(m_device->GetDevice()))
        {
            LOG_ERROR(LogCategory::Renderer, L"Failed to initialize DescriptorHeapManager");
            return false;
        }

        // RenderTargetView 생성
        if (!CreateRenderTargetViews())
        {
            LOG_ERROR(LogCategory::Renderer, L"Failed to create RenderTargetViews");
            return false;
        }

        m_initialized = true;

        LOG_INFO(LogCategory::Renderer, L"Renderer initialized ({}x{})", m_width, m_height);

        return true;
    }

    bool Renderer::CreateRenderTargetViews()
    {
        for (uint32_t i = 0; i < kBackBufferCount; ++i)
        {
            m_rtvHandles[i] = m_descriptorHeapManager->AllocateRtv();
            if (!m_rtvHandles[i].IsValid())
            {
                LOG_ERROR(LogCategory::Renderer, L"Failed to allocate RTV descriptor for back buffer {}", i);
                return false;
            }

            m_device->GetDevice()->CreateRenderTargetView(
                m_swapChain->GetBackBuffer(i), nullptr, m_rtvHandles[i].cpuHandle);
        }

        LOG_INFO(LogCategory::Renderer, L"Created {} RenderTargetViews", kBackBufferCount);
        return true;
    }

    void Renderer::ReleaseRenderTargetViews()
    {
        for (uint32_t i = 0; i < kBackBufferCount; ++i)
        {
            if (m_rtvHandles[i].IsValid())
            {
                m_descriptorHeapManager->FreeRtv(m_rtvHandles[i]);
                m_rtvHandles[i] = DescriptorHandle();
            }
        }
    }

    D3D12_CPU_DESCRIPTOR_HANDLE Renderer::GetCurrentRtvHandle() const
    {
        uint32_t index = m_swapChain->GetCurrentBackBufferIndex();
        return m_rtvHandles[index].cpuHandle;
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

        // Present
        m_swapChain->Present();

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

        if (width == m_width && height == m_height)
        {
            return;
        }

        // GPU 작업 완료 대기 (리사이즈 전 필수)
        m_commandQueue->Flush();

        m_width = width;
        m_height = height;

        // RTV 해제 (SwapChain 리사이즈 전 백 버퍼 참조 제거)
        ReleaseRenderTargetViews();

        // SwapChain 리사이즈
        m_swapChain->Resize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));

        // RTV 재생성
        CreateRenderTargetViews();

        LOG_INFO(LogCategory::Renderer, L"Renderer resized ({}x{})", m_width, m_height);
    }
}
