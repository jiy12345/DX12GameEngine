# 렌더링 개념

그래픽스 렌더링과 관련된 **API 독립적** 개념들을 다룹니다. 각 주제별 하위 폴더의 `README.md`에 개요와 구성 요소 관계가 정리되어 있습니다.

## 📑 문서 목록

### Pipeline (파이프라인 흐름 및 단계) — [📖 폴더 개요](./Pipeline/README.md)
- RenderLoop.md - CPU 측 프레임 실행 흐름 **(미작성)** ([#13](https://github.com/jiy12345/DX12GameEngine/issues/13))
- [GraphicsPipeline.md](./Pipeline/GraphicsPipeline.md) - 파이프라인 단계 전체 개관 ([#14](https://github.com/jiy12345/DX12GameEngine/issues/14))
- [VertexProcessing.md](./Pipeline/VertexProcessing.md) - Vertex Buffer, Input Layout ([#14](https://github.com/jiy12345/DX12GameEngine/issues/14))
- Rasterization.md - 래스터화 단계 **(미작성)**

### Shaders (셰이더 프로그래밍) — [📖 폴더 개요](./Shaders/README.md)
- [README.md](./Shaders/README.md) - 셰이더 기초 (종류, HLSL, 컴파일) ([#14](https://github.com/jiy12345/DX12GameEngine/issues/14))
- [ResourceBinding.md](./Shaders/ResourceBinding.md) - 셰이더 리소스 바인딩 (Constant/Texture/UAV/Sampler) ([#20](https://github.com/jiy12345/DX12GameEngine/issues/20))

### Techniques (렌더링 기법) — [📖 폴더 개요](./Techniques/README.md)
- [WaterSimulation.md](./Techniques/WaterSimulation.md) - Gerstner Wave 기반 물 시뮬레이션 ([#41](https://github.com/jiy12345/DX12GameEngine/issues/41))

## 학습 순서 권장

1. **전체 흐름**: [Pipeline/README.md](./Pipeline/README.md) → GraphicsPipeline → RenderLoop
2. **파이프라인 단계**: VertexProcessing → Rasterization
3. **프로그래밍**: [Shaders/README.md](./Shaders/README.md) → ResourceBinding
4. **응용 기법**: [Techniques/README.md](./Techniques/README.md) → 각 기법

## 관련 카테고리

- [DX12/](../DX12/README.md) — DX12 API 고유 구현
- [Platform/](../Platform/README.md) — 플랫폼 (윈도우/입력)
