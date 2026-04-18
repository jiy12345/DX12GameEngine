# Resource Barriers (리소스 배리어)

## 목차

- [개요](#개요)
- [왜 필요한가?](#왜-필요한가)
- [DX11 자동 추적 vs DX12 명시적 배리어](#dx11-자동-추적-vs-dx12-명시적-배리어)
  - [CPU 오버헤드](#cpu-오버헤드)
  - [배치 최적화](#배치-최적화)
  - [타이밍 제어 - Split Barrier](#타이밍-제어---split-barrier)
  - [멀티스레딩 친화성](#멀티스레딩-친화성)
  - [예측 가능성](#예측-가능성)
- [배리어의 종류](#배리어의-종류)
  - [Transition Barrier](#transition-barrier)
  - [UAV Barrier](#uav-barrier)
  - [Aliasing Barrier](#aliasing-barrier)
- [주요 리소스 상태](#주요-리소스-상태)
- [장단점 및 Trade-off](#장단점-및-trade-off)
- [DX12의 설계 철학과 활용 조건](#dx12의-설계-철학과-활용-조건)
- [코드 예제](#코드-예제)
- [주의사항](#주의사항)
- [관련 개념](#관련-개념)
- [참고 자료](#참고-자료)

## 개요

리소스 배리어(Resource Barrier)는 GPU 리소스의 **상태 전환**과 **메모리 접근 순서**를 명시적으로 지정하는 DX12의 동기화 메커니즘입니다.

### 배리어가 구체적으로 무엇을 하는가?

배리어 하나가 GPU에게 시키는 일은 크게 세 가지입니다:

1. **캐시 플러시 / 무효화 (Cache Flush/Invalidate)**
   - GPU 내부에는 용도별로 분리된 캐시들이 있음 (RT 캐시, 텍스처 L1/L2, UAV 쓰기 병합 버퍼 등)
   - 한 캐시에서 쓴 결과가 다른 캐시에서 보이려면 **명시적 플러시**가 필요
   - 예: RT로 쓴 데이터를 셰이더가 SRV로 읽으려면 RT 캐시를 플러시하고 텍스처 캐시를 무효화

2. **메모리 레이아웃 변환 (Layout Transition)**
   - 하드웨어는 용도에 따라 리소스를 다른 레이아웃으로 저장
     - RT: 타일 기반 압축 포맷 (예: NVIDIA DCC, AMD Delta Color Compression)
     - 텍스처: 스위즐링된 배치 (공간적 지역성 최적화)
     - UAV: 선형 배치
   - 용도가 바뀌면 **메모리 재배치/압축 해제**가 일어날 수 있음

3. **실행 순서 보장 (Execution Ordering)**
   - 이전 작업이 완료된 후에 다음 작업이 시작되도록 GPU 파이프라인에 "대기점" 삽입
   - 파이프라인 플러시 + 쓰기 완료 대기
   - UAV Barrier의 경우 같은 리소스에 대한 연속 쓰기/읽기의 순서만 보장 (레이아웃 변환 없음)

### 실제 시나리오로 보기

```cpp
// 1. 텍스처에 그림을 그림 (RT로 사용)
commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
commandList->DrawInstanced(...);  
// → 데이터는 RT 캐시에 + 타일 압축 포맷으로 저장됨

// 2. 배리어: "지금부터 이 텍스처를 셰이더에서 읽을 거야"
barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
commandList->ResourceBarrier(1, &barrier);
// → GPU가 내부적으로:
//    (a) RT 캐시 플러시 (쓰기 완료 대기)
//    (b) 텍스처 캐시 무효화 (기존 데이터 버리기)
//    (c) 타일 압축을 해제해 셰이더가 접근 가능한 포맷으로 재배치
//    (d) 이전 Draw 완료까지 대기

// 3. 이제 샘플링 가능
commandList->SetGraphicsRootDescriptorTable(0, srvHandle);
commandList->DrawInstanced(...);  // 위에서 그린 결과를 샘플링
```

### 배리어가 없으면?

- 셰이더가 **이전 Draw 결과가 완료되기 전에** 텍스처 샘플링 → race condition
- RT 캐시에 아직 남아있는 데이터 못 봄 → **구버전 데이터 읽기**
- 타일 압축 포맷을 해제 안 해서 **쓰레기 값 샘플링**
- 증상: 화면 깨짐, 프레임마다 다른 결과, TDR (GPU Timeout)

이 모든 위험을 DX11에서는 **드라이버가 자동으로** 막아줬습니다. DX12는 그 책임을 개발자에게 이관했습니다 (그 이유와 trade-off는 [DX11 vs DX12 섹션](#dx11-자동-추적-vs-dx12-명시적-배리어)에서 상세).

> Microsoft 공식 문서:
>
> "Resource barriers are used to manage resource transitions. The three main scenarios to consider are ... Transition barriers indicate that a set of subresources transition between different usages ... The caller must specify the before and after usages of the subresources."
>
> — [Microsoft: Using Resource Barriers to Synchronize Resource States](https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12)

## 왜 필요한가?

### GPU의 물리적 특성

GPU는 하드웨어 효율을 위해 리소스를 용도별로 최적화된 형태로 저장합니다:

- **렌더 타겟**: 타일 기반 압축 포맷, RT 전용 캐시
- **뎁스 버퍼**: HiZ 압축, 뎁스 전용 캐시
- **셰이더 리소스**: 텍스처 캐시 (L1/L2)
- **UAV**: 순서 없는 쓰기, 쓰기 병합 버퍼

같은 메모리라도 **어떤 용도로 접근하느냐에 따라** 다른 경로를 거칩니다. 용도를 바꿀 때는 이전 용도의 캐시를 플러시하고 새 용도에 맞게 재구성해야 합니다.

### DX11의 해결 방식: 암시적 추적

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

### DX12의 해결 방식: 명시적 배리어

DX12에서는 개발자가 직접 명시합니다:

```cpp
D3D12_RESOURCE_BARRIER barrier = {};
barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
barrier.Transition.pResource = texture;
barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

commandList->ResourceBarrier(1, &barrier);
```

## DX11 자동 추적 vs DX12 명시적 배리어

> **⚠️ 흔한 오해 정정**:
> "명시적 배리어가 없으면 GPU 작업이 직렬 실행된다"는 오해가 있을 수 있지만 **사실이 아닙니다.**
> - DX11에서도 **드라이버가 자동으로 배리어를 삽입**하므로 GPU 내부에서 적절히 동작합니다.
> - 차이점은 **"누가 배리어를 만드는가"** 입니다: DX11은 드라이버(자동), DX12는 개발자(수동).
> - DX12 명시적 배리어의 진짜 이점은 "배리어를 쓸 수 있는 것"이 아니라, **"언제/어떻게 배리어를 넣을지 최적화할 수 있는 것"** 입니다.
> - DX11 드라이버는 보수적이라 **과도하게** 배리어를 삽입하고 **매번 검증**을 반복합니다. DX12는 개발자가 최소한으로 조절 가능.

### CPU 오버헤드

**DX11 드라이버가 매 Draw마다 수행**:

```
Draw() 호출
  ↓
바인딩된 리소스 전부 순회
  ↓
각 리소스의 현재 상태 조회 (lookup)
  ↓
필요한 상태와 비교
  ↓
다르면 배리어 삽입 결정
  ↓
GPU 명령 생성
```

**DX12에서는**:

```
Draw() 호출 → 기록된 GPU 명령 실행. 끝.
```

매 프레임 수천 번 Draw를 호출하는 씬에서 이 차이가 CPU 오버헤드 제거의 핵심이 됩니다.

### 배치 최적화

리소스 배리어는 GPU에서 파이프라인 플러시 같은 동기화를 유발합니다. 여러 개를 합치면 플러시를 한 번에 처리할 수 있습니다.

**DX11 (자동 - 배치 불가)**:

```cpp
// 10개 텍스처를 RT → SRV로 전환해야 하는 상황
context->PSSetShaderResources(0, 1, &tex1SRV);  // 드라이버: "tex1 배리어 필요"
context->Draw(...);                              // 여기서 삽입

context->PSSetShaderResources(0, 1, &tex2SRV);  // 드라이버: "tex2 배리어 필요"
context->Draw(...);                              // 여기서 삽입
// ... 10번 반복, 플러시 10번
```

**DX12 (수동 - 배치 가능)**:

```cpp
// 10개를 한 번에 전환
D3D12_RESOURCE_BARRIER barriers[10] = { /* ... */ };
commandList->ResourceBarrier(10, barriers);  // 단 한 번의 호출 = 플러시 1회

for (auto& tex : textures) {
    commandList->DrawInstanced(...);  // 배리어 없이 연속 Draw
}
```

### 타이밍 제어 - Split Barrier

DX12는 **배리어를 분할**할 수 있습니다. `BEGIN_ONLY`로 전환을 시작하고, 실제 사용 직전에 `END_ONLY`로 완료합니다. 그 사이에 다른 작업을 수행하면 전환이 배경에서 병렬로 진행됩니다.

```cpp
// 전환을 지금 시작
D3D12_RESOURCE_BARRIER beginBarrier = { /* ... */ };
beginBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;
commandList->ResourceBarrier(1, &beginBarrier);

// ... 다른 작업 수행 (GPU가 전환을 병렬로 처리) ...

// 사용 직전에 전환 완료 확인
D3D12_RESOURCE_BARRIER endBarrier = { /* ... */ };
endBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
commandList->ResourceBarrier(1, &endBarrier);

commandList->DrawInstanced(...);  // 전환 완료 상태로 사용
```

> Microsoft 공식 문서:
>
> "Split barriers ... allow application to better overlap transitions with other GPU work."
>
> — [Microsoft: D3D12_RESOURCE_BARRIER_FLAGS](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_resource_barrier_flags)

DX11의 자동 추적은 "필요할 때"만 배리어를 삽입하므로 이런 숨은 병렬성을 활용할 수 없습니다.

### 멀티스레딩 친화성

**DX11 자동 추적의 근본적 문제**:

- 드라이버는 각 리소스의 "현재 상태" 테이블을 유지해야 함
- 여러 스레드가 같은 리소스에 접근하면 이 테이블에 **락**이 필요
- 결과: 멀티스레딩 확장성 저하

**DX12**:

- 각 Command List가 독립적으로 배리어를 기록
- 전역 상태 테이블 없음 → 락 없음
- 여러 스레드에서 진짜 병렬 기록 가능

### 예측 가능성

```cpp
// DX11: "왜 이 프레임만 느리지?"
// → 드라이버가 숨어서 추가 배리어를 삽입했을 수도 있음 (보이지 않음)

// DX12: 코드에 있는 배리어가 전부. 프로파일러에서 정확히 보임.
```

성능 프로파일링과 최적화가 훨씬 명확해집니다.

## 배리어의 종류

DX12는 세 가지 타입의 배리어를 제공합니다.

### Transition Barrier

가장 흔하게 사용되는 배리어. 리소스가 한 상태에서 다른 상태로 전환됨을 명시합니다.

```cpp
D3D12_RESOURCE_BARRIER barrier = {};
barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
barrier.Transition.pResource = texture;
barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

commandList->ResourceBarrier(1, &barrier);
```

**용도**: 렌더 타겟 → 셰이더 리소스, 버퍼 업로드 후 상수 버퍼로 전환 등

### UAV Barrier

같은 UAV에 대한 **쓰기 작업 간의 순서**를 보장합니다. 상태 전환은 아니지만, 이전 쓰기가 완료된 후 다음 접근이 일어남을 보장합니다.

```cpp
D3D12_RESOURCE_BARRIER barrier = {};
barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
barrier.UAV.pResource = uavResource;  // 또는 nullptr (모든 UAV에 적용)

commandList->ResourceBarrier(1, &barrier);
```

**용도**: Compute Shader가 UAV에 쓴 결과를 다음 Dispatch에서 읽을 때

### Aliasing Barrier

Placed Resource 또는 Reserved Resource가 **같은 물리 메모리를 공유**할 때, 한 리소스에서 다른 리소스로 활성 상태가 전환됨을 알립니다.

```cpp
D3D12_RESOURCE_BARRIER barrier = {};
barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
barrier.Aliasing.pResourceBefore = textureA;  // 이전에 쓰던 리소스
barrier.Aliasing.pResourceAfter = textureB;   // 지금부터 쓸 리소스

commandList->ResourceBarrier(1, &barrier);
```

**용도**: 메모리 재사용 최적화 (서로 다른 시점에 사용되는 리소스가 메모리 공유)

## 주요 리소스 상태

`D3D12_RESOURCE_STATES`의 대표적인 값들:

| 상태 | 용도 |
|------|------|
| `COMMON` | 초기 상태, CPU 접근 가능, Copy 큐 사용 가능 |
| `PRESENT` | 스왑체인 Present 직전 상태 |
| `RENDER_TARGET` | 렌더 타겟으로 쓰기 |
| `DEPTH_WRITE` | 뎁스 버퍼 쓰기 |
| `DEPTH_READ` | 뎁스 버퍼 읽기 (테스트만) |
| `PIXEL_SHADER_RESOURCE` | 픽셀 셰이더에서 텍스처 읽기 |
| `NON_PIXEL_SHADER_RESOURCE` | VS/GS/HS/DS/CS에서 텍스처 읽기 |
| `UNORDERED_ACCESS` | UAV 읽기/쓰기 |
| `COPY_SOURCE` | Copy 작업의 원본 |
| `COPY_DEST` | Copy 작업의 대상 |
| `VERTEX_AND_CONSTANT_BUFFER` | 버텍스/상수 버퍼로 사용 |
| `INDEX_BUFFER` | 인덱스 버퍼로 사용 |
| `GENERIC_READ` | 여러 읽기 상태의 합집합 (업로드 버퍼 기본값) |

> 상세 목록: [Microsoft: D3D12_RESOURCE_STATES](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_resource_states)

### 상태 결합

일부 읽기 상태는 `|` 연산자로 결합할 수 있습니다:

```cpp
// 픽셀 셰이더와 다른 셰이더 스테이지 모두에서 읽기
D3D12_RESOURCE_STATES state =
    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
```

쓰기 상태는 결합할 수 없습니다 (배타적).

## 장단점 및 Trade-off

### 자동 추적 (DX11)의 장단점

**👍 장점**
- 코드가 단순함
- 실수 위험 낮음 (드라이버가 처리)
- 학습 곡선 완만

**👎 단점**
- CPU 오버헤드 큼 (매 Draw 검증)
- 과도하게 보수적인 배리어
- 배치 불가, 플러시 반복
- 멀티스레딩 불리 (전역 상태 락)
- 성능 예측 불가 (드라이버마다 다름)
- 타이밍 제어 불가 (Split Barrier 없음)

### 명시적 배리어 (DX12)의 장단점

**👍 장점**
- CPU 오버헤드 최소
- 배치 최적화 가능
- Split Barrier로 병렬성 활용
- 멀티스레딩 자유
- 예측 가능한 성능
- 불필요한 배리어 제거 가능

**👎 단점**
- 코드량 급증
- GPU 크래시 위험 (배리어 누락 시)
- 학습 곡선 가파름
- 디버깅 어려움 (간헐적 버그)
- 리소스 상태 직접 관리 부담
- Debug Layer 의존적

## DX12의 설계 철학과 활용 조건

DX12의 명시적 배리어는 단독으로 존재하는 기능이 아니라, **"드라이버 추측 → 개발자 명시"** 라는 DX12 전반의 철학을 공유합니다. 같은 철학이 [PSO](../Pipeline/PipelineStateObject.md)(파이프라인 상태 사전 컴파일), [Root Signature](../Pipeline/RootSignature.md) 등에도 적용됩니다.

### 기회 제공형 API

```
DX11: 드라이버가 알아서 평균 70점 성능 보장
DX12: 잘 쓰면 95점, 못 쓰면 50점
```

- **바닥값**은 DX11이 더 높음 (드라이버의 방어막)
- **천장값**은 DX12가 훨씬 높음 (병렬성/제어의 자유)
- 그 차이를 메우는 것은 **엔진 개발자의 역량**

### DX12의 장점이 나오는 조건

단순 포팅만으로는 DX11보다 느려지는 사례가 많습니다. 다음 중 하나 이상을 실제로 활용해야 이득이 납니다:

| 활용 | 필요한 이해 |
|------|------------|
| 멀티스레드 커맨드 기록 | 스레드별 CommandList 분배, 동기화 |
| 비동기 컴퓨트 | Direct/Compute 큐 병렬 실행, Fence |
| **배리어 최적화** | **배치, Split Barrier, 최소 전환** |
| PSO 사전 생성 | 런타임 셰이더 컴파일 회피 |
| Bindless / Descriptor 관리 | 힙 재사용, Dynamic Indexing |

하나도 활용하지 않으면 DX11과 비슷하거나 더 나쁜 성능이 나옵니다.

### 실무 대응

대부분의 엔진(Unreal, Unity HDRP 등)은 자체 **State Tracker**를 구현합니다:

- 리소스별 현재 상태를 엔진이 자동 추적
- 배치/최적화는 엔진이 제어
- 개발자는 DX11처럼 편하게 쓰되, 성능은 DX12의 이득을 얻음
- 일종의 **하이브리드 방식**

본 프로젝트도 Phase 2 이후 State Tracker 도입이 고려됩니다 ([#21](https://github.com/jiy12345/DX12GameEngine/issues/21) 참조).

## 코드 예제

### 기본 Transition Barrier

```cpp
// 스왑체인 백버퍼: PRESENT → RENDER_TARGET
D3D12_RESOURCE_BARRIER barrier = {};
barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
barrier.Transition.pResource = backBuffer;
barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

commandList->ResourceBarrier(1, &barrier);
```

### CD3DX12 헬퍼 사용

`d3dx12.h` 헬퍼를 사용하면 더 간결합니다:

```cpp
// 동일한 전환을 한 줄로
auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
    backBuffer,
    D3D12_RESOURCE_STATE_PRESENT,
    D3D12_RESOURCE_STATE_RENDER_TARGET
);
commandList->ResourceBarrier(1, &barrier);
```

### 배치

```cpp
// 여러 리소스를 한 번에 전환
std::vector<D3D12_RESOURCE_BARRIER> barriers;

for (auto& texture : textures) {
    barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
        texture,
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    ));
}

// 한 번의 호출로 모두 처리
commandList->ResourceBarrier(
    static_cast<UINT>(barriers.size()),
    barriers.data()
);
```

### UAV Barrier

```cpp
// Compute Shader 결과를 다음 Dispatch에서 읽기 전에 동기화
commandList->Dispatch(groupX, groupY, 1);

auto uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(uavResource);
commandList->ResourceBarrier(1, &uavBarrier);

commandList->Dispatch(groupX, groupY, 1);  // 이전 결과 안전하게 사용
```

### Split Barrier

```cpp
// 전환을 미리 시작
auto beginBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
    texture,
    D3D12_RESOURCE_STATE_RENDER_TARGET,
    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
    D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
    D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY
);
commandList->ResourceBarrier(1, &beginBarrier);

// ... 다른 GPU 작업 수행 (배리어가 백그라운드로 처리됨) ...

// 사용 직전 전환 완료
auto endBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
    texture,
    D3D12_RESOURCE_STATE_RENDER_TARGET,
    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
    D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
    D3D12_RESOURCE_BARRIER_FLAG_END_ONLY
);
commandList->ResourceBarrier(1, &endBarrier);

// 이제 셰이더에서 안전하게 사용
commandList->SetGraphicsRootDescriptorTable(0, textureSrvHandle);
commandList->DrawInstanced(...);
```

## 주의사항

### 배리어 누락은 치명적

배리어를 빠뜨리면 **런타임에 즉시 크래시하지 않고** 다음 증상이 나타날 수 있습니다:

- 화면에 이전 프레임 데이터가 보임 (캐시 미플러시)
- 간헐적 화면 깨짐 (특정 GPU에서만)
- TDR (Timeout Detection Recovery) 발생
- 디버깅 불가능한 미묘한 버그

**반드시 개발 중에는 Debug Layer를 활성화**하여 배리어 검증 오류를 잡아야 합니다.

### StateBefore는 정확해야 함

`StateBefore`와 실제 현재 상태가 다르면 Debug Layer가 오류를 발생시킵니다:

```cpp
// 실제 현재 상태가 COPY_DEST인데...
barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;  // 잘못됨
// → Debug Layer: "Expected state COPY_DEST but StateBefore is RENDER_TARGET"
```

엔진 수준에서 State Tracker로 관리하는 이유 중 하나입니다.

### 서브리소스 단위 전환

텍스처의 개별 밉 레벨, 배열 슬라이스는 서브리소스로 취급됩니다. 부분 전환이 가능합니다:

```cpp
// 밉 레벨 0만 전환
barrier.Transition.Subresource = 0;

// 모든 서브리소스 전환
barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
```

### 불필요한 배리어는 GPU 스톨

"혹시 모르니" 넣은 배리어는 GPU 파이프라인을 멈추게 합니다. 실제로 필요한 경우에만 삽입하세요.

### Fence와는 다른 역할

| | Resource Barrier | Fence |
|---|---|---|
| 역할 | GPU 내부 메모리/캐시 동기화 | CPU-GPU 또는 큐 간 동기화 |
| 범위 | 같은 Command List 내 | 커맨드 큐 또는 프레임 단위 |
| 예시 | RT → SRV 전환 | 이전 프레임 완료 대기 |

자세한 내용은 [Synchronization](../Synchronization/Synchronization.md) 참조.

### 큐 타입별 허용 상태

Copy 큐는 대부분의 상태를 지원하지 않습니다. `D3D12_RESOURCE_STATE_COMMON`을 거쳐야 하는 경우가 많습니다.

## 관련 개념

### 선행 개념 (먼저 이해해야 할 것)

- [CommandList](../Commands/CommandList.md) - 배리어가 기록되는 대상
- [CommandQueue](../Commands/CommandQueue.md) - DX12의 명시적 제어 철학
- [MemoryHeaps](./MemoryHeaps.md) - 리소스 배치 위치

### 연관 개념 (함께 사용되는 것)

- [PipelineStateObject](../Pipeline/PipelineStateObject.md) - 같은 "명시적 사전 지정" 철학
- [Synchronization](../Synchronization/Synchronization.md) - Fence와 배리어의 역할 구분

### 후속 개념 (이후 학습할 것)

- AsyncCompute - 큐 간 리소스 상태 공유 **(미작성)** ([#42](https://github.com/jiy12345/DX12GameEngine/issues/42))

## 참고 자료

- [Microsoft: Using Resource Barriers to Synchronize Resource States in Direct3D 12](https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12)
- [Microsoft: D3D12_RESOURCE_BARRIER structure](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_resource_barrier)
- [Microsoft: D3D12_RESOURCE_STATES enumeration](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_resource_states)
- [Microsoft: D3D12_RESOURCE_BARRIER_FLAGS enumeration](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_resource_barrier_flags)
- [Microsoft: ID3D12GraphicsCommandList::ResourceBarrier method](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-resourcebarrier)
- [Microsoft: Design philosophy of command queues and command lists](https://learn.microsoft.com/en-us/windows/win32/direct3d12/design-philosophy-of-command-queues-and-command-lists)
