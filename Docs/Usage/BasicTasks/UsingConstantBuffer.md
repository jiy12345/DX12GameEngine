# Constant Buffer 사용하기 (DX12 구현 가이드)

## 목차

- [개요](#개요)
- [사전 요구사항](#사전-요구사항)
- [전체 흐름](#전체-흐름)
- [256바이트 정렬](#256바이트-정렬)
- [Constant Buffer 생성](#constant-buffer-생성)
- [데이터 업데이트 (Persistent Mapping)](#데이터-업데이트-persistent-mapping)
- [셰이더 바인딩: Root Signature 3가지 방식](#셰이더-바인딩-root-signature-3가지-방식)
  - [방식 1: Root Constants](#방식-1-root-constants)
  - [방식 2: Root Descriptor](#방식-2-root-descriptor-권장)
  - [방식 3: Descriptor Table](#방식-3-descriptor-table)
  - [방식 비교](#방식-비교)
- [HLSL 측 선언](#hlsl-측-선언)
- [프레임 버퍼링 구현](#프레임-버퍼링-구현-multi-buffering)
- [HLSL 패딩 주의](#hlsl-패딩-주의)
- [주의사항 체크리스트](#주의사항-체크리스트)
- [관련 문서](#관련-문서)
- [참고 자료](#참고-자료)

## 개요

본 문서는 **DX12에서 Constant Buffer를 실제로 구현하는 방법**을 단계별로 설명합니다. API 독립적인 **개념**은 [Rendering/ResourceBinding.md](../../Concepts/Rendering/ResourceBinding.md)를 먼저 참조하세요.

## 사전 요구사항

다음 개념을 먼저 이해하고 있어야 합니다:

- [Rendering/ResourceBinding](../../Concepts/Rendering/ResourceBinding.md) — 셰이더 리소스 바인딩 개념
- [DX12/Resources/MemoryHeaps](../../Concepts/DX12/Resources/MemoryHeaps.md) — UPLOAD 힙 특성
- Root Signature 기본 **(미작성)** — 셰이더 바인딩 레이아웃
- [DX12/Commands/README](../../Concepts/DX12/Commands/README.md) — 커맨드 기록 기본

## 전체 흐름

```
┌────────────────────────────────┐
│ 1. Constant Buffer 리소스 생성  │  (UPLOAD 힙, 256바이트 정렬 크기)
└────────────────────────────────┘
                ↓
┌────────────────────────────────┐
│ 2. Persistent Mapping           │  (Map 한 번, 포인터 유지)
└────────────────────────────────┘
                ↓
┌────────────────────────────────┐
│ 3. 매 프레임 데이터 업데이트     │  (memcpy로 C++ 구조체 → 매핑 포인터)
└────────────────────────────────┘
                ↓
┌────────────────────────────────┐
│ 4. Root Signature 설정          │  (3가지 방식 중 선택)
└────────────────────────────────┘
                ↓
┌────────────────────────────────┐
│ 5. 드로우 콜                    │  (셰이더가 cbuffer 읽음)
└────────────────────────────────┘
```

## 256바이트 정렬

DX12 Constant Buffer는 **256바이트 배수**로 크기를 맞춰야 합니다.

```cpp
// 256바이트 정렬 유틸리티
UINT CalcConstantBufferSize(UINT byteSize)
{
    // (byteSize + 255) & ~255 : 올림하여 256의 배수로
    return (byteSize + 255) & ~255;
}

// 예: 구조체가 80바이트라면
//   CalcConstantBufferSize(80) → 256바이트로 할당
```

> [Microsoft: Constant buffers](https://learn.microsoft.com/en-us/windows/win32/direct3d12/constants): "Constant buffer data must be aligned to 256-byte boundaries."

**왜 256바이트?** DX12 하드웨어의 메모리 접근 단위에 맞추어진 정렬 요구사항입니다. 위반 시 `CreateConstantBufferView()` 또는 `SetGraphicsRootConstantBufferView()`가 실패합니다.

## Constant Buffer 생성

`UPLOAD` 힙을 사용해 CPU에서 직접 쓸 수 있도록 생성합니다.

```cpp
struct CameraData
{
    DirectX::XMMATRIX viewProj;  // 64바이트
    DirectX::XMFLOAT3 camPos;    // 12바이트
    float padding;               // 4바이트 (16바이트 경계)
};
// C++ sizeof(CameraData) = 80바이트

const UINT bufferSize = CalcConstantBufferSize(sizeof(CameraData));  // 256

D3D12_HEAP_PROPERTIES heapProps = {};
heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

D3D12_RESOURCE_DESC bufferDesc = {};
bufferDesc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
bufferDesc.Width              = bufferSize;  // 256바이트 정렬 크기
bufferDesc.Height             = 1;
bufferDesc.DepthOrArraySize   = 1;
bufferDesc.MipLevels          = 1;
bufferDesc.Format             = DXGI_FORMAT_UNKNOWN;
bufferDesc.SampleDesc.Count   = 1;
bufferDesc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

ComPtr<ID3D12Resource> constantBuffer;
device->CreateCommittedResource(
    &heapProps,
    D3D12_HEAP_FLAG_NONE,
    &bufferDesc,
    D3D12_RESOURCE_STATE_GENERIC_READ,  // UPLOAD 힙 고정 상태
    nullptr,
    IID_PPV_ARGS(&constantBuffer));
```

**왜 `D3D12_RESOURCE_STATE_GENERIC_READ`?** UPLOAD 힙 리소스는 이 상태로 고정되며, 배리어로 변경할 수 없습니다. [MemoryHeaps 문서](../../Concepts/DX12/Resources/MemoryHeaps.md) 참조.

## 데이터 업데이트 (Persistent Mapping)

Constant Buffer는 매 프레임 변경되므로 `Map()`을 한 번만 호출하고 포인터를 유지하는 **Persistent Mapping** 패턴을 사용합니다.

```cpp
// 초기화: 한 번 Map
void* mappedData = nullptr;
D3D12_RANGE readRange = { 0, 0 };  // CPU에서 읽지 않음
constantBuffer->Map(0, &readRange, &mappedData);

// 매 프레임: 포인터에 직접 memcpy
CameraData data = {};
data.viewProj = GetViewProjMatrix();
data.camPos   = GetCameraPosition();
memcpy(mappedData, &data, sizeof(CameraData));

// 소멸 시: Unmap (생략 가능)
constantBuffer->Unmap(0, nullptr);
```

> **Tip**: UPLOAD 힙은 CPU/GPU가 동시에 접근할 수 있으므로 `Unmap()` 없이 영구적으로 매핑 상태를 유지해도 됩니다.

### readRange 설명

```cpp
D3D12_RANGE readRange = { 0, 0 };
```

`Begin == End` → "CPU가 이 리소스를 읽지 않겠다"고 드라이버에 알림. 드라이버가 CPU 캐시 전략을 최적화할 수 있게 합니다. Write-only 패턴이라면 항상 이 설정.

## 셰이더 바인딩: Root Signature 3가지 방식

Constant Buffer를 셰이더에 연결하는 방식은 세 가지가 있습니다. 성능과 사용 가능한 리소스 종류가 다릅니다.

### 방식 1: Root Constants

32비트 값을 Root Signature에 **직접 박아넣습니다.** 디스크립터 힙이 없어도 되고 가장 빠릅니다. 작은 값(프레임 인덱스, 플래그 등)에 적합.

```cpp
// Root Signature 생성 시
D3D12_ROOT_PARAMETER rootParam = {};
rootParam.ParameterType            = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
rootParam.Constants.ShaderRegister = 0;   // b0 레지스터
rootParam.Constants.RegisterSpace  = 0;
rootParam.Constants.Num32BitValues = 1;   // 전달할 32비트 값 개수

// 드로우 시: 값 직접 설정
commandList->SetGraphicsRoot32BitConstant(
    0,           // Root Parameter 인덱스
    frameIndex,  // 전달할 값
    0);          // 슬롯 내 오프셋
```

```hlsl
cbuffer Frame : register(b0) { uint frameIndex; }
```

**사용 상황**: 프레임 인덱스, 드로우 ID, 플래그 등 16개 DWORD 이하의 초소형 데이터.

### 방식 2: Root Descriptor (권장)

GPU 가상 주소를 Root Signature에 **직접 전달**합니다. 디스크립터 힙 없이 CBV/SRV/UAV **버퍼**를 바인딩할 수 있습니다. 단, 텍스처에는 사용 불가.

```cpp
// Root Signature 생성 시
D3D12_ROOT_PARAMETER rootParam = {};
rootParam.ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;  // or SRV, UAV
rootParam.Descriptor.ShaderRegister = 0;  // b0 레지스터
rootParam.Descriptor.RegisterSpace  = 0;
rootParam.ShaderVisibility          = D3D12_SHADER_VISIBILITY_ALL;

D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
rootSigDesc.NumParameters = 1;
rootSigDesc.pParameters   = &rootParam;
rootSigDesc.Flags         = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

// 드로우 시: GPU 가상 주소 직접 설정 (디스크립터 힙 불필요)
commandList->SetGraphicsRootConstantBufferView(
    0,                                          // Root Parameter 인덱스
    constantBuffer->GetGPUVirtualAddress());    // 버퍼 GPU 주소
```

**사용 상황**: 대부분의 Constant Buffer 바인딩 (프레임별 CB, 머티리얼 CB 등). **본 프로젝트의 기본 방식.**

### 방식 3: Descriptor Table

셰이더가 볼 수 있는 디스크립터 힙(CBV_SRV_UAV 또는 Sampler) 안의 **범위**를 가리킵니다. 텍스처(SRV), Sampler 바인딩에 필수이며, 한 슬롯으로 여러 리소스 범위를 가리킬 수 있습니다.

```cpp
// Root Signature 생성 시: 힙 범위 선언
D3D12_DESCRIPTOR_RANGE range = {};
range.RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
range.NumDescriptors                    = 1;
range.BaseShaderRegister                = 0;  // b0 레지스터
range.RegisterSpace                     = 0;
range.OffsetInDescriptorsFromTableStart = 0;

D3D12_ROOT_PARAMETER rootParam = {};
rootParam.ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
rootParam.DescriptorTable.NumDescriptorRanges = 1;
rootParam.DescriptorTable.pDescriptorRanges   = &range;

// 드로우 시: 셰이더가 볼 수 있는 힙 설정 후 시작 위치 지정
ID3D12DescriptorHeap* heaps[] = { cbvSrvUavHeap.Get() };
commandList->SetDescriptorHeaps(1, heaps);
commandList->SetGraphicsRootDescriptorTable(0, heapGpuHandle);
```

**사용 상황**: 텍스처, Sampler, 또는 Constant Buffer 여러 개를 한 슬롯으로 묶을 때.

### 방식 비교

| 방식 | 디스크립터 힙 필요 | 텍스처 지원 | 속도 | 용도 |
|------|:---:|:---:|------|------|
| Root Constants | ❌ | ❌ | 가장 빠름 (GPU 레지스터 직접) | 초소형 상수 |
| Root Descriptor | ❌ | ❌ (버퍼만) | 빠름 | **Constant Buffer 일반** |
| Descriptor Table | ✅ | ✅ | 상대적으로 느림 | 텍스처, 대량 리소스 |

Constant Buffer는 버퍼이므로 **Root Descriptor로 충분**합니다. 텍스처를 바인딩하려면 Descriptor Table이 필요합니다.

## HLSL 측 선언

```hlsl
// b0 레지스터로 바인딩
cbuffer CameraData : register(b0)
{
    float4x4 viewProj;
    float3   camPos;
    float    padding;
};

// Vertex Shader에서 사용
float4 VSMain(float3 pos : POSITION) : SV_POSITION
{
    return mul(float4(pos, 1.0f), viewProj);
}
```

## 프레임 버퍼링 구현 (Multi-buffering)

### 어떤 병목을 회피하는가?

프레임 버퍼링의 목적은 **CPU-GPU 동기화 병목**을 제거하는 것입니다. 단일 버퍼로 이 패턴을 구현하면 CPU와 GPU가 **순차 실행**될 수밖에 없어 전체 처리량이 크게 떨어집니다.

#### 단일 버퍼의 문제 (병목)

```
시간 →
CPU: [Buffer 쓰기 → Draw 기록 → 제출]  [GPU 완료 대기 ·····]  [다시 쓰기 → ...]
GPU:                                   [Buffer 읽고 렌더 ────]
                                       ↑ CPU는 여기서 놀아야 함
                                         (Buffer를 건드리면 GPU가 읽던 데이터 변조)
```

**왜 대기가 필요한가**:
- CPU가 다음 프레임의 Constant Buffer 값(예: 카메라 매트릭스 업데이트)을 쓰려는 순간
- GPU가 아직 이번 프레임의 같은 Buffer를 읽고 있다면
- **같은 메모리에 동시 접근** → race condition → 깨진 데이터로 렌더링
- 이를 피하려고 CPU는 GPU 완료를 기다려야 함 (Fence 등)

결과: CPU와 GPU가 **교대로** 일해야 하므로 프레임 시간 = CPU 시간 + GPU 시간. **파이프라이닝 불가**.

#### 다중 버퍼로 해결

```
시간 →
CPU: [Buffer[0] 쓰기]  [Buffer[1] 쓰기]  [Buffer[2] 쓰기]  [Buffer[0] 재사용]
GPU:                   [Buffer[0] 렌더]  [Buffer[1] 렌더]  [Buffer[2] 렌더]
                       ↑ CPU와 GPU가 **동시에 다른 버퍼**를 작업
                         → 대기 없이 파이프라이닝
```

**핵심**: 백 버퍼 수만큼 Constant Buffer를 **중복 생성**하면 CPU와 GPU가 서로 다른 버퍼를 동시에 건드릴 수 있습니다. Race condition 없이 파이프라이닝 성립.

### 동일한 원리가 적용되는 리소스들

프레임 버퍼링은 Constant Buffer 고유 기법이 아니라 **"CPU가 매 프레임 업데이트하고 GPU가 읽는 모든 리소스"** 에 공통 적용되는 패턴입니다:

| 리소스 | 중복 생성 단위 |
|--------|-------------|
| SwapChain 백 버퍼 | 기본 2~3개 |
| Command Allocator | 프레임 수만큼 |
| Constant Buffer | 프레임 수만큼 |
| 프레임별 Dynamic Buffer (인스턴스 데이터 등) | 프레임 수만큼 |
| 프레임별 Descriptor Heap 구간 | 프레임 수만큼 |

이런 리소스들을 묶어 관리하는 구조가 **"Frame-Indexed Resources"** 패턴이며, 본 프로젝트의 구현 이슈는 [#45 프레임 파이프라이닝 (Frame-Indexed Resources) 구현](https://github.com/jiy12345/DX12GameEngine/issues/45) 에서 다룹니다.

### 구현

```
프레임 N   : CPU가 Buffer[0]에 쓰기 → GPU가 Buffer[0] 읽기
프레임 N+1 : CPU가 Buffer[1]에 쓰기 → GPU가 Buffer[1] 읽기
프레임 N+2 : CPU가 Buffer[0]에 쓰기 (재사용, 이미 GPU가 다 읽음) → ...
```

```cpp
constexpr UINT kBackBufferCount = 3;

// 백 버퍼 수만큼 생성
ComPtr<ID3D12Resource> constantBuffers[kBackBufferCount];
void* mappedData[kBackBufferCount];

// 각각 생성 + Persistent Mapping
for (UINT i = 0; i < kBackBufferCount; i++) {
    device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE,
        &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, IID_PPV_ARGS(&constantBuffers[i]));

    D3D12_RANGE readRange = { 0, 0 };
    constantBuffers[i]->Map(0, &readRange, &mappedData[i]);
}

// 매 프레임: 현재 프레임 인덱스의 버퍼 사용
UINT frameIndex = swapChain->GetCurrentBackBufferIndex();
memcpy(mappedData[frameIndex], &data, sizeof(CameraData));
commandList->SetGraphicsRootConstantBufferView(
    0, constantBuffers[frameIndex]->GetGPUVirtualAddress());
```

이 패턴은 [프레임 파이프라이닝](../../Concepts/DX12/Display/SwapChain.md#백-버퍼-인덱스-관리) 의 핵심 요소입니다. 본 프로젝트의 구현 계획은 [#45](https://github.com/jiy12345/DX12GameEngine/issues/45) 참조.

## HLSL 패딩 주의

HLSL `cbuffer`는 **16바이트 경계** 규칙을 따릅니다. 구조체 내 변수가 16바이트 경계를 넘지 않도록 주의하세요.

```hlsl
// ❌ 잘못된 예: float3 + float 이후 float3 → 경계 어긋남 → 자동 패딩
cbuffer Bad : register(b0)
{
    float3 direction;   // 12바이트 (offset 0)
    float  amplitude;   // 4바이트 (offset 12) → 여기까지 16바이트 경계 딱 맞음
    float3 color;       // 12바이트 (offset 16) → OK, 다음 값이 문제
    float3 velocity;    // 12바이트 → offset 28부터 시작, 16바이트 경계 위반!
                        //   → 자동 패딩 삽입되어 C++ 구조체와 불일치
};

// ✅ 올바른 예: 16바이트 단위로 정렬 유지
cbuffer Good : register(b0)
{
    float3 direction;
    float  amplitude;   // direction+amplitude = 16바이트 완성

    float3 color;
    float  padding1;    // 명시적 패딩

    float3 velocity;
    float  padding2;    // 명시적 패딩
};
```

C++ 측 구조체도 동일한 정렬로 맞춰야 합니다:

```cpp
struct Good {
    DirectX::XMFLOAT3 direction;
    float             amplitude;
    DirectX::XMFLOAT3 color;
    float             padding1;
    DirectX::XMFLOAT3 velocity;
    float             padding2;
};
static_assert(sizeof(Good) == 48);  // 16 × 3
```

## 주의사항 체크리스트

- [ ] 버퍼 크기가 **256바이트 배수** 인가? (`CalcConstantBufferSize` 사용)
- [ ] HLSL cbuffer의 각 변수가 **16바이트 경계**를 넘지 않는가?
- [ ] C++ 구조체의 `sizeof()`가 HLSL cbuffer 레이아웃과 **정확히 일치**하는가?
- [ ] UPLOAD 힙 리소스에 **배리어를 시도**하지 않는가? (GENERIC_READ 고정)
- [ ] Persistent Mapping 중 CPU와 GPU가 **동시에 같은 위치를 쓰지** 않는가? (프레임 버퍼링으로 분리)
- [ ] `readRange = {0, 0}` 으로 **CPU 읽기 없음을 명시**했는가?
- [ ] 셰이더 `register(b0)` 과 Root Signature `ShaderRegister = 0` 이 일치하는가?
- [ ] Debug Layer 활성화 상태에서 테스트했는가?

## 관련 문서

### 개념 (먼저 이해할 것)
- [Rendering/ResourceBinding](../../Concepts/Rendering/ResourceBinding.md) — 셰이더 리소스 바인딩 개념
- [DX12/Resources/MemoryHeaps](../../Concepts/DX12/Resources/MemoryHeaps.md) — UPLOAD 힙 이유
- Root Signature **(미작성)**

### 관련 구현
- Frame Pipelining **(미작성)** — 프레임 버퍼링 패턴 (#45)
- UsingTextures.md **(미작성)** — 텍스처 바인딩 (Descriptor Table 활용)

### 구현 이슈
- [#20](https://github.com/jiy12345/DX12GameEngine/issues/20) 상수 버퍼
- [#14](https://github.com/jiy12345/DX12GameEngine/issues/14) PSO/Root Signature 구현
- [#45](https://github.com/jiy12345/DX12GameEngine/issues/45) 프레임 파이프라이닝

## 참고 자료

- [Microsoft: Constant buffers](https://learn.microsoft.com/en-us/windows/win32/direct3d12/constants)
- [Microsoft: Root descriptors](https://learn.microsoft.com/en-us/windows/win32/direct3d12/root-signature-version-1-1#root-descriptors)
- [Microsoft: D3D12_ROOT_PARAMETER_TYPE enumeration](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_root_parameter_type)
- [Microsoft: Using a constant buffer directly in the root signature](https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-constants-directly-in-the-root-signature)
- [Microsoft: HLSL packing rules](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-packing-rules)
