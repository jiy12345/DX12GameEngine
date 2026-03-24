# 메모리 힙 (Memory Heaps)

## 개요

DX12에서 GPU 리소스(버퍼, 텍스처)가 저장되는 메모리 영역으로, CPU/GPU 접근 방향에 따라 세 가지 타입으로 나뉜다.

## 배경: CPU와 GPU의 메모리 구조

### PCIe (Peripheral Component Interconnect Express)

PCIe는 메인보드에서 CPU/RAM과 GPU를 연결하는 물리적 버스(통신 통로)다.

```
┌─────────────────────────┐         ┌─────────────────────────┐
│  CPU 측                  │         │  GPU 측                  │
│  ┌──────────┐            │         │  ┌──────────────────┐   │
│  │   RAM    │◄──────────►│  PCIe   │  │  VRAM (GDDR6 등) │   │
│  │ (DDR5 등)│  버스      │◄───────►│  │  (GPU 전용 메모리)│   │
│  └──────────┘            │         │  └──────────────────┘   │
└─────────────────────────┘         └─────────────────────────┘
```

| 구간 | 세대 | 대역폭 (단방향) | 출처 |
|---|---|---|---|
| CPU ↔ RAM | DDR5-6400 듀얼채널 | ~102 GB/s | [JEDEC DDR5](https://www.jedec.org/standards-documents/docs/jesd79-5d) |
| CPU ↔ GPU | PCIe 4.0 x16 | ~32 GB/s | [PCI-SIG](https://pcisig.com/pci-express-40-specification-available-pci-sig-members) |
| CPU ↔ GPU | PCIe 5.0 x16 | ~64 GB/s | [PCI-SIG](https://pcisig.com/pci-express%C2%AE-50-specification-available-pci-sig-members) |
| GPU ↔ VRAM | GDDR6X (RTX 4090, 384-bit) | ~1,008 GB/s | [JEDEC GDDR6](https://www.jedec.org/standards-documents/docs/jesd250d) |
| GPU ↔ VRAM | GDDR7 (RTX 5090, 512-bit) | ~1,792 GB/s | [JEDEC GDDR7](https://www.jedec.org/standards-documents/docs/jesd239a) |

> **PCIe 대역폭 계산 방법**: PCIe 4.0은 레인당 16 GT/s, 128b/130b 인코딩 효율 적용 시 레인당 약 2 GB/s. x16 슬롯 = 16 레인 × 2 GB/s = **32 GB/s (단방향)**. 전이중(full-duplex) 구조이므로 동시 양방향 시 총 64 GB/s.
>
> **GDDR6X 대역폭 계산 방법**: 핀당 21 Gbps × 384-bit 버스 ÷ 8 bit/byte = **1,008 GB/s**.

**핵심**: GPU가 VRAM(GDDR6X)에서 읽는 속도와 PCIe를 넘어 RAM에서 읽는 속도의 차이는 **약 31배** (1,008 GB/s vs 32 GB/s).

> **통합 그래픽(내장 GPU)**: CPU와 GPU가 물리적으로 같은 칩에 있어 메모리를 공유하므로 PCIe 병목이 없다. DX12는 이 경우 `UMA(Unified Memory Architecture)` 구조로 힙 타입을 최적화할 수 있다.

### 왜 힙 타입을 구분하는가

데이터의 **변경 빈도**와 **크기**에 따라 메모리 위치를 선택해야 한다:

- **자주 바뀌고 작은 데이터** (Constant Buffer 등): RAM에 두고 GPU가 PCIe로 읽어도 부담 없음
- **거의 안 바뀌고 큰 데이터** (메시, 텍스처): VRAM에 올려두고 GPU가 빠르게 읽어야 함

## 왜 필요한가?

### 문제

CPU 메모리(RAM)와 GPU 메모리(VRAM)는 물리적으로 분리되어 있다. CPU가 작성한 데이터를 GPU가 읽으려면 명시적인 전송 과정이 필요하다.

### 해결책

DX12는 CPU/GPU 접근 방식에 따라 힙 타입을 명시적으로 구분한다. 개발자가 직접 타입을 선택함으로써 메모리 배치와 캐시 동작을 제어할 수 있다.

## 힙 타입

| 타입 | CPU 접근 | GPU 접근 | 주 용도 |
|---|---|---|---|
| `UPLOAD` | 읽기/쓰기 | 읽기 전용 | CPU → GPU 데이터 전송 |
| `DEFAULT` | 불가 | 읽기/쓰기 | GPU 전용 고성능 리소스 |
| `READBACK` | 읽기 전용 | 쓰기 전용 | GPU → CPU 결과 읽기 |

### UPLOAD 힙

CPU에서 직접 쓰고 GPU에서 읽는 용도. `Map()`/`Unmap()`으로 CPU에서 직접 데이터를 작성한다.

- GPU 입장에서는 시스템 메모리(RAM)에 접근하는 것이므로 **VRAM보다 느리다**
- 매 프레임 변경되는 데이터(Constant Buffer, 동적 Vertex Buffer)에 적합
- 리소스 상태는 항상 `D3D12_RESOURCE_STATE_GENERIC_READ`

```cpp
D3D12_HEAP_PROPERTIES uploadHeapProps = {};
uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

device->CreateCommittedResource(
    &uploadHeapProps,
    D3D12_HEAP_FLAG_NONE,
    &bufferDesc,
    D3D12_RESOURCE_STATE_GENERIC_READ,  // UPLOAD 힙의 고정 상태
    nullptr,
    IID_PPV_ARGS(&uploadBuffer));

// CPU에서 직접 쓰기
void* mappedData;
uploadBuffer->Map(0, nullptr, &mappedData);
memcpy(mappedData, srcData, dataSize);
uploadBuffer->Unmap(0, nullptr);
```

### DEFAULT 힙

GPU 전용 메모리(VRAM). CPU에서 직접 접근 불가.

- GPU 로컬 메모리이므로 **최고 성능**
- 정적 지오메트리(변경되지 않는 Vertex/Index Buffer), 텍스처에 적합
- 초기 데이터는 UPLOAD 힙을 거쳐 복사해야 함

```cpp
D3D12_HEAP_PROPERTIES defaultHeapProps = {};
defaultHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

device->CreateCommittedResource(
    &defaultHeapProps,
    D3D12_HEAP_FLAG_NONE,
    &bufferDesc,
    D3D12_RESOURCE_STATE_COPY_DEST,  // 복사 받을 준비 상태
    nullptr,
    IID_PPV_ARGS(&defaultBuffer));
```

### READBACK 힙

GPU가 쓴 결과를 CPU에서 읽는 용도.

- GPU 타이밍 측정, 컴퓨트 결과 읽기 등에 사용
- 리소스 상태는 항상 `D3D12_RESOURCE_STATE_COPY_DEST`

## DEFAULT 힙으로 데이터 업로드하는 패턴

UPLOAD 힙을 스테이징 버퍼로 사용해 DEFAULT 힙으로 데이터를 복사한다.

```
CPU 메모리 → [UPLOAD 힙] → CopyBufferRegion → [DEFAULT 힙] → GPU 사용
```

```cpp
// 1. UPLOAD 힙에 스테이징 버퍼 생성 및 CPU 데이터 복사
ComPtr<ID3D12Resource> uploadBuffer;
// ... UPLOAD 힙으로 생성 후 Map/memcpy/Unmap

// 2. DEFAULT 힙에 최종 버퍼 생성
ComPtr<ID3D12Resource> defaultBuffer;
// ... DEFAULT 힙으로 생성 (COPY_DEST 상태)

// 3. 커맨드 리스트로 복사 기록
commandList->CopyBufferRegion(
    defaultBuffer.Get(),  // 대상 (DEFAULT)
    0,                    // 대상 오프셋
    uploadBuffer.Get(),   // 원본 (UPLOAD)
    0,                    // 원본 오프셋
    dataSize);

// 4. DEFAULT 버퍼 상태 전환 (복사 완료 후 GPU 읽기 가능 상태로)
D3D12_RESOURCE_BARRIER barrier = {};
barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
barrier.Transition.pResource   = defaultBuffer.Get();
barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
commandList->ResourceBarrier(1, &barrier);

// 5. 커맨드 리스트 실행 및 GPU 완료 대기
// ExecuteCommandLists → Fence Wait
// → GPU 복사 완료 후 uploadBuffer 해제 가능
```

> **주의**: UPLOAD 버퍼는 GPU가 복사를 완료할 때까지 유지해야 한다. Fence로 GPU 완료를 확인한 후 해제.

## 힙 타입 선택 기준

| 데이터 성격 | 권장 힙 |
|---|---|
| 매 프레임 갱신 (Constant Buffer, 동적 메시) | `UPLOAD` |
| 초기 1회 업로드 후 변경 없음 (정적 메시, 텍스처) | `DEFAULT` (UPLOAD 스테이징) |
| GPU 연산 결과를 CPU로 읽기 | `READBACK` |

## 주의사항

- ⚠️ `UPLOAD` 힙 리소스는 배리어 없이 항상 `GENERIC_READ` 상태여야 한다.
- ⚠️ `DEFAULT` 힙 리소스에 CPU에서 직접 `Map()`을 시도하면 실패한다.
- ⚠️ `CopyBufferRegion` 호출 후 즉시 `uploadBuffer`를 해제하면 안 된다. GPU 복사가 비동기이므로 Fence로 완료 확인 후 해제.
- ⚠️ 텍스처 업로드는 `CopyBufferRegion` 대신 `CopyTextureRegion`과 `GetCopyableFootprints`를 사용한다.

## 관련 개념

### 선행 개념
- [CommandList](./CommandList.md) - 복사 커맨드 기록
- [Synchronization](./Synchronization.md) - 업로드 완료 대기

### 연관 개념
- [ConstantBuffer](./ConstantBuffer.md) - UPLOAD 힙의 대표적 사용 사례
- [ResourceBarriers](./ResourceBarriers.md) - 복사 후 상태 전환

## 참고 자료

- [Microsoft: Uploading resources](https://learn.microsoft.com/en-us/windows/win32/direct3d12/uploading-resources)
- [Microsoft: D3D12_HEAP_TYPE enumeration](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_heap_type)
- [Microsoft: Suballocating within buffers](https://learn.microsoft.com/en-us/windows/win32/direct3d12/suballocating-in-buffers)
