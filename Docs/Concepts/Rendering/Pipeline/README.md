# 그래픽스 파이프라인 (Pipeline) — 폴더 개요

이 폴더는 **GPU 그래픽스 파이프라인의 흐름과 각 단계**에 대한 개념을 다룹니다.

## 📑 이 폴더의 문서

| 문서 | 상태 | 내용 |
|------|------|------|
| RenderLoop.md | 미작성 ([#13](https://github.com/jiy12345/DX12GameEngine/issues/13)) | CPU 측 프레임 실행 흐름 |
| [GraphicsPipeline.md](./GraphicsPipeline.md) | ✅ [#14](https://github.com/jiy12345/DX12GameEngine/issues/14) | 파이프라인 단계 전체 개관, PSO/Root Signature |
| [VertexProcessing.md](./VertexProcessing.md) | ✅ [#14](https://github.com/jiy12345/DX12GameEngine/issues/14) | Vertex Buffer, Input Layout |
| Rasterization.md | 미작성 | 래스터화 단계 |

## 개요

그래픽스 파이프라인은 GPU가 **3D 장면 데이터를 2D 픽셀로 변환**하는 일련의 처리 단계입니다. 각 단계는 하드웨어 또는 프로그래머블 셰이더로 구현되며, 전체 흐름을 이해하면 렌더링의 어느 지점에서 무엇이 일어나는지 파악할 수 있습니다.

### 전체 흐름

```
[CPU]
  │
  ├─ RenderLoop: 매 프레임 어떤 일을 하는가
  │     ↓
  │  Command List에 명령 기록 (Draw, SetPSO, ...)
  │     ↓
  │  Command Queue에 제출
  │
[GPU]
  │
  ├─ Input Assembler: Vertex/Index Buffer 읽기
  ├─ Vertex Shader: 정점 변환 (VertexProcessing)
  ├─ [Optional: Hull/Domain/Geometry Shader]
  ├─ Rasterizer: 삼각형을 픽셀로 변환 (Rasterization)
  ├─ Pixel Shader: 픽셀 색상 계산
  ├─ Output Merger: Depth/Stencil 테스트, 블렌딩
  │     ↓
  │  Render Target 쓰기
```

## 학습 순서 권장

1. **전체 흐름 이해**: [GraphicsPipeline.md](./GraphicsPipeline.md)
2. **CPU 측 실행 구조**: RenderLoop.md (미작성)
3. **각 단계 상세**:
   - [VertexProcessing.md](./VertexProcessing.md) — 정점 입력과 처리
   - Rasterization.md (미작성) — 픽셀로 변환
4. **프로그래밍 관점**: [../Shaders/](../Shaders/README.md) — 각 단계를 셰이더로 작성

## 관련 폴더

- [Shaders/](../Shaders/README.md) — 셰이더 프로그래밍
- [Techniques/](../Techniques/README.md) — 특정 렌더링 기법
- [../DX12/Pipeline/](../../DX12/Pipeline/) — DX12 특화 (PSO, Root Signature)
