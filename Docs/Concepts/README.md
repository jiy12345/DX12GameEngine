# Concepts - 개념적인 설명

"이것이 무엇인가? 왜 필요한가?"

이 섹션에서는 DX12 게임 엔진의 핵심 개념들을 다룹니다. 각 개념의 정의, 목적, 동작 원리를 이해할 수 있습니다. **각 카테고리는 주제별 하위 폴더**로 구성되어 있으며, 각 폴더의 `README.md`에 해당 주제의 개요와 구성 요소 관계가 정리되어 있습니다.

## 📚 카테고리

- **[DX12/](./DX12/README.md)** — DirectX 12 API 고유 개념
- **[Rendering/](./Rendering/README.md)** — API 독립적 그래픽스 렌더링 개념
- **[Platform/](./Platform/README.md)** — 플랫폼별(OS) 시스템 개념

> 개념을 **DX12에 둘지 Rendering에 둘지** 판단하려면:
> "이 개념이 DX12 API가 없으면 존재하지 않는가?" → YES면 `DX12/`, NO면 `Rendering/ + Usage/`.

---

## 📑 DX12/ - DirectX 12 개념

전체 인덱스: [DX12/README.md](./DX12/README.md)

### Core (기초)
- Overview.md - DX12 전체 개요 (DX11과의 차이) **(미작성)**
- Device.md - 디바이스 개념 **(미작성)**

### Commands (커맨드 시스템) — [📖 폴더 개요](./DX12/Commands/README.md)
- [CommandQueue.md](./DX12/Commands/CommandQueue.md) - 커맨드 큐 개념 ([#8](https://github.com/jiy12345/DX12GameEngine/issues/8))
- [CommandList.md](./DX12/Commands/CommandList.md) - 커맨드 리스트 개념 ([#8](https://github.com/jiy12345/DX12GameEngine/issues/8))
- [CommandAllocator.md](./DX12/Commands/CommandAllocator.md) - 커맨드 할당자 개념 ([#8](https://github.com/jiy12345/DX12GameEngine/issues/8))

### Pipeline (파이프라인 구성)
- [PipelineStateObject.md](./DX12/Pipeline/PipelineStateObject.md) - PSO 개념 ([#14](https://github.com/jiy12345/DX12GameEngine/issues/14))
- RootSignature.md - 루트 시그니처 개념 **(미작성)** ([#14](https://github.com/jiy12345/DX12GameEngine/issues/14))

### Resources (리소스 관리)
- [MemoryHeaps.md](./DX12/Resources/MemoryHeaps.md) - 메모리 힙 타입 (UPLOAD/DEFAULT/READBACK) ([#20](https://github.com/jiy12345/DX12GameEngine/issues/20))
- [ResourceBarriers.md](./DX12/Resources/ResourceBarriers.md) - 리소스 배리어와 상태 전환 ([#21](https://github.com/jiy12345/DX12GameEngine/issues/21))

### Descriptors (디스크립터)
- DescriptorHeaps.md - 디스크립터 힙 개념 **(미작성)** ([#10](https://github.com/jiy12345/DX12GameEngine/issues/10))
- Descriptors.md - 디스크립터 타입 (RTV/DSV/CBV/SRV/UAV) **(미작성)**

### Display (디스플레이)
- [SwapChain.md](./DX12/Display/SwapChain.md) - 스왑체인 개념 ([#9](https://github.com/jiy12345/DX12GameEngine/issues/9))

### Synchronization (동기화)
- Synchronization.md - 동기화 (Fence) 개념 **(미작성)** ([#12](https://github.com/jiy12345/DX12GameEngine/issues/12))

### Debugging (디버깅)
- [DebugLayer.md](./DX12/Debugging/DebugLayer.md) - Debug Layer 개념 ([#7](https://github.com/jiy12345/DX12GameEngine/issues/7))

---

## 📑 Rendering/ - 렌더링 개념 (API 독립)

전체 인덱스: [Rendering/README.md](./Rendering/README.md)

### Pipeline (파이프라인 흐름 및 단계) — [📖 폴더 개요](./Rendering/Pipeline/README.md)
- RenderLoop.md - CPU 측 프레임 실행 흐름 **(미작성)** ([#13](https://github.com/jiy12345/DX12GameEngine/issues/13))
- [GraphicsPipeline.md](./Rendering/Pipeline/GraphicsPipeline.md) - 파이프라인 단계 전체 개관 ([#14](https://github.com/jiy12345/DX12GameEngine/issues/14))
- [VertexProcessing.md](./Rendering/Pipeline/VertexProcessing.md) - Vertex Buffer, Input Layout ([#14](https://github.com/jiy12345/DX12GameEngine/issues/14))
- Rasterization.md - 래스터화 단계 **(미작성)**

### Shaders (셰이더 프로그래밍) — [📖 폴더 개요](./Rendering/Shaders/README.md)
- [README.md](./Rendering/Shaders/README.md) - 셰이더 기초 (종류, HLSL, 컴파일) ([#14](https://github.com/jiy12345/DX12GameEngine/issues/14))
- [ResourceBinding.md](./Rendering/Shaders/ResourceBinding.md) - 셰이더 리소스 바인딩 (Constant/Texture/UAV/Sampler) ([#20](https://github.com/jiy12345/DX12GameEngine/issues/20))

### Techniques (렌더링 기법) — [📖 폴더 개요](./Rendering/Techniques/README.md)
- [WaterSimulation.md](./Rendering/Techniques/WaterSimulation.md) - Gerstner Wave 물 시뮬레이션 ([#41](https://github.com/jiy12345/DX12GameEngine/issues/41))

---

## 📑 Platform/ - 플랫폼 개념

전체 인덱스: [Platform/README.md](./Platform/README.md)

### Windows (Win32 API) — [📖 폴더 개요](./Platform/Windows/README.md)
- [WindowSystem.md](./Platform/Windows/WindowSystem.md) - 윈도우 시스템 개념 ([#6](https://github.com/jiy12345/DX12GameEngine/issues/6))
- [MessageLoop.md](./Platform/Windows/MessageLoop.md) - 메시지 루프 개념 ([#6](https://github.com/jiy12345/DX12GameEngine/issues/6))
- [InputHandling.md](./Platform/Windows/InputHandling.md) - 입력 처리 개념 ([#6](https://github.com/jiy12345/DX12GameEngine/issues/6))

---

## 🔗 다른 카테고리와의 관계

- **[Structure](../Structure/)**: 개념들이 어떻게 구조적으로 연결되는지
- **[Usage](../Usage/)**: 개념을 실제로 어떻게 사용하는지 (DX12 구현 walkthrough 포함)
- **[Analysis](../Analysis/)**: 개념에 대한 성능 분석 및 테스트

## 📝 문서 작성 가이드

Concepts 문서는 다음을 포함해야 합니다:
1. 간단한 정의 (한 문장)
2. 왜 필요한가? (문제와 해결책)
3. 상세 설명
4. 주요 특징
5. 관련 API
6. 코드 예제
7. 주의사항
8. 관련 개념 링크

템플릿: [ConceptTemplate.md](../Templates/ConceptTemplate.md)
