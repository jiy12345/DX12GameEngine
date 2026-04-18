# SwapChain (스왑체인)

## 목차

- [개요](#개요)
- [DX11 vs DX12 스왑체인 차이점](#dx11-vs-dx12-스왑체인-차이점)
  - [1. 스왑체인 생성 시 전달하는 객체](#1-스왑체인-생성-시-전달하는-객체)
  - [2. 백 버퍼 인덱스 관리](#2-백-버퍼-인덱스-관리)
  - [3. SwapEffect 제한](#3-swapeffect-제한)
  - [4. 리사이즈 처리](#4-리사이즈-처리)
  - [5. Tearing (VRR) 지원](#5-tearing-vrr-지원)
- [차이점 요약 표](#차이점-요약-표)
- [SwapChain과 Command Queue의 관계](#swapchain과-command-queue의-관계)
  - [SwapChain은 Queue를 소유하지 않는다](#swapchain은-queue를-소유하지-않는다)
  - [실제 데이터 흐름](#실제-데이터-흐름)
  - [여러 Queue를 써도 SwapChain은 하나](#여러-queue를-써도-swapchain은-하나)
  - [왜 Direct Queue만 넘길 수 있나?](#왜-direct-queue만-넘길-수-있나)
  - [SwapChain 개수 결정 기준](#swapchain-개수-결정-기준)
- [관련 API](#관련-api)
- [주의사항](#주의사항)
- [관련 개념](#관련-개념)
- [참고 자료](#참고-자료)

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

## SwapChain과 Command Queue의 관계

"SwapChain 생성 시 Command Queue를 전달한다"는 사실이 다음과 같은 오해를 일으키기 쉽습니다:

- ❌ "SwapChain이 내부에 Queue를 가진다"
- ❌ "사용자가 Draw 명령을 SwapChain에 넣으면 SwapChain이 처리한다"
- ❌ "여러 Queue를 쓰려면 SwapChain도 여러 개 필요하다"

**모두 잘못된 이해**입니다.

### SwapChain은 Queue를 소유하지 않는다

SwapChain은 **자체 Queue가 없습니다.** 대신 생성 시점에 외부 Queue를 **등록**받아, 자신이 해야 할 GPU 명령(주로 Present)을 그 Queue로 **제출 대행**시킵니다.

| 오해 | 실제 |
|------|------|
| SwapChain이 Queue를 내부에 갖는다 | SwapChain은 외부 Queue를 **빌려 씀** |
| SwapChain이 Command List를 받아 처리한다 | SwapChain은 **Command List를 받지 않음** |
| 사용자 렌더링이 SwapChain을 거친다 | 사용자는 **Command Queue에 직접 제출**함 |
| SwapChain이 렌더링 파이프라인의 일부다 | SwapChain은 **화면 출력만** 담당 |

SwapChain의 소속 레이어도 다릅니다:
- **SwapChain**: DXGI 레이어 (디스플레이 하드웨어와의 연결)
- **Command Queue**: D3D12 레이어 (GPU 작업 분배)

### 실제 데이터 흐름

사용자가 렌더링하고 화면에 표시하는 흐름:

```
┌─────────────────────────────────────────────────────┐
│ 1. 사용자가 Command List에 렌더링 명령 기록           │
│    commandList->Draw(...);                           │
│    commandList->Close();                             │
└─────────────────────────────────────────────────────┘
                        ↓
┌─────────────────────────────────────────────────────┐
│ 2. Command Queue에 직접 제출 (SwapChain 무관)        │
│    directQueue->ExecuteCommandLists(list);           │
└─────────────────────────────────────────────────────┘
                        ↓
┌─────────────────────────────────────────────────────┐
│ 3. GPU 실행 → 백 버퍼 갱신                           │
└─────────────────────────────────────────────────────┘
                        ↓
┌─────────────────────────────────────────────────────┐
│ 4. 사용자가 Present 호출                             │
│    swapChain->Present(1, 0);                         │
│                                                      │
│    SwapChain 내부:                                   │
│    - Present GPU 명령 생성                           │
│    - 등록된 Queue(directQueue)로 제출                │
│    - VSync 등 타이밍 관리                            │
└─────────────────────────────────────────────────────┘
                        ↓
┌─────────────────────────────────────────────────────┐
│ 5. GPU: 백 버퍼 → 프론트 버퍼 스왑                   │
│    디스플레이 하드웨어 신호                           │
└─────────────────────────────────────────────────────┘
```

**포인트**: 사용자의 렌더링 명령은 SwapChain을 전혀 거치지 않습니다. Command Queue에 직접 제출됩니다. SwapChain은 오직 **Present 명령만** 자신이 제출(등록된 Queue를 통해)합니다.

### 여러 Queue를 써도 SwapChain은 하나

DX12는 병렬 실행을 위해 여러 Queue를 권장합니다:
- **Direct Queue**: 일반 렌더링
- **Compute Queue**: 비동기 컴퓨트 (파티클, 광원 컬링 등)
- **Copy Queue**: 백그라운드 복사 (텍스처 업로드)

하지만 **최종적으로 화면에 나가는 건 Direct Queue가 만든 백 버퍼 하나**입니다. Compute/Copy Queue는 Direct Queue를 돕는 조력자 역할이고, 결과는 Direct Queue가 취합합니다.

```cpp
// 3개 Queue + 1개 SwapChain (전형적 구조)
ID3D12CommandQueue* directQueue;   // Present 수행 + 일반 렌더링
ID3D12CommandQueue* computeQueue;  // 비동기 컴퓨트
ID3D12CommandQueue* copyQueue;     // 백그라운드 복사

// SwapChain은 directQueue만 등록받음
IDXGISwapChain3* swapChain;
factory->CreateSwapChainForHwnd(directQueue, hwnd, &desc, ...);

// 렌더링 루프
while (running) {
    copyQueue->ExecuteCommandLists(...);    // 독립 실행
    computeQueue->ExecuteCommandLists(...); // 독립 실행
    directQueue->ExecuteCommandLists(...);  // Compute/Copy 결과 취합 후 렌더링

    swapChain->Present(1, 0);  // directQueue를 통해 Present 제출
}
```

### 왜 Direct Queue만 넘길 수 있나?

Present는 **그래픽 파이프라인 명령**이라 Direct Queue에서만 수행 가능합니다:

| 큐 타입 | Present 가능? |
|---------|--------------|
| Direct | ✅ 가능 |
| Compute | ❌ 불가 |
| Copy | ❌ 불가 |

Compute/Copy Queue를 SwapChain에 전달하면 생성이 실패합니다. 관례적으로 **Direct Queue**를 등록합니다.

### SwapChain 개수 결정 기준

SwapChain 수는 **Queue 수와 무관**하게, "화면 출력 대상의 수"에 의해 결정됩니다:

| 상황 | SwapChain 수 |
|------|-------------|
| 일반 게임 (창 하나) | 1 |
| 듀얼 모니터 각각 출력 | 2 (모니터당 1) |
| 에디터 도킹 등 창 여러 개 | 창당 1 |
| 멀티 GPU 각각 화면 출력 | GPU 수만큼 |
| VR (각 눈 별도 출력) | 1~2 (VR 런타임 방식에 따라) |

**정리**: SwapChain은 "화면과의 연결 창구"이고, Queue는 "GPU 내부 작업 채널"입니다. 역할이 완전히 달라서 개수도 독립적으로 결정됩니다.

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
- [Device](../Core/Device.md) - DXGI Factory를 생성
- [CommandQueue](../Commands/CommandQueue.md) - 스왑체인 생성 시 전달

### 연관 개념
- [RenderTargetView](../Descriptors/RenderTargetView.md) - 백 버퍼에 대한 RTV 생성
- [ResourceBarriers](../Resources/ResourceBarriers.md) - 백 버퍼 상태 전환 (PRESENT ↔ RENDER_TARGET)
- [Synchronization](../Synchronization/Synchronization.md) - 프레임 동기화

## 참고 자료

- [Microsoft: DXGI 1.4 Improvements](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-1-4-improvements)
- [Microsoft: IDXGISwapChain3::GetCurrentBackBufferIndex](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_4/nf-dxgi1_4-idxgiswapchain3-getcurrentbackbufferindex)
- [Microsoft: For best performance, use DXGI flip model](https://devblogs.microsoft.com/directx/dxgi-flip-model/)
- [Microsoft: IDXGISwapChain::ResizeBuffers](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-resizebuffers)
- [Microsoft: Variable refresh rate displays](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/variable-refresh-rate-displays)
- [Microsoft: IDXGISwapChain interface](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nn-dxgi-idxgiswapchain)
