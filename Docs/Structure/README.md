# Structure - 구조적인 설명

"무엇이 어떻게 연결되어 있는가?"

이 섹션에서는 DX12 게임 엔진의 구조적인 측면을 다룹니다. 시스템 아키텍처, 모듈 간 관계, 데이터 플로우 등을 이해할 수 있습니다.

## 📑 문서 목록

### 전체 구조
- Overview.md - 전체 아키텍처 한눈에 보기 **(미작성)**

### Project/ - 프로젝트 구조
- [DirectoryLayout.md](./Project/DirectoryLayout.md) - 디렉토리 구조
- BuildSystem.md - 빌드 시스템 구조 **(미작성)**
- ModuleOrganization.md - 모듈 구성 **(미작성)**
- DependencyGraph.md - 의존성 관계 **(미작성)**

### Core/ - 코어 시스템 구조
- SystemArchitecture.md - 핵심 시스템 아키텍처 **(미작성)**
- MemoryArchitecture.md - 메모리 구조 **(미작성)**
- ResourceLifecycle.md - 리소스 생명주기 **(미작성)**

### Graphics/ - 그래픽스 구조
- RenderingPipeline.md - 렌더링 파이프라인 플로우 **(미작성)**
- CommandFlow.md - 커맨드 실행 플로우 **(미작성)**
- DescriptorManagement.md - 디스크립터 관리 구조 **(미작성)**
- SynchronizationFlow.md - 동기화 플로우 **(미작성)**

### Platform/ - 플랫폼 구조
- WindowArchitecture.md - 윈도우 시스템 구조 **(미작성)**
- InputArchitecture.md - 입력 시스템 구조 **(미작성)**

## 🔗 다른 카테고리와의 관계

- **[Concepts](../Concepts/)**: 각 구조의 구성 요소에 대한 개념 설명
- **[Usage](../Usage/)**: 구조를 실제로 구현하는 방법
- **[Analysis](../Analysis/)**: 구조적 결정에 대한 성능 분석

## 📝 문서 작성 가이드

Structure 문서는 다음을 포함해야 합니다:
1. 구조 다이어그램 (ASCII 아트 또는 설명)
2. 구성 요소와 역할
3. 데이터/제어 플로우
4. 의존성 관계
5. 관련 개념 문서 링크

템플릿: [StructureTemplate.md](../Templates/StructureTemplate.md)
