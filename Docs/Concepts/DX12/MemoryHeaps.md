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

### CPU → VRAM 통신의 유일한 경로: UPLOAD 힙

CPU는 VRAM(DEFAULT 힙)에 직접 접근할 수 없다. CPU가 데이터를 VRAM에 넣는 유일한 방법은 **UPLOAD 힙을 경유**하는 것이다.

```
CPU → (Map/memcpy) → UPLOAD 힙 (RAM)
                            │
                    CopyBufferRegion (GPU 커맨드)
                            │
                            ▼
                     DEFAULT 힙 (VRAM) ← GPU가 빠르게 읽음
```

1. CPU가 `Map()`으로 UPLOAD 힙(RAM)에 데이터를 씀
2. GPU 커맨드(`CopyBufferRegion`)로 UPLOAD → DEFAULT 복사를 기록
3. `ExecuteCommandLists`로 GPU가 실제 복사 수행
4. 복사 완료 후 UPLOAD 힙(스테이징 버퍼)은 해제 가능

> UPLOAD 힙은 CPU와 GPU 모두 접근할 수 있는 **중간 지점(우편함)** 역할을 한다. CPU가 데이터를 올려두면 GPU가 가져가는 구조.

### 왜 힙 타입을 구분하는가

데이터의 **변경 빈도**와 **크기**에 따라 메모리 위치를 선택해야 한다:

- **자주 바뀌고 작은 데이터** (Constant Buffer 등): UPLOAD 힙(RAM)에 두고 GPU가 PCIe로 읽어도 부담 없음
- **거의 안 바뀌고 큰 데이터** (메시, 텍스처): UPLOAD 힙을 경유해 DEFAULT 힙(VRAM)으로 올린 뒤 GPU가 빠르게 읽음

### 힙(Heap)이란 물리적으로 무엇인가

힙은 RAM 또는 VRAM에서 **"이 영역을 GPU 리소스용으로 쓰겠다"고 예약해둔 연속된 메모리 블록**이다. C++의 `new`/`malloc`이 힙 메모리를 예약하는 것과 같은 개념이며, 자료구조의 힙(heap)과는 무관하다.

```
RAM 물리 메모리
┌─────────────────────────────────────────────────┐
│  OS 영역  │  프로세스 일반 메모리  │  UPLOAD 힙  │
│           │                      │  [예약됨]   │ ← GPU 리소스가 여기 들어감
└─────────────────────────────────────────────────┘

VRAM 물리 메모리
┌─────────────────────────────────────────────────┐
│  드라이버 내부  │       DEFAULT 힙               │
│               │       [예약됨]                  │ ← GPU 리소스가 여기 들어감
└─────────────────────────────────────────────────┘
```

**힙 타입**은 그 예약된 메모리 블록이 어디에 위치하고, CPU/GPU가 어떻게 접근하는지를 결정하는 속성이다.

**DX12에서 힙을 쓰는 두 가지 방식**

| 방식 | API | 설명 |
|---|---|---|
| 암묵적 힙 | `CreateCommittedResource` | 리소스 하나당 전용 힙이 자동 생성됨. 현재 프로젝트에서 사용 중 |
| 명시적 힙 | `CreateHeap` + `CreatePlacedResource` | 큰 블록을 직접 예약하고 그 안에 여러 리소스를 수동 배치. 메모리 단편화 제어 가능 |

> Phase 1에서는 단순함을 위해 `CreateCommittedResource`(암묵적 힙)를 사용한다. 추후 메모리 관리를 정교하게 제어해야 할 경우 명시적 힙으로 전환할 수 있다.

### READBACK 힙의 물리적 위치

READBACK 힙도 **RAM에 위치**한다. UPLOAD 힙과 물리적 위치는 같지만 데이터 흐름 방향이 반대다.

```
UPLOAD  힙 (RAM) ← CPU가 씀       →  GPU가 PCIe로 읽음   (CPU → GPU)
READBACK 힙 (RAM) ← GPU가 PCIe로 씀 →  CPU가 읽음         (GPU → CPU)
DEFAULT  힙 (VRAM)                     GPU만 직접 읽기/쓰기
```

| 힙 타입 | 물리 위치 | CPU 접근 | GPU 접근 | 캐시 방식 |
|---|---|---|---|---|
| UPLOAD | RAM | 쓰기 (`Map`) | PCIe로 읽기 | Write-Combined (순차 쓰기 최적화) |
| READBACK | RAM | 읽기 (`Map`) | PCIe로 쓰기 | Cached (CPU 읽기 최적화) |
| DEFAULT | VRAM | 불가 | 직접 읽기/쓰기 | GPU 캐시 |

READBACK의 실제 사용 예: GPU가 연산한 결과(GPU 타이밍, 컴퓨트 결과)를 CPU에서 읽어야 할 때.
`CopyBufferRegion`으로 DEFAULT 힙(VRAM) → READBACK 힙(RAM)으로 복사한 뒤 CPU가 `Map()`으로 읽는다.

### 접근 제어는 어떻게 이루어지는가

힙 타입별 접근 제어는 **컴파일 타임이 아닌 런타임**에 이루어지며, 두 계층으로 작동한다.

**1. 하드웨어 계층 (DEFAULT 힙)**

DEFAULT 힙(VRAM)은 GPU의 MMU(Memory Management Unit)가 CPU 가상 주소 공간에 매핑하지 않는다. CPU가 `Map()`을 호출하면 드라이버가 `E_FAIL`을 반환하며, 이는 하드웨어 수준에서 물리적으로 차단된 것이다.

**2. API/드라이버 계층 (UPLOAD, READBACK 힙)**

UPLOAD와 READBACK은 둘 다 CPU가 `Map()`할 수 있는 RAM에 있다. 이 두 힙의 접근 방향은 **리소스 상태(Resource State)** 로 강제된다.

| 힙 타입 | 고정 리소스 상태 | 의미 |
|---|---|---|
| UPLOAD | `GENERIC_READ` (고정, 변경 불가) | GPU는 읽기만 가능, 쓰기 불가 |
| READBACK | `COPY_DEST` (고정, 변경 불가) | GPU는 복사 대상으로만 쓸 수 있음 |

이 상태들은 배리어(Resource Barrier)로 변경할 수 없다. [Debug Layer](./DebugLayer.md)가 이 규칙 위반을 런타임에 즉시 감지하고 오류를 출력한다.

**컴파일 타임에는 감지되지 않는다.** DX12 API는 힙 타입과 리소스 상태를 모두 정수(enum) 인자로 받기 때문에, C++ 타입 시스템은 두 값의 의미론적 호환성을 알 수 없다. 잘못된 조합은 `CreateCommittedResource` 호출 시점(런타임)에 `E_INVALIDARG`로 실패한다.

```cpp
// UPLOAD 힙은 항상 이 상태로 고정
D3D12_RESOURCE_STATE_GENERIC_READ

// READBACK 힙은 항상 이 상태로 고정
D3D12_RESOURCE_STATE_COPY_DEST

// DEFAULT 힙은 상황에 따라 배리어로 전환 가능
D3D12_RESOURCE_STATE_COPY_DEST → D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER 등
```

> **요약**: DEFAULT 힙의 CPU 접근 차단은 하드웨어가, UPLOAD/READBACK의 GPU 접근 방향 제어는 리소스 상태 + Debug Layer가 런타임에 강제한다.

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
