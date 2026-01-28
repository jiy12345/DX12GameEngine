# Command Allocator (커맨드 할당기)

## 개요

Command Allocator는 Command List가 기록한 GPU 명령들이 **실제로 저장되는 메모리 백킹 스토어**입니다. Command List는 기록 인터페이스일 뿐, 실제 명령 데이터는 Allocator에 저장됩니다.

## 왜 필요한가?

### Command List와 Allocator의 관계

```
┌─────────────────────────────────────────────────────────────────┐
│                                                                 │
│   Command Allocator              Command List                   │
│   (메모리 백킹 스토어)            (기록 인터페이스)               │
│                                                                 │
│   ┌──────────────────┐          ┌──────────────────┐           │
│   │                  │          │                  │           │
│   │   ┌──────────┐   │  기록    │  Draw(...)       │           │
│   │   │ Draw A   │◄──┼──────────┤  SetPSO(...)     │           │
│   │   │ Draw B   │   │          │  Close()         │           │
│   │   │ SetPSO   │   │          │                  │           │
│   │   │ Barrier  │   │          │                  │           │
│   │   └──────────┘   │          └──────────────────┘           │
│   │                  │                                          │
│   │   [실제 메모리]   │          [인터페이스만 제공]              │
│   └──────────────────┘                                          │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

Command List의 `DrawInstanced()`, `SetPipelineState()` 등의 호출은 연결된 Allocator의 메모리에 GPU 명령으로 변환되어 저장됩니다.

### 핵심 제약사항: GPU 실행 중 Reset 불가

**가장 중요한 제약**: GPU가 Allocator의 명령을 실행 중일 때 `Reset()`을 호출하면 **정의되지 않은 동작**이 발생합니다.

Microsoft 공식 문서:

> "The application may only call Reset() on a command allocator after it has been sure that the associated command lists have completed execution on the GPU."
>
> — [Microsoft: ID3D12CommandAllocator::Reset](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12commandallocator-reset)

```cpp
// 프레임 0
commandList->Reset(allocatorA, pso);
commandList->DrawInstanced(...);
commandList->Close();
queue->ExecuteCommandLists(...);  // GPU 실행 시작

// 프레임 1 - 위험!
allocatorA->Reset();  // ❌ GPU가 아직 실행 중일 수 있음!
```

이 제약 때문에 **프레임별로 별도의 Allocator가 필요**합니다.

## Allocator vs Command List 비교

| 항목 | Command Allocator | Command List |
|------|------------------|--------------|
| **역할** | 명령 저장 메모리 | 기록 인터페이스 |
| **무게** | 무거움 (메모리 할당) | 가벼움 |
| **Reset 조건** | GPU 실행 완료 후 | 언제든 가능 (다른 Allocator로) |
| **재사용 대기** | Fence 확인 필수 | 즉시 가능 |
| **스레드 안전성** | 동시 사용 불가 | 동시 사용 불가 |

## 풀링이 필요한 이유

### 문제: 매 프레임 생성/파괴

```cpp
// ❌ 나쁜 예: 매 프레임 생성
void RenderFrame() {
    ComPtr<ID3D12CommandAllocator> allocator;
    device->CreateCommandAllocator(type, IID_PPV_ARGS(&allocator));  // 비용 높음

    commandList->Reset(allocator.Get(), pso);
    // ... 렌더링 ...

    // allocator 파괴 - 메모리 단편화, 할당/해제 오버헤드
}
```

**문제점**:
- 매 프레임 메모리 할당/해제 오버헤드
- 메모리 단편화
- GPU가 사용 중인 Allocator 파괴 시 크래시 위험

### 해결책: Allocator 풀링

```cpp
// ✅ 좋은 예: 풀에서 가져와 재사용
void RenderFrame() {
    auto* allocator = m_allocatorPool.GetAvailable(completedFenceValue);

    commandList->Reset(allocator, pso);
    // ... 렌더링 ...

    queue->ExecuteCommandLists(...);
    uint64_t fence = queue->Signal();

    m_allocatorPool.Return(allocator, fence);  // 나중에 재사용
}
```

## 풀링 전략

### 전략 A: 프레임 인덱스 기반 (단순, 권장)

가장 간단하고 효율적인 방식입니다. Triple Buffering(3프레임)을 기준으로 설명합니다.

```cpp
static constexpr uint32_t kMaxFramesInFlight = 3;

