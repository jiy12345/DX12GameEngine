/**
 * @file DescriptorHeapManager.cpp
 * @brief 디스크립터 힙 관리자 구현
 */

#include "DescriptorHeapManager.h"
#include <Utils/Logger.h>

namespace DX12GameEngine
{
    DescriptorHeapManager::DescriptorHeapManager()
        : m_initialized(false)
    {
    }

    DescriptorHeapManager::~DescriptorHeapManager()
    {
        if (m_initialized)
        {
            LOG_INFO(LogCategory::Renderer, L"DescriptorHeapManager destroyed");
        }
    }

    bool DescriptorHeapManager::Initialize(ID3D12Device* device, const DescriptorHeapManagerDesc& desc)
    {
        if (m_initialized)
        {
            LOG_WARNING(LogCategory::Renderer, L"DescriptorHeapManager already initialized");
            return true;
        }

        if (!device)
        {
            LOG_ERROR(LogCategory::Renderer, L"DescriptorHeapManager::Initialize - device is null");
            return false;
        }

        // RTV 힙 생성 (셰이더 비가시)
        m_rtvHeap = std::make_unique<DescriptorHeap>();
        if (!m_rtvHeap->Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
                                    desc.numRtvDescriptors, false))
        {
            LOG_ERROR(LogCategory::Renderer, L"Failed to create RTV heap");
            return false;
        }

        // DSV 힙 생성 (셰이더 비가시)
        m_dsvHeap = std::make_unique<DescriptorHeap>();
        if (!m_dsvHeap->Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
                                    desc.numDsvDescriptors, false))
        {
            LOG_ERROR(LogCategory::Renderer, L"Failed to create DSV heap");
            return false;
        }

        // CBV/SRV/UAV 힙 생성 (셰이더 가시)
        m_cbvSrvUavHeap = std::make_unique<DescriptorHeap>();
        if (!m_cbvSrvUavHeap->Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                                          desc.numCbvSrvUavDescriptors, true))
        {
            LOG_ERROR(LogCategory::Renderer, L"Failed to create CBV/SRV/UAV heap");
            return false;
        }

        // Sampler 힙 생성 (셰이더 가시)
        m_samplerHeap = std::make_unique<DescriptorHeap>();
        if (!m_samplerHeap->Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
                                        desc.numSamplerDescriptors, true))
        {
            LOG_ERROR(LogCategory::Renderer, L"Failed to create Sampler heap");
            return false;
        }

        m_initialized = true;

        LOG_INFO(LogCategory::Renderer, L"DescriptorHeapManager initialized");

        return true;
    }

    DescriptorHandle DescriptorHeapManager::AllocateRtv()
    {
        if (!m_initialized || !m_rtvHeap)
        {
            return DescriptorHandle();
        }
        return m_rtvHeap->Allocate();
    }

    DescriptorHandle DescriptorHeapManager::AllocateDsv()
    {
        if (!m_initialized || !m_dsvHeap)
        {
            return DescriptorHandle();
        }
        return m_dsvHeap->Allocate();
    }

    DescriptorHandle DescriptorHeapManager::AllocateCbvSrvUav()
    {
        if (!m_initialized || !m_cbvSrvUavHeap)
        {
            return DescriptorHandle();
        }
        return m_cbvSrvUavHeap->Allocate();
    }

    DescriptorHandle DescriptorHeapManager::AllocateSampler()
    {
        if (!m_initialized || !m_samplerHeap)
        {
            return DescriptorHandle();
        }
        return m_samplerHeap->Allocate();
    }

    void DescriptorHeapManager::FreeRtv(const DescriptorHandle& handle)
    {
        if (m_initialized && m_rtvHeap)
        {
            m_rtvHeap->Free(handle);
        }
    }

    void DescriptorHeapManager::FreeDsv(const DescriptorHandle& handle)
    {
        if (m_initialized && m_dsvHeap)
        {
            m_dsvHeap->Free(handle);
        }
    }

    void DescriptorHeapManager::FreeCbvSrvUav(const DescriptorHandle& handle)
    {
        if (m_initialized && m_cbvSrvUavHeap)
        {
            m_cbvSrvUavHeap->Free(handle);
        }
    }

    void DescriptorHeapManager::FreeSampler(const DescriptorHandle& handle)
    {
        if (m_initialized && m_samplerHeap)
        {
            m_samplerHeap->Free(handle);
        }
    }
}
