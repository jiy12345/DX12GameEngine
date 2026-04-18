# 렌더링 기법 (Techniques) — 폴더 개요

이 폴더는 **특정 시각 효과나 렌더링 알고리즘**에 대한 개념을 다룹니다.

## 📑 이 폴더의 문서

| 문서 | 상태 | 내용 |
|------|------|------|
| [WaterSimulation.md](./WaterSimulation.md) | ✅ [#41](https://github.com/jiy12345/DX12GameEngine/issues/41) | Gerstner Wave 기반 물 시뮬레이션 |

## 개요

`Pipeline/`과 `Shaders/`가 그래픽스 파이프라인의 **일반적 구성 요소**를 다룬다면, `Techniques/`는 그것들을 조합해 만드는 **특정 효과/기법**을 다룹니다.

예시:
- 물 시뮬레이션 (Gerstner Wave, FFT Ocean)
- 파티클 시스템
- 후처리 효과 (Bloom, DOF, Tone Mapping)
- 그림자 기법 (Shadow Map, SSAO)
- 전역 조명 (IBL, Lightmap)

## 확장 예정

현재는 WaterSimulation.md 하나지만, 프로젝트 진행에 따라 다양한 기법이 추가될 예정입니다:

- Phase 3+: 후처리, 그림자, 조명
- Phase 4+: DXR (Raytracing), Mesh Shader 기반 기법

## 관련 폴더

- [Pipeline/](../Pipeline/README.md) — 파이프라인 기본 이해 (기법은 이 위에 올라감)
- [Shaders/](../Shaders/README.md) — 셰이더 프로그래밍 (기법 구현의 기본)
