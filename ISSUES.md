# Initial Issues

프로젝트 시작을 위한 초기 이슈 목록입니다.

## Phase 1: 프로젝트 Setup

### #1 - Visual Studio 솔루션 구성
**Type**: Feature
**Priority**: High
**Description**: Visual Studio 2022 솔루션 및 프로젝트 파일 생성
- [x] 솔루션 파일 생성 (CMake 기반)
- [x] 엔진 코어 프로젝트
- [x] 샘플 애플리케이션 프로젝트
- [x] 벤치마크 프로젝트
- [x] 빌드 설정 (Debug/Release/Profile)
- [x] DirectX 12 SDK 링크 설정

---

### #2 - Window 및 입력 시스템
**Type**: Feature
**Priority**: High
**Description**: Win32 윈도우 생성 및 기본 입력 처리
- [x] Win32 윈도우 생성
- [x] 메시지 루프
- [x] 키보드 입력 처리
- [x] 마우스 입력 처리
- [x] 윈도우 리사이즈 처리

---

## Phase 1: DX12 기본 렌더링

### Core Systems

### #23 - 로깅 시스템
**Type**: Feature
**Priority**: High
**Description**: 제대로 된 로깅 시스템 구현 (OutputDebugStringW 대체)
- [ ] Logger 클래스 설계
- [ ] 로그 레벨 (Trace, Debug, Info, Warning, Error, Fatal)
- [ ] 여러 출력 대상 (Console, File, DebugView)
- [ ] 포맷팅 지원 (printf 스타일 또는 std::format)
- [ ] 스레드 안전성
- [ ] 빌드 구성별 필터링 (Debug는 모두, Release는 Warning 이상)
- [ ] 매크로로 편리한 사용 (LOG_INFO, LOG_ERROR 등)

**설계 고려사항**:
- 성능: 비동기 로깅 (큐 기반)
- 파일 로테이션: 로그 파일 크기 제한
- 카테고리: [Engine], [Renderer], [Device] 등 태그
- 타임스탬프 및 스레드 ID

**사용 예시**:
```cpp
LOG_INFO("[Engine] Initializing...");
LOG_ERROR("[Device] Failed to create: {}", error);
```

**우선순위**: #3 직후 구현 권장 - 이후 모든 시스템에서 활용

---

### DX12 Rendering Pipeline

### #3 - DX12 디바이스 초기화
**Type**: Feature
**Priority**: High
**Description**: DirectX 12 디바이스 및 기본 인프라 초기화
- [x] Debug Layer 활성화 (Debug 빌드)
- [x] DXGI Factory 생성
- [x] 하드웨어 어댑터 선택
- [x] D3D12 Device 생성
- [x] Feature Level 확인

**참고**: `Docs/DX12_Core_Concepts.md` 참조

**완료**: Device 클래스 구현 완료. Engine/Renderer 아키텍처로 캡슐화됨.

---

### #4 - 커맨드 큐 및 리스트 시스템
**Type**: Feature  
**Priority**: High  
**Description**: 커맨드 큐와 커맨드 리스트 관리 시스템 구축
- [ ] 커맨드 큐 생성 (Direct, Compute, Copy)
- [ ] 커맨드 Allocator 관리
- [ ] 커맨드 리스트 풀
- [ ] 재사용 가능한 커맨드 리스트 관리

**최적화 고려사항**:
- 커맨드 리스트 재사용 전략
- 멀티스레딩을 위한 구조 설계

---

### #5 - 스왑체인 생성 및 관리
**Type**: Feature  
**Priority**: High  
**Description**: DXGI 스왑체인 생성 및 버퍼 관리
- [ ] 스왑체인 생성
- [ ] 백 버퍼 관리 (더블/트리플 버퍼링)
- [ ] Present 구현
- [ ] 리사이즈 처리
- [ ] VSync/FreeSync 지원

---

### #6 - 디스크립터 힙 관리자
**Type**: Feature  
**Priority**: High  
**Description**: 디스크립터 힙 할당 및 관리 시스템
- [ ] 디스크립터 힙 생성 (RTV, DSV, CBV_SRV_UAV, Sampler)
- [ ] 디스크립터 할당 시스템
- [ ] 디스크립터 해제 및 재사용
- [ ] 디스크립터 핸들 래퍼 클래스

