/**
 * @file DescriptorHeapManager.h
 * @brief 모든 디스크립터 힙 통합 관리
 *
 * RTV, DSV, CBV_SRV_UAV, Sampler 힙을 한 곳에서 관리합니다.
 */

#pragma once

#include "DescriptorHeap.h"
#include <memory>

namespace DX12GameEngine
{
    /**
     * @brief 디스크립터 힙 관리자 설정
     */
    struct DescriptorHeapManagerDesc
    {
        uint32_t numRtvDescriptors;         // RTV 힙 크기
        uint32_t numDsvDescriptors;         // DSV 힙 크기
        uint32_t numCbvSrvUavDescriptors;   // CBV/SRV/UAV 힙 크기 (셰이더 가시)
        uint32_t numSamplerDescriptors;     // Sampler 힙 크기 (셰이더 가시)

        DescriptorHeapManagerDesc()
            : numRtvDescriptors(64)
            , numDsvDescriptors(16)
            , numCbvSrvUavDescriptors(1024)
            , numSamplerDescriptors(64)
        {
        }
    };

    /**
     * @brief 모든 디스크립터 힙을 통합 관리하는 클래스
     *
     * 타입별로 디스크립터 힙을 생성하고 관리합니다.
     */
    class DescriptorHeapManager
    {
    public:
        DescriptorHeapManager();
        ~DescriptorHeapManager();

        // 복사 및 이동 금지
        DescriptorHeapManager(const DescriptorHeapManager&) = delete;
        DescriptorHeapManager& operator=(const DescriptorHeapManager&) = delete;
        DescriptorHeapManager(DescriptorHeapManager&&) = delete;
        DescriptorHeapManager& operator=(DescriptorHeapManager&&) = delete;

        /**
         * @brief 초기화
         * @param device D3D12 디바이스
         * @param desc 힙 설정
         * @return 성공 시 true
         */
        bool Initialize(ID3D12Device* device, const DescriptorHeapManagerDesc& desc = {});

        /**
         * @brief RTV 디스크립터 할당
         */
        DescriptorHandle AllocateRtv();

        /**
         * @brief DSV 디스크립터 할당
         */
        DescriptorHandle AllocateDsv();

        /**
         * @brief CBV/SRV/UAV 디스크립터 할당
         */
        DescriptorHandle AllocateCbvSrvUav();

        /**
         * @brief Sampler 디스크립터 할당
         */
        DescriptorHandle AllocateSampler();

        /**
         * @brief RTV 디스크립터 해제
         */
        void FreeRtv(const DescriptorHandle& handle);

        /**
         * @brief DSV 디스크립터 해제
         */
        void FreeDsv(const DescriptorHandle& handle);

        /**
         * @brief CBV/SRV/UAV 디스크립터 해제
         */
        void FreeCbvSrvUav(const DescriptorHandle& handle);

        /**
         * @brief Sampler 디스크립터 해제
         */
        void FreeSampler(const DescriptorHandle& handle);

        /**
         * @brief RTV 힙 가져오기
         */
        DescriptorHeap* GetRtvHeap() { return m_rtvHeap.get(); }

        /**
         * @brief DSV 힙 가져오기
         */
        DescriptorHeap* GetDsvHeap() { return m_dsvHeap.get(); }

        /**
         * @brief CBV/SRV/UAV 힙 가져오기 (셰이더 가시)
         */
        DescriptorHeap* GetCbvSrvUavHeap() { return m_cbvSrvUavHeap.get(); }

        /**
         * @brief Sampler 힙 가져오기 (셰이더 가시)
         */
        DescriptorHeap* GetSamplerHeap() { return m_samplerHeap.get(); }

    private:
        std::unique_ptr<DescriptorHeap> m_rtvHeap;
        std::unique_ptr<DescriptorHeap> m_dsvHeap;
        std::unique_ptr<DescriptorHeap> m_cbvSrvUavHeap;
        std::unique_ptr<DescriptorHeap> m_samplerHeap;

        bool m_initialized;
    };
}
