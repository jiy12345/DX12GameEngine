# Constant Buffer (상수 버퍼)

## 목차

- [개요](#개요)
- [왜 필요한가?](#왜-필요한가)
- [256바이트 정렬](#256바이트-정렬-256-byte-alignment)
- [Constant Buffer 생성](#constant-buffer-생성)
- [데이터 업데이트 (Persistent Mapping)](#데이터-업데이트-persistent-mapping)
- [셰이더 바인딩: Root Signature](#셰이더-바인딩-root-signature)
  - [방식 1: Root Constants](#방식-1-root-constants)
  - [방식 2: Root Descriptor](#방식-2-root-descriptor-이-문서에서-사용하는-방식)
  - [방식 3: Descriptor Table](#방식-3-descriptor-table)
  - [방식 비교](#방식-비교)
- [HLSL 측 선언](#hlsl-측-선언)
- [프레임 버퍼링 (Multi-buffering)](#프레임-버퍼링-multi-buffering)
- [주의사항](#주의사항)
- [HLSL 패딩 주의 예시](#hlsl-패딩-주의-예시)
- [관련 개념](#관련-개념)
- [참고 자료](#참고-자료)

## 개요

CPU에서 매 프레임(또는 드로우 콜마다) 변경되는 데이터를 셰이더에 전달하는 GPU 리소스. 변환 행렬, 카메라 파라미터, Wave 파라미터 등이 대표적이다.

## 왜 필요한가?

### 문제

셰이더는 GPU에서 실행되므로 CPU 메모리에 직접 접근할 수 없다. 셰이더가 필요로 하는 데이터(예: 월드 변환 행렬, 조명 색상)를 GPU 접근 가능한 메모리에 올려야 한다.

### 해결책

**Constant Buffer**는 UPLOAD 힙에 생성된 GPU 리소스로, CPU에서 `Map()`으로 직접 쓸 수 있고 GPU 셰이더에서 읽을 수 있다. Root Signature를 통해 셰이더에 바인딩된다.

## 256바이트 정렬 (256-byte Alignment)

DX12 Constant Buffer는 **256바이트 배수**로 크기를 맞춰야 한다.

```cpp
// 256바이트 정렬 유틸리티
UINT CalcConstantBufferSize(UINT byteSize)
{
    // (byteSize + 255) & ~255 : 올림하여 256의 배수로 만들기
    return (byteSize + 255) & ~255;
}

// 예: WaveParams 구조체가 80바이트라면
//   CalcConstantBufferSize(80) → 256바이트로 할당
```

> [Microsoft: Constant buffers](https://learn.microsoft.com/en-us/windows/win32/direct3d12/constants): "Constant buffer data must be aligned to 256-byte boundaries."

## Constant Buffer 생성

UPLOAD 힙을 사용해 CPU에서 직접 쓸 수 있도록 생성한다.

```cpp
struct WaveParams
{
    float amplitude;    // 파고
    float wavelength;   // 파장
    float speed;        // 위상 속도
    float steepness;    // 뾰족함 (Gerstner Q)
    float dirX;         // 진행 방향 X
    float dirY;         // 진행 방향 Y
    float time;         // 경과 시간
    float padding;      // 16바이트 정렬 패딩
};

const UINT bufferSize = CalcConstantBufferSize(sizeof(WaveParams));  // 256

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

## 데이터 업데이트 (Persistent Mapping)

Constant Buffer는 매 프레임 변경되므로 `Map()`을 한 번만 호출하고 포인터를 유지하는 **Persistent Mapping** 패턴을 사용한다.

```cpp
// 초기화 시: 한 번 Map
void* m_mappedData = nullptr;
D3D12_RANGE readRange = { 0, 0 };  // CPU에서 읽지 않음
constantBuffer->Map(0, &readRange, &m_mappedData);

// 매 프레임: 포인터에 직접 memcpy
WaveParams params = {};
params.amplitude  = 0.5f;
params.time       = elapsedTime;
// ...
memcpy(m_mappedData, &params, sizeof(WaveParams));

// 소멸 시: Unmap
constantBuffer->Unmap(0, nullptr);
```

> UPLOAD 힙은 CPU/GPU가 동시에 접근할 수 있으므로 `Unmap()` 없이 영구적으로 매핑 상태를 유지해도 된다.

## 셰이더 바인딩: Root Signature

셰이더가 어떤 리소스에 접근할 수 있는지 레이아웃을 미리 선언하는 계약서다. 드로우 콜 전에 Root Signature를 설정하면, 셰이더는 거기에 정의된 슬롯을 통해 리소스를 읽는다.

Root Parameter는 세 가지 방식이 있으며 방식에 따라 성능과 사용 가능한 리소스 종류가 달라진다.

### 방식 1: Root Constants

32비트 값을 Root Signature에 직접 박아넣는다. 디스크립터 힙이 없어도 되고 가장 빠르다. 작은 값(프레임 인덱스, 플래그 등)에 적합하다.

```cpp
// Root Signature 생성 시
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

### 방식 2: Root Descriptor (이 문서에서 사용하는 방식)

GPU 가상 주소를 Root Signature에 직접 전달한다. 디스크립터 힙 없이 CBV/SRV/UAV 버퍼를 바인딩할 수 있다. 단, 텍스처에는 사용 불가.

```cpp
// Root Signature 생성 시
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

### 방식 3: Descriptor Table

셰이더가 볼 수 있는 디스크립터 힙(CBV_SRV_UAV 또는 Sampler) 안의 범위를 가리킨다. 텍스처(SRV), Sampler 바인딩에 필수이며, 한 슬롯으로 여러 리소스 범위를 가리킬 수 있다.

```cpp
// Root Signature 생성 시: 힙 범위 선언
D3D12_DESCRIPTOR_RANGE range = {};
range.RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
range.NumDescriptors                    = 1;
range.BaseShaderRegister                = 0;  // b0 레지스터
range.RegisterSpace                     = 0;
range.OffsetInDescriptorsFromTableStart = 0;

rootParam.ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
rootParam.DescriptorTable.NumDescriptorRanges = 1;
rootParam.DescriptorTable.pDescriptorRanges   = &range;

// 드로우 시: 셰이더가 볼 수 있는 힙 설정 후 시작 위치 지정
ID3D12DescriptorHeap* heaps[] = { m_cbvSrvUavHeap };
commandList->SetDescriptorHeaps(1, heaps);
commandList->SetGraphicsRootDescriptorTable(0, heapGpuHandle);
```

### 방식 비교

| 방식 | 디스크립터 힙 필요 | 텍스처 지원 | 속도 |
|---|---|---|---|
| Root Constants | ❌ | ❌ | 가장 빠름 (GPU 레지스터 직접) |
| Root Descriptor | ❌ | ❌ (버퍼만) | 빠름 |
| Descriptor Table | ✅ | ✅ | 상대적으로 느림 |

Constant Buffer는 버퍼이므로 Root Descriptor로 충분하다. 텍스처를 바인딩하려면 Descriptor Table 방식이 필요하다.

## HLSL 측 선언

```hlsl
// b0 레지스터로 바인딩
cbuffer WaveParams : register(b0)
{
    float amplitude;
    float wavelength;
    float speed;
    float steepness;
    float dirX;
    float dirY;
    float time;
    float padding;
};

// Vertex Shader에서 사용
float4 VSMain(float3 pos : POSITION) : SV_POSITION
{
    pos.y += amplitude * sin(dot(float2(dirX, dirY), pos.xz) * wavelength + time * speed);
    return float4(pos, 1.0f);
}
```

## 프레임 버퍼링 (Multi-buffering)

GPU가 이전 프레임 커맨드를 실행하는 동안 CPU가 다음 프레임 데이터를 써야 한다면, 백 버퍼 수만큼 Constant Buffer를 중복 생성한다.

```
프레임 N   : CPU가 Buffer[0]에 쓰기 → GPU가 Buffer[0] 읽기
프레임 N+1 : CPU가 Buffer[1]에 쓰기 → GPU가 Buffer[1] 읽기
프레임 N+2 : CPU가 Buffer[0]에 쓰기 (재사용) → ...
```

```cpp
// 백 버퍼 수만큼 생성
ComPtr<ID3D12Resource> m_constantBuffers[kBackBufferCount];
void* m_mappedData[kBackBufferCount];

// 현재 프레임 인덱스에 해당하는 버퍼 사용
UINT frameIndex = swapChain->GetCurrentBackBufferIndex();
memcpy(m_mappedData[frameIndex], &params, sizeof(WaveParams));
commandList->SetGraphicsRootConstantBufferView(
    0, m_constantBuffers[frameIndex]->GetGPUVirtualAddress());
```

## 주의사항

- ⚠️ 버퍼 크기는 반드시 256바이트 배수로 맞춰야 한다. 그렇지 않으면 `CreateConstantBufferView()` 또는 `SetGraphicsRootConstantBufferView()` 호출이 실패한다.
- ⚠️ HLSL `cbuffer` 내 각 변수는 16바이트 경계를 넘을 수 없다. `float3` 뒤에 `float`를 바로 넣으면 패딩이 자동 삽입될 수 있으므로 C++ 구조체와 정렬을 맞춰야 한다.
- ⚠️ Persistent Mapping 중 CPU와 GPU가 동시에 같은 위치를 읽고 쓰면 안 된다. 프레임 버퍼링으로 구간을 분리할 것.

## HLSL 패딩 주의 예시

```hlsl
// 잘못된 예: float3 + float는 16바이트를 넘어 패딩 발생
cbuffer Bad : register(b0)
{
    float3 direction;   // 12바이트
    float  amplitude;   // 4바이트 → 문제 없어 보이지만
    float3 color;       // 12바이트 → 앞의 float 때문에 16바이트 경계에서 시작 못 함 → 자동 패딩
};

// 올바른 예: float4로 묶거나 명시적 패딩 추가
cbuffer Good : register(b0)
{
    float3 direction;
    float  amplitude;   // float3 + float = 16바이트, 딱 맞음

    float3 color;
    float  padding;     // 명시적 패딩
};
```

## 관련 개념

### 선행 개념
- [MemoryHeaps](./MemoryHeaps.md) - UPLOAD 힙 동작 원리
- [RootSignature](./RootSignature.md) - 셰이더 리소스 바인딩 레이아웃

### 연관 개념
- [ResourceBarriers](./ResourceBarriers.md) - 버퍼 상태 전환 (UPLOAD 힙은 불필요)

## 참고 자료

- [Microsoft: Constant buffers](https://learn.microsoft.com/en-us/windows/win32/direct3d12/constants)
- [Microsoft: Root descriptors](https://learn.microsoft.com/en-us/windows/win32/direct3d12/root-signature-version-1-1#root-descriptors)
- [Microsoft: D3D12_ROOT_PARAMETER_TYPE enumeration](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_root_parameter_type)
- [Microsoft: Using a constant buffer](https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-constants-directly-in-the-root-signature)
