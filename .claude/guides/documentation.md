# 문서화 가이드

**적용 시점**:
- `Docs/` 폴더의 문서를 작성/수정할 때
- 기능 구현 전 개념 문서를 준비할 때
- README 인덱스를 업데이트할 때

---

## 문서 구조

```
Docs/
├── Structure/    # 구조적인 설명 (아키텍처, 시스템 구조)
├── Concepts/     # 개념적인 설명 (각 개념의 정의와 원리)
├── Usage/        # 활용법 (How-to 가이드, 구현 방법)
├── Analysis/     # 분석 및 테스트 (벤치마크, 성능 분석)
└── Templates/    # 문서 템플릿
```

---

## Concepts/DX12/ vs Concepts/Rendering/ 구분 (중요)

개념 문서를 작성할 때 **폴더 선택이 자주 헷갈립니다.** 판별 기준:

### 판별 질문

> **"이 개념이 DX12 API가 없으면 존재하지 않는가?"**

- **YES** → `Concepts/DX12/` (DX12가 독자적으로 제공하는 객체/개념)
- **NO** (다른 그래픽스 API에도 있는 보편 개념) → `Concepts/Rendering/` (개념) + `Usage/` (DX12 구현)

### DX12 고유 기능 → `Concepts/DX12/`

| 예시 | 근거 |
|------|------|
| CommandQueue, CommandList, CommandAllocator | DX12 API 객체 |
| Root Signature | DX12 고유 개념 |
| Pipeline State Object (PSO) | DX12 고유 통합 객체 |
| Resource Barriers | DX12의 명시적 배리어 API |
| Descriptor Heap | DX12의 디스크립터 힙 구조 |
| Memory Heaps (UPLOAD/DEFAULT/READBACK) | DX12 힙 타입 |
| SwapChain (DXGI) | DX12와 짝을 이루는 DXGI 객체 |
| Fence (ID3D12Fence) | DX12 동기화 API |

### 범용 그래픽스 개념 → `Concepts/Rendering/` + `Usage/`

| 예시 | 근거 |
|------|------|
| Constant Buffer / Uniform Buffer | 모든 API에 존재 (UBO, Push Constants 등) |
| Vertex Buffer, Index Buffer | 모든 API에 존재 |
| Texture / Sampled Resource | 모든 API에 존재 |
| Sampler | 모든 API에 존재 |
| 셰이더 리소스 바인딩 (전반) | API마다 방식 다르지만 개념 동일 |
| 래스터화, 블렌딩 | GPU 파이프라인 공통 |

범용 개념의 경우:
- **개념 설명** → `Concepts/Rendering/XXX.md` (API 독립 관점)
- **DX12 구현** → `Usage/BasicTasks/UsingXXX.md` (코드 walkthrough)
- **`Concepts/DX12/`에는 두지 않음** (애매한 이중 관리 피함)

### 왜 이렇게 구분하는가

- **학습 효율**: "DX12 고유"와 "그래픽스 보편"을 섞지 않으면 Vulkan/Metal 전이 학습 쉬움
- **중복 제거**: 같은 개념을 DX12/에도 Rendering/에도 쓰면 동기화 문제 발생
- **명확한 역할**: DX12/는 "DX12가 제공하는 것", Rendering/은 "그래픽스에서 일어나는 것", Usage/는 "DX12로 어떻게 만드는가"

---

## 하위 폴더 조직 원칙

Concepts, Structure, Usage 등 각 카테고리 내에서 **묶일 만한 소주제가 생기면 하위 폴더로 분리**합니다.

### 하위 폴더 분리 기준

다음 조건 중 하나 이상에 해당하면 하위 폴더로 분리합니다:

- 같은 주제로 **3개 이상의 문서**가 모일 것으로 예상됨
- 주제 간 **상호 참조가 빈번**함 (서로 링크가 많음)
- 학습 시 **묶어서 이해해야** 하는 주제군 (예: CommandQueue / CommandList / CommandAllocator)

### 폴더별 README.md 필수

하위 폴더를 만들면 **반드시 `README.md`를 배치**합니다. README.md는 다음을 포함합니다:

1. **해당 주제의 개요** - 왜 이 폴더가 묶여 있는지, 어떤 개념들이 들어 있는지
2. **구성 요소 간의 관계** - 내부 문서들이 어떻게 연결되는지 (필요 시 다이어그램)
3. **학습 순서 권장** - 어떤 순서로 읽는 것이 좋은지
4. **폴더 내 문서 목록 및 링크** - 각 문서로의 빠른 이동

폴더 README.md는 "목차 페이지 + 개요 문서"를 겸합니다. 단순 인덱스가 아니라 **폴더 전체를 설명하는 문서**여야 합니다.

### DX12 Concepts 폴더 예시

```
Docs/Concepts/DX12/
├── README.md               # 전체 인덱스 (폴더별 그룹화)
├── Core/                   # 기초 (Device, 전체 개요)
├── Commands/               # 커맨드 시스템
│   ├── README.md           # ← 커맨드 시스템 전체 개요 + 구성 요소 관계
│   ├── CommandQueue.md
│   ├── CommandList.md
│   └── CommandAllocator.md
├── Pipeline/               # 파이프라인 구성
│   ├── PipelineStateObject.md
│   └── RootSignature.md
├── Resources/              # 리소스 관리
│   ├── MemoryHeaps.md
│   ├── ConstantBuffer.md
│   └── ResourceBarriers.md
├── Descriptors/            # 디스크립터 시스템
├── Display/                # 디스플레이
├── Synchronization/        # 동기화
└── Debugging/              # 디버깅 도구 (Debug/는 Visual Studio 빌드 출력과 충돌하므로 사용 금지)
```

