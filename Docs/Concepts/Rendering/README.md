# 렌더링 개념

렌더링 파이프라인과 관련된 개념들을 다룹니다.

## 📑 문서 목록

| 문서 | 상태 | 내용 |
|---|---|---|
| RenderLoop.md | 미작성 | 렌더링 루프 개념 |
| [GraphicsPipeline.md](./GraphicsPipeline.md) | ✅ [#14](https://github.com/jiy12345/DX12GameEngine/issues/14) | PSO, Root Signature, 파이프라인 단계 |
| [VertexProcessing.md](./VertexProcessing.md) | ✅ [#14](https://github.com/jiy12345/DX12GameEngine/issues/14) | Vertex Buffer, Input Layout |
| [Shaders.md](./Shaders.md) | ✅ [#14](https://github.com/jiy12345/DX12GameEngine/issues/14) | HLSL 셰이더, 런타임 컴파일 |
| [WaterSimulation.md](./WaterSimulation.md) | ✅ [#38](https://github.com/jiy12345/DX12GameEngine/issues/38) | Gerstner Wave 기반 물 시뮬레이션 |
| Rasterization.md | 미작성 | 래스터화 |
| [ResourceBinding.md](./ResourceBinding.md) | ✅ [#20](https://github.com/jiy12345/DX12GameEngine/issues/20) | 셰이더 리소스 바인딩 보편 개념 (Constant/Texture/UAV/Sampler) |

## 학습 순서 권장

1. **전체 흐름**: RenderLoop → GraphicsPipeline
2. **파이프라인 단계**: VertexProcessing → Rasterization
3. **프로그래밍**: Shaders → ResourceBinding
