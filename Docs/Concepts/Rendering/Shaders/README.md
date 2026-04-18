# 셰이더 (Shaders) — 폴더 개요

이 폴더는 **셰이더 프로그래밍** 관련 개념을 다룹니다.

## 📑 이 폴더의 문서

| 문서 | 내용 |
|------|------|
| [README.md](./README.md) (이 문서) | 셰이더 기초 (종류, HLSL 기본 구조, 컴파일) |
| [ResourceBinding.md](./ResourceBinding.md) | 셰이더 리소스 바인딩 (Constant/Texture/UAV/Sampler) |

## 학습 순서

1. **셰이더 기초** — 이 문서
2. **리소스 바인딩** — [ResourceBinding.md](./ResourceBinding.md)

---

## 목차

- [개요](#개요)
- [왜 필요한가?](#왜-필요한가)
- [주요 셰이더 종류](#주요-셰이더-종류)
- [HLSL 기본 구조](#hlsl-기본-구조)
- [런타임 셰이더 컴파일](#런타임-셰이더-컴파일)
- [주의사항](#주의사항)
- [관련 개념](#관련-개념)
- [참고 자료](#참고-자료)

## 개요
셰이더는 GPU에서 실행되는 프로그램으로, HLSL(High Level Shading Language)로 작성한다. DX12에서는 런타임에 D3DCompile API로 컴파일하거나 사전 컴파일된 바이너리(`.cso`)를 사용한다.

## 왜 필요한가?

### 문제
고정 파이프라인(Fixed Function Pipeline, DX7 이전)은 조명·텍스처 적용 방식이 GPU에 내장되어 커스터마이징이 불가능했다.

### 해결책
셰이더를 사용하면 정점 변환, 픽셀 색상 계산 등 파이프라인의 각 단계를 자유롭게 프로그래밍할 수 있다.

## 주요 셰이더 종류

| 셰이더 | 진입점 (관례) | 실행 단위 | Phase 1 사용 |
|---|---|---|---|
| Vertex Shader (VS) | `VSMain` | 정점 1개 | ✅ |
| Pixel Shader (PS) | `PSMain` | 픽셀 1개 | ✅ |
| Geometry Shader (GS) | `GSMain` | 프리미티브 1개 | ❌ |
| Hull/Domain Shader | `HSMain`/`DSMain` | 테셀레이션 | ❌ |
| Compute Shader (CS) | `CSMain` | 스레드 그룹 | ❌ (Phase 4+) |
| Mesh Shader | `MSMain` | 메시렛 | ❌ (Phase 4+) |

## HLSL 기본 구조

### 시맨틱 (Semantics)
시맨틱은 셰이더 입출력 변수의 의미를 GPU에게 알려주는 레이블이다.

| 시맨틱 | 방향 | 의미 |
|---|---|---|
| `POSITION` | VS 입력 | 정점 위치 (C++ Input Layout과 매핑) |
| `SV_POSITION` | VS 출력 | 클립 공간 위치 (시스템 값, GPU가 자동 처리) |
| `COLOR` | VS 입출력 | 색상 데이터 (사용자 정의) |
| `SV_TARGET` | PS 출력 | 렌더 타겟에 쓸 최종 색상 (시스템 값) |

> `SV_` 접두사가 붙은 시맨틱은 **시스템 값(System Value)** 으로 GPU가 특별하게 처리한다.

### Phase 1 삼각형 셰이더 (`Shaders/Triangle.hlsl`)

```hlsl
struct VSInput
{
    float3 position : POSITION;
    float4 color    : COLOR;
};

struct VSOutput
{
    float4 position : SV_POSITION;  // 클립 공간 위치
    float4 color    : COLOR;
};

// Vertex Shader: 정점 위치와 색상을 그대로 전달
VSOutput VSMain(VSInput input)
{
    VSOutput output;
    output.position = float4(input.position, 1.0f);  // w=1: 원근 분할 없음
    output.color    = input.color;
    return output;
}

// Pixel Shader: 보간된 색상을 렌더 타겟에 출력
float4 PSMain(VSOutput input) : SV_TARGET
{
    return input.color;
}
```

> Phase 1 삼각형은 변환 행렬(MVP)이 없으므로 정점이 NDC 좌표를 그대로 가진다. `float4(pos, 1.0f)`의 `w=1`은 원근 나누기(perspective division) 후 좌표가 변하지 않음을 의미한다.

## 런타임 셰이더 컴파일

### D3DCompileFromFile
HLSL 파일을 런타임에 컴파일해 `ID3DBlob`(바이너리 데이터 버퍼)을 반환한다.

```cpp
ComPtr<ID3DBlob> shaderBlob;
ComPtr<ID3DBlob> errorBlob;

HRESULT hr = D3DCompileFromFile(
    L"Shaders/Triangle.hlsl",    // 파일 경로 (실행 디렉터리 기준)
    nullptr,                      // 매크로 정의
    D3D_COMPILE_STANDARD_FILE_INCLUDE, // #include 지원
    "VSMain",                     // 진입점 함수명
    "vs_5_0",                     // 셰이더 모델 (Shader Model 5.0)
    D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // Debug 빌드용 플래그
    0,
    &shaderBlob,
    &errorBlob);
```

### 셰이더 모델 (Shader Model)
| 모델 | VS | PS | 최소 Feature Level |
|---|---|---|---|
| `5_0` | `vs_5_0` | `ps_5_0` | D3D_FEATURE_LEVEL_11_0 |
| `5_1` | `vs_5_1` | `ps_5_1` | D3D_FEATURE_LEVEL_11_1 |
| `6_0` | `vs_6_0` | `ps_6_0` | D3D_FEATURE_LEVEL_12_0 (DXIL 필요) |

> 이 프로젝트는 Phase 1에서 `vs_5_0` / `ps_5_0`을 사용한다. 향후 고급 기능(Mesh Shader 등)에서는 Shader Model 6.x와 DXC 컴파일러가 필요하다.

### 컴파일 플래그
| 플래그 | 설명 | 용도 |
|---|---|---|
| `D3DCOMPILE_DEBUG` | 디버그 정보 포함 | Debug 빌드 |
| `D3DCOMPILE_SKIP_OPTIMIZATION` | 최적화 생략 (빠른 컴파일) | Debug 빌드 |
| `D3DCOMPILE_OPTIMIZATION_LEVEL3` | 최대 최적화 | Release 빌드 |

### PSO에 셰이더 등록

```cpp
D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
psoDesc.VS = { shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize() };
psoDesc.PS = { psBlob->GetBufferPointer(),     psBlob->GetBufferSize() };
```

## 주의사항
- ⚠️ 셰이더 컴파일 실패 시 `errorBlob`에 오류 메시지가 담긴다. 반드시 로그에 출력한다.
- ⚠️ `D3DCompileFromFile`의 파일 경로는 **실행 파일의 현재 작업 디렉터리** 기준이다. Visual Studio에서는 프로젝트 디렉터리가 기본값이다.
- ⚠️ Release 빌드에서는 `D3DCOMPILE_DEBUG`와 `D3DCOMPILE_SKIP_OPTIMIZATION`을 제거해야 한다.
- ⚠️ 사전 컴파일(.cso)을 사용하면 배포 시 컴파일 도구 없이도 실행 가능하다. 단, 소스 수정 시 재컴파일이 필요하다. *(TODO: Phase 2에서 빌드 시스템과 통합 고려)*

## 관련 개념

### 선행 개념
- [그래픽스 파이프라인](../Pipeline/GraphicsPipeline.md)

### 연관 개념
- [정점 처리](../Pipeline/VertexProcessing.md)
- [리소스 바인딩](./ResourceBinding.md)

### 후속 개념
- Constant Buffer / 행렬 변환 *(TODO: Phase 2)*
- Texture Sampling *(TODO: Phase 2)*
- Compute Shader *(TODO: Phase 4+)*
- Mesh Shader / DXIL *(TODO: Phase 4)*

## 참고 자료
- [Microsoft: Compiling shaders](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-part1)
- [Microsoft: D3DCompileFromFile](https://learn.microsoft.com/en-us/windows/win32/api/d3dcompiler/nf-d3dcompiler-d3dcompilefromfile)
- [Microsoft: HLSL Semantics](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-semantics)
- [Microsoft: Shader Model 5](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/d3d11-graphics-reference-sm5)