**설계 고려사항**:
- 선형 할당 vs 프리 리스트
- CPU/GPU 디스크립터 핸들 관리

---

### #7 - 렌더 타겟 뷰 관리
**Type**: Feature  
**Priority**: High  
**Description**: 렌더 타겟 뷰 생성 및 관리
- [ ] RTV 디스크립터 힙 생성
- [ ] 스왑체인 버퍼용 RTV 생성
- [ ] 렌더 타겟 바인딩

---

### #8 - 동기화 시스템 (Fence)
**Type**: Feature  
**Priority**: High  
**Description**: CPU-GPU 동기화를 위한 Fence 시스템
- [ ] Fence 생성 및 관리
- [ ] Signal/Wait 구현
- [ ] 프레임 동기화
- [ ] GPU 작업 완료 대기

**주의사항**: 데드락 방지

---

### #9 - 기본 렌더링 루프
**Type**: Feature  
**Priority**: High  
**Description**: 렌더링 루프 구현
- [ ] 프레임 시작/종료 처리
- [ ] 커맨드 리스트 기록 시작/종료
- [ ] 렌더 타겟 클리어
- [ ] Present 호출
- [ ] 프레임 동기화

**목표**: 단색으로 화면 클리어

---

### #10 - 삼각형 렌더링
**Type**: Feature  
**Priority**: High  
**Description**: 첫 번째 지오메트리 렌더링
- [ ] Vertex Buffer 생성
- [ ] Root Signature 정의
- [ ] Pipeline State Object (PSO) 생성
- [ ] 기본 Vertex Shader
- [ ] 기본 Pixel Shader
- [ ] Draw Call 실행

**마일스톤**: 화면에 삼각형 출력

---

## Phase 1.5: ECS 아키텍처 구축

**목표**: Phase 1에서 구현한 기본 렌더링을 ECS 아키텍처 위에서 확장 가능하도록 재구성

**타이밍**: #10 삼각형 렌더링 완료 후 진행

**전략**: 하이브리드 접근 - 성능이 중요한 부분(렌더링, 물리)만 ECS로 구현

### #19 - ECS 아키텍처 설계
**Type**: Architecture
**Priority**: High
**Description**: ECS 코어 아키텍처 설계 및 문서화
- [ ] ECS 아키텍처 조사 (Unity DOTS, Unreal Mass, EnTT, Flecs)
- [ ] 프로젝트에 맞는 ECS 설계 결정
- [ ] Entity ID 관리 전략
- [ ] Component 메모리 레이아웃 설계 (SoA vs AoS)
- [ ] System 실행 순서 및 의존성 관리
- [ ] `Docs/Structure/ECS_Architecture.md` 작성

**설계 고려사항**:
- 캐시 친화적 메모리 레이아웃
- 멀티스레딩 지원 (나중에)
- 쿼리 성능 최적화
- 직렬화/역직렬화 (세이브/로드)

---

### #20 - ECS 코어 구현
**Type**: Feature
**Priority**: High
**Description**: Entity, Component, System의 핵심 기능 구현
- [ ] Entity Manager (생성/삭제/ID 관리)
- [ ] Component Storage (Archetype 기반 또는 Sparse Set)
- [ ] Component Registry (타입별 저장소)
- [ ] Query System (컴포넌트 조합으로 엔티티 검색)
- [ ] 기본 테스트 코드

**참고 라이브러리**:
- EnTT (C++ 헤더 온리, 성능 우수)
- Flecs (기능 풍부)

---

### #21 - System 스케줄러
**Type**: Feature
**Priority**: Medium
**Description**: System 실행 순서 및 스케줄링 관리
- [ ] System 인터페이스 정의
- [ ] System 등록 및 실행
- [ ] System 실행 순서 정의
- [ ] Before/After 의존성 지원
- [ ] System 그룹 (예: RenderSystems, PhysicsSystems)

---

