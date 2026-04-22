# CLAUDE.md

**ℹ️ 이 파일은 Claude Code 작업 가이드입니다**
- 저장소에 커밋되며, 개발자 간 공유됩니다
- `.claude/settings.local.json`, `.claude/.idea/`는 로컬 전용 (gitignore)
- **`.claude/` 하위 문서 수정은 PR 없이 `main`에 직접 커밋/푸시합니다** (개발 가이드 특성상 반복 수정이 잦음)

---

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 프로젝트 개요

DirectX 12 기반 고성능 게임 엔진 프로젝트입니다. 최신 그래픽스 기술(Raytracing, Mesh Shaders, VRS 등)을 활용하며, 성능 최적화와 벤치마킹이 중요한 프로젝트입니다.

**현재 상태**: Phase 1 초기 단계 - 빌드 시스템 구성 및 기본 DX12 렌더링 구현 중

---

## 📖 작업별 참고 문서

작업을 시작하기 전에, 해당 작업과 관련된 가이드 문서를 **반드시 먼저 읽어야 합니다.**

### 🌿 커밋 / 브랜치 / PR / 이슈 작업
→ `.claude/guides/workflow.md` **필수**
- 브랜치 전략 (모든 작업 PR 필수, 직접 커밋 금지)
- 커밋 컨벤션 (이슈 번호 포함, Co-Author 금지)
- 작업 흐름, 문서 PR 반복 리뷰 프로세스
- 빌드 시스템, 이슈 관리
- **연구/학습 이슈 생성 패턴** (전략 비교 + 체감 실험 자매 쌍, ⚠️ 이슈 생성 전 사용자 컨펌 필수)

### 💻 C++ 코드 작성 / 수정
→ `.claude/guides/coding-style.md` **필수**
→ `.claude/guides/architecture.md` (구조 파악)
- 네이밍 규칙, 문서화 주석, 소유권 표현

### 🎨 DX12 렌더링 기능 구현
→ `.claude/guides/dx12.md` **필수**
→ `.claude/guides/architecture.md` (현재 레이어 구조)
→ `.claude/guides/coding-style.md`
- 커맨드 실행 흐름, 힙 타입, 주의사항, 공식 참고 자료

### 📝 문서 작성 / 정리 (`Docs/` 폴더)
→ `.claude/guides/documentation.md` **필수**
→ `.claude/guides/workflow.md` (문서 PR 워크플로우)
- 문서 구조, 작성 원칙 (목차 필수, 근거 기반)
- 구현 전 문서 선행 규칙, 이슈 링크, 템플릿

### ⚡ 최적화 작업 (`optimize/이슈번호`)
→ `.claude/guides/optimization.md` **필수**
→ `.claude/guides/architecture.md` (성능 원칙)
→ `.claude/guides/dx12.md` (관련 개념)
- 벤치마크 필수, Trade-off 분석, 측정 방법

### 🏗️ 아키텍처 설계 / 새 시스템 추가
→ `.claude/guides/architecture.md` **필수**
→ `.claude/guides/dx12.md` (컨텍스트)
→ `.claude/guides/coding-style.md`
- 현재 계층 구조, ECS 전략, 확장성/성능 원칙, 로드맵

---

## 🔍 빠른 참조

상황별로 여러 가이드를 동시에 참고하는 경우가 많습니다. 아래는 전형적 조합:

| 상황 | 읽을 문서 |
|------|-----------|
| 새 DX12 기능 구현 시작 | `documentation.md` → (PR 머지) → `dx12.md` + `architecture.md` + `coding-style.md` |
| 기존 클래스 리팩터링 | `architecture.md` + `coding-style.md` + `workflow.md` |
| 벤치마크 포함 PR 작성 | `optimization.md` + `workflow.md` + `documentation.md` (Analysis 문서) |
| 개념 문서 추가 | `documentation.md` + `workflow.md` (문서 PR 프로세스) |

---

## 프로젝트 외부 참고 문서

- `Docs/DX12_Core_Concepts.md`: DirectX 12 핵심 개념 상세 설명
- `Docs/ROADMAP.md`: 프로젝트 전체 로드맵
- `Docs/Concepts/DX12/`: 구현된 개념별 상세 문서
