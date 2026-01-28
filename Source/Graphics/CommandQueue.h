/**
 * @file CommandQueue.h
 * @brief DirectX 12 커맨드 큐 관리
 *
 * GPU에 작업을 제출하는 커맨드 큐를 관리합니다.
 * Fence를 통한 CPU-GPU 동기화도 담당합니다.
 */

#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

namespace DX12GameEngine
{
    using Microsoft::WRL::ComPtr;

    /**
     * @brief DirectX 12 커맨드 큐를 관리하는 클래스
     *
     * 단일 커맨드 큐와 관련된 Fence를 관리합니다.
     * Direct, Compute, Copy 타입을 지원합니다.
     */
    class CommandQueue
    {
    public:
        CommandQueue();
        ~CommandQueue();

        // 복사 및 이동 금지
        CommandQueue(const CommandQueue&) = delete;
        CommandQueue& operator=(const CommandQueue&) = delete;
        CommandQueue(CommandQueue&&) = delete;
        CommandQueue& operator=(CommandQueue&&) = delete;

        /**
         * @brief 커맨드 큐 초기화
         * @param device D3D12 디바이스
         * @param type 커맨드 리스트 타입 (Direct, Compute, Copy)
         * @return 성공 시 true
         */
        bool Initialize(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type);

        /**
         * @brief 커맨드 리스트 실행
         * @param commandLists 실행할 커맨드 리스트 배열
         * @param count 커맨드 리스트 개수
         */
        void ExecuteCommandLists(ID3D12CommandList* const* commandLists, uint32_t count);

        /**
         * @brief GPU에 시그널 전송
         * @return 시그널된 Fence 값
         */
        uint64_t Signal();

        /**
         * @brief 특정 Fence 값까지 CPU 대기
         * @param fenceValue 대기할 Fence 값
         */
        void WaitForFenceValue(uint64_t fenceValue);

        /**
         * @brief 모든 GPU 작업 완료 대기
         */
        void Flush();

        /**
         * @brief 현재 Fence 값 조회
         * @return 마지막으로 시그널된 Fence 값
         */
        uint64_t GetCurrentFenceValue() const { return m_fenceValue; }

        /**
         * @brief GPU가 완료한 Fence 값 조회
         * @return GPU가 완료한 Fence 값
         */
        uint64_t GetCompletedFenceValue() const;

        /**
         * @brief 커맨드 큐 가져오기
         * @return ID3D12CommandQueue 포인터
         */
        ID3D12CommandQueue* GetQueue() const { return m_queue.Get(); }

        /**
         * @brief 커맨드 큐 타입 가져오기
         * @return D3D12_COMMAND_LIST_TYPE
         */
        D3D12_COMMAND_LIST_TYPE GetType() const { return m_type; }

        /**
         * @brief Fence 가져오기
         * @return ID3D12Fence 포인터
         */
        ID3D12Fence* GetFence() const { return m_fence.Get(); }

        /**
         * @brief Fence 이벤트 핸들 가져오기
         * @return Fence 이벤트 HANDLE
         */
        HANDLE GetFenceEvent() const { return m_fenceEvent; }

        /**
         * @brief 커맨드 Allocator 생성
         * @param device D3D12 디바이스
         * @return 생성된 CommandAllocator (실패 시 nullptr)
         */
        ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ID3D12Device* device);

        /**
         * @brief 커맨드 리스트 생성
         * @param device D3D12 디바이스
         * @param allocator 커맨드 Allocator
         * @param pipelineState 초기 파이프라인 상태 (선택적)
         * @return 생성된 GraphicsCommandList (실패 시 nullptr)
         */
        ComPtr<ID3D12GraphicsCommandList> CreateCommandList(
            ID3D12Device* device,
            ID3D12CommandAllocator* allocator,
            ID3D12PipelineState* pipelineState = nullptr);

    private:
        ComPtr<ID3D12CommandQueue> m_queue;
        ComPtr<ID3D12Fence> m_fence;
        uint64_t m_fenceValue;
        HANDLE m_fenceEvent;
        D3D12_COMMAND_LIST_TYPE m_type;
        bool m_initialized;
    };
}
