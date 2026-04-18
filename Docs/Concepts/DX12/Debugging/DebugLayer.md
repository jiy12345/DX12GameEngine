# Debug Layer

## 개요

DX12 API 호출을 실시간으로 검증하는 레이어. 애플리케이션과 DX12 런타임 사이에 끼어들어 잘못된 사용을 즉시 감지하고 오류 메시지를 출력한다.

## 왜 필요한가?

### 문제

DX12는 명시적 제어(Explicit Control) 모델을 따른다. 힙 타입과 리소스 상태 호환성, 리소스 배리어 타이밍, API 호출 순서 등 개발자가 직접 책임져야 할 규칙이 많다.

이 규칙들은 **컴파일 타임에 검증되지 않는다.** DX12 API는 대부분 enum 값(정수)을 인자로 받기 때문에, C++ 타입 시스템은 의미론적 호환성을 알 수 없다.

```cpp
// 컴파일러 입장에서는 그냥 정수 두 개를 넘기는 것
// "READBACK 힙에 VERTEX_AND_CONSTANT_BUFFER 상태는 불가"를 컴파일 타임에 알 수 없음
device->CreateCommittedResource(
    &readbackHeapProps,
    D3D12_HEAP_FLAG_NONE,
    &desc,
    D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,  // 잘못된 조합 → 컴파일 통과
    nullptr,
    IID_PPV_ARGS(&buffer));
```

Debug Layer 없이 이런 오류가 발생하면 GPU 크래시, 이상한 렌더링 결과, 또는 조용한 실패로만 나타나 원인 추적이 매우 어렵다.

### 왜 컴파일 타임 검증이 불가능한가?

"유효한 enum 조합은 미리 결정되어 있는데, 왜 컴파일 타임에 막지 않는가?"라는 의문이 자연스럽다. 이에 대한 기술적 이유는 다음과 같다.

**1. COM(Component Object Model) 기반 API → C ABI 호환 필수**

DX12 API는 COM 기반으로, C에서도 호출 가능한 이진 인터페이스(ABI)를 제공해야 한다. COM 인터페이스는 vtable 기반 C 호환 구조이므로, C++ 템플릿이나 `enum class` 같은 C++ 전용 타입 시스템을 API 경계에서 사용할 수 없다. 결과적으로 모든 인자는 정수(UINT, enum)로 전달된다.

