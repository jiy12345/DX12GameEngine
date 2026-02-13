/**
 * @file Renderer.h
 * @brief 렌더링 서브시스템
 *
 * DX12 렌더링 전체를 관리합니다. Device, CommandQueue, SwapChain 등
 * 모든 렌더링 관련 객체를 내부적으로 캡슐화합니다.
 */

#pragma once

#include "SwapChain.h"
#include "DescriptorHeap.h"
#include <Windows.h>
#include <memory>

namespace DX12GameEngine
{
    class Device;
    class CommandQueue;
    class CommandListManager;
    class DescriptorHeapManager;

    /**
     * @brief 렌더러 설정
     *
     * 기본값은 EngineDesc에서 빌드 구성에 따라 설정됩니다.
     * 여기서는 타입만 정의합니다.
     */
    struct RendererDesc
    {
        bool enableDebugLayer;  // Debug Layer 활성화
        bool vsync;             // 수직 동기화
        int msaaSamples;        // MSAA 샘플 수 (1, 2, 4, 8)
        bool hdr;               // HDR 렌더링 (나중에)

        // TODO: Phase 2+에서 추가
        // int maxFramesInFlight;   // 동시 처리 프레임 수
        // bool raytracing;         // 레이트레이싱 활성화

        // 기본 생성자 (EngineDesc에서 설정)
        RendererDesc()
            : enableDebugLayer(false)
            , vsync(true)
            , msaaSamples(1)
            , hdr(false)
        {
        }
    };

    /**
     * @brief 렌더링 서브시스템
     *
     * DX12 렌더링 파이프라인 전체를 관리합니다.
     * Device, CommandQueue, SwapChain 등 내부 구현은 완전히 캡슐화되어
     * 외부에 노출되지 않습니다.
     */
    class Renderer
    {
    public:
        Renderer();
        ~Renderer();

        // 복사 및 이동 금지
        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;
        Renderer(Renderer&&) = delete;
        Renderer& operator=(Renderer&&) = delete;

        /**
         * @brief 렌더러 초기화
         * @param hwnd 렌더링할 윈도우 핸들
         * @param width 윈도우 너비
         * @param height 윈도우 높이
         * @param desc 렌더러 설정
         * @return 성공 시 true
         */
        bool Initialize(HWND hwnd, int width, int height, const RendererDesc& desc);

        /**
         * @brief 프레임 시작
         */
        void BeginFrame();

        /**
         * @brief 렌더링 수행
         */
        void RenderFrame();

        /**
         * @brief 프레임 종료
         */
        void EndFrame();

        /**
         * @brief 윈도우 리사이즈 처리
         * @param width 새 너비
         * @param height 새 높이
         */
        void OnResize(int width, int height);

        /**
         * @brief 현재 백 버퍼의 RTV 핸들 가져오기
         * @return 현재 백 버퍼에 대한 CPU 디스크립터 핸들
         */
        D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRtvHandle() const;

    private:
        // DX12 객체들 (완전히 캡슐화, 외부 노출 없음)
        std::unique_ptr<Device> m_device;
        std::unique_ptr<CommandQueue> m_commandQueue;
        std::unique_ptr<CommandListManager> m_commandListManager;
        std::unique_ptr<SwapChain> m_swapChain;
        std::unique_ptr<DescriptorHeapManager> m_descriptorHeapManager;

        /**
         * @brief 백 버퍼에 대한 RTV 생성
         * @return 성공 시 true
         */
        bool CreateRenderTargetViews();

        /**
         * @brief RTV 해제
         */
        void ReleaseRenderTargetViews();

        // RTV 핸들 (백 버퍼별)
        DescriptorHandle m_rtvHandles[kBackBufferCount];

        // 상태
        bool m_initialized;
        int m_width;
        int m_height;
    };
}
