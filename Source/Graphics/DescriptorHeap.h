/**
 * @file DescriptorHeap.h
 * @brief 디스크립터 힙 관리
 *
 * 디스크립터 힙 생성, 할당, 해제를 관리합니다.
 * 프리 리스트 방식으로 디스크립터를 재사용합니다.
 */

#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <vector>
#include <queue>
#include <cstdint>

namespace DX12GameEngine
{
    using Microsoft::WRL::ComPtr;

    /**
     * @brief 디스크립터 핸들 래퍼
     *
     * CPU/GPU 핸들과 힙 내 인덱스를 함께 관리합니다.
     */
    struct DescriptorHandle
    {
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;  // 셰이더 비가시적 힙은 무효
        uint32_t heapIndex;

        DescriptorHandle()
            : cpuHandle{}
            , gpuHandle{}
            , heapIndex(UINT32_MAX)
        {
        }

        bool IsValid() const { return heapIndex != UINT32_MAX; }
        bool IsShaderVisible() const { return gpuHandle.ptr != 0; }
    };

    /**
     * @brief 단일 디스크립터 힙 관리 클래스
     *
     * 특정 타입의 디스크립터 힙을 관리합니다.
     * 프리 리스트 방식으로 할당/해제를 처리합니다.
     */
    class DescriptorHeap
    {
    public:
        DescriptorHeap();
        ~DescriptorHeap();

        // 복사 및 이동 금지
        DescriptorHeap(const DescriptorHeap&) = delete;
        DescriptorHeap& operator=(const DescriptorHeap&) = delete;
        DescriptorHeap(DescriptorHeap&&) = delete;
        DescriptorHeap& operator=(DescriptorHeap&&) = delete;

        /**
         * @brief 힙 초기화
         * @param device D3D12 디바이스
         * @param type 힙 타입 (RTV, DSV, CBV_SRV_UAV, SAMPLER)
         * @param numDescriptors 최대 디스크립터 개수
         * @param shaderVisible 셰이더 가시 여부 (CBV_SRV_UAV, SAMPLER만 가능)
         * @return 성공 시 true
         */
        bool Initialize(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type,
                        uint32_t numDescriptors, bool shaderVisible = false);

        /**
         * @brief 디스크립터 할당
         * @return 할당된 디스크립터 핸들 (실패 시 IsValid() == false)
         */
        DescriptorHandle Allocate();

        /**
         * @brief 디스크립터 해제
         * @param handle 해제할 디스크립터 핸들
         */
        void Free(const DescriptorHandle& handle);

        /**
         * @brief 특정 인덱스의 CPU 핸들 가져오기
         * @param index 힙 내 인덱스
         * @return CPU 디스크립터 핸들
         */
        D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(uint32_t index) const;

        /**
         * @brief 특정 인덱스의 GPU 핸들 가져오기 (셰이더 가시 힙만)
         * @param index 힙 내 인덱스
         * @return GPU 디스크립터 핸들
         */
        D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(uint32_t index) const;

        /**
         * @brief 네이티브 힙 가져오기
         */
        ID3D12DescriptorHeap* GetHeap() const { return m_heap.Get(); }

        /**
         * @brief 힙 타입 가져오기
         */
        D3D12_DESCRIPTOR_HEAP_TYPE GetType() const { return m_type; }

        /**
         * @brief 디스크립터 크기 가져오기
         */
        uint32_t GetDescriptorSize() const { return m_descriptorSize; }

        /**
         * @brief 셰이더 가시 여부
         */
        bool IsShaderVisible() const { return m_shaderVisible; }

        /**
         * @brief 총 디스크립터 개수
         */
        uint32_t GetCapacity() const { return m_numDescriptors; }

        /**
         * @brief 할당된 디스크립터 개수
         */
        uint32_t GetAllocatedCount() const { return m_allocatedCount; }

    private:
        ComPtr<ID3D12DescriptorHeap> m_heap;
        D3D12_DESCRIPTOR_HEAP_TYPE m_type;
        uint32_t m_numDescriptors;
        uint32_t m_descriptorSize;
        bool m_shaderVisible;
        bool m_initialized;

        D3D12_CPU_DESCRIPTOR_HANDLE m_cpuStartHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE m_gpuStartHandle;

        // 프리 리스트 (사용 가능한 인덱스)
        std::queue<uint32_t> m_freeIndices;
        uint32_t m_allocatedCount;
    };
}
