# DirectX 12 핵심 개념

DirectX 11에서 DirectX 12로 전환하면서 알아야 할 핵심 개념들을 정리합니다.

## 🔄 DX11과 DX12의 주요 차이점

### 1. Explicit Control (명시적 제어)
- **DX11**: 드라이버가 대부분의 리소스 관리와 동기화를 자동으로 처리
- **DX12**: 개발자가 모든 것을 명시적으로 관리해야 함
  - 장점: 더 많은 최적화 기회, 멀티스레딩 지원 향상
  - 단점: 복잡도 증가, 실수 시 디버깅 어려움

### 2. Low-Level API
- GPU 리소스에 대한 직접적인 접근
- CPU 오버헤드 감소
- 메모리 관리의 세밀한 제어

## 🏗️ DX12 핵심 개념

### 1. Command Queue & Command List
```
[CPU]
  ↓ Record Commands
Command Allocator → Command List
  ↓ Execute
Command Queue
  ↓
[GPU]
```

- **Command Allocator**: 커맨드 메모리 할당
- **Command List**: GPU 명령어 기록
- **Command Queue**: GPU에 작업 제출

**DX11과의 차이**:
- DX11: Immediate Context에서 즉시 실행
- DX12: 커맨드 리스트에 기록 후 큐에 제출

### 2. Pipeline State Object (PSO)
모든 파이프라인 상태를 하나의 객체로 묶음:
- Shaders (VS, PS, GS, HS, DS)
- Blend State
- Rasterizer State
- Depth Stencil State
- Input Layout
- Primitive Topology Type

**장점**: 런타임 검증 최소화, 상태 전환 최적화

### 3. Root Signature
셰이더가 접근할 리소스의 레이아웃 정의:
- Root Constants
- Root Descriptors
- Descriptor Tables

**개념**: 함수 시그니처와 유사 (셰이더의 "인자 목록")

### 4. Descriptor Heaps
리소스 디스크립터(뷰)를 저장하는 GPU 메모리:
- **CBV**: Constant Buffer View
- **SRV**: Shader Resource View
- **UAV**: Unordered Access View
- **Sampler**: 샘플러
- **RTV**: Render Target View
- **DSV**: Depth Stencil View

**타입**:
- `D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV`: 셰이더 가시적
- `D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER`: 샘플러
- `D3D12_DESCRIPTOR_HEAP_TYPE_RTV`: 렌더 타겟
- `D3D12_DESCRIPTOR_HEAP_TYPE_DSV`: 깊이 스텐실

### 5. Resource Barriers
리소스 상태 전환을 명시적으로 관리:
```cpp
D3D12_RESOURCE_BARRIER barrier = {};
barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
barrier.Transition.pResource = resource;
barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
```

**주요 상태**:
- `COMMON`: 일반적인 읽기
- `RENDER_TARGET`: 렌더 타겟으로 쓰기
- `PRESENT`: 스왑체인 표시
- `COPY_SOURCE` / `COPY_DEST`: 복사 작업
- `UNORDERED_ACCESS`: UAV 접근

### 6. Synchronization (동기화)
CPU-GPU, GPU-GPU 동기화를 명시적으로 관리:

#### Fence
```cpp
// GPU에 시그널 명령 추가
commandQueue->Signal(fence, fenceValue);

// CPU에서 대기
if (fence->GetCompletedValue() < fenceValue) {
    fence->SetEventOnCompletion(fenceValue, fenceEvent);
    WaitForSingleObject(fenceEvent, INFINITE);
}
```

### 7. Memory Management
메모리 관리의 세 가지 주요 개념:

#### Upload Heap
- CPU → GPU 데이터 전송
- `D3D12_HEAP_TYPE_UPLOAD`
- CPU write, GPU read

#### Default Heap
- GPU 전용 메모리
- `D3D12_HEAP_TYPE_DEFAULT`
- 최고 성능

#### Readback Heap
- GPU → CPU 데이터 전송
- `D3D12_HEAP_TYPE_READBACK`
- GPU write, CPU read

## 🎯 DX12 초기화 순서

1. **디바이스 생성**
   ```cpp
   D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device));
   ```

2. **커맨드 큐 생성**
   ```cpp
   D3D12_COMMAND_QUEUE_DESC queueDesc = {};
   queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
   device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue));
   ```

3. **스왑체인 생성**
   ```cpp
   DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
   // ... 설정
   dxgiFactory->CreateSwapChainForHwnd(commandQueue, hwnd, &swapChainDesc, ...);
   ```

4. **디스크립터 힙 생성**
   ```cpp
   D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
   heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
   heapDesc.NumDescriptors = frameCount;
   device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeap));
   ```

5. **렌더 타겟 뷰 생성**
   ```cpp
   for (UINT i = 0; i < frameCount; i++) {
       swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));
       device->CreateRenderTargetView(renderTargets[i], nullptr, rtvHandle);
       rtvHandle.Offset(1, rtvDescriptorSize);
   }
   ```

6. **커맨드 allocator & 리스트 생성**

7. **Fence 생성 (동기화용)**

## 📚 다음 단계

- [ ] 기본 렌더링 파이프라인 구현
- [ ] 디스크립터 관리 시스템
- [ ] 리소스 관리 시스템
- [ ] 멀티스레딩 렌더링

## 🔗 참고 자료

- [Microsoft DirectX 12 Programming Guide](https://docs.microsoft.com/en-us/windows/win32/direct3d12/directx-12-programming-guide)
- [DirectX 12 Sample Code](https://github.com/Microsoft/DirectX-Graphics-Samples)
