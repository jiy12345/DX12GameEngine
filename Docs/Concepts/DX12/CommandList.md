# Command List (커맨드 리스트)

## 개요

Command List는 GPU에서 실행할 명령들을 **기록하는 인터페이스**입니다. Draw, Dispatch, Copy 등의 명령을 기록한 후 Command Queue를 통해 GPU에 제출합니다.

## 왜 필요한가?

### DX12의 명시적 제어 모델

DX11에서는 `Draw()` 호출이 즉시(또는 드라이버 판단에 따라) GPU에 제출되었습니다. DX12에서는 명령을 먼저 **기록**하고, 나중에 **일괄 제출**합니다.

```cpp
// DX11: 즉시 실행 (드라이버가 내부적으로 버퍼링할 수 있음)
context->Draw(3, 0);  // → 언제 실행될지 불명확

// DX12: 명시적 기록 → 제출
commandList->DrawInstanced(3, 1, 0, 0);  // 기록만
commandList->Close();                      // 기록 완료
queue->ExecuteCommandLists(...);           // 이 시점에 GPU로 제출
```

**장점**:
- 멀티스레드에서 병렬 기록 가능
- 명령 재사용 가능 (Bundle)
- CPU 오버헤드 최소화

## Command List 생명주기

```
┌────────────────────────────────────────────────────────────────┐
│                    Command List 생명주기                        │
├────────────────────────────────────────────────────────────────┤
│                                                                │
│   ┌─────────┐    Reset()     ┌─────────┐    Close()           │
│   │ Initial │ ─────────────► │Recording│ ─────────────►       │
│   │ (Closed)│                │         │                       │
│   └─────────┘                └─────────┘                       │
│        ▲                                           │           │
│        │                                           ▼           │
│        │                                    ┌───────────┐      │
│        │         Reset()                    │ Executable│      │
│        └────────────────────────────────────│ (Closed)  │      │
│                                             └───────────┘      │
│                                                    │           │
│                                                    ▼           │
│                                          ExecuteCommandLists() │
│                                                    │           │
│                                                    ▼           │
│                                              GPU 실행          │
│                                                                │
└────────────────────────────────────────────────────────────────┘
```

### 상태 전이

1. **Initial (Closed)**: 생성 직후 또는 `Close()` 후 상태
2. **Recording**: `Reset()` 후 명령 기록 중
3. **Executable**: `Close()` 후 GPU 제출 가능
4. **실행 중**: `ExecuteCommandLists()` 후 GPU에서 실행

## Command List vs Allocator

| 항목 | Command List | Command Allocator |
|------|--------------|-------------------|
| **역할** | 기록 인터페이스 | 메모리 백킹 스토어 |
| **무게** | 가벼움 | 무거움 (메모리) |
| **Reset 후** | 다른 Allocator 연결 가능 | GPU 완료 대기 필요 |
| **재사용** | 제출 후 즉시 가능 | Fence 확인 후 |

**핵심 차이**: Command List는 제출 후 바로 `Reset()`으로 재사용 가능하지만, 반드시 **GPU가 완료된 Allocator**와 연결해야 합니다.

```cpp
// 프레임 0
commandList->Reset(allocatorA, pso);
// ... 기록 ...
commandList->Close();
queue->ExecuteCommandLists(...);
uint64_t fence0 = Signal();

// 프레임 1 - Command List는 바로 재사용 가능!
WaitForFence(fenceFromFrame_minus_2);  // 2프레임 전 Allocator 대기
allocatorB->Reset();  // 다른 Allocator Reset
commandList->Reset(allocatorB, pso);  // ✅ 같은 Command List 재사용
```

## 풀링이 필요한 이유

### 단일 Command List의 한계

```cpp
// ❌ 문제: 모든 렌더링을 하나의 Command List에
void RenderFrame() {
    commandList->Reset(allocator, nullptr);

    RenderShadows(commandList);   // 그림자
    RenderOpaque(commandList);    // 불투명 오브젝트
    RenderTransparent(commandList); // 투명 오브젝트
    RenderUI(commandList);        // UI

    commandList->Close();
    // 직렬 기록 - 멀티스레딩 활용 불가
}
```

### 멀티스레드 병렬 기록

