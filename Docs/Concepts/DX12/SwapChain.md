# SwapChain (스왑체인)

## 개요

스왑체인은 화면에 표시할 백 버퍼들을 관리하고, 렌더링 결과를 디스플레이에 표시(Present)하는 DXGI 객체입니다. DX12에서는 DX11과 달리 개발자가 백 버퍼 인덱스를 직접 추적하고, GPU 동기화를 명시적으로 관리해야 합니다.

## DX11 vs DX12 스왑체인 차이점

### 1. 스왑체인 생성 시 전달하는 객체

**DX11**: Device를 전달

```cpp
// DX11: Device로 스왑체인 생성
factory->CreateSwapChainForHwnd(
    device,           // ID3D11Device*
    hwnd,
    &swapChainDesc,
    nullptr, nullptr,
    &swapChain);
```

**DX12**: CommandQueue를 전달

```cpp
// DX12: CommandQueue로 스왑체인 생성
factory->CreateSwapChainForHwnd(
    commandQueue,     // ID3D12CommandQueue*
    hwnd,
    &swapChainDesc,
    nullptr, nullptr,
    &swapChain);
```

**이유**: Microsoft 공식 문서:

> "In Direct3D 12, the swap chain's frame latency is determined by a queue rather than a device."
>
> "Direct3D 12 uses the command queue because that is the engine (CPU timeline) that is associated with the presentation work."
>
> — [Microsoft: DXGI 1.4 Improvements](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-1-4-improvements)

DX12에서는 Device가 아닌 CommandQueue가 실제 GPU 작업 제출을 담당하므로, Present 작업도 CommandQueue와 연관됩니다.

### 2. 백 버퍼 인덱스 관리

**DX11**: 드라이버가 자동 관리

```cpp
// DX11: 항상 버퍼 0번을 가져오면 됨
swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
// Present 후에도 같은 방식으로 접근
```

**DX12**: 개발자가 직접 추적

```cpp
// DX12: 현재 백 버퍼 인덱스를 직접 조회해야 함
UINT currentIndex = swapChain->GetCurrentBackBufferIndex();
ID3D12Resource* currentBackBuffer = backBuffers[currentIndex];
```

**이유**: Microsoft 공식 문서:

> "Use IDXGISwapChain3::GetCurrentBackBufferIndex to get the index of the current back buffer."
>
> "In Direct3D 12, the application is responsible for managing the indices of the swap chain's back buffers."
>
> — [Microsoft: IDXGISwapChain3::GetCurrentBackBufferIndex](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_4/nf-dxgi1_4-idxgiswapchain3-getcurrentbackbufferindex)

DX12의 명시적 제어 철학에 따라, 어떤 버퍼에 렌더링할지 개발자가 직접 관리합니다.

### 3. SwapEffect 제한

**DX11**: 여러 SwapEffect 지원

- `DXGI_SWAP_EFFECT_DISCARD`
- `DXGI_SWAP_EFFECT_SEQUENTIAL`
- `DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL`
- `DXGI_SWAP_EFFECT_FLIP_DISCARD`

**DX12**: FLIP 모델만 지원

- `DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL`
- `DXGI_SWAP_EFFECT_FLIP_DISCARD` (권장)

**이유**: Microsoft 공식 문서:

