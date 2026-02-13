/**
 * @file SwapChain.h
 * @brief DXGI 스왑체인 관리
 *
 * 더블/트리플 버퍼링을 지원하는 스왑체인을 관리합니다.
 * Present, 리사이즈, VSync 등을 담당합니다.
 */

#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cstdint>

namespace DX12GameEngine
{
    using Microsoft::WRL::ComPtr;

    /** @brief 백 버퍼 개수 (Triple Buffering) */
    static constexpr uint32_t kBackBufferCount = 3;

    /**
     * @brief 스왑체인 설정
     */
    struct SwapChainDesc
    {
        HWND hwnd;              // 렌더링할 윈도우 핸들
        uint32_t width;         // 너비
        uint32_t height;        // 높이
        DXGI_FORMAT format;     // 백 버퍼 포맷
        bool vsync;             // 수직 동기화
        bool allowTearing;      // Tearing 허용 (VRR/FreeSync)

        SwapChainDesc()
            : hwnd(nullptr)
            , width(1280)
            , height(720)
            , format(DXGI_FORMAT_R8G8B8A8_UNORM)
            , vsync(true)
            , allowTearing(false)
        {
        }
    };

    /**
     * @brief DXGI 스왑체인 관리 클래스
     *
     * Triple Buffering을 기본으로 사용하며,
     * VSync 및 Tearing(FreeSync/G-Sync) 모드를 지원합니다.
     */
    class SwapChain
    {
    public:
        SwapChain();
        ~SwapChain();

        // 복사 및 이동 금지
        SwapChain(const SwapChain&) = delete;
        SwapChain& operator=(const SwapChain&) = delete;
        SwapChain(SwapChain&&) = delete;
        SwapChain& operator=(SwapChain&&) = delete;

        /**
         * @brief 스왑체인 초기화
         * @param factory DXGI Factory
         * @param commandQueue 커맨드 큐 (Present용)
         * @param desc 스왑체인 설정
         * @return 성공 시 true
         */
        bool Initialize(IDXGIFactory4* factory, ID3D12CommandQueue* commandQueue,
                        const SwapChainDesc& desc);

        /**
         * @brief 화면에 표시 (Present)
         * @return 성공 시 true
         */
        bool Present();

        /**
         * @brief 리사이즈 처리
         * @param width 새 너비
         * @param height 새 높이
         * @return 성공 시 true
         */
        bool Resize(uint32_t width, uint32_t height);

        /**
         * @brief 현재 백 버퍼 인덱스 가져오기
         * @return 현재 백 버퍼 인덱스 (0 ~ kBackBufferCount-1)
         */
        uint32_t GetCurrentBackBufferIndex() const;

        /**
         * @brief 백 버퍼 리소스 가져오기
         * @param index 백 버퍼 인덱스
         * @return ID3D12Resource 포인터
         */
        ID3D12Resource* GetBackBuffer(uint32_t index) const;

        /**
         * @brief 현재 백 버퍼 리소스 가져오기
         * @return 현재 백 버퍼의 ID3D12Resource 포인터
         */
        ID3D12Resource* GetCurrentBackBuffer() const;

        /**
         * @brief 스왑체인 너비 가져오기
         */
        uint32_t GetWidth() const { return m_width; }

        /**
         * @brief 스왑체인 높이 가져오기
         */
        uint32_t GetHeight() const { return m_height; }

        /**
         * @brief 백 버퍼 포맷 가져오기
         */
        DXGI_FORMAT GetFormat() const { return m_format; }

        /**
         * @brief VSync 설정 변경
         */
        void SetVSync(bool enabled) { m_vsync = enabled; }

        /**
         * @brief VSync 상태 확인
         */
        bool IsVSyncEnabled() const { return m_vsync; }

    private:
        /**
         * @brief Tearing 지원 여부 확인
         */
        bool CheckTearingSupport(IDXGIFactory4* factory);

        /**
         * @brief 백 버퍼 리소스 획득
         */
        bool AcquireBackBuffers();

        /**
         * @brief 백 버퍼 리소스 해제
         */
        void ReleaseBackBuffers();

        ComPtr<IDXGISwapChain4> m_swapChain;
        ComPtr<ID3D12Resource> m_backBuffers[kBackBufferCount];

        uint32_t m_width;
        uint32_t m_height;
        DXGI_FORMAT m_format;
        bool m_vsync;
        bool m_tearingSupported;
        bool m_initialized;
    };
}
