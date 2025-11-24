# 빌드 가이드

## 필요 사항

- Windows 10/11 (64-bit)
- Visual Studio 2022
- CMake 3.20 이상
- DirectX 12 SDK (Windows 10/11 SDK에 포함)

## 빌드 방법

### 1. Visual Studio 솔루션 생성

프로젝트 루트 디렉토리에서 다음 명령어를 실행하세요:

```cmd
mkdir Build
cd Build
cmake .. -G "Visual Studio 17 2022" -A x64
```

### 2. Visual Studio에서 빌드

생성된 `DX12GameEngine.sln` 파일을 Visual Studio에서 열고 빌드하세요.

또는 명령줄에서 빌드:

```cmd
# Debug 빌드
cmake --build . --config Debug

# Release 빌드
cmake --build . --config Release

# Profile 빌드 (성능 측정용)
cmake --build . --config Profile
```

### 3. 실행

빌드된 실행 파일은 다음 경로에 생성됩니다:

- `Build/Bin/Debug/BasicSample.exe`
- `Build/Bin/Debug/EngineBenchmark.exe`

## 프로젝트 구조

- **Engine**: 엔진 코어 라이브러리 (정적 라이브러리)
- **BasicSample**: 기본 샘플 애플리케이션
- **EngineBenchmark**: 벤치마크 도구

## 빌드 설정

### Debug
- 디버그 심볼 포함
- 최적화 없음
- DX12 Debug Layer 활성화
- 매크로: `_DEBUG`, `DX12_DEBUG`

### Release
- 최적화 활성화 (`/O2`)
- 디버그 정보 제외
- 매크로: `NDEBUG`

### Profile
- Release 기반
- 성능 측정을 위한 추가 계측 코드 포함
- 매크로: `NDEBUG`, `PROFILE_BUILD`

## 트러블슈팅

### CMake를 찾을 수 없음
CMake를 [cmake.org](https://cmake.org/download/)에서 다운로드하고 설치하세요.

### DirectX 12 SDK를 찾을 수 없음
최신 Windows SDK를 설치하세요: [Windows SDK](https://developer.microsoft.com/en-us/windows/downloads/windows-sdk/)

### Visual Studio 버전 문제
Visual Studio 2019를 사용하는 경우:
```cmd
cmake .. -G "Visual Studio 16 2019" -A x64
```