> "Direct3D 12 only supports the flip presentation model (DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL and DXGI_SWAP_EFFECT_FLIP_DISCARD)."
>
> — [Microsoft: For best performance, use DXGI flip model](https://devblogs.microsoft.com/directx/dxgi-flip-model/)

FLIP 모델은:
- 컴포지터(DWM)와 효율적으로 연동
- 메모리 복사 대신 버퍼 포인터 교환
- VRR(Variable Refresh Rate) 지원 가능

### 4. 리사이즈 처리

**DX11**: 상대적으로 간단

```cpp
// DX11: RTV 해제 후 리사이즈
renderTargetView->Release();
backBuffer->Release();

swapChain->ResizeBuffers(0, newWidth, newHeight, DXGI_FORMAT_UNKNOWN, 0);

swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
device->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView);
```

**DX12**: 모든 참조 완전 해제 필수

```cpp
// DX12: 모든 백 버퍼 참조를 명시적으로 해제해야 함
// GPU 작업 완료 대기 필수!
commandQueue->Signal(fence, ++fenceValue);
WaitForFenceValue(fence, fenceValue);

for (UINT i = 0; i < bufferCount; i++)
{
    backBuffers[i].Reset();  // 모든 COM 참조 해제
}

swapChain->ResizeBuffers(
    bufferCount,
    newWidth,
    newHeight,
    format,
    flags);

// 새 백 버퍼 획득
for (UINT i = 0; i < bufferCount; i++)
{
    swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffers[i]));
}
```

**이유**: Microsoft 공식 문서:

> "You can't call ResizeBuffers on a swap chain unless you've released all references to the swap chain's buffers."
>
> "For Direct3D 12, this includes command lists that reference any of the swap chain's back buffers."
>
> — [Microsoft: IDXGISwapChain::ResizeBuffers](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-resizebuffers)

DX12에서는 개발자가 리소스 수명을 명시적으로 관리하므로, 리사이즈 전에 GPU 작업 완료와 모든 참조 해제를 보장해야 합니다.

### 5. Tearing (VRR) 지원

**DX11**: 제한적 지원

**DX12**: DXGI 1.5+ ALLOW_TEARING 플래그

```cpp
// Tearing 지원 확인
BOOL allowTearing = FALSE;
factory5->CheckFeatureSupport(
    DXGI_FEATURE_PRESENT_ALLOW_TEARING,
    &allowTearing,
    sizeof(allowTearing));

// 스왑체인 생성 시 플래그 설정
swapChainDesc.Flags = allowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

// Present 시 플래그 사용 (VSync OFF일 때만)
UINT presentFlags = (!vsync && tearingSupported) ? DXGI_PRESENT_ALLOW_TEARING : 0;
swapChain->Present(vsync ? 1 : 0, presentFlags);
```

**이유**: Microsoft 공식 문서:

> "Variable refresh rate displays require tearing to be enabled."
>
> "To use DXGI_PRESENT_ALLOW_TEARING, the swap chain must have been created with DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING."
>
> — [Microsoft: Variable refresh rate displays](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/variable-refresh-rate-displays)

## 차이점 요약 표

| 항목 | DX11 | DX12 | 이유 |
|------|------|------|------|
| **생성 시 전달** | Device | CommandQueue | Present가 Queue와 연관 |
| **백 버퍼 추적** | 자동 (항상 버퍼 0) | 수동 (`GetCurrentBackBufferIndex`) | 명시적 제어 철학 |
| **SwapEffect** | 모든 모드 | FLIP 모델만 | 성능 및 DWM 연동 |
| **리사이즈** | RTV만 해제 | **모든 참조 해제 + GPU 대기** | 명시적 수명 관리 |
| **Tearing/VRR** | 제한적 | ALLOW_TEARING 플래그 | VRR 디스플레이 지원 |

## 관련 API

### 스왑체인 생성

```cpp
DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
swapChainDesc.Width = width;
swapChainDesc.Height = height;
swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
swapChainDesc.SampleDesc.Count = 1;
swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
swapChainDesc.BufferCount = 3;  // Triple Buffering
swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
swapChainDesc.Flags = tearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

ComPtr<IDXGISwapChain1> swapChain1;
factory->CreateSwapChainForHwnd(
    commandQueue,     // DX12: CommandQueue 전달
    hwnd,
    &swapChainDesc,
    nullptr,
    nullptr,
    &swapChain1);

// IDXGISwapChain4로 업캐스트 (GetCurrentBackBufferIndex 사용을 위해)
swapChain1.As(&swapChain4);
```

### 주요 메서드

```cpp
// 현재 백 버퍼 인덱스 조회 (IDXGISwapChain3+)
UINT GetCurrentBackBufferIndex();

// 백 버퍼 리소스 획득
HRESULT GetBuffer(UINT Buffer, REFIID riid, void** ppSurface);

// 화면에 표시
HRESULT Present(UINT SyncInterval, UINT Flags);
// SyncInterval: 0 = VSync OFF, 1+ = VSync ON
// Flags: DXGI_PRESENT_ALLOW_TEARING 등

// 리사이즈
HRESULT ResizeBuffers(
    UINT BufferCount,
    UINT Width,
    UINT Height,
    DXGI_FORMAT NewFormat,
    UINT SwapChainFlags);
```

## 주의사항

- **리사이즈 전 동기화**: ResizeBuffers 호출 전에 반드시 GPU 작업 완료를 대기하고, 모든 백 버퍼 참조를 해제해야 합니다.
- **인덱스 추적**: Present 후 `GetCurrentBackBufferIndex()`가 반환하는 값이 변경됩니다.
- **FLIP 모델 제약**: FLIP 모델에서는 `BufferCount`가 최소 2 이상이어야 합니다.
- **Tearing 플래그**: VSync OFF이고 Tearing이 지원될 때만 `DXGI_PRESENT_ALLOW_TEARING` 사용. VSync ON일 때 사용하면 실패합니다.

## 관련 개념

### 선행 개념
- [Device](./Device.md) - DXGI Factory를 생성
- [CommandQueue](./CommandQueue.md) - 스왑체인 생성 시 전달

### 연관 개념
- [RenderTargetView](./RenderTargetView.md) - 백 버퍼에 대한 RTV 생성
- [ResourceBarriers](./ResourceBarriers.md) - 백 버퍼 상태 전환 (PRESENT ↔ RENDER_TARGET)
- [Synchronization](./Synchronization.md) - 프레임 동기화

## 참고 자료

- [Microsoft: DXGI 1.4 Improvements](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-1-4-improvements)
- [Microsoft: IDXGISwapChain3::GetCurrentBackBufferIndex](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_4/nf-dxgi1_4-idxgiswapchain3-getcurrentbackbufferindex)
- [Microsoft: For best performance, use DXGI flip model](https://devblogs.microsoft.com/directx/dxgi-flip-model/)
- [Microsoft: IDXGISwapChain::ResizeBuffers](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-resizebuffers)
- [Microsoft: Variable refresh rate displays](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/variable-refresh-rate-displays)
- [Microsoft: IDXGISwapChain interface](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nn-dxgi-idxgiswapchain)