### 상위 README의 인덱스 규칙

상위 폴더의 README.md는 **폴더별 소제목(h3)으로 문서 목록을 그룹화**합니다:

```markdown
## 📑 문서 목록

### Commands (커맨드 시스템) — [📖 폴더 개요](./Commands/README.md)

- [CommandQueue.md](./Commands/CommandQueue.md) - 커맨드 큐 개념
- [CommandList.md](./Commands/CommandList.md) - 커맨드 리스트 개념
...

### Resources (리소스 관리)

- [MemoryHeaps.md](./Resources/MemoryHeaps.md) - ...
...
```

### 링크 경로 규칙

하위 폴더로 분리 후에는 상대 경로를 정확히 유지합니다:

- 같은 폴더 내: `./Sibling.md`
- 상위 → 하위: `./Subfolder/File.md`
- 하위 → 상위: `../File.md`
- 하위 → 다른 하위: `../OtherFolder/File.md`

폴더 구조 변경 시 **모든 기존 문서의 링크를 함께 업데이트**해야 합니다.

---

## 문서 작성 원칙

### 목차는 필수

**매우 중요**: 모든 개념 문서에는 반드시 **목차(`## 목차`)** 를 제목 바로 아래에 작성합니다.

- `## 목차` 섹션을 문서 최상단(제목 다음)에 배치
- `##` 레벨 항목은 모두 포함, `###` 레벨은 중요한 것만 선택적 포함
- GitHub Markdown 앵커 링크 형식 사용: 소문자, 공백→하이픈, 특수문자 제거
- 템플릿(`Docs/Templates/ConceptTemplate.md`)에 목차 구조 포함되어 있음

### 근거 기반 작성

**매우 중요**: 문서는 반드시 다음 근거를 바탕으로만 작성합니다.

#### 1. 참고 자료 근거
- 공식 문서 (Microsoft DirectX 문서 등)
- 검증된 논문이나 아티클
- 신뢰할 수 있는 기술 자료
- **근거 없는 추측이나 가정을 작성하지 않습니다**
- **출처 URL을 반드시 명시** (예: `[Microsoft: 문서명](URL)`)

#### 2. 구현된 코드 근거
- 실제로 이 프로젝트에 구현된 코드
- 실행하고 테스트한 코드
- **구현되지 않은 내용은 "TODO" 또는 "계획"으로 명시**

### 주요 참고 자료 (DX12 관련)

- **DirectX 12 Programming Guide**: https://learn.microsoft.com/en-us/windows/win32/direct3d12/
- **DXGI Overview**: https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/
- **DirectX Blog**: https://devblogs.microsoft.com/directx/
- **DirectX-Graphics-Samples**: https://github.com/microsoft/DirectX-Graphics-Samples

---

## 작업 시 문서 업데이트 규칙

### ⚠️ 필수 규칙 1: 구현 전 문서 먼저 (PR 방식)

기능 구현을 시작하기 전에 반드시 관련 Concepts 문서를 작성 완료하고 **`docs/이슈번호` 브랜치에서 PR을 생성**해야 합니다.
사용자 리뷰 및 머지 후 구현 단계로 넘어갑니다. **코드보다 문서가 항상 선행됩니다.**

문서 PR 반복 리뷰 프로세스는 `.claude/guides/workflow.md`의 "문서 작업 PR 워크플로우" 참조.

### ⚠️ 필수 규칙 2: 문서에 이슈 링크 추가

문서 작성 및 README 인덱스 업데이트 시 반드시 관련 GitHub 이슈 링크를 명시합니다:

- README 인덱스: `[문서명](경로) - 설명 ([#이슈번호](https://github.com/jiy12345/DX12GameEngine/issues/이슈번호))`
- 미작성 문서: `문서명 - 설명 **(미작성)** ([#이슈번호](https://github.com/jiy12345/DX12GameEngine/issues/이슈번호))`
- 하나의 문서가 여러 이슈와 연관되면 모두 표기

### 이슈 작업 단계별 문서 업데이트

1. **구현 전** (필수):
   - 관련 Concepts 문서 작성 (없으면 새로 생성)
   - `docs/이슈번호` 브랜치로 PR 생성
   - 사용자 리뷰 및 머지 후 구현 진행
2. **구현 중**: 구조 문서 업데이트
3. **구현 후**:
   - Usage 가이드 작성 (실제 동작하는 코드 기반)
   - 필요시 Analysis 문서 추가 (벤치마크 결과)
   - 관련 문서 간 링크 연결
   - README 인덱스에서 해당 문서 **(미작성)** 표시 제거

---

## 문서 템플릿

각 카테고리별 템플릿을 `Docs/Templates/`에서 확인:

- `StructureTemplate.md`: 구조 문서 템플릿
- `ConceptTemplate.md`: 개념 문서 템플릿
- `UsageTemplate.md`: 활용법 문서 템플릿
- `AnalysisTemplate.md`: 분석 문서 템플릿

---

## 문서 간 링크

문서는 서로 연결되어야 합니다:

```markdown
# Concepts/DX12/Device.md (개념)
↓ 링크
# Structure/Graphics/RenderingPipeline.md (구조 속 위치)
↓ 링크
# Usage/BasicTasks/InitializingDevice.md (구현 방법)
↓ 링크
# Analysis/Performance/DeviceCreation.md (성능 분석)
```

---

## 관련 문서

- 문서 PR 워크플로우: `.claude/guides/workflow.md`
- DX12 관련 내용 작성 시 참고: `.claude/guides/dx12.md`
