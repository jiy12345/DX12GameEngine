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
- [Draw 호출이 실제로 하는 일](#draw-호출이-실제로-하는-일)
- [멀티스레드 기록의 실제 의미](#멀티스레드-기록의-실제-의미)
  - [흔한 오해](#흔한-오해)
  - [CPU에서 기록이 왜 무거운가](#cpu에서-기록이-왜-무거운가)
  - [실제 숫자로 보기](#실제-숫자로-보기)
  - [단일 스레드 + 여러 Allocator로 해결되나?](#단일-스레드--여러-allocator로-해결되나)
  - [멀티스레드 기록 효과](#멀티스레드-기록-효과)
  - [GPU는 병렬 기록과 무관](#gpu는-병렬-기록과-무관)
- [하드웨어 스레드와 실제 병렬 실행](#하드웨어-스레드와-실제-병렬-실행)
  - [소프트웨어 스레드와 하드웨어 스레드](#소프트웨어-스레드와-하드웨어-스레드)
  - [Command List 기록이 병렬화에 이상적인 이유](#command-list-기록이-병렬화에-이상적인-이유)
  - [실제 스케일링 사례](#실제-스케일링-사례)
  - [Amdahl의 법칙과 실무 상한](#amdahl의-법칙과-실무-상한)
  - [실측 검증](#실측-검증)
- [구현 패턴: 페어링 vs 풀](#구현-패턴-페어링-vs-풀)
- [주의사항](#주의사항)
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

이 둘이 합쳐져 있지 않은 이유는 **수명 주기 차이**와 **유연성** 때문입니다. 흔히 "메모리 효율"이 주된 이유로 언급되지만, 실무 환경(멀티스레드 기록)에서는 그 이득이 크지 않습니다. 진짜 이점은 다음 항목들에 있습니다.

### 1. 메모리 효율 (제한적 이점)

단일 스레드 파이프라이닝에서는 `N Allocator + 1 List` 구조로 List 상태 메모리를 절약할 수 있습니다. 하지만:

```
단일 스레드 파이프라이닝:
  N Allocator + 1 List     → 분리로 List 상태 (N-1)개 절약 ✅

멀티스레드 기록 (실제 환경):
  Thread × Frame 개 Allocator + Thread 개 List
  → 결국 List도 여러 개 필요 → 메모리 절약 이득이 작아짐 ⚠️
```

> **솔직한 평가**: 멀티스레드 환경에서 **메모리 이득은 크지 않습니다.** List 객체는 수백 바이트 수준이라 Thread 수(보통 4~8)만큼 있어도 무시할 크기. 분리의 진짜 이득은 **2번~5번** 입니다.

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

### 4. Reset 비용의 분리 (중요)

Allocator Reset과 List Reset의 **동작과 비용이 다릅니다**. 합쳐져 있다면 항상 둘 다 초기화해야 하지만, 분리되어 있으면 **필요한 것만** 초기화할 수 있습니다:

| | Allocator Reset | List Reset |
|---|----------------|-----------|
| 하는 일 | 쓰기 오프셋만 0으로 리셋 | State machine 초기화 (PSO, RS, viewport, ...) |
| 비용 | **매우 저렴** (포인터 조작만) | 중간 (상태 리셋 + 초기 PSO 바인딩) |
| 사용 시점 | 프레임 끝, GPU 완료 후 | 매 기록 시작 전 |
| GPU 대기 필요 | ✅ Fence로 확인 | ❌ |

분리 덕분에:
- **프레임 끝**: Allocator만 Reset (GPU 완료 확인 후), List는 그대로 두거나 다음 Allocator로 Reset
- **기록 중간**: List만 Reset해서 다른 Allocator/PSO로 전환

### 5. List 하나로 여러 Allocator 전환 (유연성)

같은 Command List 객체를 **다른 Allocator와 번갈아** 사용 가능합니다:

```cpp
// 한 List가 여러 Allocator를 순회
commandList->Reset(renderAllocator, ...);
commandList->Draw(...);
commandList->Close();
// 제출 후 같은 List를 다른 용도로 재사용 가능

commandList->Reset(uiAllocator, ...);
commandList->DrawUI(...);
commandList->Close();
```

### 정리

| 이점 | 단일 스레드 이득 | 멀티스레드 이득 |
|------|--------------|--------------|
| 1. 메모리 효율 | ⭐⭐ | ⭐ (제한적) |
| 2. CPU-GPU 수명 분리 (파이프라이닝) | ⭐⭐⭐ | ⭐⭐⭐ |
| 3. 멀티스레딩 (락 없는 기록) | — | ⭐⭐⭐ |
| 4. Reset 비용 분리 | ⭐⭐ | ⭐⭐ |
| 5. List 재사용 유연성 | ⭐⭐ | ⭐⭐ |

**핵심**: 분리의 진짜 이득은 **"GPU가 쓰는 중인 메모리(Allocator)"와 "CPU가 기록하는 인터페이스(List)"의 수명이 근본적으로 다르기 때문**입니다. 메모리 효율은 부수적 이득이고, 수명 분리 + 유연성이 본질.

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

## Draw 호출이 실제로 하는 일

멀티스레드 기록의 필요성을 이해하려면 먼저 `Draw()` 호출이 실제로 무엇인지 정확히 알아야 합니다.

### 흔한 오해: "Draw() 시점에 GPU가 그리기 시작한다"

```cpp
commandList->DrawInstanced(3, 1, 0, 0);
```

이 한 줄을 보면 "GPU가 이 시점에 그리기 시작한다"고 오해하기 쉽지만, **GPU는 아직 이 명령의 존재조차 모릅니다.**

### 실제 동작

`Draw()` 호출이 실제로 하는 일:

1. CPU가 Draw 명령을 **GPU 명령 바이트로 인코딩**
2. 현재 바인딩된 **리소스 참조들을 함께 기록** (PSO 주소, RootSig 슬롯, VB/IB 주소, 디스크립터 핸들 등)
3. 그 바이트 시퀀스를 **Allocator 메모리에 쓰기**
4. 함수 리턴 (끝. GPU는 아직 아무것도 안 함)

### GPU는 언제 시작하는가?

GPU가 실제로 명령을 읽기 시작하는 시점은 `Draw()`가 아니라:

```cpp
commandQueue->ExecuteCommandLists(1, &commandList);
```

여기서 처음으로 GPU가 Allocator 메모리를 DMA로 읽기 시작합니다.

### 타임라인

```
CPU timeline:
[Draw encode (1-10μs)] [Draw encode] [Draw encode] ... [Close] [ExecuteCommandLists ~0μs]
                                                                ↓ 여기서 처음으로
GPU timeline:                                                   [GPU가 명령 읽고 실행]
```

### 왜 Draw 한 번이 1~10μs나 걸리나?

단순히 `DRAW 3 1 0 0` 같은 짧은 바이트만 쓰는 게 아닙니다. GPU가 **독립적으로 실행할 수 있는 자족적 명령 패킷**이 되어야 하므로:

- 현재 PSO 주소
- Root Signature 슬롯마다의 디스크립터 핸들
- 바인딩된 Vertex Buffer / Index Buffer 주소
- Viewport / Scissor 상태
- 기타 파이프라인 상태

이 모든 참조를 함께 인코딩해야 합니다. 결과적으로 Draw 하나가 수십~수백 바이트의 GPU 명령 바이트로 변환되고, 이 과정이 CPU에서 1~10μs 걸립니다.

### 결론

**"Draw call이 비싸다"** 의 진짜 의미:
- ❌ "GPU가 그리는 게 비싸다"
- ✅ "CPU가 그 명령을 Allocator에 쓰는 게 비싸다"

이 인식이 있어야 다음의 "멀티스레드 기록"이 왜 중요한지가 체감됩니다.

## 멀티스레드 기록의 실제 의미

### 흔한 오해

> "Command List의 주요 작업은 GPU에서 일어나니까, 기록을 병렬화해도 의미 없지 않나?"

이 오해는 세 가지 혼동에서 비롯됩니다:

1. **Draw()는 GPU 작업이 아니라 CPU 작업** - 앞 섹션 ["Draw 호출이 실제로 하는 일"](#draw-호출이-실제로-하는-일) 참조
2. **병렬화 대상은 "제출"이 아니라 "기록"** - 제출(ExecuteCommandLists)은 거의 공짜, 기록이 무거움
3. **기록은 단순 포인터 조작이 아님** - 리소스 참조 포함 상세 인코딩이 필요

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

### 단일 스레드 + 여러 Allocator로 해결되나?

"그냥 메인 스레드 하나로 Allocator를 여러 개 할당받아서 순차적으로 쓰면 되지 않나?" 라고 생각할 수 있습니다. **안 됩니다.** 이유:

```
여러 Allocator (단일 스레드):
CPU: [F0 기록 → A: 30ms] [F1 기록 → B: 30ms]
GPU:                      [F0 실행 A: 10ms] [F1 실행 B: 10ms]
                          ↑ CPU가 B 기록 중 GPU가 A 실행 (병렬)

프레임 시간 = max(CPU=30, GPU=10) = 30ms → 33fps (CPU 병목)
```

여러 Allocator는 **CPU-GPU 병렬**만 만들지, CPU 자체 속도는 그대로입니다. 여전히 30ms 상한.

CPU 시간 자체를 줄이려면 **여러 스레드**가 필요합니다:

```
4 스레드 + 여러 Allocator:
T1: [2,500 Draw: 7.5ms]
T2: [2,500 Draw: 7.5ms]  병렬
T3: [2,500 Draw: 7.5ms]
T4: [2,500 Draw: 7.5ms]

프레임 시간 = max(CPU=7.5, GPU=10) = 10ms → 100fps 가능
```

### 두 전략의 역할 구분

| 전략 | 해결하는 문제 | 필요한 객체 |
|------|-------------|-----------|
| **여러 Allocator** | GPU가 CPU를 기다리는 문제 (CPU-GPU 병렬) | `1 List + N Allocator` |
| **여러 스레드** | CPU 단일 스레드가 느린 문제 (CPU 내부 병렬) | `N List + N Allocator` |

실제 엔진은 **두 전략을 동시에 사용**합니다. 스레드별로 List를 두고, 각 스레드가 N개 Allocator를 돌려씁니다. 총 `Thread × Frame` 개의 Allocator + `Thread` 개의 List.

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

## 하드웨어 스레드와 실제 병렬 실행

"멀티스레드 기록"이 실제 성능 이득이 되려면 **여러 물리 CPU 코어에서 동시 실행**되어야 합니다. 이게 실제로 되는지, 어떤 메커니즘인지 다룹니다.

### 소프트웨어 스레드와 하드웨어 스레드

| | 정의 |
|---|------|
| **소프트웨어 스레드** | `std::thread` 등으로 만드는 OS 실행 단위 |
| **하드웨어 스레드** | 물리 CPU 코어 (+ SMT/HyperThreading으로 배수) |

소프트웨어 스레드를 만들기만 해서는 병렬이 안 됩니다. **OS 스케줄러가 각 소프트웨어 스레드를 서로 다른 CPU 코어에 할당**해야 진짜 병렬입니다.

현대 CPU:
- 일반 데스크탑: 6~16 코어 (+ SMT로 12~32 논리 코어)
- 콘솔 (PS5/Xbox Series X): 8 물리 코어 (16 논리)

```cpp
// 이렇게 스레드 만들면
std::thread t1([]{ commandList1->Draw(...); });
std::thread t2([]{ commandList2->Draw(...); });
std::thread t3([]{ commandList3->Draw(...); });
std::thread t4([]{ commandList4->Draw(...); });

// OS 스케줄러(Windows):
// - CPU 상태 확인 (각 코어가 바쁜가?)
// - 유휴 코어에 스레드 배치
// - 8코어 CPU면 4개 스레드가 서로 다른 4개 코어에서 동시 실행
```

개발자가 "어떤 코어에 배치할지"를 지시할 필요는 없지만, 최적화를 위해 **Thread Affinity**를 힌트 또는 강제로 줄 수는 있습니다.

#### Thread Affinity란?

**Thread Affinity(스레드 친화도)**: 특정 스레드를 **특정 CPU 코어에만 실행**되도록 제한하거나 선호하게 설정하는 기능.

- **Hard Affinity**: 강제 — 지정된 코어에서만 실행 (`SetThreadAffinityMask`)
- **Soft Affinity (Ideal Processor)**: 힌트 — 가능하면 이 코어에서 (`SetThreadIdealProcessor`)

```cpp
// 예: 렌더 스레드를 2번 코어에 고정
HANDLE renderThread = GetCurrentThread();
DWORD_PTR mask = (1ULL << 2);  // 비트 2 = 코어 2
SetThreadAffinityMask(renderThread, mask);

// 또는 힌트만 주기
SetThreadIdealProcessor(renderThread, 2);
```

#### 게임 엔진에서의 실제 활용

| 시나리오 | 활용 방법 |
|---------|---------|
| **하이브리드 CPU** (Intel 12세대+, ARM) | 렌더 스레드를 P-core에 고정, 백그라운드는 E-core |
| **렌더 스레드 지연 최소화** | 시스템 스레드와 분리된 코어에 고정 |
| **오디오 스레드** | 전용 코어에 고정 (glitch 방지) |
| **캐시 친화성** | 같은 L3/CCX를 공유하는 코어에 관련 스레드 배치 (AMD Ryzen) |
| **NUMA 시스템** | 같은 NUMA 노드의 코어에 작업 + 데이터 배치 |

#### 주의사항

- **OS 스케줄러를 방해할 수 있음**: 과도하게 affinity를 고정하면 부하 분산 저하
- **사용자 환경 다양성**: 하드코딩된 affinity는 CPU별로 다른 코어 구성(코어 수, 하이브리드 여부)에 취약
- **일반적 권장**: 대부분의 워커 스레드는 **OS에 맡기고**, 렌더 스레드/오디오 스레드 등 **특별한 요구사항**이 있는 소수 스레드만 affinity 설정
- **런타임 감지 필수**: `GetLogicalProcessorInformation` 등으로 CPU 구성 파악 후 동적 affinity 결정

#### 본 프로젝트에서의 활용 계획

Phase 5 멀티스레딩 단계에서:
- 렌더 스레드: 독립된 고성능 코어 (하이브리드 CPU의 P-core)
- 워커 스레드 풀: OS 자동 배치 (Affinity 미설정)
- 검증: [#44 하드웨어 스레드 활용 실측 실험](https://github.com/jiy12345/DX12GameEngine/issues/44) 에서 Affinity 유/무 비교

### Command List 기록이 병렬화에 이상적인 이유

모든 작업이 코어 수만큼 선형 스케일되진 않습니다. Command List 기록은 병렬화의 교과서 같은 케이스입니다:

| 병렬화 장애 요인 | 일반 작업 | Command List 기록 |
|---|---|---|
| I/O 대기 | 디스크/네트워크 때문에 코어가 놂 | **없음** (순수 CPU) |
| 공유 자원 경합 | 락으로 스레드 간 대기 | **거의 없음** (각 스레드 자기 List) |
| 의존성 체인 | 앞 단계가 끝나야 다음 시작 | **없음** (객체별 독립) |
| 메모리 경합 | 캐시 라인 공유 | **거의 없음** (별도 Allocator 메모리) |

그래서 **N배 코어 = N배에 가까운 속도 향상**(선형 스케일링)이 실제로 나옵니다.

DX12 설계 문서 (Microsoft 공식):

> "The primary motivation for Direct3D 12 is **to reduce the CPU overhead** of rendering and to **better utilize multi-core CPUs**."
>
> — [Microsoft: What is Direct3D 12?](https://learn.microsoft.com/en-us/windows/win32/direct3d12/direct3d-12-graphics)

"멀티코어 활용"은 DX12의 **설계 목적 그 자체**입니다.

### 실제 스케일링 사례

- **3DMark API Overhead Feature Test**: 단일 스레드 대비 4 스레드에서 약 6~8배 (DX11 대비 DX12 이득 포함)
- **AAA 게임 일반**: 8 코어에서 4 코어 대비 30~60% FPS 향상
- **Ashes of the Singularity (early DX12 타이틀)**: 4 스레드 활용 시 약 3.5배

### Amdahl의 법칙과 실무 상한

"그러면 32 코어 쓰면 32배 빠르겠네?" → **아닙니다.**

```
프레임 전체 = [직렬: 장면 준비] [병렬 가능: Draw 기록] [직렬: 제출/Present]
              2ms               30ms                    1ms
```

- 32 코어: `2 + (30/32) + 1 = 3.9ms` (이론상)
- 8 코어: `2 + (30/8) + 1 = 6.75ms`
- 4 코어: `2 + (30/4) + 1 = 10.5ms`

**직렬 부분(3ms)이 하한**을 정합니다. 일정 코어 수 이상에서는 추가 이득이 미미하고, 다음의 실무적 한계가 있습니다:

- 스레드 스케줄링 오버헤드 누적
- 메모리 대역폭 포화 (많은 코어가 동시 접근)
- CPU 캐시 경합 (False Sharing)

**보통 4~8 스레드가 실무 스윗스팟**입니다.

### 실측 검증

이론적 스케일링이 실제 하드웨어에서 제대로 나오는지 확인하려면 벤치마크가 필요합니다. 본 프로젝트의 관련 실험:

- [#44 하드웨어 스레드 활용 실측 실험](https://github.com/jiy12345/DX12GameEngine/issues/44) (예정)
  - N threads로 동일한 Draw call 개수를 분할 기록
  - 코어별 사용률 모니터링 (Task Manager, WPA, Intel VTune)
  - 스레드 수별 frame time 측정
  - 선형 스케일링 여부 검증

## 구현 패턴: 페어링 vs 풀

실제 엔진이 List와 Allocator를 관리하는 두 가지 대표 패턴입니다.

### 패턴 A: 페어링 (1:1 매핑)

```cpp
struct CommandPair {
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> list;
    UINT64 fenceValue;
};

std::vector<CommandPair> pairs;  // 미리 N개 생성
```

**장점**:
- 단순한 멘탈 모델 (1:1 관계)
- Fence 추적 용이 (페어 단위)
- 디버깅 쉬움 (페어에 이름 붙이기)

**단점**:
- List 상태가 N회분 중복 (메모리 낭비)
- 유연성 부족 (List가 다른 Allocator 전환 못 함)
- 규모 확장 시 `Thread × Frame × Pass` 페어 폭발

**적합한 경우**: 프로토타입, 학습, 단순 렌더러

### 패턴 B: 풀 기반 관리

```cpp
class CommandContext {
    std::vector<ComPtr<ID3D12CommandAllocator>> allocatorPool;
    std::vector<ComPtr<ID3D12GraphicsCommandList>> listPool;

    auto AcquireAllocator() -> ID3D12CommandAllocator*;
    auto AcquireList(ID3D12CommandAllocator* allocator) -> ID3D12GraphicsCommandList*;
    void Return(...);
};
```

**장점**:
- List를 적게 유지하고 재활용 (메모리 효율)
- 유연한 조합 (어떤 List + 어떤 Allocator)
- Bundle 재사용에 유리
- 규모 확장성 우수

**단점**:
- 구현 복잡 (수명 관리, 동시 사용 방지)
- 디버깅 난이도 ↑
- 초기 설계 비용

**적합한 경우**: 프로덕션 엔진, 멀티스레드 + 대규모 씬

### 선택 기준

| 상황 | 추천 |
|------|------|
| 프로토타입 / 학습 / 단순 렌더러 | 페어링 |
| 프로덕션 엔진 / 대규모 씬 | 풀 기반 |
| 멀티스레드 사용 안 함 | 페어링 충분 |
| 수십 스레드 + 복잡한 패스 | 풀 기반 필수 |

본 프로젝트는 Phase 1이니 페어링으로 시작하고, Phase 5(멀티스레딩)에서 풀 기반으로 리팩터링하는 전략이 합리적입니다.

## 주의사항

### Allocator 생성 실패 가능

Allocator는 **CPU 시스템 메모리를 실제로 할당**합니다. 시스템 메모리 부족 시 실패 가능:

```cpp
HRESULT hr = device->CreateCommandAllocator(...);
if (FAILED(hr)) {
    // E_OUTOFMEMORY 등 - 에러 처리 필요
}
```

### Reset 시점 주의

GPU가 아직 해당 Allocator의 명령을 읽고 있을 때 `Reset()` 호출 시:
- Debug Layer: 경고 + `E_FAIL`
- Release: **GPU 크래시 / 데이터 손상**

→ Fence로 GPU 완료를 반드시 확인 후 Reset.

### Allocator 내부 메모리 확장

기록 중 Allocator 공간 부족 시 드라이버가 내부적으로 페이지를 추가 할당합니다. 이 과정에서도 실패 가능:
- 증상: `Close()` 또는 이후 `Execute()`에서 HR 실패 또는 Device Removed
- 대응: 프레임당 Allocator 크기 모니터링, 과도한 명령 기록 방지

### Allocator 누수

프레임마다 새 Allocator를 만들고 해제하지 않으면 시스템 메모리가 누적 고갈됩니다. 반드시 재사용 패턴으로 관리하세요.

### 개별 List의 스레드 안전성

개별 Command List는 **스레드 안전하지 않습니다**. 하나의 List를 여러 스레드가 동시에 기록하면 안 됩니다. 병렬 기록은 반드시 **스레드별 독립 List**로 해야 합니다.

## 학습 순서

1. [CommandQueue.md](./CommandQueue.md) - 전체 철학과 큐의 역할
2. [CommandAllocator.md](./CommandAllocator.md) - 메모리 관리 기반
3. [CommandList.md](./CommandList.md) - 실제 명령 기록

본 개요 문서는 학습 중간에도 다시 돌아와서 전체 그림을 재확인하는 용도로 활용하세요.

## 참고 자료

- [Microsoft: Design philosophy of command queues and command lists](https://learn.microsoft.com/en-us/windows/win32/direct3d12/design-philosophy-of-command-queues-and-command-lists)
- [Microsoft: Multi-engine synchronization](https://learn.microsoft.com/en-us/windows/win32/direct3d12/user-mode-heap-synchronization)
- [Microsoft: Recording Command Lists and Bundles](https://learn.microsoft.com/en-us/windows/win32/direct3d12/recording-command-lists-and-bundles)
