# Command Queue (커맨드 큐)

## 개요

CPU가 GPU에게 작업을 제출하는 명시적인 제출 창구입니다. DX12에서 GPU 작업은 Command List에 기록된 후, Command Queue를 통해 GPU에 제출됩니다.

## 왜 필요한가?

### 문제: DX11의 암시적 제어와 드라이버 오버헤드

DX11에서는 드라이버가 많은 것을 자동으로 처리했습니다:
- 상태 검증
- 리소스 상태 추적
- 셰이더 조합 및 컴파일
- 동기화

이로 인해 **드라이버 오버헤드**가 발생했고, 멀티스레딩 활용이 제한되었습니다.

### 해결책: DX12의 명시적 제어

Microsoft 공식 문서에서 밝힌 DX12의 세 가지 핵심 설계 변경:

> 1. **Removal of the Immediate Context** - Enables multi-threading
> 2. **Apps now control GPU work grouping** - Enables reuse of rendering work
> 3. **Apps now explicitly submit work** - Gives apps control over when work goes to the GPU
>
> — [Microsoft: Design philosophy of command queues and command lists](https://learn.microsoft.com/en-us/windows/win32/direct3d12/design-philosophy-of-command-queues-and-command-lists)

## DX11은 왜 비효율적이었나?

### DX11도 커맨드 기록을 지원했다

DX11에도 **Deferred Context**가 있어서 커맨드를 기록하고 나중에 실행할 수 있었습니다:

```cpp
// DX11: Deferred Context 생성
ID3D11DeviceContext* deferredContext;
device->CreateDeferredContext(0, &deferredContext);

// 별도 스레드에서 커맨드 기록
deferredContext->Draw(3, 0);
deferredContext->FinishCommandList(FALSE, &commandList);

// 메인 스레드에서 실행
immediateContext->ExecuteCommandList(commandList, TRUE);
```

### DX11 Deferred Context의 한계

Microsoft 공식 문서에서 명시한 제약사항들:

> - **Cannot playback multiple command lists simultaneously** on the immediate context
> - **Map()** is only supported for dynamic resources
> - Cannot call **GetData()** to retrieve query results
>
> — [Microsoft: Deferred Rendering](https://learn.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-render-multi-thread-render)

**핵심 문제**: 커맨드 리스트를 실행할 때 **Immediate Context를 통해서만** 가능했고, 이 과정에서 드라이버 검증이 다시 발생했습니다.

## 드라이버 검증(Validation)이란?

DX11에서 Draw 호출 시 드라이버가 수행하는 작업들입니다.

### 1. 상태 유효성 검사

매 Draw 호출마다 드라이버가 확인하는 것들:

```cpp
// DX11: Draw 호출
context->Draw(3, 0);

// 드라이버가 내부적으로 체크:
// - 셰이더가 바인딩되어 있는가?
// - Vertex Buffer가 설정되어 있는가?
// - Render Target이 설정되어 있는가?
// - 셰이더가 요구하는 리소스가 모두 바인딩되어 있는가?
// - 리소스 포맷이 호환되는가?
```

### 2. 리소스 상태 추적

DX11에서는 드라이버가 리소스 상태를 자동으로 추적하고 전환했습니다:

```cpp
// DX11: 같은 텍스처를 렌더 타겟으로 사용 후 셰이더에서 읽기
context->OMSetRenderTargets(1, &textureRTV, nullptr);
context->Draw(...);  // 텍스처에 쓰기

context->PSSetShaderResources(0, 1, &textureSRV);
context->Draw(...);  // 같은 텍스처 읽기

// 드라이버가 암시적으로:
// "이 텍스처 상태를 RENDER_TARGET에서 SHADER_RESOURCE로 전환해야겠다"
// → 내부적으로 배리어 삽입 (개발자는 모름)
```

**DX12에서는 개발자가 직접 명시합니다:**

```cpp
// DX12: 명시적 리소스 배리어
D3D12_RESOURCE_BARRIER barrier = {};
barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
barrier.Transition.pResource = texture;
barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

commandList->ResourceBarrier(1, &barrier);
```

### 3. 셰이더 조합 및 컴파일 (가장 큰 오버헤드)

DX11에서는 파이프라인 상태를 개별적으로 설정했습니다:

```cpp
// DX11: 개별 상태 설정
context->VSSetShader(vertexShader, nullptr, 0);
context->PSSetShader(pixelShader, nullptr, 0);
context->IASetInputLayout(inputLayout);
context->OMSetBlendState(blendState, blendFactor, 0xFFFFFFFF);
context->RSSetState(rasterizerState);
context->OMSetDepthStencilState(depthStencilState, 0);

context->Draw(3, 0);
// ↑ 이 시점에 드라이버가:
// "이 조합의 실제 GPU 명령을 만들어야 하는데..."
// → 런타임에 상태 조합 검증 및 최적화 수행
```

**DX12의 PSO(Pipeline State Object)는 이를 사전에 처리합니다:**

Microsoft 공식 문서:

> "PSOs in Direct3D 12 were designed to allow the GPU to **pre-process all of the dependent settings in each pipeline state, typically during initialization**, to make switching between states at render time as efficient as possible."
>
> "With today's graphics hardware, there are dependencies between the different hardware units. For example, the hardware blend state might have dependencies on the raster state as well as the blend state."
>
> — [Microsoft: Managing Graphics Pipeline State](https://learn.microsoft.com/en-us/windows/win32/direct3d12/managing-graphics-pipeline-state-in-direct3d-12)

```cpp
// DX12: Pipeline State Object - 초기화 시 모든 상태를 한 번에 정의
D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
psoDesc.pRootSignature = rootSignature;
psoDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
psoDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
// ... 기타 상태들

// 초기화 시 한 번 생성 (여기서 검증 및 컴파일 완료)
device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));

// 렌더링 시에는 단순히 전환만
commandList->SetPipelineState(pipelineState);
commandList->DrawInstanced(3, 1, 0, 0);
// ↑ 드라이버가 할 일이 거의 없음
```

### 검증 오버헤드 비교 요약

| 검증 항목 | DX11 | DX12 |
|----------|------|------|
| 상태 유효성 | 매 Draw마다 검사 | PSO 생성 시 한 번 |
| 리소스 상태 | 드라이버가 암시적 추적 | 개발자가 명시적 배리어 |
| 셰이더 조합 | 런타임에 조합/최적화 | PSO로 사전 컴파일 |
| 결과 | CPU 오버헤드 높음 | CPU 오버헤드 최소화 |

## 커맨드 큐의 종류

DX12는 세 가지 타입의 커맨드 큐를 제공합니다:

| 타입 | `D3D12_COMMAND_LIST_TYPE` | 지원 작업 | 용도 |
|------|---------------------------|----------|------|
| **Direct** | `D3D12_COMMAND_LIST_TYPE_DIRECT` | 모든 GPU 작업 | 일반 렌더링 |
| **Compute** | `D3D12_COMMAND_LIST_TYPE_COMPUTE` | Compute, Copy | 컴퓨트 셰이더 |
| **Copy** | `D3D12_COMMAND_LIST_TYPE_COPY` | Copy만 | 리소스 전송 |

### 병렬 실행의 이점

여러 큐를 사용하면 GPU에서 병렬 실행이 가능합니다:

```
Direct Queue:  [렌더링 작업 ██████████████████]
Compute Queue: [물리 계산 ████████]              ← 동시에 실행 가능
Copy Queue:    [텍스처 업로드 ████]              ← 동시에 실행 가능
```

## 커맨드 실행 흐름

```
┌─────────────────────────────────────────────────────────┐
│  Command Allocator (메모리 할당기)                       │
│  - 커맨드 리스트가 기록될 메모리 공간                     │
│  - GPU가 사용 중일 때 재사용 불가 (Fence로 확인 필요)     │
└─────────────────────────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────┐
│  Command List (명령 목록)                                │
│  - Draw, Dispatch, Copy 등의 명령을 기록                 │
│  - 기록 완료 후 Close() 호출 필수                        │
│  - Reset() 후 재사용 가능                                │
└─────────────────────────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────┐
│  Command Queue (제출 큐)                                 │
│  - ExecuteCommandLists()로 GPU에 제출                   │
│  - FIFO 순서로 실행                                     │
│  - 여러 스레드에서 동시에 제출 가능                       │
└─────────────────────────────────────────────────────────┘
                         ↓
                   GPU에서 실행
```

## 관련 API

### 커맨드 큐 생성

```cpp
D3D12_COMMAND_QUEUE_DESC queueDesc = {};
queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;  // 큐 타입
queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
queueDesc.NodeMask = 0;  // 단일 GPU인 경우 0

ID3D12CommandQueue* commandQueue;
HRESULT hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue));
```

### 주요 메서드

```cpp
// 커맨드 리스트 실행
void ExecuteCommandLists(
    UINT NumCommandLists,
    ID3D12CommandList* const* ppCommandLists
);

// GPU에 시그널 전송 (동기화용)
HRESULT Signal(
    ID3D12Fence* pFence,
    UINT64 Value
);

// GPU 대기 (다른 큐의 작업 완료 대기)
HRESULT Wait(
    ID3D12Fence* pFence,
    UINT64 Value
);

// 타임스탬프 주파수 조회 (프로파일링용)
HRESULT GetTimestampFrequency(
    UINT64* pFrequency
);
```

## 코드 예제: 프레임당 실행 흐름

```cpp
// 1. Allocator 리셋 (GPU가 이전 프레임 완료 후에만 가능)
//    - Fence로 GPU 완료 확인 필수
commandAllocator->Reset();

// 2. 커맨드 리스트 리셋 및 기록 시작
commandList->Reset(commandAllocator, pipelineState);

// 3. 명령 기록
commandList->SetGraphicsRootSignature(rootSignature);
commandList->RSSetViewports(1, &viewport);
commandList->RSSetScissorRects(1, &scissorRect);

// 리소스 배리어: Present → Render Target
CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
    renderTarget,
    D3D12_RESOURCE_STATE_PRESENT,
    D3D12_RESOURCE_STATE_RENDER_TARGET
);
commandList->ResourceBarrier(1, &barrier);

commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
commandList->DrawInstanced(3, 1, 0, 0);

// 리소스 배리어: Render Target → Present
barrier = CD3DX12_RESOURCE_BARRIER::Transition(
    renderTarget,
    D3D12_RESOURCE_STATE_RENDER_TARGET,
    D3D12_RESOURCE_STATE_PRESENT
);
commandList->ResourceBarrier(1, &barrier);

// 4. 기록 완료
commandList->Close();

// 5. GPU에 제출
ID3D12CommandList* commandLists[] = { commandList };
commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

// 6. 동기화 - 다음 프레임에서 Allocator 재사용 전 확인용
commandQueue->Signal(fence, ++fenceValue);
```

## 주의사항

- **Command Allocator 재사용**: GPU가 해당 Allocator의 명령을 모두 실행 완료한 후에만 Reset() 가능. Fence로 확인 필수.
- **Command List 스레드 안전성**: 개별 Command List는 스레드 안전하지 않음. 하지만 **여러 Command List를 여러 스레드에서 동시에 기록**하는 것은 가능.
- **제출 순서**: 같은 큐에 제출된 커맨드 리스트는 제출 순서대로 실행됨.
- **큐 간 동기화**: 서로 다른 큐 간에는 Fence를 사용해 명시적으로 동기화해야 함.

## 관련 개념

### 선행 개념 (먼저 이해해야 할 것)
- [Device](./Device.md) - 커맨드 큐를 생성하는 팩토리

### 연관 개념 (함께 사용되는 것)
- [CommandAllocator](./CommandAllocator.md) - 커맨드 메모리 할당
- [CommandList](./CommandList.md) - 실제 명령을 기록하는 객체
- [Synchronization](./Synchronization.md) - Fence를 사용한 동기화
- [PipelineStateObject](./PipelineStateObject.md) - 사전 컴파일된 파이프라인 상태

### 후속 개념 (이후 학습할 것)
- [ResourceBarriers](./ResourceBarriers.md) - 리소스 상태 전환

## 참고 자료

- [Microsoft: Design philosophy of command queues and command lists](https://learn.microsoft.com/en-us/windows/win32/direct3d12/design-philosophy-of-command-queues-and-command-lists)
- [Microsoft: Managing Graphics Pipeline State in Direct3D 12](https://learn.microsoft.com/en-us/windows/win32/direct3d12/managing-graphics-pipeline-state-in-direct3d-12)
- [Microsoft: Direct3D 11 Deferred Context](https://learn.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-render-multi-thread-render)
- [Microsoft: ID3D12CommandQueue interface](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nn-d3d12-id3d12commandqueue)