```cpp
// ✅ 개선: 여러 Command List를 병렬로 기록
void RenderFrame() {
    // 각 스레드가 별도 Command List 사용
    std::thread t1([&]{ RenderShadows(cmdListPool.Get()); });
    std::thread t2([&]{ RenderOpaque(cmdListPool.Get()); });
    std::thread t3([&]{ RenderTransparent(cmdListPool.Get()); });

    t1.join(); t2.join(); t3.join();

    // 모든 Command List를 한 번에 제출
    ID3D12CommandList* lists[] = { shadowList, opaqueList, transparentList };
    queue->ExecuteCommandLists(3, lists);
}
```

## 풀링 전략

### 전략 A: 단순 풀 (권장)

Command List는 가볍기 때문에 단순한 풀로 충분합니다.

```cpp
class CommandListPool {
    ID3D12Device* m_device;
    D3D12_COMMAND_LIST_TYPE m_type;

    std::vector<ComPtr<ID3D12GraphicsCommandList>> m_pool;
    std::vector<bool> m_inUse;

public:
    void Initialize(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type,
                    uint32_t initialCount = 4) {
        m_device = device;
        m_type = type;

        for (uint32_t i = 0; i < initialCount; i++) {
            CreateNewCommandList();
        }
    }

    ID3D12GraphicsCommandList* Get() {
        // 사용 가능한 Command List 찾기
        for (size_t i = 0; i < m_pool.size(); i++) {
            if (!m_inUse[i]) {
                m_inUse[i] = true;
                return m_pool[i].Get();
            }
        }

        // 모두 사용 중이면 새로 생성
        size_t idx = CreateNewCommandList();
        m_inUse[idx] = true;
        return m_pool[idx].Get();
    }

    void Return(ID3D12GraphicsCommandList* cmdList) {
        for (size_t i = 0; i < m_pool.size(); i++) {
            if (m_pool[i].Get() == cmdList) {
                m_inUse[i] = false;
                return;
            }
        }
    }

private:
    size_t CreateNewCommandList() {
        ComPtr<ID3D12GraphicsCommandList> cmdList;

        // 임시 Allocator로 생성 (바로 Close)
        ComPtr<ID3D12CommandAllocator> tempAllocator;
        m_device->CreateCommandAllocator(m_type, IID_PPV_ARGS(&tempAllocator));
        m_device->CreateCommandList(0, m_type, tempAllocator.Get(), nullptr,
                                    IID_PPV_ARGS(&cmdList));
        cmdList->Close();  // 생성 직후 Close (Initial 상태)

        m_pool.push_back(cmdList);
        m_inUse.push_back(false);
        return m_pool.size() - 1;
    }
};
```

### 전략 B: Allocator와 페어링

Command List는 항상 Allocator와 함께 사용되므로, 페어로 관리하면 편리합니다.

```cpp
struct CommandListPair {
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> commandList;
    uint64_t fenceValue = 0;
    bool inUse = false;
};

class CommandListManager {
    static constexpr uint32_t kFrameCount = 3;
    static constexpr uint32_t kListsPerFrame = 4;  // 스레드 수 등

    std::array<std::array<CommandListPair, kListsPerFrame>, kFrameCount> m_pairs;
    uint32_t m_frameIndex = 0;

public:
    CommandListPair* GetPair() {
        auto& framePairs = m_pairs[m_frameIndex];

        for (auto& pair : framePairs) {
            if (!pair.inUse) {
                pair.inUse = true;
                pair.allocator->Reset();
                return &pair;
            }
        }

        return nullptr;  // 또는 동적으로 생성
    }

    void ReturnPair(CommandListPair* pair, uint64_t fenceValue) {
        pair->fenceValue = fenceValue;
        pair->inUse = false;
    }

    void BeginFrame(uint64_t completedFence) {
        m_frameIndex = (m_frameIndex + 1) % kFrameCount;

        // 현재 프레임의 모든 Pair가 GPU 완료되었는지 확인
        for (auto& pair : m_pairs[m_frameIndex]) {
            if (pair.fenceValue > completedFence) {
                WaitForFence(pair.fenceValue);
            }
        }
    }
};
```

## 사용 패턴

### 기본 사용

