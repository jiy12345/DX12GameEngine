# 커맨드 시스템 (Commands)

## 목차

- [개요](#개요)
- [세 가지 핵심 객체](#세-가지-핵심-객체)
  - [Command Allocator](#command-allocator)
  - [Command List](#command-list)
  - [Command Queue](#command-queue)
- [우편 시스템 비유](#우편-시스템-비유)
- [Command Queue vs Command List 구분](#command-queue-vs-command-list-구분)
- [Allocator와 List는 왜 분리되어 있나?](#allocator와-list는-왜-분리되어-있나)
- [전체 실행 흐름](#전체-실행-흐름)
- [여러 큐의 병렬 실행](#여러-큐의-병렬-실행)
  - [주체: 엔진이 큐를 관리](#주체-엔진이-큐를-관리)
  - [GPU 하드웨어 엔진](#gpu-하드웨어-엔진)
  - [큐 타입과 엔진 매핑](#큐-타입과-엔진-매핑)
  - [Fence를 통한 큐 간 동기화](#fence를-통한-큐-간-동기화)
- [멀티스레드 기록의 실제 의미](#멀티스레드-기록의-실제-의미)
  - [흔한 오해](#흔한-오해)
  - [CPU에서 기록이 왜 무거운가](#cpu에서-기록이-왜-무거운가)
  - [실제 숫자로 보기](#실제-숫자로-보기)
  - [멀티스레드 기록 효과](#멀티스레드-기록-효과)
  - [GPU는 병렬 기록과 무관](#gpu는-병렬-기록과-무관)
- [학습 순서](#학습-순서)
- [참고 자료](#참고-자료)

## 개요

DX12의 커맨드 시스템은 CPU가 GPU에게 작업을 전달하는 파이프라인입니다. 세 가지 핵심 객체(Allocator, List, Queue)가 협력하며, 각자 다른 책임을 갖습니다.

이 폴더의 문서들은 함께 이해해야 의미가 명확해집니다. 본 개요 문서는 세 객체의 관계, 병렬 실행 메커니즘, 그리고 흔한 오해를 다룹니다. 개별 상세는 각 문서를 참고하세요.

## 세 가지 핵심 객체

### Command Allocator

GPU 명령이 **기록되는 메모리 공간**입니다. Command List가 실제로 명령 바이트를 적어 넣는 백업 저장소(CPU 메모리)입니다.

- CPU 시스템 메모리에 존재하는 버퍼
- GPU가 해당 Allocator의 명령을 모두 실행 완료한 후에만 Reset 가능 (Fence 필요)
- 스레드별로 하나씩 두는 것이 일반적

상세: [CommandAllocator.md](./CommandAllocator.md)

### Command List

Draw, Dispatch, Copy, ResourceBarrier 등의 **명령을 인코딩하여 Allocator에 쓰는** 인터페이스입니다.

- 자체적으로 큰 데이터를 가지지 않음 (현재 PSO, RootSig 같은 상태만 추적)
- 기록 완료 후 Close() 호출 필수
- Reset()으로 재사용 가능 (다른 Allocator를 지정할 수도 있음)

상세: [CommandList.md](./CommandList.md)

### Command Queue

GPU에 작업을 **제출하는 채널**입니다. 여러 Command List를 한꺼번에 제출하고, Fence로 동기화합니다.

- 큐 타입별로 작동 (Direct / Compute / Copy)
- ExecuteCommandLists()로 제출
- FIFO 순서로 GPU 처리

상세: [CommandQueue.md](./CommandQueue.md)

## 우편 시스템 비유

| | Command Allocator | Command List | Command Queue |
|---|------------------|--------------|---------------|
| 비유 | **편지지** (종이) | **타자기** (기록 도구) | **우체통** (제출 창구) |
| 역할 | 명령이 기록되는 메모리 | 명령을 인코딩하는 인터페이스 | GPU에 제출하는 채널 |
| 데이터 | 실제 GPU 명령 바이트 (수 MB) | 상태만 (수백 바이트) | 내부 큐 구조 |

```
타자기(List) ──쓴다──▶ 편지지(Allocator)
                            │
                            └──우체통(Queue)──▶ 수신자(GPU)
```

## Command Queue vs Command List 구분

가장 혼동되는 부분입니다.

| | Command List | Command Queue |
|---|---|---|
| 역할 | 명령을 **기록**하는 객체 | GPU에 **제출**하는 채널 |
| 상태 | 아직 실행 안 됨 | 실행 관리 |
| 주 작업 | Draw, Dispatch, Copy 등 기록 | ExecuteCommandLists |
| 스레드 안전성 | 개별 List는 비안전, 여러 List 병렬 기록은 안전 | 여러 스레드가 같은 큐에 제출 가능 |

**Command List** (기록):
```cpp
// 명령들을 "적어두는" 것. 아직 GPU는 모름.
commandList->SetPipelineState(pso);
commandList->IASetVertexBuffers(...);
commandList->DrawInstanced(3, 1, 0, 0);  // 실제 실행 X
commandList->Close();  // 기록 종료
// ↑ 여기까지는 Allocator 메모리에 명령 바이트가 쌓여있을 뿐
```

**Command Queue** (제출):
```cpp
// 적어둔 편지를 "GPU에게 보내는" 것
ID3D12CommandList* lists[] = { commandList };
commandQueue->ExecuteCommandLists(1, lists);
// ↑ 이제야 GPU가 실제로 일을 시작함
```

## Allocator와 List는 왜 분리되어 있나?

이 둘이 합쳐져 있지 않은 이유는 **성능과 파이프라이닝** 때문입니다.

### 1. 메모리 재사용

```cpp
// 분리 덕분에 매 프레임 메모리 할당 없이 Reset만으로 재사용
allocator->Reset();                        // 메모리 그대로, 내용만 초기화
commandList->Reset(allocator, initialPSO); // List는 Allocator를 다시 가리킬 뿐
commandList->Draw(...);                    // 같은 메모리에 새 명령 쓰기
```

Allocator는 수 MB 크기가 될 수 있어, 매 프레임 새로 할당하면 큰 오버헤드입니다.

### 2. CPU와 GPU의 수명 차이

```
시간 →
CPU: [프레임 N 기록] [Close] [제출]
GPU:                          [프레임 N 실행 ··········]
                              ↑ 이 동안 Allocator[N] 건드리면 GPU 크래시
```

분리되어 있기 때문에 **GPU가 Allocator[N]을 읽는 동안 CPU는 Allocator[N+1]에 다음 프레임을 기록**할 수 있습니다 (더블/트리플 버퍼링).

### 3. 멀티스레딩

각 스레드가 자기 Allocator + 자기 List를 쓰면, 서로 다른 메모리를 건드리므로 **락 없이 병렬 기록** 가능. 하나로 합쳐져 있었다면 내부 락이 필요했을 것입니다.

### 4. List 하나로 여러 Allocator 전환

같은 Command List 객체를 **다른 Allocator와 번갈아** 사용 가능합니다:

```cpp
commandList->Reset(renderAllocator, ...);
commandList->Draw(...);
commandList->Close();

commandList->Reset(uiAllocator, ...);
commandList->DrawUI(...);
```

정리: **"GPU가 쓰는 중인 메모리"** 와 **"CPU가 기록 중인 인터페이스"** 의 수명이 다르므로 분리하는 게 자연스럽고, 이 분리 덕에 파이프라이닝/멀티스레딩이 가능합니다.

## 전체 실행 흐름

```
┌─────────────────────────────────────────┐
│ 1. Command Allocator 확보 (재사용 가능)   │
└─────────────────────────────────────────┘
                ↓
┌─────────────────────────────────────────┐
│ 2. Command List가 Allocator를 가리킴      │
│    → Reset(allocator, initialPSO)       │
└─────────────────────────────────────────┘
                ↓
┌─────────────────────────────────────────┐
│ 3. Command List로 명령 기록              │
│    (편지 쓰기 - Allocator 메모리에 직접)  │
│    - Draw, Dispatch, Copy               │
│    - ResourceBarrier                    │
│    - SetPipelineState, SetRootSignature │
└─────────────────────────────────────────┘
                ↓ Close()
┌─────────────────────────────────────────┐
│ 4. Command Queue에 제출                  │
│    (우체통에 넣기)                       │
└─────────────────────────────────────────┘
                ↓ ExecuteCommandLists()
┌─────────────────────────────────────────┐
│ 5. GPU가 DMA로 Allocator 읽고 실행       │
└─────────────────────────────────────────┘
                ↓ Signal(fence)
┌─────────────────────────────────────────┐
│ 6. Fence로 완료 확인 → Allocator 재사용  │
└─────────────────────────────────────────┘
```

## 여러 큐의 병렬 실행

### 주체: 엔진이 큐를 관리

큐를 생성하고 작업을 제출하는 주체는 **CPU 측 애플리케이션**(엔진 코드)입니다. 개발자는 원하는 만큼 큐를 직접 생성합니다.

```cpp
D3D12_COMMAND_QUEUE_DESC directDesc = { D3D12_COMMAND_LIST_TYPE_DIRECT };
device->CreateCommandQueue(&directDesc, IID_PPV_ARGS(&directQueue));

D3D12_COMMAND_QUEUE_DESC computeDesc = { D3D12_COMMAND_LIST_TYPE_COMPUTE };
device->CreateCommandQueue(&computeDesc, IID_PPV_ARGS(&computeQueue));

D3D12_COMMAND_QUEUE_DESC copyDesc = { D3D12_COMMAND_LIST_TYPE_COPY };
device->CreateCommandQueue(&copyDesc, IID_PPV_ARGS(&copyQueue));
```

### GPU 하드웨어 엔진

현대 GPU에는 **물리적으로 분리된 여러 엔진**이 있습니다:

```
┌─────────────────────────────────────────┐
│              GPU 하드웨어                 │
├─────────────────────────────────────────┤
│  Graphics Engine                         │  ← 3D 파이프라인, 래스터라이저
├─────────────────────────────────────────┤
│  Compute Engine (ACE)                    │  ← 컴퓨트 전용 실행 유닛
├─────────────────────────────────────────┤
│  DMA Engine (Copy Engine)                │  ← PCIe/VRAM 복사 전용
└─────────────────────────────────────────┘
```

이 엔진들은 **전기 회로가 별개**이기 때문에 서로 독립적으로 동시에 작동할 수 있습니다.

### 큐 타입과 엔진 매핑

| 큐 타입 | 매핑되는 엔진 | 가능한 작업 |
|---------|--------------|-----------|
| Direct | Graphics Engine | 렌더링, 컴퓨트, 복사 모두 가능 |
| Compute | Compute Engine | 컴퓨트, 복사만 |
| Copy | DMA Engine | 복사만 |

**큐를 여러 개 만든다 = 여러 하드웨어 엔진에 독립적인 작업 스트림을 공급한다.**

병렬 실행 예시:

```cpp
// 1. Copy Queue: 다음 프레임에서 쓸 텍스처 업로드
copyQueue->ExecuteCommandLists(1, &textureUploadList);

// 2. Compute Queue: 파티클 시뮬레이션
computeQueue->ExecuteCommandLists(1, &particleSimList);

// 3. Direct Queue: 현재 프레임 렌더링
directQueue->ExecuteCommandLists(1, &renderList);

// 세 명령 모두 즉시 반환 - GPU에서는 병렬 실행 중
```

시간축으로 보면:

```
시간 →
Graphics Engine:  [렌더링 ████████████████████]
Compute Engine:   [파티클 ████████]
DMA Engine:       [텍스처 업로드 ██████]
                  ↑ 세 엔진이 동시에 작동
```

### Fence를 통한 큐 간 동기화

큐끼리는 기본적으로 **독립 실행**이지만, 의존성이 있으면 **Fence**로 명시합니다:

```cpp
// 시나리오: Compute로 광원 컬링 후, Direct로 포워드 라이팅
//          컬링 결과를 라이팅이 읽어야 함

// Compute Queue에서 컬링 작업 제출
computeQueue->ExecuteCommandLists(1, &cullingList);
computeQueue->Signal(fence, 100);

// Direct Queue에서 컬링이 끝날 때까지 대기 후 라이팅 실행
directQueue->Wait(fence, 100);
directQueue->ExecuteCommandLists(1, &lightingList);
```

**핵심**: `Signal`과 `Wait`는 **GPU 측에서** 대기합니다. CPU가 막히지 않고 계속 다음 프레임을 준비할 수 있습니다.

## 멀티스레드 기록의 실제 의미

### 흔한 오해

> "Command List의 주요 작업은 GPU에서 일어나니까, 기록을 병렬화해도 의미 없지 않나?"

이 오해는 두 가지 혼동에서 비롯됩니다:

1. **병렬화 대상은 "제출"이 아니라 "기록"** - 제출(ExecuteCommandLists)은 거의 공짜, 기록이 무거움
2. **기록은 단순 포인터 조작이 아님** - 실제로는 상당한 CPU 작업

### CPU에서 기록이 왜 무거운가

`Draw()` 호출 한 번 뒤에 숨은 CPU 작업:

1. 파라미터 검증
2. 현재 바인딩된 PSO/Root Signature 상태 체크
3. 디스크립터 테이블 갱신 여부 확인
4. Command Buffer(Allocator 메모리)에 GPU 명령어 인코딩
5. 배리어 추적 (State Tracker 사용 시)
6. 통계 업데이트

Draw 하나당 **1~10 마이크로초** 정도 걸립니다. 전후 세팅(`SetPipelineState`, `IASetVertexBuffers` 등)도 비슷한 비용입니다.

### 실제 숫자로 보기

현대 AAA 게임의 Draw Call 수:

| 렌더 패스 | Draw Call |
|-----------|-----------|
| Shadow Map (4 cascades) | ~2,000 |
| G-Buffer | ~3,000 |
| Lighting | ~500 |
| Transparents | ~500 |
| Post-processing | ~100 |
| UI | ~500 |
| **총합** | **~6,600** (중간급) |

복잡한 씬은 10,000~50,000 Draw Call도 흔합니다.

```
Draw Call 하나당 3μs 가정
10,000 Draw Call × 3μs = 30ms

60fps 목표 = 16.6ms/frame

단일 스레드 기록으로는 이미 목표 달성 불가!
```

CPU가 기록을 못 따라가면 **GPU가 논다** (GPU starvation). 그래픽 카드가 아무리 빨라도 CPU 병목 때문에 FPS가 안 나옵니다.

### 멀티스레드 기록 효과

```
10,000 Draw Call을 4개 스레드로 분할 기록
각 스레드: 2,500 Draw × 3μs = 7.5ms
병렬이므로 전체 기록 시간: ~7.5ms

→ 30ms → 7.5ms = 60fps 확보 가능
```

### GPU는 병렬 기록과 무관

```
CPU: Thread 1 [기록 ████]
     Thread 2 [기록 ████]    ← 이들은 동시에 기록
     Thread 3 [기록 ████]
     Thread 4 [기록 ████]
     ───────────────────
     Main     [취합+제출 ▌]   ← 빠름

GPU:          [실행 ██████████████]  ← 순서대로 실행
```

**GPU는 받은 순서대로 처리**합니다. 병렬 기록의 목적은 GPU를 빠르게 만드는 것이 아니라, **CPU 기록이 GPU 실행 속도를 못 따라가는 병목을 해소하는 것**입니다.

DX11은 Deferred Context에서 드라이버 내부 락 때문에 진짜 병렬이 안 됐지만, DX12는 Command List가 완전히 독립적이라 **선형에 가까운 스레드 확장성**을 얻습니다. 이것이 DX12가 DX11보다 실제로 빨라지는 핵심 이유 중 하나입니다.

## 학습 순서

1. [CommandQueue.md](./CommandQueue.md) - 전체 철학과 큐의 역할
2. [CommandAllocator.md](./CommandAllocator.md) - 메모리 관리 기반
3. [CommandList.md](./CommandList.md) - 실제 명령 기록

본 개요 문서는 학습 중간에도 다시 돌아와서 전체 그림을 재확인하는 용도로 활용하세요.

## 참고 자료

- [Microsoft: Design philosophy of command queues and command lists](https://learn.microsoft.com/en-us/windows/win32/direct3d12/design-philosophy-of-command-queues-and-command-lists)
- [Microsoft: Multi-engine synchronization](https://learn.microsoft.com/en-us/windows/win32/direct3d12/user-mode-heap-synchronization)
- [Microsoft: Recording Command Lists and Bundles](https://learn.microsoft.com/en-us/windows/win32/direct3d12/recording-command-lists-and-bundles)
