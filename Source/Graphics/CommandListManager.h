/**
 * @file CommandListManager.h
 * @brief Command Allocator와 Command List 풀링 관리
 *
 * 프레임 기반 Allocator 풀링과 CommandList 재사용을 관리합니다.
 * Triple Buffering(3프레임)을 기본으로 사용합니다.
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

    /** @brief 동시 처리 가능한 최대 프레임 수 */
    static constexpr uint32_t kMaxFramesInFlight = 3;

    /**
     * @brief Command Allocator와 Command List 풀링 관리자
     *
     * 프레임 기반으로 Allocator를 관리하고, CommandList를 풀링하여
     * 효율적인 재사용을 지원합니다.
     *
     * 사용 흐름:
     * 1. BeginFrame() - 현재 프레임의 Allocator 준비
     * 2. GetCommandList() - CommandList 획득
     * 3. 명령 기록 후 Close()
     * 4. ReturnCommandList() - CommandList 반환
     * 5. EndFrame() - 프레임 종료 및 Fence 값 기록
     */
    class CommandListManager
    {
    public:
        CommandListManager();
        ~CommandListManager();

        // 복사 및 이동 금지
        CommandListManager(const CommandListManager&) = delete;
        CommandListManager& operator=(const CommandListManager&) = delete;
        CommandListManager(CommandListManager&&) = delete;
        CommandListManager& operator=(CommandListManager&&) = delete;

        /**
         * @brief 초기화
         * @param device D3D12 디바이스
         * @param type Command List 타입 (Direct, Compute, Copy)
         * @param initialListCount 초기 CommandList 풀 크기
         * @return 성공 시 true
         */
        bool Initialize(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type,
                        uint32_t initialListCount = 4);

        /**
         * @brief 프레임 시작
         *
         * 현재 프레임의 Allocator가 GPU에서 사용 완료되었는지 확인하고 Reset합니다.
         * 매 프레임 렌더링 시작 시 호출해야 합니다.
         *
         * @param fence 동기화용 Fence
         * @param fenceEvent Fence 이벤트 핸들
         */
        void BeginFrame(ID3D12Fence* fence, HANDLE fenceEvent);

        /**
         * @brief 프레임 종료
         *
         * 현재 프레임의 Fence 값을 기록하고 다음 프레임으로 이동합니다.
         *
         * @param fenceValue 현재 프레임의 Fence 값
         */
        void EndFrame(uint64_t fenceValue);

        /**
         * @brief CommandList 획득
         *
         * 풀에서 사용 가능한 CommandList를 가져옵니다.
         * 없으면 새로 생성합니다.
         * 반환된 CommandList는 이미 Reset된 상태입니다.
         *
         * @param pipelineState 초기 파이프라인 상태 (선택적)
         * @return CommandList 포인터 (실패 시 nullptr)
         */
        ID3D12GraphicsCommandList* GetCommandList(ID3D12PipelineState* pipelineState = nullptr);

        /**
         * @brief CommandList 반환
         *
         * 사용 완료된 CommandList를 풀에 반환합니다.
         * Close() 호출 후에 반환해야 합니다.
         *
         * @param commandList 반환할 CommandList
         */
        void ReturnCommandList(ID3D12GraphicsCommandList* commandList);

        /**
         * @brief 현재 프레임의 Allocator 가져오기
         * @return 현재 프레임의 CommandAllocator
         */
        ID3D12CommandAllocator* GetCurrentAllocator() const;

        /**
         * @brief 현재 프레임 인덱스 가져오기
         * @return 현재 프레임 인덱스 (0 ~ kMaxFramesInFlight-1)
         */
        uint32_t GetCurrentFrameIndex() const { return m_currentFrameIndex; }

    private:
        /**
         * @brief 새 CommandList 생성
         * @return 생성된 CommandList의 풀 내 인덱스
         */
        size_t CreateNewCommandList();

        ID3D12Device* m_device;
        D3D12_COMMAND_LIST_TYPE m_type;
        bool m_initialized;

        // 프레임별 Allocator (Triple Buffering)
        ComPtr<ID3D12CommandAllocator> m_allocators[kMaxFramesInFlight];
        uint64_t m_fenceValues[kMaxFramesInFlight];
        uint32_t m_currentFrameIndex;

        // CommandList 풀
        std::vector<ComPtr<ID3D12GraphicsCommandList>> m_commandListPool;
        std::queue<size_t> m_availableIndices;  // 사용 가능한 CommandList 인덱스
    };
}
