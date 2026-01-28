/**
 * @file CommandQueue.cpp
 * @brief DirectX 12 커맨드 큐 구현
 */

#include "CommandQueue.h"
#include <Utils/Logger.h>

namespace DX12GameEngine
{
    CommandQueue::CommandQueue()
        : m_fenceValue(0)
        , m_fenceEvent(nullptr)
        , m_type(D3D12_COMMAND_LIST_TYPE_DIRECT)
        , m_initialized(false)
    {
    }

    CommandQueue::~CommandQueue()
    {
        if (m_initialized)
        {
            // 모든 GPU 작업이 완료될 때까지 대기
            Flush();

            if (m_fenceEvent)
            {
                CloseHandle(m_fenceEvent);
                m_fenceEvent = nullptr;
            }

            LOG_INFO(LogCategory::Renderer, L"CommandQueue destroyed");
        }
    }

    bool CommandQueue::Initialize(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type)
    {
        if (m_initialized)
        {
            LOG_WARNING(LogCategory::Renderer, L"CommandQueue already initialized");
            return true;
        }

        if (!device)
        {
            LOG_ERROR(LogCategory::Renderer, L"CommandQueue::Initialize - device is null");
            return false;
        }

        m_type = type;

        // 커맨드 큐 생성
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = type;
        queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.NodeMask = 0;

        HRESULT hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_queue));
        if (FAILED(hr))
        {
            LOG_ERROR(LogCategory::Renderer, L"Failed to create CommandQueue (HRESULT: {:#x})", static_cast<uint32_t>(hr));
            return false;
        }

        // Fence 생성
        hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
        if (FAILED(hr))
        {
            LOG_ERROR(LogCategory::Renderer, L"Failed to create Fence (HRESULT: {:#x})", static_cast<uint32_t>(hr));
            return false;
        }

        // Fence 이벤트 생성
        m_fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (!m_fenceEvent)
        {
            LOG_ERROR(LogCategory::Renderer, L"Failed to create Fence event");
            return false;
        }

        m_fenceValue = 0;
        m_initialized = true;

        const wchar_t* typeStr = L"Unknown";
        switch (type)
        {
        case D3D12_COMMAND_LIST_TYPE_DIRECT:
            typeStr = L"Direct";
            break;
        case D3D12_COMMAND_LIST_TYPE_COMPUTE:
            typeStr = L"Compute";
            break;
        case D3D12_COMMAND_LIST_TYPE_COPY:
            typeStr = L"Copy";
            break;
        }

        LOG_INFO(LogCategory::Renderer, L"CommandQueue initialized (Type: {})", typeStr);

        return true;
    }

    void CommandQueue::ExecuteCommandLists(ID3D12CommandList* const* commandLists, uint32_t count)
    {
        if (!m_initialized || !m_queue)
        {
            LOG_ERROR(LogCategory::Renderer, L"CommandQueue::ExecuteCommandLists - not initialized");
            return;
        }

        m_queue->ExecuteCommandLists(count, commandLists);
    }

    uint64_t CommandQueue::Signal()
    {
        if (!m_initialized || !m_queue || !m_fence)
        {
            LOG_ERROR(LogCategory::Renderer, L"CommandQueue::Signal - not initialized");
            return 0;
        }

        m_fenceValue++;
        HRESULT hr = m_queue->Signal(m_fence.Get(), m_fenceValue);
        if (FAILED(hr))
        {
            LOG_ERROR(LogCategory::Renderer, L"Failed to signal fence (HRESULT: {:#x})", static_cast<uint32_t>(hr));
        }

        return m_fenceValue;
    }

    void CommandQueue::WaitForFenceValue(uint64_t fenceValue)
    {
        if (!m_initialized || !m_fence || !m_fenceEvent)
        {
            return;
        }

        if (m_fence->GetCompletedValue() < fenceValue)
        {
            HRESULT hr = m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent);
            if (FAILED(hr))
            {
                LOG_ERROR(LogCategory::Renderer, L"Failed to set fence event (HRESULT: {:#x})", static_cast<uint32_t>(hr));
                return;
            }

            WaitForSingleObject(m_fenceEvent, INFINITE);
        }
    }

    void CommandQueue::Flush()
    {
        uint64_t fenceValue = Signal();
        WaitForFenceValue(fenceValue);
    }

    uint64_t CommandQueue::GetCompletedFenceValue() const
    {
        if (!m_fence)
        {
            return 0;
        }

        return m_fence->GetCompletedValue();
    }

    ComPtr<ID3D12CommandAllocator> CommandQueue::CreateCommandAllocator(ID3D12Device* device)
    {
        if (!device)
        {
            LOG_ERROR(LogCategory::Renderer, L"CommandQueue::CreateCommandAllocator - device is null");
            return nullptr;
        }

        ComPtr<ID3D12CommandAllocator> allocator;
        HRESULT hr = device->CreateCommandAllocator(m_type, IID_PPV_ARGS(&allocator));
        if (FAILED(hr))
        {
            LOG_ERROR(LogCategory::Renderer, L"Failed to create CommandAllocator (HRESULT: {:#x})", static_cast<uint32_t>(hr));
            return nullptr;
        }

        LOG_DEBUG(LogCategory::Renderer, L"CommandAllocator created");
        return allocator;
    }

    ComPtr<ID3D12GraphicsCommandList> CommandQueue::CreateCommandList(
        ID3D12Device* device,
        ID3D12CommandAllocator* allocator,
        ID3D12PipelineState* pipelineState)
    {
        if (!device)
        {
            LOG_ERROR(LogCategory::Renderer, L"CommandQueue::CreateCommandList - device is null");
            return nullptr;
        }

        if (!allocator)
        {
            LOG_ERROR(LogCategory::Renderer, L"CommandQueue::CreateCommandList - allocator is null");
            return nullptr;
        }

        ComPtr<ID3D12GraphicsCommandList> commandList;
        HRESULT hr = device->CreateCommandList(
            0,                  // nodeMask
            m_type,             // type
            allocator,          // allocator
            pipelineState,      // initial pipeline state
            IID_PPV_ARGS(&commandList));

        if (FAILED(hr))
        {
            LOG_ERROR(LogCategory::Renderer, L"Failed to create CommandList (HRESULT: {:#x})", static_cast<uint32_t>(hr));
            return nullptr;
        }

        // 생성 직후 Close - 사용 전에 Reset 해야 함
        commandList->Close();

        LOG_DEBUG(LogCategory::Renderer, L"CommandList created");
        return commandList;
    }
}
