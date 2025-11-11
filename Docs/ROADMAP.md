# Project Roadmap

DX12 게임 엔진 개발 로드맵

## 🎯 Phase 1: 기초 설정 (1-2주)

### 프로젝트 Setup
- [x] Git 저장소 초기화
- [x] 프로젝트 구조 설계
- [x] 문서 템플릿 작성
- [ ] Visual Studio 솔루션 구성
- [ ] 빌드 시스템 설정

### DX12 기본 렌더링
- [ ] 디바이스 및 스왑체인 초기화
- [ ] 커맨드 큐/리스트 관리 시스템
- [ ] 디스크립터 힙 관리자
- [ ] 기본 렌더링 루프
- [ ] 단일 삼각형 렌더링

**마일스톤**: 화면에 삼각형 하나 출력

## 🏗️ Phase 2: 핵심 시스템 구축 (3-4주)

### 리소스 관리
- [ ] 버퍼 관리 시스템 (Vertex, Index, Constant)
- [ ] 텍스처 로딩 및 관리
- [ ] 리소스 상태 추적
- [ ] 메모리 풀링 시스템

### 셰이더 시스템
- [ ] 셰이더 컴파일 시스템
- [ ] PSO 캐싱
- [ ] Root Signature 관리

### 기본 렌더링 기능
- [ ] 카메라 시스템
- [ ] 메시 렌더링
- [ ] 기본 라이팅 (Blinn-Phong)
- [ ] 텍스처 매핑

**마일스톤**: 간단한 3D 모델 렌더링

## 🚀 Phase 3: 고급 렌더링 (4-6주)

### PBR (Physically Based Rendering)
- [ ] PBR 머티리얼 시스템
- [ ] Image-Based Lighting (IBL)
- [ ] BRDF LUT 생성

### 후처리
- [ ] HDR 렌더링
- [ ] Tone Mapping
- [ ] Bloom
- [ ] Anti-Aliasing (TAA)

### 최적화
- [ ] Frustum Culling
- [ ] Occlusion Culling
- [ ] LOD 시스템

**마일스톤**: PBR로 복잡한 씬 렌더링

## ⚡ Phase 4: 최신 기술 통합 (6-8주)

### DirectX Raytracing (DXR)
- [ ] 레이트레이싱 파이프라인 설정
- [ ] BLAS/TLAS 생성
- [ ] Reflection
- [ ] Ambient Occlusion
- [ ] Global Illumination

### Mesh Shaders
- [ ] 메시 셰이더 파이프라인
- [ ] 동적 LOD
- [ ] 지오메트리 생성

### Variable Rate Shading (VRS)
- [ ] VRS 구현
- [ ] 적응형 셰이딩

**마일스톤**: 레이트레이싱이 적용된 실시간 렌더링

## 🔧 Phase 5: 멀티스레딩 & 최적화 (4-6주)

### 멀티스레딩
- [ ] 멀티스레드 커맨드 리스트 기록
- [ ] Job System 구현
- [ ] Async Compute

### 최적화
- [ ] Bindless 렌더링
- [ ] GPU-Driven Rendering
- [ ] Indirect Drawing
- [ ] 메모리 최적화

### 프로파일링
- [ ] GPU 타이밍 측정
- [ ] PIX 통합
- [ ] 커스텀 프로파일러

**마일스톤**: 멀티스레딩으로 프레임률 2배 향상

## 📊 Phase 6: 벤치마킹 & 문서화 (2-3주)

### 벤치마크
- [ ] 다양한 시나리오 벤치마크
- [ ] 성능 비교 분석
- [ ] 최적화 전/후 비교

### 문서화
- [ ] API 문서 생성
- [ ] 튜토리얼 작성
- [ ] 아키텍처 다이어그램

**마일스톤**: 완성된 벤치마크 리포트

## 🎓 Phase 7: 고급 기능 (선택적)

### DirectStorage
- [ ] DirectStorage 통합
- [ ] GPU 디컴프레션

### Sampler Feedback
- [ ] Sampler Feedback Streaming

### Machine Learning
- [ ] DirectML 통합
- [ ] DLSS/FSR

## 📈 진행 상황

```
Phase 1: ████░░░░░░ 40% (진행 중)
Phase 2: ░░░░░░░░░░  0%
Phase 3: ░░░░░░░░░░  0%
Phase 4: ░░░░░░░░░░  0%
Phase 5: ░░░░░░░░░░  0%
Phase 6: ░░░░░░░░░░  0%
Phase 7: ░░░░░░░░░░  0%
```

## 📝 Notes

- 각 Phase는 유연하게 조정 가능
- 우선순위에 따라 순서 변경 가능
- 새로운 기술이나 아이디어는 언제든 추가
