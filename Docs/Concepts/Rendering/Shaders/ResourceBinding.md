# 셰이더 리소스 바인딩 (Resource Binding)

## 목차

- [개요](#개요)
- [왜 필요한가?](#왜-필요한가)
- [리소스 바인딩의 4가지 유형](#리소스-바인딩의-4가지-유형)
  - [Constant Data](#1-constant-data)
  - [Sampled Resource (Texture)](#2-sampled-resource-texture)
  - [Storage / UAV](#3-storage--uav-read-write-resource)
  - [Sampler](#4-sampler)
- [API별 용어 대응](#api별-용어-대응)
- [바인딩 모델의 진화](#바인딩-모델의-진화)
- [주요 고려사항](#주요-고려사항)
  - [정렬 요구사항](#정렬-요구사항)
  - [업데이트 빈도](#업데이트-빈도)
  - [크기 제약](#크기-제약)
  - [프레임 동기화](#프레임-동기화-multi-buffering)
- [DX12 구현으로의 연결](#dx12-구현으로의-연결)
- [관련 개념](#관련-개념)
- [참고 자료](#참고-자료)

## 개요

**셰이더 리소스 바인딩**은 CPU 측 데이터(상수, 텍스처, 버퍼 등)를 GPU에서 실행되는 셰이더가 **접근할 수 있도록 연결**하는 방법입니다.

모든 그래픽스 API(DirectX, Vulkan, OpenGL, Metal, WebGPU)에 존재하는 **보편 개념**입니다. API마다 이름과 구현이 다르지만 본질은 동일합니다.

## 왜 필요한가?

### 문제

셰이더는 GPU에서 실행되는 프로그램입니다. 그러나:

- **CPU 메모리와 GPU 메모리는 분리**되어 있음 (discrete GPU 기준)
- 셰이더는 CPU 메모리에 직접 접근 불가
- CPU가 계산한 데이터(카메라 매트릭스, 조명, 텍스처 등)를 **어떻게든 GPU에 넘겨야** 함

### 해결책

GPU가 접근 가능한 메모리에 데이터를 배치하고, 셰이더가 이를 **이름/인덱스**로 참조할 수 있도록 바인딩 레이아웃을 정의합니다.

```
CPU                                    GPU
───────────────────────────────────────────────────
App 데이터                    ┌─ 상수 → Constant Buffer → Shader
                              │
                              ├─ 이미지 → Texture → Shader
 업로드 /──────────────────── ┤
(각종 경로로)                 ├─ 버퍼 → Storage Buffer → Shader
                              │
                              └─ 샘플링 설정 → Sampler → Shader
───────────────────────────────────────────────────
                              ↑
                        바인딩 모델이
                      이 연결을 정의
```

## 리소스 바인딩의 4가지 유형

셰이더가 접근하는 리소스는 **접근 패턴과 크기**에 따라 네 가지로 분류됩니다.

### 1. Constant Data

- **용도**: 매 프레임 또는 드로우 콜마다 변경되는 **작은** 상수 데이터
- **접근 패턴**: 셰이더 read-only, CPU write
- **대표적 사용처**: 카메라 매트릭스, 라이트 파라미터, 머티리얼 속성, 시간
- **크기**: 일반적으로 수 KB 이하 (API별 제약 있음)
- **특성**: GPU가 모든 스레드에서 동일한 값을 읽음 → **브로드캐스트 최적화** 가능

**개념적 특징**:
- "셰이더 프로그램의 파라미터" 역할
- 드로우 콜 단위로 바뀔 수 있음
- 매우 자주 바인딩됨 → 저렴한 바인딩 경로 필요

### 2. Sampled Resource (Texture)

- **용도**: 셰이더에서 **읽기 전용**으로 접근하는 대용량 데이터
- **접근 패턴**: 셰이더 read, 샘플러로 필터링
- **대표적 사용처**: 디퓨즈 맵, 노말 맵, 환경 맵, 룩업 테이블
- **크기**: MB~GB 규모도 가능
- **특성**: **하드웨어 텍스처 유닛** 사용, 자동 보간, 밉맵, 압축 지원

**개념적 특징**:
- GPU의 전용 캐시/하드웨어 활용 (가장 빠른 읽기 경로)
- 샘플링 좌표로 보간된 값 획득
- 포맷에 따라 자동 decompression

### 3. Storage / UAV (Read-Write Resource)

- **용도**: 셰이더가 **읽고 쓸 수 있는** 버퍼/텍스처
- **접근 패턴**: 셰이더 read + write
- **대표적 사용처**: Compute Shader 출력, 파티클 시뮬레이션, 원자 연산, GPU 히스토그램
- **크기**: 수 MB~GB
- **특성**: 쓰기 순서 동기화 필요 (UAV barrier), 원자 연산 지원

**개념적 특징**:
- GPU-only 작업 파이프라인 구성 가능
- Compute → Render 연계의 기반
- 쓰기 비용이 읽기보다 큼

### 4. Sampler

- **용도**: 텍스처 샘플링 방법을 정의 (필터링, 주소 모드 등)
- **접근 패턴**: 텍스처와 함께 사용
- **대표적 사용처**: Linear/Point 필터, Wrap/Clamp 주소 모드, Anisotropic
- **크기**: 설정 몇 바이트
- **특성**: 별도 리소스가 아니라 **샘플링 설정 객체**

**개념적 특징**:
- 상태 정보만 담음 (실제 데이터 없음)
- 하드웨어 텍스처 유닛과 짝을 이룸
- 보통 자주 바뀌지 않음 (정적 샘플러로도 선언 가능)

## API별 용어 대응

같은 개념이 API마다 다른 이름으로 존재합니다.

| 개념 | DX9 | DX10/11 | **DX12** | OpenGL | Vulkan | Metal |
|------|-----|---------|----------|--------|--------|-------|
| Constant Data | Constant Register | Constant Buffer (`cbuffer`) | **Constant Buffer** (또는 Root Constants) | Uniform Buffer (UBO) | Uniform Buffer / Push Constants | Buffer argument |
| Texture 읽기 | Texture | SRV (Shader Resource View) | **SRV** (Descriptor Heap) | Sampler2D + Texture | Sampled Image + Sampler | Texture argument |
| Read-Write 버퍼 | 없음 | UAV (DX11.1+) | **UAV** | Storage Buffer (SSBO) / Image | Storage Buffer / Image | Buffer (MTLResourceUsage) |
| Sampler | Sampler State | Sampler State | **Sampler** (Descriptor Heap) | Sampler Object | Sampler | Sampler State |

모든 API에 **4가지 기본 유형이 공통**으로 존재하며, 각자의 바인딩 모델로 연결합니다.

## 바인딩 모델의 진화

바인딩을 CPU에서 셰이더로 전달하는 방식은 시대에 따라 진화했습니다.

### 1세대: 고정 슬롯 / 레지스터 (DX9, OpenGL Legacy)

```
cbuffer Frame : register(b0) { ... }  // 고정 슬롯
Texture2D tex : register(t0);          // 고정 슬롯
```

- 미리 정해진 슬롯에 리소스를 바인딩
- 단순하지만 **슬롯 수 제약** (예: 16개 샘플러 상한)
- 드라이버가 리소스 관리

### 2세대: 명시적 슬롯 + 리소스 뷰 (DX11, Vulkan 초기)

```cpp
// DX11
VSSetConstantBuffers(0, 1, &cb);
PSSetShaderResources(0, 1, &srv);
PSSetSamplers(0, 1, &sampler);
```

- 명시적으로 어느 슬롯에 어느 리소스를
- 드라이버가 내부 검증 수행
- 아직은 **슬롯 단위 바인딩** 오버헤드 존재

### 3세대: 바인딩 레이아웃 + 디스크립터 (DX12, Vulkan 현대)

```
Root Signature / Pipeline Layout = 바인딩 레이아웃 선언
Descriptor Heap / Descriptor Set = 리소스 참조들의 집합
```

- 레이아웃을 **파이프라인 생성 시** 확정
- 실행 중에는 디스크립터 묶음을 교체만
- **낮은 CPU 오버헤드 + 대량 리소스 지원**

### 4세대: Bindless (DX12 Tier 3, Vulkan Extension)

```hlsl
// 셰이더가 인덱스로 어떤 리소스든 접근
Texture2D textures[] : register(t0, space0);
textures[materialIndex].Sample(...)
```

- 슬롯 개념 사라짐
- 셰이더가 **인덱스**로 디스크립터 힙 직접 참조
- 거의 무제한 리소스, 머티리얼 시스템 단순화

각 세대는 **더 많은 리소스를 더 적은 CPU 비용으로** 바인딩하는 방향으로 진화했습니다.

## 주요 고려사항

API 무관하게 셰이더 리소스를 다룰 때 공통으로 고려할 사항들.

### 정렬 요구사항

GPU는 특정 경계(alignment)에 맞춰 메모리에 접근할 때 가장 효율적입니다.

- **Constant Buffer**: 보통 16/64/256 바이트 경계 (API별 다름)
  - DX12: 256바이트
  - Vulkan: `minUniformBufferOffsetAlignment` (하드웨어별)
  - OpenGL: `GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT`
- **셰이더 변수 내부**: `vec3` 뒤 `vec4`는 경계 맞추기 위해 **패딩 삽입** 가능 (std140, std430 등 규칙)

정렬 미준수 시 → 리소스 생성 실패 또는 잘못된 값 읽기.

### 업데이트 빈도

리소스를 얼마나 자주 업데이트하는가가 **힙 선택**을 좌우합니다.

| 업데이트 빈도 | 권장 메모리 특성 | 이유 |
|-------------|--------------|------|
| 한 번 (정적) | GPU 전용 (DX12 DEFAULT, Vulkan DEVICE_LOCAL) | GPU 읽기 최적화 |
| 매 프레임 | CPU 접근 가능 (DX12 UPLOAD, Vulkan HOST_VISIBLE) | CPU 쓰기 빈번 |
| 매 드로우 | CPU 접근 가능 + 작은 단위 | 매우 빠른 업데이트 |
| GPU 생성 | GPU 전용 (UAV) | GPU 파이프라인 |

### 크기 제약

API마다 리소스 최대 크기 제약이 다릅니다:

- Constant Buffer: DX12 ~64KB (cbuffer), Vulkan 하드웨어별 (보통 ~16KB~64KB)
- Storage Buffer: 일반적으로 훨씬 큼 (수 GB까지)
- Texture: 16384×16384 등 하드웨어 한계

용도에 맞는 리소스 타입 선택 필요.

### 프레임 동기화 (Multi-buffering)

GPU가 이전 프레임을 실행하는 동안 CPU가 다음 프레임 데이터를 준비하려면, **자주 업데이트되는 리소스는 여러 복사본**이 필요합니다.

```
프레임 N: CPU → Buffer[0] 쓰기 → GPU → Buffer[0] 읽기
프레임 N+1: CPU → Buffer[1] 쓰기 (이전과 다른 것!) → GPU → Buffer[1] 읽기
프레임 N+2: CPU → Buffer[0] 재사용 (GPU가 Buffer[0] 다 읽었음)
```

이는 [프레임 파이프라이닝](../../DX12/Commands/README.md) 개념과 직결됩니다.

## DX12 구현으로의 연결

본 문서는 개념을 설명합니다. **DX12에서 이 개념들을 실제로 어떻게 구현하는지**는 별도 Usage 문서를 참조하세요:

| 리소스 유형 | DX12 구현 가이드 |
|-----------|---------------|
| Constant Buffer | [UsingConstantBuffer.md](../../../Usage/BasicTasks/UsingConstantBuffer.md) |
| Sampled Resource (SRV) | UsingTextures.md **(미작성)** |
| UAV / Storage | UsingUAV.md **(미작성)** |
| Sampler | UsingSamplers.md **(미작성)** |

DX12 특화 바인딩 메커니즘:
- [Root Signature](../../DX12/Pipeline/RootSignature.md) **(미작성)** — 바인딩 레이아웃 선언
- [Descriptor Heap](../../DX12/Descriptors/DescriptorHeaps.md) **(미작성)** — 디스크립터 저장소
- [Memory Heaps](../../DX12/Resources/MemoryHeaps.md) — 리소스가 배치되는 메모리 유형

## 관련 개념

### 선행 개념
- [Shaders](./README.md) - 셰이더의 기본 개념 (같은 폴더)
- [GraphicsPipeline](../Pipeline/GraphicsPipeline.md) - 파이프라인 내 바인딩 시점

### DX12 구현
- [Memory Heaps](../../DX12/Resources/MemoryHeaps.md) - 힙 타입 선택
- Root Signature **(미작성)**
- Descriptor Heap **(미작성)**

### 후속 개념
- Rasterization **(미작성)**
- ResourceStates **(미작성)** - 바인딩과 상태 전환

## 참고 자료

- [Microsoft: Resource binding in DirectX 12](https://learn.microsoft.com/en-us/windows/win32/direct3d12/resource-binding)
- [Vulkan Spec: Descriptor Sets](https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#descriptorsets)
- [Khronos: Shader Resource Binding Models](https://www.khronos.org/opengl/wiki/Buffer_Object)
- [Apple: Metal Shading Language — Resource Binding](https://developer.apple.com/metal/Metal-Shading-Language-Specification.pdf)
- [NVIDIA: Bindless Graphics (OpenGL)](https://www.nvidia.com/en-us/geforce/news/bindless-graphics-opengl-4-5/)
