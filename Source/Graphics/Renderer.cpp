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
        : m_commandList(nullptr)
        , m_initialized(false)
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

        // 커맨드 리스트 획득
        m_commandList = m_commandListManager->GetCommandList();

        // 백 버퍼 상태 전환: PRESENT → RENDER_TARGET
        ID3D12Resource* backBuffer = m_swapChain->GetCurrentBackBuffer();
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = backBuffer;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        m_commandList->ResourceBarrier(1, &barrier);

        // 뷰포트 설정
        D3D12_VIEWPORT viewport = {};
        viewport.Width = static_cast<float>(m_width);
        viewport.Height = static_cast<float>(m_height);
        viewport.MaxDepth = 1.0f;
        m_commandList->RSSetViewports(1, &viewport);

        // 시저 렉트 설정
        D3D12_RECT scissorRect = { 0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height) };
        m_commandList->RSSetScissorRects(1, &scissorRect);

        // 렌더 타겟 설정
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetCurrentRtvHandle();
        m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
    }

    void Renderer::RenderFrame()
    {
        // 렌더 타겟 클리어 (Cornflower Blue)
        const float clearColor[] = { 0.39f, 0.58f, 0.93f, 1.0f };
        m_commandList->ClearRenderTargetView(GetCurrentRtvHandle(), clearColor, 0, nullptr);

        // TODO: #14 - 삼각형 렌더링
    }

    void Renderer::EndFrame()
    {
        // 백 버퍼 상태 전환: RENDER_TARGET → PRESENT
        ID3D12Resource* backBuffer = m_swapChain->GetCurrentBackBuffer();
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = backBuffer;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        m_commandList->ResourceBarrier(1, &barrier);

        // 커맨드 리스트 닫기
        m_commandList->Close();

        // 커맨드 리스트 실행
        ID3D12CommandList* commandLists[] = { m_commandList };
        m_commandQueue->ExecuteCommandLists(commandLists, 1);

        // 커맨드 리스트 반환
        m_commandListManager->ReturnCommandList(m_commandList);
        m_commandList = nullptr;

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
