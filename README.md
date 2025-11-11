# DX12 Game Engine

DirectX 12를 기반으로 한 현대적인 게임 엔진 프로젝트

## 🎯 목표

- DirectX 12의 low-level 렌더링 API를 활용한 고성능 게임 엔진 구현
- 최신 그래픽스 기술 (Raytracing, Mesh Shaders, Variable Rate Shading 등) 적극 활용
- 확장 가능하고 유지보수하기 쉬운 아키텍처 설계
- 다양한 최적화 기법 실험 및 벤치마킹

## 🛠️ 기술 스택

- **Graphics API**: DirectX 12
- **언어**: C++17/20
- **빌드 시스템**: (추후 결정)
- **셰이더**: HLSL 6.x

## 📁 프로젝트 구조

```
DX12GameEngine/
├── Source/
│   ├── Core/          # 엔진 핵심 시스템
│   ├── Graphics/      # DX12 렌더링 시스템
│   ├── Platform/      # 플랫폼별 추상화 계층
│   ├── Math/          # 수학 라이브러리
│   └── Utils/         # 유틸리티 및 헬퍼
├── Shaders/           # HLSL 셰이더 파일
├── Assets/            # 테스트용 에셋
├── Benchmarks/        # 성능 벤치마크 코드
├── Docs/              # 문서화
└── ThirdParty/        # 외부 라이브러리
```

## 🔧 개발 워크플로우

### Git 브랜치 전략
- `main`: 안정적인 릴리스 브랜치
- `develop`: 개발 통합 브랜치
- `feature/*`: 새로운 기능 개발
- `bugfix/*`: 버그 수정
- `optimize/*`: 최적화 작업

### 커밋 컨벤션
```
#이슈번호 커밋 내용
```
예: `#1 초기 프로젝트 구조 설정`

## 📊 주요 기능 (계획)

### 렌더링
- [ ] DX12 디바이스 및 스왑체인 초기화
- [ ] 커맨드 큐 및 커맨드 리스트 관리
- [ ] 디스크립터 힙 관리 시스템
- [ ] PSO (Pipeline State Object) 캐싱
- [ ] 리소스 관리 및 메모리 풀링
- [ ] 멀티스레드 렌더링

### 최신 기술
- [ ] DirectX Raytracing (DXR)
- [ ] Mesh Shaders
- [ ] Variable Rate Shading (VRS)
- [ ] Sampler Feedback
- [ ] DirectStorage

### 최적화
- [ ] GPU 기반 Culling
- [ ] Async Compute
- [ ] 리소스 바인딩 최적화 (Bindless)
- [ ] 메모리 관리 최적화

## 🚀 시작하기

(빌드 및 실행 가이드 추가 예정)

## 📝 문서

자세한 문서는 [Docs](./Docs) 폴더를 참조하세요.

## 📈 벤치마크

성능 벤치마크 결과는 [Benchmarks](./Benchmarks) 폴더를 참조하세요.

## 📄 라이선스

(라이선스 추가 예정)