```cpp
// 1. Allocator Reset (GPU 완료 확인 후)
allocator->Reset();

// 2. Command List Reset (Allocator 연결)
commandList->Reset(allocator, pipelineState);

// 3. 명령 기록
commandList->SetGraphicsRootSignature(rootSig);
commandList->RSSetViewports(1, &viewport);
commandList->RSSetScissorRects(1, &scissor);
commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
commandList->IASetVertexBuffers(0, 1, &vbView);
commandList->DrawInstanced(3, 1, 0, 0);

// 4. 기록 완료
HRESULT hr = commandList->Close();
if (FAILED(hr)) {
    // 기록 중 오류 발생 - Debug Layer 메시지 확인
}

// 5. 제출
ID3D12CommandList* lists[] = { commandList };
queue->ExecuteCommandLists(1, lists);
```

### 여러 Command List 제출

```cpp
// 여러 Command List를 한 번에 제출
ID3D12CommandList* lists[] = {
    shadowCommandList,
    opaqueCommandList,
    transparentCommandList,
    uiCommandList
};
queue->ExecuteCommandLists(_countof(lists), lists);

// 제출된 순서대로 GPU에서 실행됨
```

## Bundle (재사용 가능한 Command List)

자주 반복되는 명령 시퀀스를 Bundle로 만들어 재사용할 수 있습니다.

```cpp
// Bundle 생성 (초기화 시 한 번)
ComPtr<ID3D12CommandAllocator> bundleAllocator;
device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE,
                               IID_PPV_ARGS(&bundleAllocator));

ComPtr<ID3D12GraphicsCommandList> bundle;
device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE,
                          bundleAllocator.Get(), pso, IID_PPV_ARGS(&bundle));

bundle->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
bundle->IASetVertexBuffers(0, 1, &vbView);
bundle->DrawInstanced(36, 1, 0, 0);
bundle->Close();

// 렌더링 시 Bundle 실행
commandList->ExecuteBundle(bundle.Get());
```

**Bundle 제약사항**:
- `D3D12_COMMAND_LIST_TYPE_BUNDLE` 타입 전용
- Direct Command List에서만 실행 가능
- 일부 명령 사용 불가 (SetRenderTargets, Clear, ResourceBarrier 등)

## 관련 API

### Command List 생성

```cpp
HRESULT ID3D12Device::CreateCommandList(
    UINT nodeMask,                           // 0 (단일 GPU)
    D3D12_COMMAND_LIST_TYPE type,           // DIRECT, COMPUTE, COPY, BUNDLE
    ID3D12CommandAllocator* pAllocator,     // 연결할 Allocator
    ID3D12PipelineState* pInitialState,     // 초기 PSO (선택적)
    REFIID riid,
    void** ppCommandList
);
```

### Reset

```cpp
HRESULT ID3D12GraphicsCommandList::Reset(
    ID3D12CommandAllocator* pAllocator,     // 새 Allocator 연결
    ID3D12PipelineState* pInitialState      // 초기 PSO (선택적)
);
```

### Close

```cpp
HRESULT ID3D12GraphicsCommandList::Close();
```

## 주의사항

- **Close 전 제출 불가**: `Close()` 호출 전에는 `ExecuteCommandLists()` 불가.
- **Reset 전 Close 필수**: Recording 상태에서 `Reset()` 호출 불가.
- **Allocator 타입 일치**: Command List와 Allocator의 타입이 일치해야 함.
- **스레드 안전성**: 하나의 Command List는 한 스레드에서만 사용.
- **오류 확인**: `Close()`의 반환값으로 기록 중 오류 확인 (Debug Layer 활성화 권장).

## 관련 개념

### 선행 개념
- [Device](./Device.md) - Command List를 생성하는 팩토리
- [CommandAllocator](./CommandAllocator.md) - 명령이 저장되는 메모리

### 연관 개념
- [CommandQueue](./CommandQueue.md) - Command List를 GPU에 제출
- [PipelineStateObject](./PipelineStateObject.md) - 렌더링 파이프라인 상태
- [RootSignature](./RootSignature.md) - 셰이더 리소스 바인딩 정의

## 참고 자료

- [Microsoft: ID3D12GraphicsCommandList interface](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nn-d3d12-id3d12graphicscommandlist)
- [Microsoft: Recording Command Lists and Bundles](https://learn.microsoft.com/en-us/windows/win32/direct3d12/recording-command-lists-and-bundles)
- [Microsoft: Executing and Synchronizing Command Lists](https://learn.microsoft.com/en-us/windows/win32/direct3d12/executing-and-synchronizing-command-lists)
