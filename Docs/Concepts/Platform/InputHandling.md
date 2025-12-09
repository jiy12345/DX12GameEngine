# Input Handling

## 개요
키보드와 마우스 입력을 처리하는 시스템입니다. Win32 입력 메시지를 구조화된 이벤트로 변환하고 콜백을 통해 전달합니다.

## 왜 필요한가?

### 문제
- Win32 입력 메시지는 저수준이고 사용하기 불편함
- 키 반복(repeat), 마우스 좌표 등을 직접 처리해야 함
- 입력 처리 코드가 윈도우 프로시저에 섞여 복잡해짐

### 해결책
- 입력 메시지를 구조화된 이벤트로 변환
- 콜백 패턴으로 입력 처리 로직 분리
- 이벤트 데이터 구조화 (KeyboardEvent, MouseEvent)

## 상세 설명

### 키보드 입력 처리

#### 입력 메시지
- **WM_KEYDOWN**: 키 눌림
- **WM_KEYUP**: 키 떼짐
- **WM_SYSKEYDOWN**: 시스템 키 눌림 (Alt 등)
- **WM_SYSKEYUP**: 시스템 키 떼짐

#### 키 반복 (Repeat)
- Windows는 키를 누르고 있으면 WM_KEYDOWN을 반복 전송
- lParam의 비트 30으로 반복 여부 판단:
  ```cpp
  bool isRepeat = (lParam & 0x40000000) != 0;
  ```

### 마우스 입력 처리

#### 입력 메시지
- **WM_MOUSEMOVE**: 마우스 이동
- **WM_LBUTTONDOWN/UP**: 좌클릭
- **WM_RBUTTONDOWN/UP**: 우클릭
- **WM_MBUTTONDOWN/UP**: 휠 클릭
- **WM_MOUSEWHEEL**: 휠 스크롤

#### 마우스 좌표
- lParam에 x, y 좌표가 packed:
  ```cpp
  int x = GET_X_LPARAM(lParam);
  int y = GET_Y_LPARAM(lParam);
  ```

## 주요 특징

- **이벤트 구조화**: 원시 메시지를 구조체로 변환
- **콜백 패턴**: 입력 처리 로직을 외부로 분리
- **키 반복 감지**: 자동으로 repeat 플래그 설정
- **마우스 버튼 구분**: 좌/우/휠 버튼 이벤트 분리

## 이벤트 구조체

### KeyboardEvent
```cpp
struct KeyboardEvent
{
    WPARAM keyCode;    // 가상 키 코드 (VK_ESCAPE 등)
    bool isPressed;    // true = 눌림, false = 떼짐
    bool isRepeat;     // true = 키 반복
};
```

### MouseEvent
```cpp
struct MouseEvent
{
    enum class Type {
        Move,
        LeftButtonDown,
        LeftButtonUp,
        RightButtonDown,
        RightButtonUp,
        MiddleButtonDown,
        MiddleButtonUp,
        Wheel
    };

    Type type;         // 이벤트 타입
    int x;             // 마우스 X 좌표
    int y;             // 마우스 Y 좌표
    int wheelDelta;    // 휠 스크롤 양
};
```

## 코드 예제

### 콜백 등록
```cpp
// 키보드 콜백
window.SetKeyboardCallback([](const KeyboardEvent& event) {
    if (event.isPressed && event.keyCode == VK_ESCAPE) {
        PostQuitMessage(0);  // ESC로 종료
    }
});

// 마우스 콜백
window.SetMouseCallback([](const MouseEvent& event) {
    switch (event.type) {
        case MouseEvent::Type::LeftButtonDown:
            // 좌클릭 처리
            break;
        case MouseEvent::Type::Move:
            // 마우스 이동 처리
            break;
    }
});
```

### 구현 예제
```cpp
// Source/Platform/Window.cpp 참조
void Window::HandleKeyboard(UINT msg, WPARAM wParam, LPARAM lParam)
{
    KeyboardEvent event = {};
    event.keyCode = wParam;
    event.isPressed = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
    event.isRepeat = (lParam & 0x40000000) != 0;

    if (m_keyboardCallback) {
        m_keyboardCallback(event);
    }
}
```

## 주의사항

- ⚠️ 가상 키 코드는 물리적 키 위치가 아닌 논리적 키
- ⚠️ 키 반복은 OS 설정에 따라 다름
- ⚠️ 마우스 좌표는 클라이언트 영역 기준
- ⚠️ 콜백이 설정되지 않으면 입력이 처리되지 않음

## 향후 개선 (TODO)

- [ ] 키 상태 추적 (IsKeyDown 등)
- [ ] 마우스 캡처 지원
- [ ] 게임패드 입력 지원
- [ ] 입력 버퍼링/리플레이

## 관련 개념

### 선행 개념
- [WindowSystem.md](./WindowSystem.md) - 윈도우 시스템
- [MessageLoop.md](./MessageLoop.md) - 메시지 루프

## 관련 문서

### 구조
- [Window Architecture](../../Structure/Platform/WindowArchitecture.md)

### 활용법
- [Creating Window](../../Usage/BasicTasks/CreatingWindow.md)

## 참고 자료
- [Microsoft Keyboard Input 문서](https://learn.microsoft.com/en-us/windows/win32/inputdev/keyboard-input)
- [Microsoft Mouse Input 문서](https://learn.microsoft.com/en-us/windows/win32/inputdev/mouse-input)
- [Virtual Key Codes](https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes)

## 구현 위치
- `Source/Platform/Window.h`: 30-60 (이벤트 구조체)
- `Source/Platform/Window.cpp`: HandleKeyboard(), HandleMouse() 메서드
- `Samples/Basic/Main.cpp`: 19-82 (사용 예제)
