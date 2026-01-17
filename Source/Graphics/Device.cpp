/**
 * @file Device.cpp
 * @brief DirectX 12 디바이스 구현
 */

#include "Device.h"
#include <sstream>
#include <stdexcept>

namespace DX12GameEngine
{
    Device::Device()
        : m_featureLevel(D3D_FEATURE_LEVEL_11_0)
        , m_initialized(false)
    {
    }

    Device::~Device()
    {
        // ComPtr이 자동으로 Release 처리
    }

    bool Device::Initialize(bool enableDebugLayer)
    {
        if (m_initialized)
        {
            OutputDebugStringW(L"[Device] Already initialized\n");
            return true;
        }

        OutputDebugStringW(L"[Device] Initializing DirectX 12 Device...\n");

        // 1. Debug Layer 활성화 (선택적)
        if (enableDebugLayer)
        {
            if (!EnableDebugLayer())
            {
                OutputDebugStringW(L"[Device] Warning: Failed to enable Debug Layer\n");
                // Debug Layer 실패는 치명적이지 않으므로 계속 진행
            }
        }

        // 2. DXGI Factory 생성
        if (!CreateFactory())
        {
            OutputDebugStringW(L"[Device] Error: Failed to create DXGI Factory\n");
            return false;
        }

        // 3. 하드웨어 어댑터 선택
        if (!SelectAdapter())
        {
            OutputDebugStringW(L"[Device] Error: Failed to select adapter\n");
            return false;
        }

        // 4. D3D12 Device 생성
        if (!CreateDevice())
        {
            OutputDebugStringW(L"[Device] Error: Failed to create D3D12 Device\n");
            return false;
        }

        // 5. Feature Level 확인
        if (!CheckFeatureLevel())
        {
            OutputDebugStringW(L"[Device] Error: Feature Level check failed\n");
            return false;
        }

        m_initialized = true;

        // 성공 메시지
        std::wstringstream ss;
        ss << L"[Device] Successfully initialized\n";
        ss << L"  - Adapter: " << m_adapterDescription << L"\n";
        ss << L"  - Feature Level: ";

        switch (m_featureLevel)
        {
        case D3D_FEATURE_LEVEL_11_0: ss << L"11.0\n"; break;
        case D3D_FEATURE_LEVEL_11_1: ss << L"11.1\n"; break;
        case D3D_FEATURE_LEVEL_12_0: ss << L"12.0\n"; break;
        case D3D_FEATURE_LEVEL_12_1: ss << L"12.1\n"; break;
        case D3D_FEATURE_LEVEL_12_2: ss << L"12.2\n"; break;
        default: ss << L"Unknown\n"; break;
        }

        OutputDebugStringW(ss.str().c_str());

        return true;
    }

    bool Device::EnableDebugLayer()
    {
        // Debug Layer는 런타임 설정으로 제어
        // 항상 시도하고, 실패하면 경고만 출력
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
            OutputDebugStringW(L"[Device] Debug Layer enabled\n");
            return true;
        }
        return false;
    }

    bool Device::CreateFactory()
    {
        UINT factoryFlags = 0;

        // Debug Layer가 활성화된 경우 Debug Factory 사용
        // (이미 EnableDebugLayer가 호출된 경우에만 유효)
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            factoryFlags = DXGI_CREATE_FACTORY_DEBUG;
        }

        HRESULT hr = CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&m_factory));
        if (FAILED(hr))
        {
            return false;
        }

        OutputDebugStringW(L"[Device] DXGI Factory created\n");
        return true;
    }

    bool Device::SelectAdapter()
    {
        ComPtr<IDXGIAdapter1> adapter;
        SIZE_T maxDedicatedVideoMemory = 0;
        UINT adapterIndex = 0;

        // 모든 어댑터를 순회하며 전용 비디오 메모리가 가장 큰 것을 선택
        while (m_factory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            // 소프트웨어 어댑터는 제외
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                adapterIndex++;
                continue;
            }

            // D3D12 Device 생성이 가능한지 테스트
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0,
                __uuidof(ID3D12Device), nullptr)))
            {
                // 전용 비디오 메모리가 더 큰 어댑터 선택
                if (desc.DedicatedVideoMemory > maxDedicatedVideoMemory)
                {
                    maxDedicatedVideoMemory = desc.DedicatedVideoMemory;
                    m_adapter = adapter;
                    m_adapterDescription = desc.Description;
                }
            }

            adapterIndex++;
        }

        if (!m_adapter)
        {
            OutputDebugStringW(L"[Device] No compatible adapter found\n");
            return false;
        }

        std::wstringstream ss;
        ss << L"[Device] Selected adapter: " << m_adapterDescription << L"\n";
        ss << L"  - Dedicated Video Memory: " << (maxDedicatedVideoMemory / 1024 / 1024) << L" MB\n";
        OutputDebugStringW(ss.str().c_str());

        return true;
    }

    bool Device::CreateDevice()
    {
        // D3D12 Device 생성 (Feature Level 11.0부터 시도)
        HRESULT hr = D3D12CreateDevice(
            m_adapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&m_device)
        );

        if (FAILED(hr))
        {
            OutputDebugStringW(L"[Device] Failed to create D3D12 Device\n");
            return false;
        }

        // Debug Layer가 활성화된 경우 추가 디버그 정보 설정
        ComPtr<ID3D12InfoQueue> infoQueue;
        if (SUCCEEDED(m_device.As(&infoQueue)))
        {
            // 심각도에 따라 메시지 필터링
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, FALSE);

            OutputDebugStringW(L"[Device] Debug Info Queue configured\n");
        }

        OutputDebugStringW(L"[Device] D3D12 Device created\n");
        return true;
    }

    bool Device::CheckFeatureLevel()
    {
        // 지원되는 Feature Level 확인
        D3D_FEATURE_LEVEL featureLevels[] = {
            D3D_FEATURE_LEVEL_12_2,
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0
        };

        D3D12_FEATURE_DATA_FEATURE_LEVELS featureLevelInfo = {};
        featureLevelInfo.NumFeatureLevels = _countof(featureLevels);
        featureLevelInfo.pFeatureLevelsRequested = featureLevels;

        HRESULT hr = m_device->CheckFeatureSupport(
            D3D12_FEATURE_FEATURE_LEVELS,
            &featureLevelInfo,
            sizeof(featureLevelInfo)
        );

        if (SUCCEEDED(hr))
        {
            m_featureLevel = featureLevelInfo.MaxSupportedFeatureLevel;
            return true;
        }

        // 실패하면 기본값 사용
        m_featureLevel = D3D_FEATURE_LEVEL_11_0;
        return true;
    }
}