> Microsoft 공식 문서: ["Programming DirectX with COM"](https://learn.microsoft.com/en-us/windows/win32/prog-dx-with-com)에서 COM이 언어 중립적 바이너리 표준임을 명시한다.

**2. CUSTOM 힙 타입 — 완전 자유 구성이 가능한 케이스 존재**

`D3D12_HEAP_TYPE_CUSTOM`은 CPU 페이지 속성과 메모리 풀을 개발자가 직접 지정할 수 있는 힙 타입이다. 이 경우 UPLOAD/DEFAULT/READBACK의 명확한 분류를 벗어나 임의 조합이 가능하므로, 정적 타입 시스템으로 모든 유효한 조합을 표현하는 것이 어렵다.

**3. "명시적 제어(Explicit Control)" 설계 철학**

Microsoft는 DX12를 "낮은 수준의 하드웨어 추상화"로 설계했다. [Direct3D 12 Programming Guide](https://learn.microsoft.com/en-us/windows/win32/direct3d12/directx-12-programming-guide)에서 명시하듯, DX12는 CPU 오버헤드를 줄이기 위해 드라이버가 암묵적으로 처리하던 검증과 상태 관리를 개발자에게 이전했다. 런타임 검증(Debug Layer)은 이 명시적 계약(Explicit Contract)의 일부다.

**Microsoft도 문제를 인식했다**

Microsoft는 이 불편함을 인식하여 `d3dx12.h`라는 헬퍼 라이브러리를 제공한다. 이 헬퍼는 힙 속성, 배리어, 서술자 등을 안전하게 초기화하는 C++ 래퍼 구조체를 제공한다:

```cpp
// d3dx12.h 없이: 직접 초기화 (실수 가능)
D3D12_HEAP_PROPERTIES props = {};
props.Type = D3D12_HEAP_TYPE_UPLOAD;

// d3dx12.h 사용: 타입 안전 헬퍼
auto props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
```

> 출처: [Helper structures and functions for Direct3D 12](https://learn.microsoft.com/en-us/windows/win32/direct3d12/helper-structures-and-functions-for-d3d12)

**결론**: DX12 API 자체는 COM ABI 제약으로 인해 컴파일 타임 검증이 불가하다. Debug Layer는 이 한계를 보완하는 런타임 안전망이며, `d3dx12.h`는 C++ 수준에서 가능한 부분을 타입 안전하게 래핑한다.

### 해결책

Debug Layer가 모든 API 호출을 가로채 유효성을 검사하고, 위반 시 즉시 상세한 오류 메시지를 출력한다.

## 동작 구조

```
애플리케이션
     │  API 호출
     ▼
┌─────────────────────────────┐
│  Debug Layer                │  ← 검증: 호환성, 상태, 순서
│  (ID3D12Debug)              │    위반 시 → OutputDebugString + E_INVALIDARG
└─────────────────────────────┘
     │  통과 시
     ▼
DX12 런타임 / GPU 드라이버
     │
     ▼
GPU
```

## 감지하는 오류 유형

| 오류 유형 | 예시 |
|---|---|
| 힙 타입 ↔ 리소스 상태 불일치 | READBACK 힙에 GENERIC_READ 상태 지정 |
| 리소스 배리어 누락 | PRESENT 상태 백버퍼에 바로 렌더링 시도 |
| 잘못된 API 호출 순서 | CommandList Close 전에 Execute |
| 리소스 메모리 누락 해제 | 앱 종료 시 미해제 리소스 목록 출력 |
| 동기화 오류 | GPU 실행 중인 Allocator Reset 시도 |

## Debug Layer 활성화

반드시 **Device 생성 이전**에 호출해야 한다.

```cpp
#if defined(_DEBUG) || defined(DEBUG)
ComPtr<ID3D12Debug> debugController;
if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
{
    debugController->EnableDebugLayer();
}
#endif
```

> 이 프로젝트에서는 `Device::Initialize(bool enableDebugLayer)`의 `enableDebugLayer` 인자로 제어하며, `BuildConfig.h`가 빌드 구성에 따라 자동으로 값을 결정한다.

## Debug Layer 유무에 따른 차이

| 상황 | 잘못된 API 호출 결과 |
|---|---|
| Debug Layer **활성화** | 즉시 상세 오류 메시지 + `E_INVALIDARG` 반환 |
| Debug Layer **비활성화** | 조용히 실패 또는 GPU 크래시, 메시지 없음 |

## 성능 비용

Debug Layer는 모든 API 호출을 검사하므로 **상당한 CPU 오버헤드**가 발생한다. 개발 중에는 반드시 활성화하되, Release 빌드에서는 반드시 비활성화해야 한다.

| 빌드 | Debug Layer |
|---|---|
| Debug | 활성화 |
| Profile | 비활성화 |
| Release | 비활성화 |

## GPU 기반 검증 (GPU Validation)

Debug Layer보다 더 강력한 검증이 필요한 경우 GPU Validation을 추가로 활성화할 수 있다. 셰이더 내부에서의 메모리 접근 오류, 초기화되지 않은 리소스 읽기 등을 감지한다. 단, 성능 비용이 매우 크므로 특정 버그 추적 시에만 사용한다.

```cpp
ComPtr<ID3D12Debug3> debugController3;
debugController->QueryInterface(IID_PPV_ARGS(&debugController3));
debugController3->SetEnableGPUBasedValidation(TRUE);
```

## 관련 개념

### 선행 개념
- [Device](../Core/Device.md) - Debug Layer는 Device 생성 전에 활성화해야 함

### 연관 개념
- [ResourceBarriers](../Resources/ResourceBarriers.md) - 배리어 누락을 Debug Layer가 감지
- [MemoryHeaps](../Resources/MemoryHeaps.md) - 힙 타입 ↔ 리소스 상태 불일치를 Debug Layer가 감지

## 참고 자료

- [Microsoft: Direct3D 12 Debug Layer](https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-d3d12-debug-layer-gpu-based-validation)
- [Microsoft: ID3D12Debug interface](https://learn.microsoft.com/en-us/windows/win32/api/d3d12sdklayers/nn-d3d12sdklayers-id3d12debug)
- [Microsoft: Direct3D 12 Debug Layer Reference](https://learn.microsoft.com/en-us/windows/win32/direct3d12/direct3d-12-sdklayers-reference)
