/**
 * @file SwapChain.cpp
 * @brief DXGI 스왑체인 관리 구현
 */

#include "SwapChain.h"
#include <Utils/Logger.h>

namespace DX12GameEngine
{
    SwapChain::SwapChain()
        : m_width(0)
        , m_height(0)
        , m_format(DXGI_FORMAT_R8G8B8A8_UNORM)
        , m_vsync(true)
        , m_tearingSupported(false)
        , m_initialized(false)
    {
    }

    SwapChain::~SwapChain()
    {
        if (m_initialized)
        {
            ReleaseBackBuffers();
            LOG_INFO(LogCategory::Renderer, L"SwapChain destroyed");
        }
    }

    bool SwapChain::Initialize(IDXGIFactory4* factory, ID3D12CommandQueue* commandQueue,
                                const SwapChainDesc& desc)
    {
        if (m_initialized)
        {
            LOG_WARNING(LogCategory::Renderer, L"SwapChain already initialized");
            return true;
        }

        if (!factory || !commandQueue || !desc.hwnd)
        {
            LOG_ERROR(LogCategory::Renderer, L"SwapChain::Initialize - invalid parameters");
            return false;
        }

        m_width = desc.width;
        m_height = desc.height;
        m_format = desc.format;
        m_vsync = desc.vsync;

        // Tearing 지원 확인
        m_tearingSupported = desc.allowTearing && CheckTearingSupport(factory);

        // 스왑체인 생성
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = m_width;
        swapChainDesc.Height = m_height;
        swapChainDesc.Format = m_format;
        swapChainDesc.Stereo = FALSE;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = kBackBufferCount;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        swapChainDesc.Flags = m_tearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

        ComPtr<IDXGISwapChain1> swapChain1;
        HRESULT hr = factory->CreateSwapChainForHwnd(
            commandQueue,
            desc.hwnd,
            &swapChainDesc,
            nullptr,  // 전체화면 설명 (창 모드)
            nullptr,  // 제한 출력
            &swapChain1);

        if (FAILED(hr))
        {
            LOG_ERROR(LogCategory::Renderer,
                      L"Failed to create SwapChain (HRESULT: {:#x})",
                      static_cast<uint32_t>(hr));
            return false;
        }

        // ALT+ENTER 전체화면 전환 비활성화
        hr = factory->MakeWindowAssociation(desc.hwnd, DXGI_MWA_NO_ALT_ENTER);
        if (FAILED(hr))
        {
            LOG_WARNING(LogCategory::Renderer, L"Failed to disable ALT+ENTER");
        }

        // IDXGISwapChain4로 업캐스트
        hr = swapChain1.As(&m_swapChain);
        if (FAILED(hr))
        {
            LOG_ERROR(LogCategory::Renderer, L"Failed to get IDXGISwapChain4");
            return false;
        }

        // 백 버퍼 획득
        if (!AcquireBackBuffers())
        {
            return false;
        }

        m_initialized = true;

        LOG_INFO(LogCategory::Renderer,
                 L"SwapChain initialized ({}x{}, {} buffers, VSync: {}, Tearing: {})",
                 m_width, m_height, kBackBufferCount,
                 m_vsync ? L"ON" : L"OFF",
                 m_tearingSupported ? L"Supported" : L"Not supported");

        return true;
    }

    bool SwapChain::Present()
    {
        if (!m_initialized)
        {
            return false;
        }

        UINT syncInterval = m_vsync ? 1 : 0;
        UINT presentFlags = 0;

        // VSync OFF이고 Tearing 지원 시 ALLOW_TEARING 플래그 사용
        if (!m_vsync && m_tearingSupported)
        {
            presentFlags = DXGI_PRESENT_ALLOW_TEARING;
        }

        HRESULT hr = m_swapChain->Present(syncInterval, presentFlags);

        if (FAILED(hr))
        {
            if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
            {
                LOG_ERROR(LogCategory::Renderer, L"Device lost during Present");
                // TODO: 디바이스 복구 처리
            }
            else
            {
                LOG_ERROR(LogCategory::Renderer,
                          L"Present failed (HRESULT: {:#x})",
                          static_cast<uint32_t>(hr));
            }
            return false;
        }

        return true;
    }

