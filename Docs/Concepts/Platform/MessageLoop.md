# Message Loop

## 개요
Windows 메시지 큐에서 메시지를 가져와 처리하는 루프입니다. 애플리케이션의 메인 이벤트 루프 역할을 합니다.

## 왜 필요한가?

### 문제
- Windows는 이벤트 기반 시스템
- 사용자 입력, 시스템 이벤트가 메시지로 전달됨
- 메시지를 지속적으로 처리해야 애플리케이션이 반응함

### 해결책
- 메시지 루프를 통해 메시지 큐를 모니터링
- 메시지를 가져와 윈도우 프로시저로 디스패치
- WM_QUIT 메시지 수신 시 종료

## 상세 설명

### 메시지 루프의 종류

#### 1. 표준 메시지 루프
```cpp
MSG msg = {};
while (GetMessage(&msg, nullptr, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
}
return (int)msg.wParam;
```
- `GetMessage`: 메시지가 올 때까지 대기 (블로킹)
- WM_QUIT 메시지 수신 시 0 반환하여 루프 종료

#### 2. 게임 루프 (PeekMessage 사용)
```cpp
MSG msg = {};
while (true) {
    if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) break;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    } else {
        // 렌더링 및 업데이트
        Update();
        Render();
    }
}
```
- `PeekMessage`: 논블로킹, 메시지가 없으면 즉시 반환
- 메시지가 없을 때 렌더링/게임 로직 실행

## 주요 특징

- **이벤트 기반**: 메시지 수신 시 처리
- **블로킹 vs 논블로킹**: GetMessage vs PeekMessage
- **메시지 필터링**: 특정 메시지만 처리 가능
- **우선순위**: Paint 메시지는 우선순위가 낮음

## 이 프로젝트의 구현

현재 프로젝트는 표준 메시지 루프를 사용:

```cpp
// Source/Platform/Window.cpp
int Window::Run()
{
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
```

향후 DX12 렌더링 루프 통합 시 PeekMessage 방식으로 변경 예정 (TODO: #9).

## 주요 함수

### GetMessage
```cpp
BOOL GetMessage(
    LPMSG lpMsg,        // 메시지 구조체 포인터
    HWND  hWnd,         // 윈도우 핸들 (nullptr = 모든 윈도우)
    UINT  wMsgFilterMin, // 필터 최소값
    UINT  wMsgFilterMax  // 필터 최댓값
);
```
- 블로킹 함수
- WM_QUIT 수신 시 0 반환

### PeekMessage
```cpp
BOOL PeekMessage(
    LPMSG lpMsg,
    HWND  hWnd,
    UINT  wMsgFilterMin,
    UINT  wMsgFilterMax,
    UINT  wRemoveMsg     // PM_REMOVE or PM_NOREMOVE
);
```
- 논블로킹 함수
- 메시지가 있으면 TRUE, 없으면 FALSE

### TranslateMessage
```cpp
BOOL TranslateMessage(const MSG *lpMsg);
```
- 가상 키 메시지를 문자 메시지로 변환
- WM_KEYDOWN → WM_CHAR

### DispatchMessage
```cpp
LRESULT DispatchMessage(const MSG *lpMsg);
```
- 메시지를 윈도우 프로시저로 전달

## 코드 예제

### 현재 구현 (표준 루프)
```cpp
int Window::Run()
{
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
```

### 향후 구현 (게임 루프 - 계획)
```cpp
int Window::Run()
{
    MSG msg = {};
    while (true)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            // TODO: #9 - 렌더링 루프 통합
            // m_renderCallback();
        }
    }
    return (int)msg.wParam;
}
```

## 주의사항

- ⚠️ GetMessage는 블로킹 함수 - 실시간 렌더링에 부적합
- ⚠️ TranslateMessage를 빼면 문자 입력이 안 됨
- ⚠️ DispatchMessage를 빼면 윈도우가 반응하지 않음
- ⚠️ 메시지 루프는 메인 스레드에서만 실행

## 관련 개념

### 선행 개념
- [WindowSystem.md](./WindowSystem.md) - 윈도우 시스템

### 연관 개념
- [InputHandling.md](./InputHandling.md) - 입력 처리

## 관련 문서

### 구조
- [Window Architecture](../../Structure/Platform/WindowArchitecture.md)

## 참고 자료
- [Microsoft Message Loop 문서](https://learn.microsoft.com/en-us/windows/win32/winmsg/about-messages-and-message-queues)
- [Microsoft GetMessage vs PeekMessage](https://learn.microsoft.com/en-us/windows/win32/winmsg/using-messages-and-message-queues)

## 구현 위치
- `Source/Platform/Window.cpp`: Window::Run() 메서드
