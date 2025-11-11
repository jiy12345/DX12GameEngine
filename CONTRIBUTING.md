# Contributing to DX12 Game Engine

DX12 게임 엔진 프로젝트에 기여하는 방법을 안내합니다.

## 🌿 브랜치 전략

### 주요 브랜치
- **main**: 안정적인 릴리스 브랜치
  - 프로덕션 레디 코드만 포함
  - 태그를 통한 버전 관리
  
- **develop**: 개발 통합 브랜치
  - 다음 릴리스를 위한 최신 개발 코드
  - Feature 브랜치들이 병합되는 곳

### 작업 브랜치

#### Feature 브랜치
```bash
git checkout develop
git checkout -b feature/descriptor-heap-manager
# 작업 수행
git commit -m "#5 디스크립터 힙 관리자 구현"
# develop에 PR
```

#### Bugfix 브랜치
```bash
git checkout develop
git checkout -b bugfix/memory-leak-fix
# 버그 수정
git commit -m "#12 메모리 누수 수정"
# develop에 PR
```

#### Optimization 브랜치
```bash
git checkout develop
git checkout -b optimize/command-list-recording
# 최적화 작업
git commit -m "#8 커맨드 리스트 기록 최적화"
# develop에 PR
```

## 📝 커밋 컨벤션

### 형식
```
#이슈번호 커밋 내용
```

### 예시
```bash
git commit -m "#1 DX12 디바이스 초기화 구현"
git commit -m "#2 스왑체인 생성 로직 추가"
git commit -m "#3 메모리 누수 버그 수정"
```

### 좋은 커밋 메시지 작성법
- 명령형 현재 시제 사용 ("추가한다" ❌ "추가" ✅)
- 간결하고 명확하게
- 필요시 본문에 상세 설명 추가

```bash
git commit -m "#15 PSO 캐싱 시스템 구현

- 해시 기반 PSO 캐시 추가
- 런타임 PSO 생성 최소화
- 약 30% 성능 향상 확인"
```

## 🔄 워크플로우

### 1. 이슈 생성
작업을 시작하기 전에 항상 이슈를 먼저 생성합니다.

```
Title: [FEATURE] 디스크립터 힙 관리자 구현
Label: enhancement
```

### 2. 브랜치 생성
```bash
git checkout develop
git pull origin develop
git checkout -b feature/descriptor-heap-manager
```

### 3. 개발 & 커밋
```bash
# 작업 수행
git add .
git commit -m "#5 디스크립터 힙 기본 구조 구현"

# 추가 작업
git commit -m "#5 디스크립터 할당 로직 추가"
```

### 4. Pull Request 생성
- PR 템플릿을 사용하여 작성
- 관련 이슈 링크 (#5)
- 변경사항 설명
- 벤치마크 결과 (최적화의 경우)

### 5. 코드 리뷰 & 병합
- 리뷰 받기
- 피드백 반영
- develop 브랜치에 병합

## 🧪 테스트

### 빌드 확인
PR 전에 반드시 빌드가 성공하는지 확인:
```bash
# Debug 빌드
msbuild DX12GameEngine.sln /p:Configuration=Debug

# Release 빌드
msbuild DX12GameEngine.sln /p:Configuration=Release
```

### 벤치마크 (최적화의 경우)
```bash
DX12GameEngine.exe --benchmark <affected_category>
```

## 📊 성능 최적화 PR

최적화 PR은 다음을 포함해야 합니다:

1. **Before/After 비교**
   ```
   Before: 120 FPS
   After: 180 FPS
   Improvement: +50%
   ```

2. **벤치마크 결과**
   - CSV 또는 JSON 형식으로 첨부
   - 여러 시나리오 테스트

3. **Trade-off 분석**
   - 메모리 사용량 변화
   - 복잡도 증가
   - 기타 고려사항

## 💻 코딩 스타일

### C++ 스타일
```cpp
// 클래스명: PascalCase
class DescriptorHeapManager 
{
public:
    // 멤버 함수: PascalCase
    void AllocateDescriptor();
    
private:
    // 멤버 변수: m_ 접두사 + camelCase
    ID3D12DescriptorHeap* m_heap;
    UINT m_descriptorSize;
};

// 지역 변수: camelCase
int frameIndex = 0;

// 상수: k 접두사 + PascalCase
constexpr UINT kMaxDescriptors = 1024;
```

### 주석
```cpp
// 단일 줄 주석: // 사용

/**
 * @brief 디스크립터를 할당합니다
 * @param type 디스크립터 타입
 * @return 할당된 디스크립터 핸들
 */
D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE type);
```

## 📚 문서화

### 코드 문서화
- 복잡한 로직에는 주석 추가
- 공개 API에는 문서화 주석 작성

### README 업데이트
새로운 기능 추가 시 README 업데이트:
- 기능 체크리스트 업데이트
- 사용 예시 추가 (필요시)

### 기술 문서
주요 시스템 구현 시 `Docs/` 폴더에 문서 추가

## ⚠️ 주의사항

### 하지 말아야 할 것
- ❌ main 브랜치에 직접 커밋
- ❌ 이슈 없이 작업 시작
- ❌ 빌드가 깨진 상태로 PR
- ❌ 테스트 없이 최적화 주장

### 해야 할 것
- ✅ 항상 이슈 먼저 생성
- ✅ 커밋 메시지에 이슈 번호 포함
- ✅ PR 전에 빌드 확인
- ✅ 의미 있는 커밋 단위로 분리

## 🆘 도움이 필요하신가요?

- 이슈 트래커에 질문 생성
- 문서 확인: `Docs/` 폴더
- 기존 코드 참고

## 📖 추가 자료

- [DX12 Core Concepts](./Docs/DX12_Core_Concepts.md)
- [Project Roadmap](./Docs/ROADMAP.md)
- [Benchmark Guide](./Benchmarks/README.md)
