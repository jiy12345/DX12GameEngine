# Concepts - 개념적인 설명

"이것이 무엇인가? 왜 필요한가?"

이 섹션에서는 DX12 게임 엔진의 핵심 개념들을 다룹니다. 각 개념의 정의, 목적, 동작 원리를 이해할 수 있습니다.

## 📑 문서 목록

### DX12/ - DirectX 12 개념
- [README.md](./DX12/README.md) - DX12 개념 인덱스
- Overview.md - DX12 전체 개요 (DX11과의 차이) **(미작성)**
- Device.md - 디바이스 개념 **(미작성)**
- [CommandQueue.md](./DX12/CommandQueue.md) - 커맨드 큐 개념
- [CommandList.md](./DX12/CommandList.md) - 커맨드 리스트 개념
- [CommandAllocator.md](./DX12/CommandAllocator.md) - 커맨드 할당자 개념
- DescriptorHeaps.md - 디스크립터 힙 개념 **(미작성)**
- Descriptors.md - 디스크립터 타입 (RTV/DSV/CBV/SRV/UAV) **(미작성)**
- PipelineStateObject.md - PSO 개념 **(미작성)**
- RootSignature.md - 루트 시그니처 개념 **(미작성)**
- ResourceBarriers.md - 리소스 배리어와 상태 전환 **(미작성)**
- Synchronization.md - 동기화 (Fence) 개념 **(미작성)**
- [SwapChain.md](./DX12/SwapChain.md) - 스왑체인 개념
- MemoryHeaps.md - 메모리 힙 타입 **(미작성)**

### Rendering/ - 렌더링 개념
- [README.md](./Rendering/README.md) - 렌더링 개념 인덱스
- RenderLoop.md - 렌더링 루프 개념 **(미작성)**
- [GraphicsPipeline.md](./Rendering/GraphicsPipeline.md) - 그래픽스 파이프라인 단계
- [VertexProcessing.md](./Rendering/VertexProcessing.md) - 정점 처리
- Rasterization.md - 래스터화 **(미작성)**
- [Shaders.md](./Rendering/Shaders.md) - 셰이더 개념
- ResourceBinding.md - 리소스 바인딩 **(미작성)**

### Platform/ - 플랫폼 개념
- [README.md](./Platform/README.md) - 플랫폼 개념 인덱스
- [WindowSystem.md](./Platform/WindowSystem.md) - 윈도우 시스템 개념
- [MessageLoop.md](./Platform/MessageLoop.md) - 메시지 루프 개념
- [InputHandling.md](./Platform/InputHandling.md) - 입력 처리 개념

## 🔗 다른 카테고리와의 관계

- **[Structure](../Structure/)**: 개념들이 어떻게 구조적으로 연결되는지
- **[Usage](../Usage/)**: 개념을 실제로 어떻게 사용하는지
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
