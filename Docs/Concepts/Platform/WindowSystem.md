# Window System

## 개요
Windows 플랫폼에서 애플리케이션 윈도우를 생성하고 관리하는 시스템입니다. Win32 API를 래핑하여 사용하기 쉬운 인터페이스를 제공합니다.

## 왜 필요한가?

### 문제
- Win32 API는 C 스타일 API로 복잡하고 사용하기 어려움
- 윈도우 프로시저, 메시지 루프 등 보일러플레이트 코드가 많음
- 에러 처리와 리소스 관리가 번거로움

### 해결책
- Win32 API를 C++ 클래스로 래핑
- RAII 패턴으로 리소스 자동 관리
- 콜백 함수를 통한 이벤트 처리 추상화

## 상세 설명

### 윈도우 생성 과정
1. **윈도우 클래스 등록**: `RegisterClassEx`로 윈도우 클래스 등록
2. **윈도우 생성**: `CreateWindowEx`로 윈도우 인스턴스 생성
3. **표시**: `ShowWindow`, `UpdateWindow`로 화면에 표시

### 메시지 처리
Windows는 메시지 기반 시스템으로 동작합니다:
- 사용자 입력, 시스템 이벤트가 메시지로 전달됨
- 메시지 루프에서 메시지를 가져와 윈도우 프로시저로 전달
- 윈도우 프로시저에서 메시지 처리

### 이벤트 콜백
이 프로젝트의 Window 클래스는 콜백 패턴을 사용:
```cpp
window.SetKeyboardCallback([](const KeyboardEvent& event) {
    // 키보드 이벤트 처리
});
```

## 주요 특징

- **RAII 리소스 관리**: 윈도우가 소멸 시 자동으로 정리
- **이벤트 기반 아키텍처**: 콜백 함수로 이벤트 처리
- **입력 처리 통합**: 키보드, 마우스 입력을 통합 관리
- **리사이즈 지원**: 윈도우 크기 변경 이벤트 처리

## 관련 API

### 주요 클래스
```cpp
namespace DX12GameEngine
{
    class Window
    {
    public:
        explicit Window(const WindowDesc& desc);
        ~Window();

        bool Create();
        void Destroy();
        int Run();
        bool ProcessMessages();

        // Getters
        HWND GetHandle() const;
        int GetWidth() const;
        int GetHeight() const;

        // Callbacks
        void SetKeyboardCallback(KeyboardCallback callback);
        void SetMouseCallback(MouseCallback callback);
        void SetResizeCallback(ResizeCallback callback);
    };
}
```

### 구조체
```cpp
struct WindowDesc
{
    std::wstring title = L"DX12 Game Engine";
    int width = 1280;
    int height = 720;
    bool fullscreen = false;
    bool resizable = true;
};

struct KeyboardEvent
{
    WPARAM keyCode;
    bool isPressed;
    bool isRepeat;
};

struct MouseEvent
{
    enum class Type { Move, LeftButtonDown, LeftButtonUp, ... };
    Type type;
    int x, y;
    int wheelDelta;
};
```

## 코드 예제

### 윈도우 생성 및 사용
```cpp
// Source/Samples/Basic/Main.cpp 참조

// 윈도우 파라미터 설정
WindowDesc desc;
desc.title = L"My Application";
desc.width = 1280;
desc.height = 720;
desc.resizable = true;

// 윈도우 생성
Window window(desc);
if (!window.Create()) {
    return -1;
}

// 이벤트 콜백 등록
window.SetKeyboardCallback(OnKeyboard);
window.SetMouseCallback(OnMouse);
window.SetResizeCallback(OnResize);

// 메시지 루프 실행
return window.Run();
```

## 주의사항

- ⚠️ Window 객체는 복사 불가 (복사 생성자/대입 연산자 삭제됨)
- ⚠️ Create()를 호출하기 전에는 윈도우가 생성되지 않음
- ⚠️ 윈도우 프로시저는 정적 멤버 함수여야 함 (Win32 제약)
- ⚠️ 메시지 루프는 메인 스레드에서 실행되어야 함

## 관련 개념

### 연관 개념
- [MessageLoop.md](./MessageLoop.md) - 메시지 루프 개념
- [InputHandling.md](./InputHandling.md) - 입력 처리 개념

## 관련 문서

### 구조
- [Window Architecture](../../Structure/Platform/WindowArchitecture.md)

### 활용법
- [Creating Window](../../Usage/BasicTasks/CreatingWindow.md)

## 참고 자료
- [Microsoft Win32 Window Class 문서](https://learn.microsoft.com/en-us/windows/win32/winmsg/about-window-classes)
- [Microsoft Win32 Window Procedures 문서](https://learn.microsoft.com/en-us/windows/win32/winmsg/about-window-procedures)

## 구현 위치
- `Source/Platform/Window.h`: 42-206
- `Source/Platform/Window.cpp`
- `Samples/Basic/Main.cpp`: 사용 예제
