# 그래픽스 파이프라인 (Graphics Pipeline)

## 개요
GPU가 3D 장면을 2D 픽셀로 변환하는 일련의 처리 단계이며, DX12에서는 이 상태 전체를 Pipeline State Object(PSO) 하나로 묶어 관리한다.

## 왜 필요한가?

### 문제
DX11에서는 셰이더, 블렌드 상태, 래스터라이저 상태 등을 런타임에 개별적으로 설정·검증했기 때문에 드라이버 오버헤드가 컸다.

### 해결책
DX12는 모든 파이프라인 상태를 **PSO(Pipeline State Object)** 하나로 사전 컴파일한다. 드로우 콜 직전에 검증이 필요 없어 CPU 오버헤드가 줄어든다.

## 파이프라인 단계

```
Vertex Buffer
     ↓
[Input Assembler]   ← 입력 레이아웃 (IA)
     ↓
[Vertex Shader]     ← VS: 정점 위치·색상 변환
     ↓
[Rasterizer]        ← 삼각형 → 픽셀 변환
     ↓
[Pixel Shader]      ← PS: 픽셀 색상 결정
     ↓
[Output Merger]     ← 렌더 타겟에 최종 출력 (OM)
```

> ⚠️ Geometry Shader, Hull/Domain Shader, Tessellation 등은 선택적 단계로 이 프로젝트 Phase 1에서는 사용하지 않는다.

## Root Signature

### 개념
Root Signature는 셰이더가 접근할 리소스(CBV, SRV, UAV, Sampler)의 레이아웃을 정의한다. C++ 함수 시그니처와 유사하게 "셰이더의 입력 인자 목록"이라 이해할 수 있다.

Root Signature는 PSO와 별도로 생성되며, 드로우 콜 전에 `SetGraphicsRootSignature()`로 바인딩한다.

### 구성 요소
- **Root Constants**: 셰이더 상수를 직접 설정 (가장 빠름, 최대 64 DWORD)
- **Root Descriptors**: CBV/SRV/UAV 하나를 GPU 가상 주소로 직접 지정
- **Descriptor Tables**: 디스크립터 힙의 범위를 참조

### Phase 1 (삼각형): 빈 Root Signature
삼각형 렌더링에는 외부 리소스(텍스처, 상수 버퍼 등)가 없으므로 파라미터 없는 빈 Root Signature를 사용한다.

```cpp
D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
rootSigDesc.NumParameters = 0;
rootSigDesc.pParameters   = nullptr;
rootSigDesc.Flags         = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

ComPtr<ID3DBlob> serialized;
D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serialized, nullptr);
device->CreateRootSignature(0, serialized->GetBufferPointer(),
                            serialized->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
```

> `D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT` 플래그는 Input Assembler 단계를 활성화한다. Vertex Buffer를 사용하려면 반드시 설정해야 한다.

## Pipeline State Object (PSO)

### 개념
PSO는 다음 상태를 하나의 불변(immutable) 객체로 묶는다.

| 구성 요소 | 설명 |
|---|---|
| Root Signature | 셰이더 리소스 레이아웃 |
| Vertex Shader | 정점 처리 |
| Pixel Shader | 픽셀 색상 결정 |
| Input Layout | 정점 버퍼 포맷 |
| Blend State | 알파 블렌딩 |
| Rasterizer State | 컬링, 와이어프레임 등 |
| Depth Stencil State | 깊이/스텐실 테스트 |
| Render Target Format | 출력 포맷 |
| Primitive Topology Type | 삼각형/점/선 등 |

### Phase 1 (삼각형): PSO 생성 예시

```cpp
D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
psoDesc.pRootSignature        = rootSignature.Get();
psoDesc.VS                    = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
psoDesc.PS                    = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
psoDesc.InputLayout           = { inputLayout, _countof(inputLayout) };
psoDesc.BlendState            = /* default */;
psoDesc.RasterizerState       = /* default, CULL_BACK */;
psoDesc.DepthStencilState.DepthEnable   = FALSE;  // Phase 1: 깊이 버퍼 없음
psoDesc.DepthStencilState.StencilEnable = FALSE;
psoDesc.SampleMask            = UINT_MAX;
psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
psoDesc.NumRenderTargets      = 1;
psoDesc.RTVFormats[0]         = DXGI_FORMAT_R8G8B8A8_UNORM;
psoDesc.SampleDesc.Count      = 1;

device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
```

> PSO 생성은 셰이더 컴파일과 파이프라인 검증을 포함하므로 비용이 크다. 초기화 시 한 번만 생성하고 재사용한다.

## 드로우 콜 순서

```cpp
// 1. Root Signature 바인딩
commandList->SetGraphicsRootSignature(rootSignature.Get());

// 2. PSO 설정
commandList->SetPipelineState(pipelineState.Get());

// 3. Input Assembler 설정
commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

// 4. 드로우
commandList->DrawInstanced(vertexCount, instanceCount, startVertex, startInstance);
```

## 주의사항
- ⚠️ PSO와 Root Signature는 반드시 일치해야 한다. PSO 생성 시 Root Signature를 넘기고, 드로우 콜 전에도 동일한 Root Signature를 바인딩해야 한다.
- ⚠️ PSO 생성 비용이 크므로 모든 PSO를 초기화 시점에 미리 생성한다.
- ⚠️ `RTVFormats[0]`은 실제 렌더 타겟 포맷과 일치해야 한다.
- ⚠️ `PrimitiveTopologyType`(PSO)과 `IASetPrimitiveTopology()`(커맨드 리스트)는 호환 가능해야 한다. `TYPE_TRIANGLE`은 `TRIANGLELIST`, `TRIANGLESTRIP` 모두 허용.

## 관련 개념

### 선행 개념
- [커맨드 큐 & 커맨드 리스트](../../DX12/CommandQueue.md)
- [셰이더](./Shaders.md)
- [정점 처리](./VertexProcessing.md)

### 연관 개념
- [리소스 바인딩](./ResourceBinding.md)
- [래스터화](./Rasterization.md)

## 참고 자료
- [Microsoft: Introduction to a pipeline state in Direct3D 12](https://learn.microsoft.com/en-us/windows/win32/direct3d12/managing-graphics-pipeline-state-in-direct3d-12)
- [Microsoft: Creating a basic Direct3D 12 component](https://learn.microsoft.com/en-us/windows/win32/direct3d12/creating-a-basic-direct3d-12-component)
- [Microsoft: ID3D12Device::CreateGraphicsPipelineState](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device-creategraphicspipelinestate)
- [Microsoft: Root Signatures Overview](https://learn.microsoft.com/en-us/windows/win32/direct3d12/root-signatures-overview)
