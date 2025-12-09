# Directory Layout

## 개요
DX12 게임 엔진 프로젝트의 디렉토리 구조와 각 디렉토리의 역할을 설명합니다.

## 전체 구조

```
DX12GameEngine/
├── .git/                  # Git 저장소
├── .github/               # GitHub 설정 (이슈 템플릿 등)
├── .idea/                 # IDE 설정 (개인 설정, gitignore)
├── Assets/                # 테스트용 에셋
├── Benchmarks/            # 성능 벤치마크 코드
├── Build/                 # 빌드 산출물 (gitignore)
├── Docs/                  # 프로젝트 문서
├── Samples/               # 샘플 애플리케이션
├── Shaders/               # HLSL 셰이더 파일
├── Source/                # 엔진 소스 코드
├── ThirdParty/            # 외부 라이브러리
├── .gitignore            # Git ignore 규칙
├── BUILD.md              # 빌드 가이드
├── CMakeLists.txt        # CMake 설정
├── CONTRIBUTING.md       # 기여 가이드
├── ISSUES.md             # 이슈 목록
└── README.md             # 프로젝트 개요
```

## 주요 디렉토리 상세

### Source/ - 엔진 소스 코드
```
Source/
├── Core/          # 엔진 핵심 시스템
├── Graphics/      # DX12 렌더링 시스템
├── Math/          # 수학 라이브러리
├── Platform/      # 플랫폼 추상화
└── Utils/         # 유틸리티
```

#### Core/
- 메모리 관리
- 로깅 시스템
- 시간 관리
- 이벤트 시스템

**현재 상태**: 비어있음 (TODO)

#### Graphics/
- DirectX 12 래퍼
- 렌더링 시스템
- 리소스 관리
- 셰이더 관리

**현재 상태**: 비어있음 (TODO: #3-10)

#### Platform/
- Win32 윈도우 시스템
- 입력 처리
- 파일 시스템
- 스레딩

**현재 상태**: ✅ Window 시스템 구현 완료 (#2)
- `Window.h`, `Window.cpp`

#### Math/
- 벡터, 행렬
- 쿼터니언
- 수학 유틸리티

**현재 상태**: 비어있음 (TODO)

#### Utils/
- 헬퍼 함수
- 공통 유틸리티

**현재 상태**: 비어있음 (TODO)

### Docs/ - 프로젝트 문서
```
Docs/
├── Structure/     # 구조 문서
├── Concepts/      # 개념 문서
├── Usage/         # 활용법 문서
├── Analysis/      # 분석 문서
├── Templates/     # 문서 템플릿
├── README.md      # 문서 인덱스
├── ROADMAP.md     # 로드맵
└── DX12_Core_Concepts.md  # DX12 핵심 개념
```

자세한 내용은 [Docs/README.md](../../README.md) 참조.

### Samples/ - 샘플 애플리케이션
```
Samples/
└── Basic/
    └── Main.cpp   # 기본 샘플 (Window 시스템 시연)
```

샘플 애플리케이션은 엔진 사용법을 보여주는 예제입니다.

**현재 상태**: ✅ BasicSample 구현 완료
- Win32 윈도우 생성
- 키보드/마우스 입력 처리
- 윈도우 리사이즈 처리

### Benchmarks/ - 벤치마크
벤치마크 코드와 결과를 저장하는 디렉토리입니다.

**현재 상태**: 비어있음 (TODO: #14-15)

### Shaders/ - HLSL 셰이더
```
Shaders/
├── Common/        # 공통 셰이더 코드
├── Basic/         # 기본 셰이더
└── Advanced/      # 고급 셰이더
```

**현재 상태**: 비어있음 (TODO: #10 이후)

### ThirdParty/ - 외부 라이브러리
서드파티 라이브러리를 저장하는 디렉토리입니다.

**계획**:
- DirectXTex (텍스처 로딩)
- DirectXMath (수학 라이브러리)
- stb_image (이미지 로딩)

**현재 상태**: 비어있음

### Assets/ - 테스트 에셋
테스트용 모델, 텍스처, 오디오 파일 등을 저장합니다.

**현재 상태**: 비어있음

## 빌드 산출물

### Build/ (gitignore에 포함)
```
Build/
├── Bin/           # 실행 파일
│   ├── Debug/
│   └── Release/
├── Lib/           # 라이브러리
└── Obj/           # 오브젝트 파일
```

CMake 빌드 시 생성됩니다.

## 파일 네이밍 규칙

### C++ 파일
- **헤더**: `ClassName.h`
- **소스**: `ClassName.cpp`
- PascalCase 사용

### 셰이더 파일
- **Vertex Shader**: `SomeName_VS.hlsl`
- **Pixel Shader**: `SomeName_PS.hlsl`
- **Compute Shader**: `SomeName_CS.hlsl`

### 문서 파일
- PascalCase 사용
- 단어 구분은 대문자로

## 관련 문서

### 구조
- [BuildSystem.md](./BuildSystem.md) - 빌드 시스템 구조
- [ModuleOrganization.md](./ModuleOrganization.md) - 모듈 구성

### 참고
- [BUILD.md](../../../BUILD.md) - 빌드 가이드
- [CONTRIBUTING.md](../../../CONTRIBUTING.md) - 기여 가이드
