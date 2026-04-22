# 개발 워크플로우 가이드

**적용 시점**: 커밋, 브랜치 생성, PR 생성 등 Git 관련 모든 작업

---

## 빌드 시스템

**주의**: 빌드 시스템이 아직 최종 결정되지 않았습니다. Visual Studio 솔루션 구성이 필요합니다(ISSUES.md #1 참조).

예상 빌드 명령어 (구성 후):
```bash
# Debug 빌드
msbuild DX12GameEngine.sln /p:Configuration=Debug

# Release 빌드
msbuild DX12GameEngine.sln /p:Configuration=Release

# Profile 빌드 (성능 측정용)
msbuild DX12GameEngine.sln /p:Configuration=Profile
```

---

## 브랜치 전략

- `main`: 안정적인 릴리스 브랜치 (**직접 커밋 금지 - 모든 작업은 PR 필수**)
- `feature/이슈번호`: 새로운 기능 개발
- `bugfix/이슈번호`: 버그 수정
- `optimize/이슈번호`: 최적화 작업
- `docs/이슈번호`: 문서 작업 (`Docs/` 폴더)

**⚠️ 중요**: 문서 작업(`#38` 포함)도 예외 없이 PR을 통해 진행합니다.

### 예외: `.claude/` 하위 문서
- `.claude/CLAUDE.md`, `.claude/guides/**` 수정은 **PR 없이 `main`에 직접 커밋/푸시**합니다.
- 이유: Claude Code 작업 가이드는 반복 수정이 잦고, 개발 프로세스에 영향을 주지 않음.
- 커밋 메시지는 이슈 번호 없이 `chore(claude): ...` 형식 허용.

---

## 커밋 컨벤션

모든 커밋 메시지는 반드시 이슈 번호를 포함해야 합니다:
```
#이슈번호 커밋 내용
```

예시:
- `#3 DX12 디바이스 초기화 구현`
- `#15 PSO 캐싱 시스템 구현`
- `#38 MemoryHeaps.md 대역폭 수치 추가`

**중요**:
- **Co-Author를 커밋에 포함하지 마세요** (예: `Co-Authored-By: Claude` 등)
- 커밋 메시지는 간결하고 명확하게 작성
- 필요시 커밋 본문에 상세 내용 추가

---

## 작업 흐름

### 일반 기능 작업 (코드 구현)

1. **이슈 선택/생성**: GitHub Issues에서 이슈를 선택하거나 새로 생성
2. **문서 선행 작업** (필수)
   - 관련 Concepts 문서를 먼저 작성
   - `docs/이슈번호` 브랜치로 PR 생성
   - 사용자 리뷰 및 머지 완료 후 다음 단계 진행
   - 자세한 내용: `.claude/guides/documentation.md`
3. **구현 브랜치 생성**: `git checkout -b feature/이슈번호`
4. **작업 수행 및 커밋** (항상 이슈 번호 포함)
   - 커밋 전에 어떤 내용을 구현했는지, 왜 했는지 간단히 설명 후 허가 받고 커밋
5. **PR 전 빌드 확인 필수**
6. **PR 생성** (main 브랜치로)

### 문서 작업 PR 워크플로우

문서 작업은 **반복적 리뷰 프로세스**를 따릅니다:

1. **초안 작성 & PR 생성**
   - `docs/이슈번호` 브랜치 생성
   - 개념 문서 작성 및 커밋
   - `gh pr create`로 PR 생성 (main 타겟)
   - PR URL 사용자에게 전달

2. **사용자 리뷰**
   - GitHub에서 PR 검토
   - 이해 안 가는 부분에 리뷰 코멘트 작성

3. **리뷰 반영**
   - `gh api repos/jiy12345/DX12GameEngine/pulls/{번호}/comments`로 리뷰 코멘트 수집
   - 각 코멘트를 반영하여 문서 수정
   - 수정 커밋 후 push (같은 PR에 반영됨)
   - 필요 시 PR 코멘트로 "이 부분은 이렇게 수정했습니다" 답변

4. **반복**
   - 이해가 완료될 때까지 2~3단계 반복

5. **머지**
   - 사용자 승인 시 `gh pr merge`로 머지

---

## 이슈 관리

GitHub Issues 사용: https://github.com/jiy12345/DX12GameEngine/issues

**마일스톤:**
- Phase 1: 기초 설정 및 DX12 기본 렌더링
- Phase 1.5: ECS 아키텍처
- Phase 2: 리소스 관리

---

## 연구/학습 이슈 생성 패턴

새로운 DX12/렌더링 주제를 **깊이 학습**하거나 **성능 검증**할 때는 다음의 **자매 이슈 쌍 패턴**을 따릅니다.

### ⚠️ 필수 규칙: 이슈 생성 전 사용자 컨펌

**절대 임의로 이슈를 생성하지 말 것.** 새 주제의 연구/학습이 필요해 보여도:

1. 먼저 어떤 주제로, 어떤 방향으로 이슈를 만들지 **사용자에게 제안**
2. 옵션 여러 개 제시하여 사용자가 고르게 (예: A/B/C 중 택일)
3. 사용자가 **명시적으로 승인한 이슈만** `gh issue create` 실행

"이슈 하나 파야겠네" 같은 짧은 승인은 OK. 하지만 "이런 것도 실험해보면 좋을 것 같다" 같은 제안형 발언은 컨펌 아님 → 옵션 제안 후 추가 컨펌.

### 자매 이슈 쌍 패턴

한 주제에 대해 **두 가지 관점의 이슈**를 쌍으로 구성합니다:

#### 1. "전략 비교" 이슈 (최적화/성능 중심)

- **목적**: 같은 기능을 여러 구현 전략으로 벤치마크, 수치로 최적 전략 도출
- **산출물**: 벤치마크 코드 + `Docs/Analysis/Performance/XXXBenchmark.md`
- **라벨**: `optimization`, `question`
- **제목 패턴**: `XXX 최적화 전략 연구` / `XXX 벤치마크` / `XXX 최적화 전략`

#### 2. "체감 실험" 이슈 (학습/이해 중심)

- **목적**: 올바른 사용 vs 잘못된 사용 vs 불가능한 조합을 직접 구현하여 "왜"를 체감
- **산출물**: 실험 코드 + `Docs/Analysis/Debug/XXXExperiments.md` (Debug Layer 출력 카탈로그 포함)
- **라벨**: `question`, `priority: medium`
- **제목 패턴**: `XXX 활용 실험` / `XXX 적합성 체감 실험` / `XXX 타입별 활용 실험`

### 기존 적용 사례

| 주제 | 전략 비교 | 체감 실험 |
|------|---------|---------|
| 메모리 힙 | [#46](https://github.com/jiy12345/DX12GameEngine/issues/46) | [#47](https://github.com/jiy12345/DX12GameEngine/issues/47) |
| 리소스 배리어 | [#48](https://github.com/jiy12345/DX12GameEngine/issues/48) | [#49](https://github.com/jiy12345/DX12GameEngine/issues/49) |

### 패턴의 장점

- **학습(이해)과 최적화(성능)의 명확한 분리**
- 둘 다 Analysis 문서로 축적되어 지식 베이스 구축
- 자연스러운 학습 흐름: **체감 실험으로 개념 익힘 → 최적화로 성능 추구**
- 각 이슈의 범위가 명확해져 PR 단위로 쪼개기 쉬움

### 예외 상황

모든 주제가 반드시 쌍으로 갈 필요는 없음:

- **구현 이슈**: 기능 구현 자체는 일반 `feature/` 이슈 (예: #45 프레임 파이프라이닝 구현)
- **단순 연구**: 하나의 관점만 필요한 경우 단일 이슈 (예: #42 Command Queue 최적화, #44 하드웨어 스레드)
- **기존 이슈 확장**: 이미 있는 이슈에 포함 가능하면 새로 만들지 말 것

판단 기준: **"체감해볼 가치"와 "성능 측정 가치"가 모두 있으면 쌍으로**.

---

## 검증-기반 구현 패턴 (Validation-Driven Implementation)

성능에 민감한 기능을 엔진에 도입할 때는 **벤치마크 검증 없이 구현하지 않습니다**. 검증된 데이터로 전략을 결정한 후 엔진에 통합하는 파이프라인입니다.

### 이슈 계층 구조

**핵심 원칙**: **시스템 구현 이슈가 parent, 검증(벤치마크/체감) 이슈들이 children**.

```
[시스템 구현 이슈] (parent, 최종 목표)
  │
  ├── [Stage 1 체감 실험 이슈] (optional child)
  │     - 개념 이해, Debug Layer 출력 분석
  │     - 산출물: Docs/Analysis/Debug/XXX.md
  │
  ├── [Stage 2 벤치마크 이슈 - 측면 A] (child)
  ├── [Stage 2 벤치마크 이슈 - 측면 B] (child)
  ├── [Stage 2 벤치마크 이슈 - 측면 C] (child)
  │     - 각 측면별 전략 비교 + 최적 선정
  │     - 산출물: Docs/Analysis/Performance/XXX.md
  │
  └── [Stage 3 실제 구현] (보통 parent 본체, 또는 별도 child)
        - 모든 Stage 2 결론을 통합 반영
        - 산출물: 실제 엔진 코드
```

**왜 이 구조인가**:

1. **의미 정합성**: 이슈의 목적은 "무엇을 완성할지". 시스템 구현이 최종 목표이고, 벤치마크는 그 목표 달성을 위한 **선행 작업** → 자연스러운 부모-자식 관계
2. **여러 벤치마크 수용**: 한 시스템에 여러 측면의 검증이 필요할 때 sub-issue로 깔끔히 분리 (메모리 / 생성 비용 / 전환 비용 등)
3. **확장성**: 구현 중 추가 벤치마크 필요 시 sub-issue 추가만 하면 됨
4. **진행 상황 추적**: GitHub UI에서 parent 이슈 열면 모든 검증 이슈 진행도 한눈에

### 단계별 역할

| Stage | 역할 | 산출물 폴더 | Parent와의 관계 |
|-------|------|-----------|---------------|
| Stage 1 | 체감 실험 (학습) | `Docs/Analysis/Debug/` | Sub-issue (optional) |
| Stage 2 | 전략 벤치마크 (검증) | `Docs/Analysis/Performance/` | Sub-issue (핵심 선행작업) |
| Stage 3 | 엔진 구현 (통합) | 엔진 코드 | **Parent 본체** 또는 sub |

### 적용 단계

1. **시스템 식별**: 무엇을 엔진에 구현할지 정의 → **Parent 이슈** 생성
2. **검증 분해**: 이 시스템 구현을 위해 무엇을 검증해야 하는지 분해 → **Sub-issue (Stage 1/2)** 생성
3. **Sub-issue 관계 설정**: GitHub Sub-issue API로 parent-child 연결
4. **검증 수행**: 각 Sub-issue 완료 → Analysis 문서 축적
5. **구현**: Parent 이슈에서 (또는 별도 Stage 3 sub-issue에서) 검증 결과 반영한 구현

### 자매 쌍 패턴과의 관계

Stage 1 + Stage 2가 **자매 쌍 패턴**에 해당 (같은 주제의 두 관점). 이 둘이 모두 parent의 sub-issue로 들어감:

```
[시스템 구현] (parent)
  ├── [Stage 1 체감 실험]  ─┐
  │                          ├── 자매 쌍 (둘 다 parent의 sub)
  └── [Stage 2 벤치마크]   ─┘
```

### GitHub Sub-issue 관계 설정

GitHub Sub-issues (2024년 GA) 기능으로 공식화:

```bash
# 1. Child 이슈의 REST id 확인
CHILD_ID=$(gh api repos/jiy12345/DX12GameEngine/issues/{sub_issue_number} --jq .id)

# 2. Parent에 sub-issue 등록
gh api -X POST repos/jiy12345/DX12GameEngine/issues/{parent_number}/sub_issues \
  -F sub_issue_id=$CHILD_ID
```

UI에 parent 이슈에서 sub 목록 표시 + 진행률 tracking 지원.

### 판단 기준

| 상황 | 적용 구조 |
|------|--------|
| 여러 측면 검증 필요한 시스템 | **Full**: Parent + 여러 Stage 2 sub + (optional Stage 1) + Stage 3 |
| 단일 측면 검증 | Parent + 1 Stage 2 sub |
| 전략 명확한 경우 | Parent 자체로 구현 (sub 없이) |
| 순수 학습 목적 | Stage 1만 독립 이슈 |
| 성능 무관 단순 기능 | 일반 이슈 (패턴 적용 X) |

### 다층 계층 구조 (Multi-Level Hierarchy)

**원칙**: **문서 계층이 깊으면 이슈 계층도 깊어질 수 있다.**

시스템이 여러 하위 시스템으로 구성되고 각 하위 시스템마다 별도의 최적화 주제가 있다면, 2단 구조(Parent → Sub)를 넘어 **3단 이상**의 계층이 필요할 수 있습니다.

#### 판별 기준

다음 조건을 모두 만족하면 **중간 계층 Epic 추가**:

1. 상위 개념 문서가 이미 여러 하위 개념 문서를 포함 (예: `Commands/README.md` → `CommandQueue.md`, `CommandList.md`, `CommandAllocator.md`)
2. 각 하위 개념마다 독립적인 최적화/검증 주제가 발생
3. 하위 개념 간 sub-issue를 섞어 두면 관련성 파악이 어려워짐

#### 예시 구조

```
[Epic] 상위 시스템 (top)
  ├── [Epic] 하위 시스템 A (middle)
  │     ├── 벤치마크 A-1
  │     └── 벤치마크 A-2
  ├── [Epic] 하위 시스템 B (middle)
  │     ├── 벤치마크 B-1
  │     └── 체감 실험 B
  └── [Epic] 하위 시스템 C (middle)
        └── 벤치마크 C
```

#### 실제 적용 사례: 커맨드 시스템

```
#54 [Epic] 커맨드 시스템 최적화 (top)
  ├── #55 [Epic] 커맨드 큐 최적화 (middle)
  │     └── #42 Command Queue 최적화 전략 연구 (leaf)
  └── #56 [Epic] 커맨드 리스트 최적화 (middle)
        └── #44 하드웨어 스레드 활용 실측 (leaf)
```

**근거**:
- `Docs/Concepts/DX12/Commands/` 폴더가 이미 README + 3개 하위 문서로 구조화
- Queue와 List는 각각 독립적 최적화 주제 보유 (비동기 컴퓨트 vs 멀티스레드 기록)
- 하위 최적화를 중간 Epic으로 묶으면 관심 주제별로 추적 편리

#### 주의사항

- **과도한 계층 금지**: 2단으로 충분하면 2단 사용. "일단 깊게" 금지
- **중간 Epic 자체는 작업 아님**: 진행 상황 tracking용 meta 이슈
- **하위 계층이 비면 생성 금지**: 각 중간 Epic 아래 최소 1개 leaf 이슈가 있을 때만 생성
- **문서 구조와 매핑 확인**: 문서가 폴더로 분리되어 있지 않은데 이슈만 깊게 계층화하면 혼란

### ⚠️ "측정 없는 최적화" 의심

Stage 2 없이 Stage 3를 진행하면 "추측 기반 최적화"가 될 수 있음. 과거 구현 중 측정 근거 없는 것은:

- 성능 회귀 의심 시 되짚어 벤치마크 추가 (parent에 Stage 2 sub 추가)
- 결과로 원래 선택이 잘못된 것이 밝혀지면 재구현

본 엔진의 성능 주장은 **구현 이슈가 검증 sub-issue 링크를 갖는 것**이 목표.

### 적용 사례

| 시스템 구현 이슈 | Stage 1 (체감) | Stage 2 (벤치) | Stage 3 (구현) |
|---------------|--------------|--------------|--------------|
| 리소스 상태 관리 (#21) | #49 | #48 | #21 본체 |
| 프레임 파이프라이닝 (#45) | — | 향후 추가 예정 | #45 본체 |
| 커맨드 큐 활용 | — | #42, #44 | 향후 Phase 5 구현 이슈 |
| 메모리 힙 활용 | #47 | #46 | (구현 이슈 미생성) |

현재는 과거 이슈들이 평면 구조라 완전한 parent-child 관계는 이후 설정. 신규 시스템 이슈부터는 본 계층 구조 적용.

---

## 관련 문서

- 코딩 스타일: `.claude/guides/coding-style.md`
- 문서 작성 원칙: `.claude/guides/documentation.md`
- 최적화 PR 요구사항: `.claude/guides/optimization.md`