class FrameBasedAllocatorPool {
    ComPtr<ID3D12CommandAllocator> m_allocators[kMaxFramesInFlight];
    uint64_t m_fenceValues[kMaxFramesInFlight] = {};
    uint32_t m_currentFrame = 0;

public:
    void Initialize(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type) {
        for (uint32_t i = 0; i < kMaxFramesInFlight; i++) {
            device->CreateCommandAllocator(type, IID_PPV_ARGS(&m_allocators[i]));
        }
    }

    ID3D12CommandAllocator* GetCurrentAllocator() {
        return m_allocators[m_currentFrame].Get();
    }

    void BeginFrame(ID3D12Fence* fence, HANDLE fenceEvent) {
        // 현재 프레임의 Allocator가 GPU에서 완료되었는지 확인
        if (fence->GetCompletedValue() < m_fenceValues[m_currentFrame]) {
            fence->SetEventOnCompletion(m_fenceValues[m_currentFrame], fenceEvent);
            WaitForSingleObject(fenceEvent, INFINITE);
        }

        // 안전하게 Reset
        m_allocators[m_currentFrame]->Reset();
    }

    void EndFrame(uint64_t fenceValue) {
        m_fenceValues[m_currentFrame] = fenceValue;
        m_currentFrame = (m_currentFrame + 1) % kMaxFramesInFlight;
    }
};
```

**실행 흐름:**
```
시간 →
프레임:     0        1        2        3        4        5
Allocator:  A        B        C        A        B        C
            │        │        │        │
            └─GPU 0──┘        │        └─재사용 (GPU 0 완료)
                     └─GPU 1──┘               └─재사용 (GPU 1 완료)
```

**장점**:
- 구현이 단순함
- 메모리 사용량 예측 가능 (고정)
- 대부분의 게임에서 충분

**단점**:
- 멀티스레딩 시 추가 Allocator 필요
- 동적 워크로드에 유연하지 않음

### 전략 B: Fence 기반 동적 풀 (유연)

워크로드가 가변적이거나 멀티스레딩이 필요할 때 사용합니다.

```cpp
class DynamicAllocatorPool {
    struct AllocatorEntry {
        ComPtr<ID3D12CommandAllocator> allocator;
        uint64_t fenceValue;  // 이 값 이후에 재사용 가능
    };

    ID3D12Device* m_device;
    D3D12_COMMAND_LIST_TYPE m_type;
    std::queue<AllocatorEntry> m_available;  // 사용 가능한 Allocator
    std::queue<AllocatorEntry> m_inFlight;   // GPU 실행 중인 Allocator

public:
    ID3D12CommandAllocator* GetAllocator(uint64_t completedFenceValue) {
        // 1. GPU가 완료한 Allocator를 available로 이동
        while (!m_inFlight.empty()) {
            auto& front = m_inFlight.front();
            if (front.fenceValue > completedFenceValue) {
                break;  // 아직 GPU가 사용 중
            }

            // GPU 완료 - Reset 후 available로
            front.allocator->Reset();
            m_available.push(std::move(front));
            m_inFlight.pop();
        }

        // 2. available에서 가져오기
        if (!m_available.empty()) {
            auto entry = std::move(m_available.front());
            m_available.pop();
            return entry.allocator.Get();
        }

        // 3. 풀이 비었으면 새로 생성
        ComPtr<ID3D12CommandAllocator> newAllocator;
        m_device->CreateCommandAllocator(m_type, IID_PPV_ARGS(&newAllocator));
        return newAllocator.Get();
    }

