/**
 * @file DescriptorHeap.cpp
 * @brief 디스크립터 힙 관리 구현
 */

#include "DescriptorHeap.h"
#include <Utils/Logger.h>

namespace DX12GameEngine
{
    namespace
    {
        const wchar_t* GetHeapTypeName(D3D12_DESCRIPTOR_HEAP_TYPE type)
        {
            switch (type)
            {
            case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV: return L"CBV_SRV_UAV";
            case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:    return L"SAMPLER";
            case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:        return L"RTV";
            case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:        return L"DSV";
            default:                                     return L"UNKNOWN";
            }
        }
    }

    DescriptorHeap::DescriptorHeap()
        : m_type(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
        , m_numDescriptors(0)
        , m_descriptorSize(0)
        , m_shaderVisible(false)
        , m_initialized(false)
        , m_cpuStartHandle{}
        , m_gpuStartHandle{}
        , m_allocatedCount(0)
    {
    }

    DescriptorHeap::~DescriptorHeap()
    {
        if (m_initialized && m_allocatedCount > 0)
        {
            LOG_WARNING(LogCategory::Renderer,
                        L"DescriptorHeap ({}) destroyed with {} descriptors still allocated",
                        GetHeapTypeName(m_type), m_allocatedCount);
        }
    }

    bool DescriptorHeap::Initialize(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type,
                                     uint32_t numDescriptors, bool shaderVisible)
    {
        if (m_initialized)
        {
            LOG_WARNING(LogCategory::Renderer, L"DescriptorHeap already initialized");
            return true;
        }

        if (!device || numDescriptors == 0)
        {
            LOG_ERROR(LogCategory::Renderer, L"DescriptorHeap::Initialize - invalid parameters");
            return false;
        }

        // RTV, DSV는 셰이더 가시 불가
        if (shaderVisible &&
            (type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV || type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV))
        {
            LOG_WARNING(LogCategory::Renderer,
                        L"RTV/DSV heaps cannot be shader visible, ignoring flag");
            shaderVisible = false;
        }

        m_type = type;
        m_numDescriptors = numDescriptors;
        m_shaderVisible = shaderVisible;
        m_descriptorSize = device->GetDescriptorHandleIncrementSize(type);

        // 힙 생성
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.Type = type;
        heapDesc.NumDescriptors = numDescriptors;
        heapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
                                        : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        heapDesc.NodeMask = 0;

        HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_heap));
        if (FAILED(hr))
        {
            LOG_ERROR(LogCategory::Renderer,
                      L"Failed to create DescriptorHeap ({}) (HRESULT: {:#x})",
                      GetHeapTypeName(type), static_cast<uint32_t>(hr));
            return false;
        }

        // 시작 핸들 저장
        m_cpuStartHandle = m_heap->GetCPUDescriptorHandleForHeapStart();
        if (shaderVisible)
        {
            m_gpuStartHandle = m_heap->GetGPUDescriptorHandleForHeapStart();
        }

        // 프리 리스트 초기화 (모든 인덱스 사용 가능)
        for (uint32_t i = 0; i < numDescriptors; i++)
        {
            m_freeIndices.push(i);
        }

        m_initialized = true;
        m_allocatedCount = 0;

        LOG_INFO(LogCategory::Renderer,
                 L"DescriptorHeap ({}) initialized: {} descriptors, shader visible: {}",
                 GetHeapTypeName(type), numDescriptors, shaderVisible ? L"YES" : L"NO");

        return true;
    }

    DescriptorHandle DescriptorHeap::Allocate()
    {
        DescriptorHandle handle;

        if (!m_initialized)
        {
            LOG_ERROR(LogCategory::Renderer, L"DescriptorHeap::Allocate - not initialized");
            return handle;
        }

        if (m_freeIndices.empty())
        {
            LOG_ERROR(LogCategory::Renderer,
                      L"DescriptorHeap ({}) is full (capacity: {})",
                      GetHeapTypeName(m_type), m_numDescriptors);
            return handle;
        }

        // 프리 리스트에서 인덱스 획득
        uint32_t index = m_freeIndices.front();
        m_freeIndices.pop();
        m_allocatedCount++;

        // 핸들 계산
        handle.heapIndex = index;
        handle.cpuHandle.ptr = m_cpuStartHandle.ptr + static_cast<SIZE_T>(index) * m_descriptorSize;

        if (m_shaderVisible)
        {
            handle.gpuHandle.ptr = m_gpuStartHandle.ptr + static_cast<UINT64>(index) * m_descriptorSize;
        }

        return handle;
    }

    void DescriptorHeap::Free(const DescriptorHandle& handle)
    {
        if (!m_initialized)
        {
            return;
        }

        if (!handle.IsValid() || handle.heapIndex >= m_numDescriptors)
        {
            LOG_WARNING(LogCategory::Renderer, L"DescriptorHeap::Free - invalid handle");
            return;
        }

        // 프리 리스트에 반환
        m_freeIndices.push(handle.heapIndex);
        m_allocatedCount--;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetCpuHandle(uint32_t index) const
    {
        D3D12_CPU_DESCRIPTOR_HANDLE handle = {};

        if (!m_initialized || index >= m_numDescriptors)
        {
            return handle;
        }

        handle.ptr = m_cpuStartHandle.ptr + static_cast<SIZE_T>(index) * m_descriptorSize;
        return handle;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGpuHandle(uint32_t index) const
    {
        D3D12_GPU_DESCRIPTOR_HANDLE handle = {};

        if (!m_initialized || !m_shaderVisible || index >= m_numDescriptors)
        {
            return handle;
        }

        handle.ptr = m_gpuStartHandle.ptr + static_cast<UINT64>(index) * m_descriptorSize;
        return handle;
    }
}
