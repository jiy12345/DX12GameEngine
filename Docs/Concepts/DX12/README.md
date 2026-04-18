# DirectX 12 개념

DirectX 12의 핵심 개념들을 주제별 폴더로 정리했습니다. 각 하위 폴더의 `README.md`에 해당 주제의 개요와 구성 요소 관계가 정리되어 있습니다.

## 📑 문서 목록

### Core (기초)
- Overview.md - DX12 전체 개요 (DX11과의 차이) **(미작성)**
- Device.md - 디바이스 개념 **(미작성)**

### Commands (커맨드 시스템) — [📖 폴더 개요](./Commands/README.md)
- [CommandQueue.md](./Commands/CommandQueue.md) - 커맨드 큐 개념 ([#8](https://github.com/jiy12345/DX12GameEngine/issues/8))
- [CommandList.md](./Commands/CommandList.md) - 커맨드 리스트 개념 ([#8](https://github.com/jiy12345/DX12GameEngine/issues/8))
- [CommandAllocator.md](./Commands/CommandAllocator.md) - 커맨드 할당자 개념 ([#8](https://github.com/jiy12345/DX12GameEngine/issues/8))

### Pipeline (파이프라인 구성)
- PipelineStateObject.md - PSO 개념 **(미작성)** ([#14](https://github.com/jiy12345/DX12GameEngine/issues/14))
- RootSignature.md - 루트 시그니처 개념 **(미작성)** ([#14](https://github.com/jiy12345/DX12GameEngine/issues/14))

### Resources (리소스 관리)
- [MemoryHeaps.md](./Resources/MemoryHeaps.md) - 메모리 힙 타입 (UPLOAD/DEFAULT/READBACK) ([#20](https://github.com/jiy12345/DX12GameEngine/issues/20))
- [ConstantBuffer.md](./Resources/ConstantBuffer.md) - 상수 버퍼 ([#20](https://github.com/jiy12345/DX12GameEngine/issues/20))
- ResourceBarriers.md - 리소스 배리어와 상태 전환 **(미작성)** ([#21](https://github.com/jiy12345/DX12GameEngine/issues/21))

### Descriptors (디스크립터)
- DescriptorHeaps.md - 디스크립터 힙 개념 **(미작성)** ([#10](https://github.com/jiy12345/DX12GameEngine/issues/10))
- Descriptors.md - 디스크립터 타입 (RTV/DSV/CBV/SRV/UAV) **(미작성)**

### Display (디스플레이)
- [SwapChain.md](./Display/SwapChain.md) - 스왑체인 개념 ([#9](https://github.com/jiy12345/DX12GameEngine/issues/9))

### Synchronization (동기화)
- Synchronization.md - 동기화 (Fence) 개념 **(미작성)** ([#12](https://github.com/jiy12345/DX12GameEngine/issues/12))

### Debugging (디버깅)
- [DebugLayer.md](./Debugging/DebugLayer.md) - Debug Layer 개념 ([#7](https://github.com/jiy12345/DX12GameEngine/issues/7))

## 학습 순서 권장

1. **기초**: Overview → Device → DebugLayer (Debugging/) → SwapChain
2. **커맨드 시스템**: [Commands/README.md](./Commands/README.md) → CommandQueue → CommandAllocator → CommandList
3. **리소스**: DescriptorHeaps → Descriptors → MemoryHeaps → ResourceBarriers
4. **파이프라인**: RootSignature → PipelineStateObject
5. **동기화**: Synchronization
6. **버퍼 활용**: ConstantBuffer
