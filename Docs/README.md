# DX12 Game Engine 문서

DX12 게임 엔진의 전체 문서 허브입니다.

## 📚 문서 카테고리

### 🏗️ [Structure](./Structure/) - 구조적인 설명
"무엇이 어떻게 연결되어 있는가?"

시스템 아키텍처, 모듈 간 관계, 데이터 플로우를 이해할 수 있습니다.

- **프로젝트 구조**: 디렉토리 레이아웃, 빌드 시스템, 모듈 구성
- **코어 시스템**: 핵심 아키텍처, 메모리 구조, 리소스 생명주기
- **그래픽스**: 렌더링 파이프라인, 커맨드 플로우, 디스크립터 관리
- **플랫폼**: 윈도우 시스템, 입력 시스템 구조

### 💡 [Concepts](./Concepts/) - 개념적인 설명
"이것이 무엇인가? 왜 필요한가?"

각 개념의 정의, 목적, 동작 원리를 배울 수 있습니다.

- **[DX12](./Concepts/DX12/)**: DirectX 12 핵심 개념 (Device, CommandQueue, Descriptors, PSO 등)
- **[Rendering](./Concepts/Rendering/)**: 렌더링 개념 (RenderLoop, Pipeline, Shaders 등)
- **[Platform](./Concepts/Platform/)**: 플랫폼 개념 (Window, MessageLoop, Input 등)

### 🎯 [Usage](./Usage/) - 활용법
"어떻게 사용하는가?"

실제 구현 방법을 단계별 가이드와 코드 예제로 배울 수 있습니다.

- **시작하기**: 프로젝트 설정, 빌드 시스템
- **기본 작업**: 윈도우 생성, 디바이스 초기화, 삼각형 렌더링
- **고급 작업**: 리소스 관리, 멀티스레드 렌더링, PSO 최적화
- **문제 해결**: 트러블슈팅 가이드

### 🔬 [Analysis](./Analysis/) - 분석 및 테스트
"성능은 어떠한가? 어떻게 검증하는가?"

벤치마크, 프로파일링, 테스트, 디버깅 방법을 배울 수 있습니다.

- **성능 분석**: 벤치마크 전략, 프로파일링 도구, GPU 타이밍
- **테스트**: 테스트 전략, 유닛 테스트, 통합 테스트
- **디버깅**: 디버깅 가이드, Debug Layer, PIX 사용법
- **연구**: 실험 및 삽질 기록

---

## 🎓 학습 경로

### 입문자
1. [프로젝트 시작하기](./Usage/GettingStarted.md)
2. [DX12 개요](./Concepts/DX12/Overview.md)
3. [윈도우 생성하기](./Usage/BasicTasks/CreatingWindow.md)

### 개발자
1. [전체 아키텍처](./Structure/Overview.md)
2. [DX12 개념들](./Concepts/DX12/)
3. [구현 가이드](./Usage/BasicTasks/)

### 최적화 담당자
1. [렌더링 파이프라인 구조](./Structure/Graphics/RenderingPipeline.md)
2. [벤치마크 전략](./Analysis/Performance/BenchmarkStrategy.md)
3. [성능 분석 결과](./Analysis/Performance/Results/)

---

## 📝 문서 작성 가이드

### 문서 작성 원칙
1. **근거 기반 작성**: 참고 자료나 구현된 코드를 근거로만 작성
2. **링크 연결**: 관련 문서 간 링크를 적극 활용
3. **템플릿 사용**: 각 카테고리의 템플릿을 따름

### 템플릿
- [StructureTemplate.md](./Templates/StructureTemplate.md)
- [ConceptTemplate.md](./Templates/ConceptTemplate.md)
- [UsageTemplate.md](./Templates/UsageTemplate.md)
- [AnalysisTemplate.md](./Templates/AnalysisTemplate.md)

### 작업 시 문서 업데이트
각 이슈 작업 시:
1. **구현 전**: 관련 개념 문서 확인/작성
2. **구현 중**: 구조 문서 업데이트
3. **구현 후**: Usage 가이드 작성, Analysis 추가

---

## 🔗 다른 문서들

- [ROADMAP.md](./ROADMAP.md) - 프로젝트 전체 로드맵
- [DX12_Core_Concepts.md](./DX12_Core_Concepts.md) - DirectX 12 핵심 개념 (기존 문서)

---

## 📊 문서 현황

### 구현 완료 문서
- ✅ Window System (Platform)

### 작성 예정 문서
- ⏳ DX12 Device
- ⏳ Command Queue & List
- ⏳ Descriptor Heaps
- ⏳ SwapChain
- ⏳ Synchronization
- ⏳ PSO
- ⏳ Root Signature

문서는 기능 구현과 함께 지속적으로 추가됩니다.