    bool SwapChain::Resize(uint32_t width, uint32_t height)
    {
        if (!m_initialized)
        {
            return false;
        }

        if (width == 0 || height == 0)
        {
            LOG_WARNING(LogCategory::Renderer, L"SwapChain::Resize - invalid size ({}x{})", width, height);
            return false;
        }

        if (width == m_width && height == m_height)
        {
            return true;  // 크기 변경 없음
        }

        LOG_INFO(LogCategory::Renderer, L"Resizing SwapChain: {}x{} -> {}x{}",
                 m_width, m_height, width, height);

        // 백 버퍼 해제 (리사이즈 전에 모든 참조 해제 필요)
        ReleaseBackBuffers();

        // 스왑체인 리사이즈
        DXGI_SWAP_CHAIN_DESC1 desc;
        m_swapChain->GetDesc1(&desc);

        HRESULT hr = m_swapChain->ResizeBuffers(
            kBackBufferCount,
            width,
            height,
            m_format,
            desc.Flags);

        if (FAILED(hr))
        {
            LOG_ERROR(LogCategory::Renderer,
                      L"Failed to resize SwapChain (HRESULT: {:#x})",
                      static_cast<uint32_t>(hr));
            return false;
        }

        m_width = width;
        m_height = height;

        // 새 백 버퍼 획득
        if (!AcquireBackBuffers())
        {
            return false;
        }

        LOG_INFO(LogCategory::Renderer, L"SwapChain resized to {}x{}", m_width, m_height);

        return true;
    }

    uint32_t SwapChain::GetCurrentBackBufferIndex() const
    {
        if (!m_initialized)
        {
            return 0;
        }
        return m_swapChain->GetCurrentBackBufferIndex();
    }

    ID3D12Resource* SwapChain::GetBackBuffer(uint32_t index) const
    {
        if (index >= kBackBufferCount)
        {
            return nullptr;
        }
        return m_backBuffers[index].Get();
    }

    ID3D12Resource* SwapChain::GetCurrentBackBuffer() const
    {
        return GetBackBuffer(GetCurrentBackBufferIndex());
    }

    bool SwapChain::CheckTearingSupport(IDXGIFactory4* factory)
    {
        // IDXGIFactory5로 업캐스트해서 Tearing 지원 확인
        ComPtr<IDXGIFactory5> factory5;
        HRESULT hr = factory->QueryInterface(IID_PPV_ARGS(&factory5));

        if (SUCCEEDED(hr))
        {
            BOOL allowTearing = FALSE;
            hr = factory5->CheckFeatureSupport(
                DXGI_FEATURE_PRESENT_ALLOW_TEARING,
                &allowTearing,
                sizeof(allowTearing));

            if (SUCCEEDED(hr) && allowTearing)
            {
                LOG_INFO(LogCategory::Renderer, L"Tearing (VRR) supported");
                return true;
            }
        }

        LOG_INFO(LogCategory::Renderer, L"Tearing (VRR) not supported");
        return false;
    }

    bool SwapChain::AcquireBackBuffers()
    {
        for (uint32_t i = 0; i < kBackBufferCount; i++)
        {
            HRESULT hr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_backBuffers[i]));
            if (FAILED(hr))
            {
                LOG_ERROR(LogCategory::Renderer,
                          L"Failed to get back buffer {} (HRESULT: {:#x})",
                          i, static_cast<uint32_t>(hr));
                return false;
            }
        }

        LOG_DEBUG(LogCategory::Renderer, L"Acquired {} back buffers", kBackBufferCount);
        return true;
    }

    void SwapChain::ReleaseBackBuffers()
    {
        for (uint32_t i = 0; i < kBackBufferCount; i++)
        {
            m_backBuffers[i].Reset();
        }
    }
}
