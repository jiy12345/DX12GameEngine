/**
 * @file CommandListManager.cpp
 * @brief Command Allocator와 Command List 풀링 관리 구현
 */

#include "CommandListManager.h"
#include <Utils/Logger.h>

namespace DX12GameEngine
{
    CommandListManager::CommandListManager()
        : m_device(nullptr)
        , m_type(D3D12_COMMAND_LIST_TYPE_DIRECT)
        , m_initialized(false)
        , m_fenceValues{}
        , m_currentFrameIndex(0)
    {
    }

    CommandListManager::~CommandListManager()
    {
        if (m_initialized)
        {
            LOG_INFO(LogCategory::Renderer, L"CommandListManager destroyed");
        }
    }

    bool CommandListManager::Initialize(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type,
                                         uint32_t initialListCount)
    {
        if (m_initialized)
        {
            LOG_WARNING(LogCategory::Renderer, L"CommandListManager already initialized");
            return true;
        }

        if (!device)
        {
            LOG_ERROR(LogCategory::Renderer, L"CommandListManager::Initialize - device is null");
            return false;
        }

        m_device = device;
        m_type = type;

        // 프레임별 Allocator 생성
        for (uint32_t i = 0; i < kMaxFramesInFlight; i++)
        {
            HRESULT hr = device->CreateCommandAllocator(type, IID_PPV_ARGS(&m_allocators[i]));
            if (FAILED(hr))
            {
                LOG_ERROR(LogCategory::Renderer,
                          L"Failed to create CommandAllocator[{}] (HRESULT: {:#x})",
                          i, static_cast<uint32_t>(hr));
                return false;
            }
            m_fenceValues[i] = 0;
        }

        LOG_INFO(LogCategory::Renderer,
                 L"Created {} CommandAllocators for frame buffering",
                 kMaxFramesInFlight);

        // 초기 CommandList 풀 생성
        for (uint32_t i = 0; i < initialListCount; i++)
        {
            CreateNewCommandList();
        }

        LOG_INFO(LogCategory::Renderer,
                 L"Created {} CommandLists in pool",
                 initialListCount);

        m_initialized = true;
        m_currentFrameIndex = 0;

        LOG_INFO(LogCategory::Renderer, L"CommandListManager initialized");

        return true;
    }

    void CommandListManager::BeginFrame(ID3D12Fence* fence, HANDLE fenceEvent)
    {
        if (!m_initialized)
        {
            LOG_ERROR(LogCategory::Renderer, L"CommandListManager::BeginFrame - not initialized");
            return;
        }

        if (!fence)
        {
            LOG_ERROR(LogCategory::Renderer, L"CommandListManager::BeginFrame - fence is null");
            return;
        }

        // 현재 프레임의 Allocator가 GPU에서 완료되었는지 확인
        uint64_t completedValue = fence->GetCompletedValue();
        uint64_t requiredValue = m_fenceValues[m_currentFrameIndex];

        if (completedValue < requiredValue)
        {
            // GPU가 아직 이 Allocator를 사용 중 - 대기
            HRESULT hr = fence->SetEventOnCompletion(requiredValue, fenceEvent);
            if (FAILED(hr))
            {
                LOG_ERROR(LogCategory::Renderer,
                          L"Failed to set fence event (HRESULT: {:#x})",
                          static_cast<uint32_t>(hr));
                return;
            }

            WaitForSingleObject(fenceEvent, INFINITE);
            LOG_DEBUG(LogCategory::Renderer,
                      L"Waited for frame {} fence (value: {})",
                      m_currentFrameIndex, requiredValue);
        }

        // Allocator Reset
        HRESULT hr = m_allocators[m_currentFrameIndex]->Reset();
        if (FAILED(hr))
        {
            LOG_ERROR(LogCategory::Renderer,
                      L"Failed to reset CommandAllocator[{}] (HRESULT: {:#x})",
                      m_currentFrameIndex, static_cast<uint32_t>(hr));
        }
    }

    void CommandListManager::EndFrame(uint64_t fenceValue)
    {
        if (!m_initialized)
        {
            return;
        }

        // 현재 프레임의 Fence 값 기록
        m_fenceValues[m_currentFrameIndex] = fenceValue;

        // 다음 프레임으로 이동
        m_currentFrameIndex = (m_currentFrameIndex + 1) % kMaxFramesInFlight;
    }

    ID3D12GraphicsCommandList* CommandListManager::GetCommandList(ID3D12PipelineState* pipelineState)
    {
        if (!m_initialized)
        {
            LOG_ERROR(LogCategory::Renderer, L"CommandListManager::GetCommandList - not initialized");
            return nullptr;
        }

        size_t index;

        if (m_availableIndices.empty())
        {
            // 풀에 사용 가능한 CommandList가 없으면 새로 생성
            index = CreateNewCommandList();
            LOG_DEBUG(LogCategory::Renderer, L"Created new CommandList (pool size: {})",
                      m_commandListPool.size());
        }
        else
        {
            // 풀에서 가져오기
            index = m_availableIndices.front();
            m_availableIndices.pop();
        }

        auto* commandList = m_commandListPool[index].Get();
        auto* allocator = m_allocators[m_currentFrameIndex].Get();

        // CommandList Reset (현재 프레임의 Allocator와 연결)
        HRESULT hr = commandList->Reset(allocator, pipelineState);
        if (FAILED(hr))
        {
            LOG_ERROR(LogCategory::Renderer,
                      L"Failed to reset CommandList (HRESULT: {:#x})",
                      static_cast<uint32_t>(hr));
            // 실패 시 다시 풀에 반환
            m_availableIndices.push(index);
            return nullptr;
        }

        return commandList;
    }

    void CommandListManager::ReturnCommandList(ID3D12GraphicsCommandList* commandList)
    {
        if (!commandList)
        {
            return;
        }

        // 풀에서 해당 CommandList의 인덱스 찾기
        for (size_t i = 0; i < m_commandListPool.size(); i++)
        {
            if (m_commandListPool[i].Get() == commandList)
            {
                m_availableIndices.push(i);
                return;
            }
        }

        LOG_WARNING(LogCategory::Renderer,
                    L"ReturnCommandList - CommandList not found in pool");
    }

    ID3D12CommandAllocator* CommandListManager::GetCurrentAllocator() const
    {
        if (!m_initialized)
        {
            return nullptr;
        }
        return m_allocators[m_currentFrameIndex].Get();
    }

    size_t CommandListManager::CreateNewCommandList()
    {
        ComPtr<ID3D12GraphicsCommandList> commandList;

        // 현재 프레임의 Allocator로 생성
        HRESULT hr = m_device->CreateCommandList(
            0,
            m_type,
            m_allocators[m_currentFrameIndex].Get(),
            nullptr,
            IID_PPV_ARGS(&commandList));

        if (FAILED(hr))
        {
            LOG_ERROR(LogCategory::Renderer,
                      L"Failed to create CommandList (HRESULT: {:#x})",
                      static_cast<uint32_t>(hr));
            return SIZE_MAX;
        }

        // 생성 직후 Close (Initial 상태로)
        commandList->Close();

        size_t index = m_commandListPool.size();
        m_commandListPool.push_back(commandList);
        m_availableIndices.push(index);

        return index;
    }
}
