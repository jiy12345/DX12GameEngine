# 아키텍처 가이드

**적용 시점**:
- 새로운 시스템/클래스 설계 시
- 기존 구조 변경 시
- 프로젝트 구조 파악이 필요할 때

---

## 프로젝트 구조

```
Source/
├── Core/          # 엔진 핵심 시스템
│   ├── Engine.h/cpp        # 엔진 메인 클래스 (Phase 1 완료)
│   ├── BuildConfig.h       # 빌드 구성 관리 (Phase 1 완료)
│   └── ECS/                # Entity Component System (Phase 1.5에서 추가)
├── Graphics/      # DX12 렌더링 시스템
│   ├── Device.h/cpp        # DX12 디바이스 (#3 완료)
│   ├── Renderer.h/cpp      # 렌더링 서브시스템 (Phase 1 완료)
│   └── ...                 # CommandQueue, SwapChain 등 (Phase 1 진행 중)
├── Platform/      # 플랫폼 추상화
│   └── Window.h/cpp        # Win32 윈도우 (#2 완료)
├── Math/          # 벡터, 행렬, 쿼터니언 등
└── Utils/         # 유틸리티 및 헬퍼 함수
```

---

## 현재 아키텍처 (Phase 1)

### 계층 구조

```
Sample (게임 코드)
    ↓
Engine (엔진 메인)
    ├─ Window (서비스 - 틱 없음)
    └─ Renderer (서브시스템 - 매 프레임 Update)
        └─ Device (서비스 - 틱 없음)
            ├─ Debug Layer
            ├─ DXGI Factory
            └─ GPU 어댑터
```

### 책임 분리

**Engine 클래스** (`Source/Core/Engine.h/cpp`):
- 역할: 모든 서브시스템 관리 및 게임 루프 실행
- 책임: 초기화, 게임 루프, 종료
- 틱: 매 프레임 Renderer Update 호출
- 캡슐화: 내부 구현(Window, Renderer) 완전히 숨김

**Renderer 클래스** (`Source/Graphics/Renderer.h/cpp`):
- 역할: 렌더링 전체 관리
- 책임: Device, CommandQueue, SwapChain 등 관리
- 틱: BeginFrame/RenderFrame/EndFrame 매 프레임 호출
- 캡슐화: Device 등 내부 구현 숨김

**Device 클래스** (`Source/Graphics/Device.h/cpp`):
- 역할: DX12 디바이스 초기화
- 책임: GPU 어댑터 선택, Device 생성
- 틱: 없음 (한 번 초기화하면 끝)
- 캡슐화: DXGI, Debug Layer 세부사항 숨김

**Window 클래스** (`Source/Platform/Window.h/cpp`):
- 역할: 윈도우 및 입력 관리
- 책임: Win32 윈도우 생성, 메시지 처리
- 틱: 없음 (이벤트 기반)

### 설정 시스템

**BuildConfig** (`Source/Core/BuildConfig.h`):
- 모든 빌드 구성(Debug/Release/Profile) 기본값을 한 곳에서 관리
- `constexpr`로 컴파일 타임 최적화
- 한 파일만 보면 각 빌드가 무엇을 활성화하는지 파악 가능

**계층적 설정 구조**:
```cpp
EngineDesc {
    WindowDesc window;      // 윈도우 설정
    RendererDesc renderer;  // 렌더러 설정
}
```

**사용 예시**:
```cpp
// 1. 자동 (현재 빌드 구성)
EngineDesc desc;  // Debug 빌드면 Debug 기본값

// 2. 명시적 구성 선택
EngineDesc desc = EngineDesc::ForRelease();

// 3. 개별 오버라이드
desc.renderer.vsync = false;
```

---

## 엔진 아키텍처 설계 원칙

이 프로젝트는 **확장성**과 **성능**을 동시에 추구하는 현대적 게임 엔진을 목표로 합니다.

### ECS (Entity Component System) 아키텍처

