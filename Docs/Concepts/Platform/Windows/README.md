# Windows (Win32) 플랫폼 — 폴더 개요

이 폴더는 **Windows 플랫폼(Win32 API) 특화** 개념들을 다룹니다.

## 📑 이 폴더의 문서

| 문서 | 상태 | 내용 |
|------|------|------|
| [WindowSystem.md](./WindowSystem.md) | ✅ [#6](https://github.com/jiy12345/DX12GameEngine/issues/6) | Win32 윈도우 클래스 등록, 윈도우 생성, WndProc |
| [MessageLoop.md](./MessageLoop.md) | ✅ [#6](https://github.com/jiy12345/DX12GameEngine/issues/6) | GetMessage/PeekMessage 기반 메시지 펌프 |
| [InputHandling.md](./InputHandling.md) | ✅ [#6](https://github.com/jiy12345/DX12GameEngine/issues/6) | 키보드/마우스 입력, 가상 키코드 |

## 개요

게임이 화면에 그리고 사용자 입력을 받으려면 OS와 상호작용해야 합니다. Windows에서는 **Win32 API**가 이 역할을 담당합니다. 이 폴더의 문서들은 Win32 기반의 윈도우, 메시지, 입력 처리를 다룹니다.

### Win32 상호작용의 기본 흐름

```
[앱 시작]
  │
  ├─ RegisterClassEx() : 윈도우 클래스 등록 (WndProc 연결)
  ├─ CreateWindowEx()  : 윈도우 생성
  ├─ ShowWindow()
  │
[메시지 루프]
  │
  ├─ PeekMessage() / GetMessage() : 메시지 수신
  ├─ TranslateMessage() + DispatchMessage()
  │     ↓
  │  WndProc 호출 (윈도우별 메시지 처리)
  │     ├─ WM_KEYDOWN / WM_MOUSEMOVE → 입력 처리
  │     ├─ WM_SIZE → 리사이즈 처리
  │     └─ WM_DESTROY → 종료
  │
[종료]
  └─ DestroyWindow(), UnregisterClass()
```

## 학습 순서 권장

1. **기초**: [WindowSystem.md](./WindowSystem.md) — 윈도우 생성과 WndProc 이해
2. **루프**: [MessageLoop.md](./MessageLoop.md) — 메시지 수신/분배 메커니즘
3. **상호작용**: [InputHandling.md](./InputHandling.md) — 키보드/마우스 처리

## 왜 Windows/ 서브폴더로 분리했나?

현재는 Windows 전용이지만, 추후 **Linux/X11, macOS/Cocoa, Android** 등 다른 플랫폼 지원 시 같은 개념들이 플랫폼별로 나뉠 수 있습니다:

```
Platform/
├── Windows/     ← 현재
├── Linux/       ← 향후
├── macOS/       ← 향후
└── Android/     ← 향후
```

각 플랫폼 폴더에 동일 구조(WindowSystem, MessageLoop, InputHandling 등)가 들어갈 수 있도록 설계되었습니다.

## 관련 카테고리

- [Structure/Platform/](../../../Structure/Platform/) — 플랫폼 추상화 아키텍처
- [Usage/BasicTasks/](../../../Usage/BasicTasks/) — 플랫폼 활용 가이드
