# 플랫폼 개념

플랫폼별 시스템(윈도우 관리, 입력 처리 등)과 관련된 개념들을 다룹니다. 플랫폼별로 하위 폴더를 구성하여 향후 멀티 플랫폼 확장에 대비합니다.

## 📑 문서 목록

### Windows (Win32 API) — [📖 폴더 개요](./Windows/README.md)
- [WindowSystem.md](./Windows/WindowSystem.md) - 윈도우 클래스 등록 및 생성, WndProc ([#6](https://github.com/jiy12345/DX12GameEngine/issues/6))
- [MessageLoop.md](./Windows/MessageLoop.md) - 메시지 펌프 (GetMessage/PeekMessage) ([#6](https://github.com/jiy12345/DX12GameEngine/issues/6))
- [InputHandling.md](./Windows/InputHandling.md) - 키보드/마우스 입력 처리 ([#6](https://github.com/jiy12345/DX12GameEngine/issues/6))

### 향후 확장
- Linux/ (미생성) — X11 기반
- macOS/ (미생성) — Cocoa 기반
- Android/ (미생성) — ANativeActivity 기반

## 학습 순서 권장

1. 현재 지원 플랫폼: [Windows/README.md](./Windows/README.md)
2. 각 플랫폼 내 순서: WindowSystem → MessageLoop → InputHandling

## 관련 카테고리

- [DX12/](../DX12/README.md) — DX12 그래픽스 API
- [Rendering/](../Rendering/README.md) — 렌더링 개념