**전략**: 하이브리드 접근 방식
- **성능이 중요한 부분**: ECS 사용 (렌더링, 물리, 파티클 등)
- **관리 시스템**: 전통적 OOP (리소스 관리자, 파일 로더 등)
- **타이밍**: Phase 1.5 (#19-#22)에서 도입

**ECS 도입 이유**:
1. **캐시 친화적**: Data-Oriented Design으로 성능 향상
2. **확장성**: 새 기능을 Component/System으로 쉽게 추가
3. **멀티스레딩**: System 병렬화 용이
4. **유연성**: 런타임 컴포넌트 조합

**ECS 적용 범위**:
- ✅ 렌더링 오브젝트 (Transform, Mesh, Material)
- ✅ 물리 객체 (Rigidbody, Collider) - Phase 4+
- ✅ 파티클 시스템 - Phase 3+
- ✅ AI/Gameplay 로직 - Phase 5+
- ❌ 리소스 관리자 (Device, TextureLoader 등)
- ❌ 렌더링 인프라 (CommandQueue, SwapChain 등)

**구현 참고**:
- **EnTT**: C++ 헤더 온리, 매우 빠름, 템플릿 기반
- **Flecs**: 풍부한 기능, 관계형 쿼리
- 또는 직접 구현 (학습 목적)

**개발 단계**:
```
Phase 1 (#3-#10): 기본 렌더링 (순수 OOP)
     ↓
Phase 1.5 (#19-#22): ECS 코어 구축 + 렌더링 마이그레이션
     ↓
Phase 2+: 모든 새 기능은 ECS Component/System으로
```

### 확장성 설계 원칙

1. **인터페이스 기반 설계**
   ```cpp
   // 나쁜 예: 구체 타입에 의존
   void Render(TriangleMesh* mesh);

   // 좋은 예: 인터페이스/추상화
   void Render(IMesh* mesh);
   ```

2. **의존성 주입**
   ```cpp
   // 나쁜 예: 하드코딩된 의존성
   class Renderer {
       D3D12Device device; // 직접 생성
   };

   // 좋은 예: 주입
   class Renderer {
       Renderer(ID3D12Device* device) : m_device(device) {}
   };
   ```

3. **단일 책임 원칙**
   - 각 클래스는 하나의 명확한 책임만
   - 예: `Renderer`는 렌더링만, `ResourceLoader`는 로딩만

4. **컴포지션 우선**
   - 상속보다 컴포지션 선호
   - ECS는 본질적으로 컴포지션 기반

5. **명시적 소유권**
   ```cpp
   std::unique_ptr<Texture> texture;     // 독점 소유
   std::shared_ptr<Material> material;   // 공유 소유
   Mesh* mesh;                            // 비소유 참조
   ```

### 성능 최적화 원칙 (설계 레벨)

1. **측정 우선**
   - 최적화 전 반드시 벤치마크
   - "추측하지 말고 측정하라"

2. **캐시 친화성**
   - 데이터는 연속된 메모리에 배치 (ECS의 강점)
   - 핫 패스에서 포인터 체이싱 최소화

3. **배치 처리**
   - Draw Call 배칭
   - State Change 최소화

4. **비동기 작업**
   - 리소스 로딩은 별도 스레드
   - 커맨드 리스트 기록 병렬화 (Phase 5)

> 실제 최적화 작업 프로세스(벤치마크 필수 등)는 `.claude/guides/optimization.md` 참조

---

## 개발 단계 (Roadmap)

현재 Phase 1 진행 중 (40%). 전체 로드맵:

1. **Phase 1**: 기초 설정 및 DX12 기본 렌더링 (현재)
2. **Phase 2**: 핵심 시스템 구축 (리소스 관리, 셰이더, 기본 렌더링)
3. **Phase 3**: 고급 렌더링 (PBR, 후처리, 최적화)
4. **Phase 4**: 최신 기술 (DXR, Mesh Shaders, VRS)
5. **Phase 5**: 멀티스레딩 & 최적화
6. **Phase 6**: 벤치마킹 & 문서화

자세한 마일스톤은 `Docs/ROADMAP.md` 참조.

---

## 관련 문서

- DX12 핵심 아키텍처: `.claude/guides/dx12.md`
- 코딩 스타일: `.claude/guides/coding-style.md`
- 최적화: `.claude/guides/optimization.md`