### #22 - 기존 렌더링을 ECS로 포팅
**Type**: Refactoring
**Priority**: High
**Description**: #10에서 구현한 삼각형 렌더링을 ECS로 재구성
- [ ] Transform Component (위치, 회전, 스케일)
- [ ] MeshRenderer Component (메시 + 머티리얼)
- [ ] RenderSystem (모든 MeshRenderer 쿼리 및 렌더링)
- [ ] 기존 하드코딩된 삼각형을 Entity로 변환
- [ ] 동일한 결과 확인 (삼각형 여전히 보여야 함)

**마일스톤**: 삼각형이 Entity/Component로 렌더링됨

---

## Phase 2: 리소스 관리 (ECS 기반)

### #11 - 버퍼 관리 시스템
**Type**: Feature  
**Priority**: High  
**Description**: 버퍼 생성 및 관리 추상화
- [ ] 버퍼 생성 헬퍼
- [ ] Upload 버퍼
- [ ] Default 버퍼
- [ ] 버퍼 업데이트 유틸리티
- [ ] 버퍼 풀링 (선택적)

---

### #12 - 리소스 상태 추적
**Type**: Feature  
**Priority**: Medium  
**Description**: 리소스 배리어 자동 관리
- [ ] 리소스 상태 추적 시스템
- [ ] 자동 배리어 삽입
- [ ] 상태 전환 최적화

---

### #13 - 텍스처 로더
**Type**: Feature  
**Priority**: Medium  
**Description**: 이미지 파일에서 텍스처 로딩
- [ ] DDS 포맷 지원
- [ ] PNG/JPG 로딩 (stb_image)
- [ ] 밉맵 생성
- [ ] 텍스처 업로드

---

## 벤치마크

### #14 - 기본 벤치마크 프레임워크
**Type**: Feature  
**Priority**: Medium  
**Description**: 성능 측정을 위한 기본 프레임워크
- [ ] 타이머 클래스
- [ ] FPS 측정
- [ ] 프레임 타임 측정
- [ ] 결과 CSV/JSON 출력

---

### #15 - GPU 타이밍
**Type**: Feature  
**Priority**: Medium  
**Description**: GPU 작업 시간 측정
- [ ] Timestamp Query 구현
- [ ] GPU 타이밍 측정
- [ ] Pass별 시간 측정

---

## 최적화

### #16 - PSO 캐싱
**Type**: Optimization  
**Priority**: Medium  
**Description**: Pipeline State Object 캐싱 시스템
- [ ] PSO 해시 생성
- [ ] PSO 캐시 구현
- [ ] 벤치마크: PSO 생성 시간 비교

---

## 문서화

### #18 - 문서화 시스템 구축
**Type**: Documentation
**Priority**: High
**Description**: 프로젝트 전체에 대한 체계적인 문서화 시스템 구축
- [ ] Docs 디렉토리 구조 생성 (Structure/Concepts/Usage/Analysis)
- [ ] 문서 템플릿 작성 (4가지 카테고리)
- [ ] 문서 인덱스 작성 (Docs/README.md)
- [ ] DX12 개념 문서 작성 (Device, CommandQueue, Descriptors 등)
- [ ] 구조 문서 작성 (아키텍처, 렌더링 파이프라인 등)
- [ ] 활용법 문서 작성 (윈도우 생성, DX12 초기화 등)
- [ ] 기존 DX12_Core_Concepts.md를 새 구조로 마이그레이션
- [ ] CLAUDE.md에 문서화 가이드라인 추가

**목표**: 매 작업마다 관련 문서를 업데이트하여 지식 베이스 구축

---

### #17 - API 문서 생성
**Type**: Documentation
**Priority**: Low
**Description**: 코드 문서 자동 생성
- [ ] Doxygen 설정
- [ ] 문서화 주석 추가
- [ ] API 레퍼런스 생성

---

## 사용 가이드

1. 새로운 작업을 시작할 때 이 파일에서 이슈를 선택합니다
2. 이슈 번호를 기억하고 브랜치를 생성합니다
3. 커밋 시 이슈 번호를 포함합니다
4. 작업 완료 후 체크박스를 체크합니다

## 이슈 추가

새로운 이슈가 필요할 때 여기에 추가하고 번호를 부여합니다.
