# SwapChain (스왑체인)

## 목차

- [개요](#개요)
- [DX11 vs DX12 스왑체인 차이점](#dx11-vs-dx12-스왑체인-차이점)
  - [1. 스왑체인 생성 시 전달하는 객체](#1-스왑체인-생성-시-전달하는-객체)
  - [2. 백 버퍼 인덱스 관리](#2-백-버퍼-인덱스-관리)
    - [설계 철학: 왜 수동 관리인가?](#설계-철학-왜-수동-관리인가)
    - [장단점과 실무 패턴](#장단점과-실무-패턴)
  - [3. SwapEffect 제한](#3-swapeffect-제한)
    - [각 SwapEffect 상세](#각-swapeffect-상세)
    - [BitBlt 모델 vs FLIP 모델](#bitblt-모델-vs-flip-모델)
    - [왜 DX12는 FLIP만 지원하나?](#왜-dx12는-flip만-지원하나)
    - [FLIP_DISCARD vs FLIP_SEQUENTIAL 선택](#flip_discard-vs-flip_sequential-선택)
  - [4. 리사이즈 처리](#4-리사이즈-처리)
  - [5. Tearing과 VRR 지원](#5-tearing과-vrr-지원)
    - [Tearing이란?](#tearing이란)
    - [왜 Tearing이 발생하는가](#왜-tearing이-발생하는가)
    - [VSync: 전통적 해결책과 한계](#vsync-전통적-해결책과-한계)
    - [VRR: 현대적 해결책](#vrr-현대적-해결책)
    - [DX12의 ALLOW_TEARING 플래그](#dx12의-allow_tearing-플래그)
    - [실무 권장: 언제 어떤 모드를 쓰나](#실무-권장-언제-어떤-모드를-쓰나)
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

#### 설계 철학: 왜 수동 관리인가?

DX12가 백 버퍼 인덱스를 개발자에게 노출한 것은 세 가지 근본 이유가 맞물린 결과입니다.

##### 1. FLIP 모델이 기본이 되었기 때문

DX11은 주로 **BitBlt 모델**을 썼습니다:
- 백 버퍼 1개 + 프론트 버퍼 1개
- Present = 백 → 프론트로 **복사**
- "현재 백 버퍼"가 항상 같은 리소스 → 인덱스 고정 가능

DX12는 **FLIP 모델**만 지원합니다:
- 여러 버퍼(보통 2~3개)를 링으로 돌려씀
- Present = 버퍼 **포인터 스왑** (복사 없음, 빠름)
- "현재 백 버퍼"가 매 프레임 바뀜 → 인덱스 추적 필요

##### 2. 명시적 리소스 관리와의 일관성

DX12에서 백 버퍼는 **다른 리소스와 동일한 `ID3D12Resource`** 입니다. RTV, 배리어, 디스크립터 등 모든 리소스 시스템이 "어느 버퍼를 지금 다루는지"를 명시적으로 알아야 동작합니다. 드라이버가 이를 숨기면 나머지 리소스 관리와 일관성이 깨집니다.

##### 3. 멀티스레드 기록과 프레임 파이프라이닝

여러 스레드가 **다음 프레임**의 커맨드를 미리 기록하려면, 해당 프레임이 어느 버퍼를 대상으로 할지 **미리 결정**할 수 있어야 합니다. 드라이버가 인덱스를 동적으로 숨기면 예측 불가능해지고, 이는 CPU-GPU 파이프라이닝과 멀티스레드 기록 전략을 방해합니다.

#### 장단점과 실무 패턴

##### 장점

| 장점 | 설명 |
|------|------|
| **예측 가능성** | 언제 인덱스가 바뀌는지 개발자가 앎 (Present 직후). 멀티스레드 race condition 방지 |
| **프레임별 독립 리소스** | 버퍼 인덱스로 `{백버퍼, Allocator, CBV, Fence}` 세트를 선택 가능 |
| **드라이버 오버헤드 제거** | 현재 버퍼 추적 로직 없음. FLIP 모델로 복사 없음 |
| **N-buffering 자유** | Double(2)/Triple(3)/Quad(4) buffering 선택 가능 |
| **리소스 수명 명시** | 각 백 버퍼의 GPU 사용 시점을 Fence로 명확히 추적 |

##### 단점

| 단점 | 설명 |
|------|------|
| **코드 복잡도** | `BufferCount`만큼 리소스/RTV/Fence 관리 보일러플레이트 |
| **실수 위험** | Present 후 인덱스 갱신 누락 시 잘못된 버퍼에 렌더링 (증상 미묘) |
| **N-buffering 변경 부담** | 버퍼 수 전환 시 구조 조정 필요 |
| **리사이즈 처리 복잡** | 모든 백 버퍼 해제 + 재생성 필요 (아래 "리사이즈 처리" 섹션 참조) |
| **학습 곡선** | DX11 대비 초심자 진입 장벽 ↑ |

##### Trade-off 요약

| 축 | DX11 (자동) | DX12 (수동) |
|---|-----------|-----------|
| 코드 간결성 | **간단** | 복잡 |
| 성능 | 적당 | **높음** |
| 예측 가능성 | 낮음 | **높음** |
| 멀티스레드 기록 | 어려움 | **자연스러움** |
| 프레임 파이프라이닝 | 숨겨져 있음 | **명시적 제어** |

##### 실무 패턴: Frame-Indexed Resources

엔진은 "프레임별 리소스 세트"를 묶어 관리하는 패턴으로 보일러플레이트를 추상화합니다:

```cpp
struct FrameResources {
    ComPtr<ID3D12Resource> backBuffer;
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12Resource> constantBuffer;  // 프레임별 CBV 등
    UINT64 fenceValue;
};

std::vector<FrameResources> frames;  // BufferCount 만큼

void RenderFrame() {
    UINT idx = swapChain->GetCurrentBackBufferIndex();
    auto& frame = frames[idx];

    // GPU가 이 프레임의 이전 작업을 완료할 때까지 대기
    WaitForFenceValue(frame.fenceValue);

    // idx 하나로 모든 리소스 선택
    commandList->Reset(frame.allocator.Get(), ...);
    // ... 렌더링 (frame.constantBuffer, frame.backBuffer 사용)
    commandQueue->ExecuteCommandLists(1, &list);

    swapChain->Present(1, 0);
    commandQueue->Signal(fence, ++nextFenceValue);
    frame.fenceValue = nextFenceValue;
}
```

**핵심**: 버퍼 인덱스를 "기준 키"로 삼아 프레임별 자원을 모두 묶어 관리하면, **DX12의 명시적 제어 이득은 챙기면서 사용 측 코드는 간결**해집니다.

이 패턴은 CPU와 GPU가 **동시에 서로 다른 프레임을 다룰 수 있게** 하는 파이프라이닝의 기반이기도 합니다. 구현 이슈: [#45 프레임 파이프라이닝 (Frame-Indexed Resources) 구현](https://github.com/jiy12345/DX12GameEngine/issues/45)

### 3. SwapEffect 제한

**DX11**: 네 가지 SwapEffect를 지원 (BitBlt 모델과 FLIP 모델 혼재)

- `DXGI_SWAP_EFFECT_DISCARD`
- `DXGI_SWAP_EFFECT_SEQUENTIAL`
- `DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL`
- `DXGI_SWAP_EFFECT_FLIP_DISCARD`

**DX12**: FLIP 모델만 지원

- `DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL`
- `DXGI_SWAP_EFFECT_FLIP_DISCARD` (권장)

#### 각 SwapEffect 상세

##### `DXGI_SWAP_EFFECT_DISCARD` (Legacy, BitBlt 모델)

- **동작**: Present 시 백 버퍼 내용을 **프론트 버퍼로 복사(BitBlt)**, Present 후 백 버퍼 내용은 **폐기** (드라이버가 다음 프레임용으로 임의 재활용 가능)
- **주로 쓰던 시기**: DX9 ~ 초기 DX11. 가장 오래된 기본값
- **장점**:
  - 드라이버 자유도 ↑ (내용 폐기되니 버퍼 재배치 최적화 가능)
  - 개념적으로 단순 (백 버퍼는 항상 하나, 인덱스 신경 쓸 필요 없음)
- **단점**:
  - Present마다 **실제 메모리 복사 발생** → 느림
  - DWM(Desktop Window Manager)이 **추가 복사**를 한 번 더 해야 화면에 표시됨 (총 2~3번 복사)
  - VRR / 저지연 Present 지원 제한
- **DX12**: 지원 안 함

##### `DXGI_SWAP_EFFECT_SEQUENTIAL` (Legacy, BitBlt 모델)

- **동작**: Present 시 백 버퍼 → 프론트 복사, 백 버퍼 내용은 **유지**됨. 여러 백 버퍼가 있다면 순차 접근 가능
- **주로 쓰던 시기**: 이전 프레임 내용을 부분 업데이트해야 하는 경우 (일부 UI 렌더링, 창 기반 앱)
- **장점**:
  - 프레임 간 연속성 보장 (이전 내용이 남음)
  - 부분 업데이트 패턴에 적합
- **단점**:
  - 여전히 BitBlt 기반 → 복사 오버헤드
  - DWM 비효율
  - 드라이버 최적화 여지 적음 (내용 보존 보장)
- **DX12**: 지원 안 함

##### `DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL` (DX11.1+, DX12 지원)

- **동작**: 백/프론트 버퍼 **포인터만 교환(스왑)**. 복사 없음. 백 버퍼 내용은 **유지**됨
- **장점**:
  - 복사 없는 zero-copy Present
  - DWM과 **직접 합성**(direct composition) 가능 → 추가 복사 없음
  - 이전 프레임 내용 보존
- **단점**:
  - 내용 보존 보장 때문에 드라이버 최적화 여지 적음
  - 버퍼 수만큼 VRAM 소비
- **언제 쓰나**: 이전 프레임 내용을 재참조해야 하거나, 증분 업데이트 패턴이 필요할 때

##### `DXGI_SWAP_EFFECT_FLIP_DISCARD` (DX12 권장)

- **동작**: 포인터 스왑, Present 후 이전 백 버퍼 내용은 **폐기 가능**(드라이버가 자유롭게 재활용)
- **장점**:
  - 가장 빠름 (FLIP + 드라이버 자유도)
  - DWM 직접 합성
  - VRR, Tearing, 저지연 Present 등 현대 기능 모두 지원
- **단점**:
  - 이전 프레임 내용 사용 불가 (매 프레임 완전히 새로 그려야 함)
- **언제 쓰나**: **대부분의 게임 렌더링** (매 프레임 전체 씬을 새로 그리므로 내용 폐기 무방)

#### BitBlt 모델 vs FLIP 모델

| 축 | BitBlt (DISCARD / SEQUENTIAL) | FLIP (FLIP_DISCARD / FLIP_SEQUENTIAL) |
|----|------------------------------|--------------------------------------|
| Present 방식 | 메모리 **복사** | 포인터 **교환** |
| DWM 합성 | 추가 복사 필요 (비효율) | **직접 합성** (zero-copy) |
| 총 복사 횟수 | 2~3회 (앱→DWM→화면) | 0회 |
| VRR / Tearing 플래그 | 지원 제한 | **완전 지원** |
| 저지연 Present | 어려움 | `IDXGISwapChain2::SetFrameLatencyWaitableObject` 등 지원 |
| 버퍼 수 | 보통 1 (백 버퍼 1개) | 최소 2 이상 |
| VRAM 사용 | 적음 | 버퍼 수만큼 ↑ |
| 성능 | 낮음 | **높음** |

Microsoft의 FLIP 모델 설명:

> "The flip model is more efficient because it **allows the DWM to compose the back buffer directly from the application's swap chain**, without an intermediate copy."
>
> — [Microsoft: For best performance, use DXGI flip model](https://devblogs.microsoft.com/directx/dxgi-flip-model/)

#### 왜 DX12는 FLIP만 지원하나?

BitBlt 모델이 제거된 근본 이유:

1. **성능**: 매 Present마다 복사 + DWM 재복사 → 수 ms 오버헤드
2. **현대 디스플레이 기능**: VRR, Tearing, HDR 등이 FLIP 모델 전제로 설계됨
3. **메모리 대역폭**: BitBlt는 매 프레임 수 MB~수십 MB의 복사를 반복 → 대역폭 낭비
4. **DWM 통합**: Windows 10+ DWM이 FLIP을 우선 처리
5. **명시적 제어 철학**: 버퍼를 "하나"로 숨기는 BitBlt는 DX12의 "모든 리소스 명시" 철학과 상충

요약: BitBlt는 **DX9 시대의 1:1 CRT 디스플레이에 맞춰진 옛 모델**이고, FLIP은 **현대 컴포지팅 OS + 플랫 패널 + 고주사율 디스플레이**에 적합한 모델입니다.

Microsoft 공식 문서:

> "Direct3D 12 only supports the flip presentation model."
>
> — [Microsoft: DXGI_SWAP_EFFECT enumeration](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/ne-dxgi-dxgi_swap_effect)

#### FLIP_DISCARD vs FLIP_SEQUENTIAL 선택

실제 DX12 프로젝트에서는 둘 중 하나를 고릅니다.

| 상황 | 권장 |
|------|------|
| 일반 게임 (매 프레임 완전 재렌더) | **`FLIP_DISCARD`** |
| 3D 렌더링 전반 | **`FLIP_DISCARD`** |
| 이전 프레임 내용을 합성해야 함 (Temporal AA, TAA 등은 다른 방식으로 해결) | `FLIP_DISCARD` (TAA는 별도 history 텍스처로 관리) |
| 일부 UI/2D에서 증분 업데이트가 명확히 이득일 때 | `FLIP_SEQUENTIAL` |
| 모바일 앱 / 저전력 시나리오 | 경우에 따라 `FLIP_SEQUENTIAL` |

**대부분의 3D 게임 엔진은 `FLIP_DISCARD`가 기본값**입니다. 본 프로젝트도 `FLIP_DISCARD`를 사용합니다.

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

### 5. Tearing과 VRR 지원

#### Tearing이란?

**Tearing(화면 찢어짐)** 은 한 화면에 **서로 다른 두 프레임의 내용이 섞여 표시되는** 시각적 결함입니다.

```
모니터 한 프레임 (화면 전체):
┌─────────────────────────┐
│                         │
│   프레임 N의 상반부       │  ← 이전 프레임의 그림
│                         │
├─────────────────────────┤  ← 여기서 "찢어짐"
│                         │
│   프레임 N+1의 하반부     │  ← 새 프레임의 그림
│                         │
└─────────────────────────┘
```

화면을 좌우로 빠르게 회전시키는 장면에서 특히 눈에 띕니다. 마치 **종이를 중간에 찢어서 두 장을 겹쳐 놓은 것처럼** 수평선이 선명하게 생기고, 위아래의 물체 위치가 어긋나 보입니다.

#### 왜 Tearing이 발생하는가

모니터와 GPU의 타이밍이 독립적이기 때문입니다.

##### 모니터의 동작

- 모니터는 **고정된 주사율**로 화면을 갱신 (예: 60Hz = 초당 60회)
- 각 갱신 주기마다 **위에서 아래로 한 줄씩** 픽셀을 그려나감 (래스터 스캔)
- 한 프레임 갱신에 16.6ms (60Hz 기준) 소요

##### GPU의 동작

- GPU는 **자기 속도로** 프레임을 완성해 백 버퍼에 저장
- Present 호출 시 백/프론트 버퍼를 **즉시 교체**
- 프레임마다 걸리는 시간은 가변 (10ms, 20ms 등)

##### 충돌 시나리오

```
시간 →
모니터 읽기: [━━━━━ 프레임 N을 위에서 아래로 스캔 중 ━━━━━]
                        ↑
                     이 시점에 GPU가 Present!
                     백/프론트 버퍼 교체됨
             [■■ 프레임 N ■]
                        [▓▓ 프레임 N+1 ▓▓]
                        ↑ 모니터는 중간부터 새 버퍼 읽음
                        → 화면 위는 프레임 N, 아래는 프레임 N+1
```

모니터가 한 프레임을 그리는 도중에 GPU가 백 버퍼 내용을 바꿔버리면, **그 순간부터 모니터는 새 버퍼를 읽게 되어** 화면 중간에 경계선이 생깁니다.

#### VSync: 전통적 해결책과 한계

##### 동작 원리

**VSync(Vertical Synchronization)**: Present를 모니터의 **수직 블랭크**(VBlank - 한 프레임 스캔 완료 후 다음 프레임 시작 전 쉬는 구간)까지 **대기**시킵니다.

```
시간 →
모니터: [프레임 스캔 ────][VBlank][다음 프레임 스캔 ────]
                          ↑
GPU: Present 호출 → 여기서 대기 → VBlank 진입 시 버퍼 교체
                                   → 다음 스캔은 새 버퍼 읽음
                                   → Tearing 없음!
```

VSync ON 시 `Present(1, 0)`의 첫 인자가 SyncInterval = 1 (VBlank 1회 대기).

##### VSync의 장점

- **Tearing 완전 제거**
- 프레임이 모니터 주사율과 일치 (60Hz 모니터면 60fps 캡)

##### VSync의 치명적 단점

**1. Input Lag 증가**
- Present 호출 후 VBlank까지 최대 16.6ms 대기
- 입력 → 화면 반영 지연이 최대 1~2 프레임 증가
- 경쟁 FPS 게임에서 치명적

**2. 프레임 드랍 시 급격한 FPS 하락 (Stutter)**
- GPU가 16.6ms를 살짝 넘기면 (예: 17ms) → 다음 VBlank까지 또 대기
- 실효 FPS가 **60 → 30으로 반토막**
- 미세한 성능 변동에도 프레임 시간이 불규칙해짐

```
정상 VSync (16ms씩):
[16ms][16ms][16ms][16ms]  → 60fps

프레임 하나가 17ms로 지연:
[16ms][17ms → 32ms 대기][16ms]  → FPS 급변, 체감 stutter
```

**3. 모니터 주사율의 배수로만 동작**
- 60Hz 모니터: 60/30/20/15 fps만 가능
- 45fps 같은 중간값은 표현 불가

#### VRR: 현대적 해결책

**VRR(Variable Refresh Rate)**: 모니터의 주사율을 GPU의 프레임 생성 속도에 맞춰 **실시간으로 변동**시킵니다.

##### VSync와의 핵심 차이: "누가 타이밍을 결정하는가"

혼동하기 쉬운 부분: "VSync에서도 VBlank가 프레임마다 바뀌는 거 아닌가?"

**아닙니다.** 두 모드의 근본 차이:

| | 고정 주사율 (VSync) | VRR |
|---|-------------------|------|
| **모니터 동작** | **자체 클럭**으로 일정 간격 반복 | **GPU 신호 대기**, 신호 오면 스캔 시작 |
| **VBlank 타이밍** | 고정 (예: 60Hz → 정확히 16.6ms마다) | 가변 (GPU가 Present할 때까지 연장됨) |
| **GPU가 빠를 때** | 다음 VBlank까지 **CPU/GPU 대기** | 즉시 Present → 새 스캔 시작 |
| **GPU가 느릴 때** | 다음 VBlank 놓치면 또 16.6ms 대기 → Stutter | VBlank가 자연스럽게 연장 → 대기 없음 |
| **타이밍 결정자** | **모니터** (GPU가 맞춰야 함) | **GPU** (모니터가 맞춰줌) |

즉 고정 주사율 모니터는 GPU 상태와 **독립적으로** 16.6ms마다 VBlank를 만들고, GPU가 여기에 "맞춰야" 합니다. VRR 모니터는 **GPU의 Present를 기다리고**, 받을 때마다 스캔을 시작합니다.

시각적 비교:

```
고정 주사율 (모니터 주도):
모니터 클럭: │16.6ms│16.6ms│16.6ms│16.6ms│16.6ms│  ← 절대 안 변함
GPU 프레임:   [12ms][──17ms──][──18ms──][14ms]
Present:      ▼      ▼ (17ms에 준비됐지만)  ▼ (18ms)
대기 발생:       ✅                ❌ 놓침, 다음 VBlank까지 +13ms 대기
                                  → 이번 프레임은 32ms = 31fps

VRR (GPU 주도):
GPU 프레임:   [12ms][17ms][18ms][14ms]
Present:      ▼     ▼     ▼     ▼
모니터 스캔: │12ms│17ms│18ms│14ms│  ← GPU 타이밍에 맞춰 가변
             항상 즉시 Present → Tearing 없음 + 대기 없음
```

##### 동작 요약

```
고정 주사율 (기존):
모니터: [16.6ms][16.6ms][16.6ms][16.6ms]  ← 항상 60Hz 자체 클럭

VRR:
모니터: [12ms][14ms][20ms][11ms]  ← GPU 속도에 맞춰 변동
GPU:   [12ms][14ms][20ms][11ms]  ← 프레임 완성되는 대로 Present
       → 항상 동기화됨 → Tearing 없음 + Input Lag 없음 + Stutter 없음
```

GPU가 프레임을 완성하면 **즉시** Present하고, 모니터는 그 타이밍에 맞춰 새 스캔을 시작합니다. 고정 주사율의 모든 단점을 해결합니다.

> **추가 제약**: VRR은 모니터 지원 범위가 있음 (보통 40~144Hz 또는 48~240Hz). 프레임 타임이 이 범위를 벗어나면 VRR이 동작하지 않거나 LFC(Low Framerate Compensation)로 프레임 복제가 일어납니다.

##### VRR 표준들

| 표준 | 제조사 | 특징 |
|------|-------|------|
| **G-Sync** | NVIDIA | 전용 모듈 필요, 엄격한 품질 인증 |
| **G-Sync Compatible** | NVIDIA | 모듈 없이 Adaptive-Sync 기반 |
| **FreeSync** | AMD | VESA Adaptive-Sync 기반, 무료/개방 |
| **VESA Adaptive-Sync** | 표준 | DisplayPort 1.2a+ 표준 |
| **HDMI VRR** | HDMI Forum | HDMI 2.1+ 표준 (콘솔/TV) |

모두 원리는 비슷: DisplayPort/HDMI의 adaptive refresh 프로토콜을 사용.

#### DX12의 ALLOW_TEARING 플래그

여기서 핵심 질문: **"VRR은 Tearing을 없애는 건데, 왜 ALLOW_TEARING 플래그가 VRR을 위해 필요한가?"**

##### 이유: VRR의 기술적 요구

VRR이 동작하려면 **GPU가 모니터 VBlank를 기다리지 않고 즉시 Present**해야 합니다. 그런데 DXGI는 기본적으로 안전을 위해 Present를 VBlank에 맞춰 정렬시키거나, 어떤 식으로든 **타이밍을 조정**합니다. 이 조정이 VRR을 무력화합니다.

**ALLOW_TEARING 플래그의 진짜 의미**:
> "DXGI야, Present를 **VBlank와 맞추지 말고** 호출된 그 순간 그대로 전달해라."

이 플래그가 있어야 GPU → 모니터 Present가 **즉시** 일어나고, VRR 모니터가 그 타이밍에 맞춰 refresh 주기를 조정할 수 있습니다.

##### 플래그 이름이 혼란스러운 이유

- 플래그 이름은 "Tearing을 허용한다"지만
- 실제 의도는 "Present 타이밍을 건드리지 마라"
- **결과로**:
  - VRR 지원 모니터 + VRR 동작 조건 충족 → Tearing 없이 VRR 동작
  - VRR 미지원 / 동작 조건 불충족 → 진짜 Tearing 발생

즉 "Tearing이 날 수 있다"는 플래그이지 "Tearing을 강제하는" 플래그는 아닙니다. VRR 환경에서는 실제로 Tearing이 안 납니다.

##### 코드 사용

```cpp
// 1. Tearing 지원 여부 확인 (DXGI Factory 5+)
BOOL allowTearing = FALSE;
factory5->CheckFeatureSupport(
    DXGI_FEATURE_PRESENT_ALLOW_TEARING,
    &allowTearing,
    sizeof(allowTearing));

// 2. 스왑체인 생성 시 플래그 설정
swapChainDesc.Flags = allowTearing
    ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING
    : 0;

// 3. Present 시 플래그 사용 (VSync OFF일 때만)
UINT presentFlags = (!vsync && allowTearing)
    ? DXGI_PRESENT_ALLOW_TEARING
    : 0;
swapChain->Present(vsync ? 1 : 0, presentFlags);
```

**주의**: `DXGI_PRESENT_ALLOW_TEARING`은 반드시 `SyncInterval = 0`(VSync OFF)일 때만 사용. VSync ON(`SyncInterval >= 1`)과 함께 쓰면 Present가 실패합니다.

##### Windowed vs Fullscreen

DXGI Flip 모델은 **Borderless Windowed(창 없는 창모드)** 를 기본 권장합니다. 하지만 기본 Windowed에서는 DWM 합성 때문에 VRR이 제한됩니다. `ALLOW_TEARING` 플래그가 있으면 **Windowed에서도 VRR/Tearing이 가능** (Windows 10 1903+).

#### 실무 권장: 언제 어떤 모드를 쓰나

| 사용자 환경 | 권장 설정 | `Present` 호출 |
|------------|---------|---------------|
| VRR 모니터 + VRR 활성 | VSync OFF + `ALLOW_TEARING` | `Present(0, DXGI_PRESENT_ALLOW_TEARING)` |
| 일반 모니터 + 경쟁 FPS 게임 (Input Lag 민감) | VSync OFF + `ALLOW_TEARING` | `Present(0, DXGI_PRESENT_ALLOW_TEARING)` (Tearing 감수) |
| 일반 모니터 + 일반 게임 | VSync ON | `Present(1, 0)` |
| 싱글플레이 게임 + Stutter 기피 | VSync ON + Triple Buffering | `Present(1, 0)` |
| 모바일 / 저전력 | VSync ON + 낮은 fps 캡 | `Present(2, 0)` (30fps 캡) |

**엔진 구현 권장**:
- Tearing 지원 플래그는 **항상 확인**하고 스왑체인 생성 시 설정
- 런타임에 VSync ON/OFF 전환 가능하게 구성
- 사용자 설정에서 선택 가능하게 제공 ("VSync", "VRR", "Immediate" 등)

Microsoft 공식 문서:

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
