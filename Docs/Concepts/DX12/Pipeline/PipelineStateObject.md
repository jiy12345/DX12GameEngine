# Pipeline State Object (PSO)

## 목차

- [개요](#개요)
- [왜 필요한가?](#왜-필요한가)
  - [DX11의 문제: 개별 상태 설정](#dx11의-문제-개별-상태-설정)
  - [드라이버의 런타임 조합 검증 오버헤드](#드라이버의-런타임-조합-검증-오버헤드)
- [DX12의 해결책: 사전 컴파일된 파이프라인 상태](#dx12의-해결책-사전-컴파일된-파이프라인-상태)
- ["컴파일 타임"이 아니라 "초기화 타임"](#컴파일-타임이-아니라-초기화-타임)
- [PSO의 구성 요소](#pso의-구성-요소)
  - [Graphics PSO](#graphics-pso)
  - [Compute PSO](#compute-pso)
- [장점](#장점)
- [단점과 Trade-off](#단점과-trade-off)
  - [조합 폭발](#조합-폭발)
  - [런타임 스터터](#런타임-스터터)
  - [메모리 비용](#메모리-비용)
  - [유연성 상실](#유연성-상실)
  - [예측 부담](#예측-부담)
- [실무 대응 전략](#실무-대응-전략)
- [DX12의 설계 철학과 연관성](#dx12의-설계-철학과-연관성)
- [코드 예제](#코드-예제)
- [주의사항](#주의사항)
- [관련 개념](#관련-개념)
- [참고 자료](#참고-자료)

## 개요

Pipeline State Object (PSO)는 그래픽스 또는 컴퓨트 파이프라인의 **모든 상태를 하나로 묶은 불변 객체**입니다. 셰이더, 블렌드 상태, 래스터라이저 상태, 뎁스/스텐실 상태, 입력 레이아웃, 렌더 타겟 포맷 등을 한 번에 정의하고 GPU가 바로 사용할 수 있는 형태로 사전 컴파일합니다.

> Microsoft 공식 문서:
>
> "PSOs in Direct3D 12 were designed to allow the GPU to pre-process all of the dependent settings in each pipeline state, typically during initialization, to make switching between states at render time as efficient as possible."
>
> — [Microsoft: Managing Graphics Pipeline State in Direct3D 12](https://learn.microsoft.com/en-us/windows/win32/direct3d12/managing-graphics-pipeline-state-in-direct3d-12)

## 왜 필요한가?

### DX11의 문제: 개별 상태 설정

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
```

### 드라이버의 런타임 조합 검증 오버헤드

위 코드의 문제는 `Draw()` 호출 순간에 드라이버가 "이 조합이 유효한가? 어떻게 GPU 명령으로 변환할까?" 를 런타임에 계산해야 한다는 점입니다.

> Microsoft 공식 문서:
>
> "With today's graphics hardware, there are dependencies between the different hardware units. For example, the hardware blend state might have dependencies on the raster state as well as the blend state."
>
> — [Microsoft: Managing Graphics Pipeline State in Direct3D 12](https://learn.microsoft.com/en-us/windows/win32/direct3d12/managing-graphics-pipeline-state-in-direct3d-12)

즉 상태들이 서로 독립적이지 않아서, 어떤 조합을 실제 GPU 명령으로 변환하려면 **상호 의존성을 전부 고려**해야 하고 이게 런타임에 발생하면 오버헤드가 큽니다.

## DX12의 해결책: 사전 컴파일된 파이프라인 상태

DX12는 이 모든 상태를 **초기화 시점에 미리** 하나의 객체로 묶어둡니다:

```cpp
D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
psoDesc.pRootSignature = rootSignature;
psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
psoDesc.NumRenderTargets = 1;
psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
psoDesc.SampleDesc.Count = 1;

// 초기화 시 한 번 생성 (여기서 검증 및 컴파일 완료)
device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));

// 렌더링 시에는 단순히 전환만
commandList->SetPipelineState(pipelineState);
commandList->DrawInstanced(3, 1, 0, 0);
// ↑ 드라이버가 할 일이 거의 없음
```

Draw 시점의 드라이버 오버헤드가 사실상 사라집니다.

## "컴파일 타임"이 아니라 "초기화 타임"

PSO에 대한 흔한 오해: **"PSO는 컴파일 타임에 만들어진다"** 는 잘못된 이해입니다.

정확히는:

- PSO는 **런타임에** `CreateGraphicsPipelineState()` 호출 시점에 컴파일됨
- "사전 컴파일" 이라는 말은 **Draw 호출 전에** 컴파일되어 있다는 의미
- 실행 파일(바이너리)에 박혀 있는 것은 아님

```
빌드 타임: 셰이더 바이트코드 컴파일 (HLSL → DXBC/DXIL)
              ↓
초기화 타임: PSO 생성 (바이트코드 + 상태 → GPU ISA 컴파일)  ← 여기
              ↓
렌더 타임:    SetPipelineState()로 전환만 (매우 가벼움)
```

이 구분이 중요한 이유는 뒤의 "[런타임 스터터](#런타임-스터터)" 문제와 직접 연결되기 때문입니다.

## PSO의 구성 요소

DX12는 두 종류의 PSO를 제공합니다.

### Graphics PSO

`D3D12_GRAPHICS_PIPELINE_STATE_DESC`에 포함되는 상태:

| 구성 요소 | 설명 |
|----------|------|
| Root Signature | 셰이더가 접근할 리소스 레이아웃 |
| Vertex Shader (VS) | 정점 처리 |
| Pixel Shader (PS) | 픽셀 처리 |
| Hull / Domain Shader | 테셀레이션 (선택) |
| Geometry Shader | 지오메트리 처리 (선택) |
| Stream Output | 스트림 출력 설정 (선택) |
| Blend State | 블렌딩 모드 (Opaque/Alpha/Additive 등) |
| Sample Mask | 멀티샘플 마스크 |
| Rasterizer State | 컬링, 와이어프레임, 뎁스 바이어스 등 |
| Depth Stencil State | 뎁스 테스트, 스텐실 테스트 |
| Input Layout | 정점 입력 포맷 |
| Index Buffer Strip Cut | 트라이앵글 스트립 절단 |
| Primitive Topology Type | Triangle / Line / Point |
| Num Render Targets | 렌더 타겟 개수 |
| RTV Formats | 각 렌더 타겟 포맷 |
| DSV Format | 뎁스 스텐실 포맷 |
| Sample Desc | MSAA 설정 |
| Node Mask | 멀티 GPU 설정 |
| Cached PSO | 캐시된 PSO (선택) |
| Flags | 디버그 플래그 등 |

### Compute PSO

컴퓨트 파이프라인은 훨씬 단순합니다:

```cpp
D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
psoDesc.pRootSignature = rootSignature;
psoDesc.CS = { csBlob->GetBufferPointer(), csBlob->GetBufferSize() };

device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
```

래스터라이저, 블렌드 같은 그래픽스 상태가 없으므로 조합 폭발 문제가 덜합니다.

## 장점

### 1. Draw 시점 CPU 오버헤드 제거

드라이버가 런타임에 할 일이 거의 없습니다. `SetPipelineState()`는 사실상 "포인터 교체" 수준의 작업입니다.

### 2. GPU 상태 전환 효율

상태 간 의존성을 사전에 해결해두었기 때문에, GPU가 상태 전환 시 내부 파이프라인을 덜 건드립니다.

### 3. 예측 가능한 성능

DX11처럼 "어떤 조합을 만나면 드라이버가 갑자기 느려질까?" 같은 불확실성이 없습니다. 한번 만든 PSO의 성능 특성은 일정합니다.

### 4. 검증을 초기화 시점에 수행

"잘못된 상태 조합"은 PSO 생성 시점에 오류로 드러납니다. 런타임에 느려지거나 이상 동작하는 것보다 훨씬 발견이 쉽습니다.

## 단점과 Trade-off

### 조합 폭발

PSO는 다음 **모든 조합**에 대해 별도 인스턴스가 필요합니다:

| 축 | 예시 | 가짓수 |
|---|------|-------|
| 셰이더 순열 | 스키닝/정적, 노말맵 유무, 라이트 수 등 | 수백 ~ 수천 |
| 블렌드 상태 | Opaque, Additive, Alpha, Multiply ... | ~10 |
| 래스터라이저 | CullMode × FillMode × DepthBias | ~10 |
| 뎁스/스텐실 상태 | 읽기/쓰기 조합 | ~10 |
| RT 포맷 | RGBA8, RGBA16F, 개수 | ~10 |

곱하면 **수만~수십만 개의 PSO**가 이론적으로 가능합니다. 실제 AAA 게임은 5천~5만 개 범위의 PSO를 사용합니다.

### 런타임 스터터

**현대 PC 게임의 악명 높은 문제**입니다:

```
게임 진행 중 새 무기를 처음 장착
  ↓
해당 머티리얼 PSO가 아직 없음
  ↓
CreateGraphicsPipelineState() 호출 (수십~수백 ms 소요)
  ↓
그 프레임이 프리징 → 스터터
```

DX11에서는 드라이버가 백그라운드에서 처리해서 덜 눈에 띄었지만, DX12는 엔진이 명시적으로 해야 하므로 스터터가 그대로 노출됩니다. Elden Ring, Callisto Protocol, Star Wars Jedi: Survivor 등이 모두 이 문제로 비판받은 바 있습니다.

### 메모리 비용

각 PSO는 컴파일된 GPU 코드와 상태 정보를 보유합니다:

- 개당 수 KB ~ 수백 KB
- 수만 개 단위가 되면 VRAM을 무시할 수 없는 수준으로 소비

### 유연성 상실

DX11에서는 개별 상태를 독립적으로 바꿀 수 있었습니다:

```cpp
// DX11: 블렌드만 바꾸기 간단
context->OMSetBlendState(additiveBlend, ...);
```

DX12에서는 블렌드 하나만 달라도 **완전히 다른 PSO**가 필요합니다:

```cpp
// DX12: 두 PSO를 미리 만들어두거나, 필요할 때 만들거나
commandList->SetPipelineState(opaquePSO);
commandList->SetPipelineState(additivePSO);
// 중간에 "블렌드만 살짝 바꾼다"는 선택지 없음
```

### 예측 부담

개발자가 **모든 필요한 조합을 미리 알고 있어야** 합니다:

- 게임플레이 중 동적으로 생성되는 이펙트
- 모드/DLC로 추가되는 머티리얼
- 플레이어가 커스터마이즈하는 장비

예측 실패 → 런타임 PSO 생성 → 스터터.

## 실무 대응 전략

| 전략 | 설명 | 한계 |
|------|------|------|
| **PSO 캐시 직렬화** | `ID3D12PipelineState::GetCachedBlob()` 으로 컴파일 결과를 디스크에 저장, 다음 실행 시 재사용 | 드라이버/GPU 바뀌면 무효화됨 |
| **로딩 화면 프리워밍** | 게임 시작 시 필요한 PSO 목록을 미리 컴파일 | 로딩 시간 증가, 목록 관리 부담 |
| **백그라운드 스레드 컴파일** | 게임 실행 중 별도 스레드에서 PSO를 미리 준비 | 컴파일 완료 전에는 사용 불가 → 대체 PSO 필요 |
| **PSO 로깅 후 재배포** | 플레이어 플레이 데이터로 실제 사용 PSO 목록을 수집하여 패치에 포함 | UE5 방식. 초기 플레이어는 스터터 경험 |
| **Shader Permutation 축소** | 조합 수를 의도적으로 제한 (예: 라이트 수 상한, 머티리얼 타입 통합) | 비주얼 제약, 확장성 저하 |
| **Fallback PSO** | 원하는 PSO가 아직 없으면 단순한 PSO로 대체 렌더링 | 한 프레임 비주얼 품질 저하 |

실제 엔진은 보통 **여러 전략을 조합**하여 사용합니다.

## DX12의 설계 철학과 연관성

PSO는 단독으로 존재하는 기능이 아니라, **"드라이버 추측 → 개발자 명시"** 라는 DX12 전반의 철학을 공유합니다. 같은 철학이 [Resource Barriers](../Resources/ResourceBarriers.md)(리소스 상태 명시), [Root Signature](./RootSignature.md) 등에도 적용됩니다.

### 기회 제공형 API

```
DX11: 드라이버가 알아서 평균 70점 성능 보장
DX12: 잘 쓰면 95점, 못 쓰면 50점
```

- **바닥값**은 DX11이 더 높음 (드라이버의 방어막)
- **천장값**은 DX12가 훨씬 높음 (사전 컴파일, 검증 제거)
- 그 차이를 메우는 것은 **엔진 개발자의 역량**

### PSO에 투자한 만큼 이득

- PSO 관리 시스템, 캐시, 프리워밍 전략을 제대로 구축하면 → Draw 피크 성능 확보
- 단순 포팅만 하면 → 런타임 스터터로 체감 성능이 DX11보다 나쁠 수 있음

## 코드 예제

### Graphics PSO 생성

```cpp
D3D12_INPUT_ELEMENT_DESC inputElements[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
      D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
      D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};

D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
psoDesc.InputLayout = { inputElements, _countof(inputElements) };
psoDesc.pRootSignature = rootSignature;
psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
psoDesc.DepthStencilState.DepthEnable = FALSE;
psoDesc.DepthStencilState.StencilEnable = FALSE;
psoDesc.SampleMask = UINT_MAX;
psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
psoDesc.NumRenderTargets = 1;
psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
psoDesc.SampleDesc.Count = 1;

ID3D12PipelineState* pipelineState;
HRESULT hr = device->CreateGraphicsPipelineState(
    &psoDesc, IID_PPV_ARGS(&pipelineState));
```

### PSO 사용

```cpp
// Command List에 PSO 바인딩
commandList->SetPipelineState(pipelineState);
commandList->SetGraphicsRootSignature(rootSignature);

// 버텍스/인덱스 버퍼 바인딩 후 Draw
commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
commandList->DrawInstanced(3, 1, 0, 0);
```

### PSO 캐시 직렬화

```cpp
// 1. PSO 생성 후 캐시 저장
ID3DBlob* cachedBlob = nullptr;
pipelineState->GetCachedBlob(&cachedBlob);
// → cachedBlob 내용을 파일로 저장 (다음 실행 시 재사용)

// 2. 다음 실행에서 캐시 재사용
psoDesc.CachedPSO.pCachedBlob = savedBlob->GetBufferPointer();
psoDesc.CachedPSO.CachedBlobSizeInBytes = savedBlob->GetBufferSize();

// 캐시가 유효하면 빠르게 생성됨. 무효하면 D3D12_ERROR_ADAPTER_NOT_FOUND
// 또는 D3D12_ERROR_DRIVER_VERSION_MISMATCH 반환
HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
if (FAILED(hr)) {
    // 캐시 무효 → 캐시 없이 재시도
    psoDesc.CachedPSO = {};
    device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
}
```

### Compute PSO 생성

```cpp
D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
psoDesc.pRootSignature = rootSignature;
psoDesc.CS = { csBlob->GetBufferPointer(), csBlob->GetBufferSize() };

device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&computePSO));
```

## 주의사항

### 상태 간 호환성은 엔진이 관리

PSO와 Command List의 현재 상태가 호환되지 않으면 Debug Layer가 경고합니다:

- PSO의 RTV Format과 실제 바인딩된 RTV의 Format이 다름
- PSO의 Sample Count와 RTV의 Sample Count가 다름
- PSO의 Primitive Topology Type과 `IASetPrimitiveTopology()`가 호환되지 않음

이런 실수를 방지하려면 엔진 수준에서 **Material → PSO 매핑 시스템**을 구축합니다.

### Root Signature는 PSO에 포함되지만 별도 바인딩 필요

```cpp
commandList->SetPipelineState(pso);           // PSO 설정
commandList->SetGraphicsRootSignature(rs);    // Root Signature도 별도 호출
```

같은 Root Signature를 공유하는 PSO들은 서로 호환되므로 Root Signature 전환 없이 PSO만 바꿀 수 있습니다.

### PSO 생성은 비싸다

실시간 루프에서 PSO를 생성하지 마세요. 반드시 로딩 단계 또는 백그라운드 스레드에서 수행합니다.

### Debug Layer를 켜두자

잘못된 PSO 사용은 런타임에 이상 동작으로 나타나지만 즉시 크래시하진 않는 경우가 많습니다. Debug Layer 경고를 놓치지 마세요.

## 관련 개념

### 선행 개념 (먼저 이해해야 할 것)

- [Device](../Core/Device.md) - PSO를 생성하는 팩토리 **(미작성)**
- [CommandList](../Commands/CommandList.md) - PSO가 바인딩되는 대상
- [CommandQueue](../Commands/CommandQueue.md) - DX12의 명시적 제어 철학

### 연관 개념 (함께 사용되는 것)

- [Root Signature](./RootSignature.md) - 셰이더 리소스 레이아웃 **(미작성)** ([#14](https://github.com/jiy12345/DX12GameEngine/issues/14))
- [ResourceBarriers](../Resources/ResourceBarriers.md) - 같은 "명시적 사전 지정" 철학

### 후속 개념 (이후 학습할 것)

- PSO 캐시 전략 및 프리워밍 **(미작성)**
- Shader Permutation 관리 **(미작성)**

## 참고 자료

- [Microsoft: Managing Graphics Pipeline State in Direct3D 12](https://learn.microsoft.com/en-us/windows/win32/direct3d12/managing-graphics-pipeline-state-in-direct3d-12)
- [Microsoft: D3D12_GRAPHICS_PIPELINE_STATE_DESC structure](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_graphics_pipeline_state_desc)
- [Microsoft: D3D12_COMPUTE_PIPELINE_STATE_DESC structure](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_compute_pipeline_state_desc)
- [Microsoft: ID3D12Device::CreateGraphicsPipelineState method](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device-creategraphicspipelinestate)
- [Microsoft: PSO Caching](https://learn.microsoft.com/en-us/windows/win32/direct3d12/cached-pso)
