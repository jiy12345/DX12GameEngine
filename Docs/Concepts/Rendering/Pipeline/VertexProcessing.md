# 정점 처리 (Vertex Processing)

## 목차

- [개요](#개요)
- [왜 필요한가?](#왜-필요한가)
- [Vertex Buffer](#vertex-buffer)
- [Input Layout](#input-layout)
- [NDC 좌표계](#ndc-좌표계)
- [주의사항](#주의사항)
- [관련 개념](#관련-개념)
- [참고 자료](#참고-자료)

## 개요
정점(Vertex)은 3D 오브젝트를 구성하는 점이며, DX12에서는 정점 데이터를 **Vertex Buffer**에 담아 GPU에 전달하고, **Input Layout**으로 데이터 포맷을 정의한다.

## 왜 필요한가?

### 문제
GPU는 메모리 상의 바이트 배열이 어떤 의미인지 알지 못한다. 각 바이트가 위치인지 색상인지 UV인지 명시적으로 알려줘야 한다.

### 해결책
**Input Layout**이 정점 구조체의 각 필드를 셰이더 시맨틱(POSITION, COLOR, TEXCOORD 등)에 매핑한다. DX12는 이를 PSO의 일부로 사전 컴파일한다.

## Vertex Buffer

### 개념
정점 데이터를 저장하는 GPU 리소스. 삼각형 하나에는 정점 3개가 필요하다.

### 메모리 종류
| 힙 타입 | 용도 | 비고 |
|---|---|---|
| `UPLOAD` | CPU가 쓰고 GPU가 읽음 | 간단하지만 GPU 최적 성능은 아님 |
| `DEFAULT` | GPU 전용 | 최고 성능, 초기 데이터 복사에 Upload 힙 필요 |

> Phase 1에서는 단순화를 위해 **UPLOAD 힙**을 직접 사용한다. Phase 2 이후에는 정적 지오메트리를 DEFAULT 힙으로 옮길 예정이다.

### Vertex Buffer 생성 (Phase 1: UPLOAD 힙)

```cpp
// 1. 정점 데이터 정의
struct Vertex
{
    float position[3];  // POSITION: x, y, z
    float color[4];     // COLOR: r, g, b, a
};

Vertex vertices[] =
{
    {  0.0f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f, 1.0f },  // 위쪽: 빨강
    {  0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f, 1.0f },  // 오른쪽 아래: 파랑
    { -0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f, 1.0f },  // 왼쪽 아래: 초록
};

// 2. UPLOAD 힙에 버퍼 생성
D3D12_HEAP_PROPERTIES heapProps = {};
heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

D3D12_RESOURCE_DESC bufferDesc = {};
bufferDesc.Dimension  = D3D12_RESOURCE_DIMENSION_BUFFER;
bufferDesc.Width      = sizeof(vertices);
bufferDesc.Height     = 1;
bufferDesc.DepthOrArraySize = 1;
bufferDesc.MipLevels  = 1;
bufferDesc.Format     = DXGI_FORMAT_UNKNOWN;
bufferDesc.SampleDesc.Count = 1;
bufferDesc.Layout     = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

device->CreateCommittedResource(
    &heapProps, D3D12_HEAP_FLAG_NONE,
    &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
    nullptr, IID_PPV_ARGS(&vertexBuffer));

// 3. CPU에서 정점 데이터 복사 (Map/Unmap)
void* mappedData = nullptr;
D3D12_RANGE readRange = { 0, 0 };  // CPU에서 읽지 않음
vertexBuffer->Map(0, &readRange, &mappedData);
memcpy(mappedData, vertices, sizeof(vertices));
vertexBuffer->Unmap(0, nullptr);
```

### Vertex Buffer View 설정

Vertex Buffer View는 GPU에게 버퍼의 위치, 크기, 정점 하나의 크기를 알려준다.

```cpp
D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
vertexBufferView.SizeInBytes    = sizeof(vertices);
vertexBufferView.StrideInBytes  = sizeof(Vertex);  // 정점 하나의 크기 (바이트)
```

## Input Layout

### 개념
Input Layout은 정점 구조체의 각 필드를 셰이더의 `POSITION`, `COLOR` 등의 시맨틱에 매핑한다. PSO에 포함되어 사전 컴파일된다.

### D3D12_INPUT_ELEMENT_DESC 구조

```cpp
D3D12_INPUT_ELEMENT_DESC inputLayout[] =
{
    //  SemanticName  SemanticIdx  Format                          InputSlot  AlignedOffset  InputSlotClass                       InstanceDataStepRate
    { "POSITION",    0,           DXGI_FORMAT_R32G32B32_FLOAT,    0,          0,            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "COLOR",       0,           DXGI_FORMAT_R32G32B32A32_FLOAT, 0,         12,            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};
```

| 필드 | 의미 |
|---|---|
| `SemanticName` | HLSL 시맨틱 이름 (셰이더의 `: POSITION`과 일치) |
| `Format` | 데이터 포맷 (`float3` → `R32G32B32_FLOAT`) |
| `InputSlot` | Vertex Buffer 슬롯 번호 (0~15) |
| `AlignedOffset` | 구조체 내 바이트 오프셋 (`offsetof()` 사용 가능) |
| `InputSlotClass` | 정점별(`PER_VERTEX_DATA`) vs 인스턴스별(`PER_INSTANCE_DATA`) |

### HLSL 시맨틱과 C++ 포맷 대응

| HLSL 타입 | DXGI_FORMAT |
|---|---|
| `float` | `R32_FLOAT` |
| `float2` | `R32G32_FLOAT` |
| `float3` | `R32G32B32_FLOAT` |
| `float4` | `R32G32B32A32_FLOAT` |

## NDC 좌표계

정점 좌표는 **NDC(Normalized Device Coordinates)** 기준이다.

```
NDC 공간:
  y=+1
  ┌───┐
  │   │   x: -1(왼쪽) ~ +1(오른쪽)
  │   │   y: -1(아래) ~ +1(위쪽)
  └───┘   z:  0(near) ~ +1(far)
  y=-1
```

Phase 1 삼각형은 뷰 변환 없이 NDC 좌표를 직접 사용한다.

## 주의사항
- ⚠️ `StrideInBytes`는 정점 구조체 전체 크기여야 한다. 잘못 설정하면 GPU가 엉뚱한 메모리를 읽는다.
- ⚠️ `AlignedOffset`의 시맨틱 순서는 구조체 필드 순서와 일치해야 한다.
- ⚠️ UPLOAD 힙 리소스는 `D3D12_RESOURCE_STATE_GENERIC_READ`로 생성해야 한다.
- ⚠️ `Map()`한 포인터는 `Unmap()` 전에 GPU가 읽어선 안 된다. CPU 쓰기가 끝난 후 Unmap한다.

## 관련 개념

### 선행 개념
- [커맨드 리스트](../../DX12/Commands/CommandList.md)

### 연관 개념
- [그래픽스 파이프라인](./GraphicsPipeline.md)
- [셰이더](../Shaders/README.md)
- [리소스 바인딩](../Shaders/ResourceBinding.md)

### 후속 개념
- [Index Buffer](./IndexBuffer.md) *(TODO: Phase 2)*
- [Instance Rendering](./InstanceRendering.md) *(TODO: Phase 3+)*

## 참고 자료
- [Microsoft: Creating a vertex buffer](https://learn.microsoft.com/en-us/windows/win32/direct3d12/creating-a-vertex-buffer)
- [Microsoft: D3D12_INPUT_ELEMENT_DESC](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_input_element_desc)
- [Microsoft: D3D12_VERTEX_BUFFER_VIEW](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_vertex_buffer_view)
- [Microsoft: Upload heaps](https://learn.microsoft.com/en-us/windows/win32/direct3d12/uploading-resources)
