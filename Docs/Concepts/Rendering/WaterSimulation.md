# 물 시뮬레이션 (GPU Gems Ch.1 방식)

## 개요

주기적인 Gerstner Wave 함수들의 합으로 물 표면 형상과 외관을 실시간 렌더링하는 기법.
Compute Shader 없이 Vertex/Pixel Shader만으로 구현 가능하며, GPU Gems Chapter 1의 모델을 따른다.

## 핵심 알고리즘: Sum of Sines

물 표면 높이는 여러 개의 단순 주기 함수의 합으로 표현된다.

```
H(x, y, t) = Σ A_i * sin(D_i · (x, y) * w_i + t * φ_i)
```

### 웨이브 파라미터 (파당)
| 파라미터 | 설명 |
|---|---|
| Wavelength (L) | 마루~마루 거리 |
| Amplitude (A) | 기준선~마루 높이 |
| Speed (S) | 위상 상수로 변환 (`φ = 2π/L`) |
| Direction (D) | 파 진행 방향 (수평 벡터) |
| Steepness (Q) | Gerstner 파의 뾰족함 조절 (0=사인파, 1=뾰족) |

---

## 웨이브 타입

### 1. Geometric Waves (정점 변위용, ~4개)
메시 정점을 실제로 움직여 물 표면의 기복을 만든다.

**단순 사인파:**
```
P.y = A * sin(D · (P.x, P.z) * w + t * φ)
```

**Gerstner Wave** (더 현실적):
```
P.x += Q * A * D.x * cos(w * D · P + φ * t)
P.z += Q * A * D.y * cos(w * D · P + φ * t)
P.y  = A * sin(w * D · P + φ * t)
```
- 마루는 뾰족하고 골은 넓은 파형 (실제 수면에 가까움)
- Q가 너무 크면 루프(루핑) 아티팩트 발생 주의

### 2. Texture Waves (노멀맵용, ~15개)
실제 정점을 움직이지 않고, 노멀맵을 동적으로 생성해 세밀한 표면 질감을 표현한다.
- 동일한 파라미터 구조 사용, 텍스처 공간에서 연산
- 픽셀 셰이더에서 처리

---

## 렌더링 파이프라인

```
[CPU] Wave Params (Constant Buffer)
         ↓
[Vertex Shader]
  - Gerstner Wave 방정식으로 정점 변위
  - Binormal / Tangent / Normal (접선 공간) 계산 (편미분)
  - 깊이 기반 감쇠, 파장 필터링
         ↓
[Pixel Shader]
  - Texture Wave로 노멀맵 생성
  - Fresnel 계수 계산 (시선 각도 기반 반사율)
  - 환경맵 룩업 (반사)
  - 물 색상 + 반사 블렌딩
```

### Tangent Space 계산 (Vertex Shader)
Gerstner Wave의 편미분으로 접선 벡터를 구한다:
```
Binormal = (1 - Q*A*wx*Dx*sin(θ), A*Dx*cos(θ), -Q*A*wx*Dx*Dy*sin(θ))
Tangent  = (-Q*A*wx*Dx*Dy*sin(θ), A*Dy*cos(θ), 1 - Q*A*wx*Dy*sin(θ))
Normal   = cross(Binormal, Tangent)
```

### Fresnel 효과 (Pixel Shader)
시선과 법선의 각도에 따라 반사율이 달라지는 효과:
```hlsl
float fresnel = pow(1.0 - saturate(dot(viewDir, normal)), 5.0);
float4 color  = lerp(waterColor, envMapColor, fresnel);
```

---

## 성능 최적화

### 룩업 테이블
Texture Wave 연산 시 픽셀 셰이더에서 `cos()` 반복 호출 비용을 줄이기 위해
코사인 값을 사전 계산한 텍스처를 사용한다.

### 웨이브 수 제한
- Geometric: ~4개 (정점 셰이더 부하)
- Texture: ~15개 (픽셀 셰이더 부하)

---

## 구현에 필요한 선행 이슈

| 이슈 | 이유 |
|---|---|
| [#20 버퍼 관리 시스템](https://github.com/jiy12345/DX12GameEngine/issues/20) | Wave 파라미터 Constant Buffer, 정점 버퍼 |
| [#21 리소스 상태 추적](https://github.com/jiy12345/DX12GameEngine/issues/21) | 텍스처 리소스 상태 전환 |
| [#22 텍스처 로더](https://github.com/jiy12345/DX12GameEngine/issues/22) | 환경맵(Cube Map), 룩업 텍스처 |

---

## 구현 예정 이슈 (TODO)

- [ ] Water Grid Mesh 생성 (테셀레이션 가능한 평면 메시)
- [ ] Gerstner Wave Vertex Shader (정점 변위 + 접선 공간)
- [ ] Texture Wave Normal Map Pixel Shader (동적 노멀맵)
- [ ] Fresnel + 환경 반사 Pixel Shader
- [ ] 물 렌더링 샘플 통합

---

## 참고 자료

- [GPU Gems Chapter 1: Effective Water Simulation from Physical Models](https://developer.nvidia.com/gpugems/gpugems/part-i-natural-effects/chapter-1-effective-water-simulation-physical-models)