    void ReturnAllocator(ID3D12CommandAllocator* allocator, uint64_t fenceValue) {
        // inFlight 큐에 추가 (나중에 재사용)
        m_inFlight.push({allocator, fenceValue});
    }
};
```

**장점**:
- 워크로드에 따라 자동 확장/축소
- 멀티스레딩 지원 용이 (각 스레드가 독립적으로 요청)

**단점**:
- 구현 복잡도 증가
- 메모리 사용량 예측 어려움
- 큐 관리 오버헤드

## 멀티스레딩 고려사항

각 스레드는 **자신만의 Command Allocator**가 필요합니다.

```
┌─────────────────────────────────────────────────────────────────┐
│                     멀티스레드 렌더링                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Thread 0                Thread 1                Thread 2       │
│  (Main Rendering)        (Shadow)                (UI)           │
│                                                                 │
│  ┌─────────────┐        ┌─────────────┐        ┌─────────────┐ │
│  │ Allocator A │        │ Allocator B │        │ Allocator C │ │
│  │ CmdList 0   │        │ CmdList 1   │        │ CmdList 2   │ │
│  └──────┬──────┘        └──────┬──────┘        └──────┬──────┘ │
│         │                      │                      │         │
│         └──────────────────────┼──────────────────────┘         │
│                                ▼                                │
│                    ┌───────────────────┐                       │
│                    │   Command Queue   │                       │
│                    │ (동시 제출 가능)   │                       │
│                    └───────────────────┘                       │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

Microsoft 공식 문서:

> "A given allocator can be associated with only one currently recording command list at a time."
>
> — [Microsoft: Recording Command Lists and Bundles](https://learn.microsoft.com/en-us/windows/win32/direct3d12/recording-command-lists-and-bundles)

## 관련 API

### Allocator 생성

```cpp
HRESULT ID3D12Device::CreateCommandAllocator(
    D3D12_COMMAND_LIST_TYPE type,  // DIRECT, COMPUTE, COPY, BUNDLE
    REFIID riid,
    void** ppCommandAllocator
);
```

### Allocator Reset

```cpp
HRESULT ID3D12CommandAllocator::Reset();
```

- GPU 실행 완료 후에만 호출 가능
- 내부 메모리를 초기 상태로 되돌림
- 메모리는 해제되지 않음 (재사용)

## 코드 예제: 프레임 기반 풀링

```cpp
class Renderer {
    static constexpr uint32_t kFrameCount = 3;

    ComPtr<ID3D12CommandAllocator> m_allocators[kFrameCount];
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    uint64_t m_fenceValues[kFrameCount] = {};
    uint32_t m_frameIndex = 0;

public:
    void RenderFrame() {
        // 1. 현재 프레임의 Allocator 대기 및 Reset
        WaitForFence(m_fenceValues[m_frameIndex]);
        m_allocators[m_frameIndex]->Reset();

        // 2. Command List 기록
        m_commandList->Reset(m_allocators[m_frameIndex].Get(), nullptr);
        // ... 렌더링 명령 기록 ...
        m_commandList->Close();

        // 3. 실행 및 Signal
        ID3D12CommandList* lists[] = { m_commandList.Get() };
        m_commandQueue->ExecuteCommandLists(1, lists);
        m_fenceValues[m_frameIndex] = Signal();

        // 4. 다음 프레임으로
        m_frameIndex = (m_frameIndex + 1) % kFrameCount;
    }
};
```

## 주의사항

- **Reset 타이밍**: 반드시 GPU 실행 완료 후에 Reset. Fence로 확인 필수.
- **타입 일치**: Allocator와 Command List의 타입이 일치해야 함 (Direct, Compute, Copy).
- **1:1 관계**: 하나의 Allocator에는 한 번에 하나의 Command List만 기록 가능.
- **Bundle**: Bundle 타입의 Command List는 별도의 Bundle 타입 Allocator 필요.

## 관련 개념

### 선행 개념
- [Device](./Device.md) - Allocator를 생성하는 팩토리
- [CommandQueue](./CommandQueue.md) - GPU 작업 제출

### 연관 개념
- [CommandList](./CommandList.md) - Allocator와 함께 사용되는 기록 인터페이스
- [Synchronization](./Synchronization.md) - Fence를 사용한 GPU 완료 확인

## 참고 자료

- [Microsoft: ID3D12CommandAllocator interface](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nn-d3d12-id3d12commandallocator)
- [Microsoft: ID3D12CommandAllocator::Reset](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12commandallocator-reset)
- [Microsoft: Recording Command Lists and Bundles](https://learn.microsoft.com/en-us/windows/win32/direct3d12/recording-command-lists-and-bundles)
