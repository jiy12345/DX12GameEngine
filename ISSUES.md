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

### #3 - DX12 디바이스 초기화
**Type**: Feature  
**Priority**: High  
**Description**: DirectX 12 디바이스 및 기본 인프라 초기화
- [ ] Debug Layer 활성화 (Debug 빌드)
- [ ] DXGI Factory 생성
- [ ] 하드웨어 어댑터 선택
- [ ] D3D12 Device 생성
- [ ] Feature Level 확인

**참고**: `Docs/DX12_Core_Concepts.md` 참조

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

## Phase 2: 리소스 관리

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
