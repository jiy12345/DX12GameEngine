/**
 * @file Device.h
 * @brief DirectX 12 디바이스 관리
 *
 * D3D12 Device는 모든 리소스와 객체 생성의 팩토리입니다.
 * DXGI Factory, Adapter 선택, Device 생성을 담당합니다.
 */

#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <string>

namespace DX12GameEngine
{
    using Microsoft::WRL::ComPtr;

    /**
     * @brief DirectX 12 디바이스를 초기화하고 관리하는 클래스
     */
    class Device
    {
    public:
        Device();
        ~Device();

        // 복사 및 이동 금지
        Device(const Device&) = delete;
        Device& operator=(const Device&) = delete;
        Device(Device&&) = delete;
        Device& operator=(Device&&) = delete;

        /**
         * @brief 디바이스 초기화
         * @param enableDebugLayer Debug Layer 활성화 여부 (Debug 빌드에서 권장)
         * @return 성공 시 true, 실패 시 false
         */
        bool Initialize(bool enableDebugLayer = true);

        /**
         * @brief D3D12 Device 가져오기
         * @return ID3D12Device 포인터
         */
        ID3D12Device* GetDevice() const { return m_device.Get(); }

        /**
         * @brief DXGI Factory 가져오기
         * @return IDXGIFactory4 포인터
         */
        IDXGIFactory4* GetFactory() const { return m_factory.Get(); }

        /**
         * @brief 선택된 어댑터 정보 가져오기
         * @return 어댑터 설명 문자열
         */
        const std::wstring& GetAdapterDescription() const { return m_adapterDescription; }

        /**
         * @brief Feature Level 가져오기
         * @return D3D_FEATURE_LEVEL
         */
        D3D_FEATURE_LEVEL GetFeatureLevel() const { return m_featureLevel; }

    private:
        /**
         * @brief Debug Layer 활성화
         * @return 성공 시 true
         */
        bool EnableDebugLayer();

        /**
         * @brief DXGI Factory 생성
         * @return 성공 시 true
         */
        bool CreateFactory();

        /**
         * @brief 최적의 하드웨어 어댑터 선택
         * @return 성공 시 true
         */
        bool SelectAdapter();

        /**
         * @brief D3D12 Device 생성
         * @return 성공 시 true
         */
        bool CreateDevice();

        /**
         * @brief Feature Level 확인
         * @return 성공 시 true
         */
        bool CheckFeatureLevel();

    private:
        // DXGI 객체
        ComPtr<IDXGIFactory4> m_factory;
        ComPtr<IDXGIAdapter1> m_adapter;

        // D3D12 객체
        ComPtr<ID3D12Device> m_device;

        // 디바이스 정보
        std::wstring m_adapterDescription;
        D3D_FEATURE_LEVEL m_featureLevel;

        // 초기화 플래그
        bool m_initialized;
    };
}
