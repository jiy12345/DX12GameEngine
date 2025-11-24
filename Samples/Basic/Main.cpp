/**
 * @file Main.cpp
 * @brief Basic Sample Application Entry Point
 *
 * 이 샘플은 DX12GameEngine의 기본 사용법을 보여줍니다.
 * 현재는 Win32 윈도우 생성 및 입력 처리를 시연합니다.
 */

#include <Windows.h>
#include <Platform/Window.h>
#include <string>
#include <sstream>

using namespace DX12GameEngine;

/**
 * @brief 키보드 이벤트 핸들러
 */
void OnKeyboard(const KeyboardEvent& event)
{
    if (event.isPressed && event.keyCode == VK_ESCAPE)
    {
        PostQuitMessage(0);
    }

    if (event.isPressed && !event.isRepeat)
    {
        std::wstringstream ss;
        ss << L"Key Pressed: " << event.keyCode << L"\n";
        OutputDebugStringW(ss.str().c_str());
    }
}

/**
 * @brief 마우스 이벤트 핸들러
 */
void OnMouse(const MouseEvent& event)
{
    std::wstringstream ss;

    switch (event.type)
    {
    case MouseEvent::Type::Move:
        ss << L"Mouse Move: (" << event.x << L", " << event.y << L")\n";
        break;

    case MouseEvent::Type::LeftButtonDown:
        ss << L"Left Button Down at (" << event.x << L", " << event.y << L")\n";
        break;

    case MouseEvent::Type::LeftButtonUp:
        ss << L"Left Button Up at (" << event.x << L", " << event.y << L")\n";
        break;

    case MouseEvent::Type::RightButtonDown:
        ss << L"Right Button Down at (" << event.x << L", " << event.y << L")\n";
        break;

    case MouseEvent::Type::RightButtonUp:
        ss << L"Right Button Up at (" << event.x << L", " << event.y << L")\n";
        break;

    case MouseEvent::Type::Wheel:
        ss << L"Mouse Wheel: " << event.wheelDelta << L"\n";
        break;
    }

    if (!ss.str().empty())
    {
        OutputDebugStringW(ss.str().c_str());
    }
}

/**
 * @brief 윈도우 리사이즈 핸들러
 */
void OnResize(int width, int height)
{
    std::wstringstream ss;
    ss << L"Window Resized: " << width << L" x " << height << L"\n";
    OutputDebugStringW(ss.str().c_str());
}

/**
 * @brief 애플리케이션 진입점
 */
int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nShowCmd);

    // 윈도우 생성 파라미터 설정
    WindowDesc desc;
    desc.title = L"DX12 Game Engine - Basic Sample";
    desc.width = 1280;
    desc.height = 720;
    desc.resizable = true;

    // 윈도우 생성
    Window window(desc);

    if (!window.Create())
    {
        MessageBoxW(nullptr, L"윈도우 생성 실패!", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    // 이벤트 콜백 등록
    window.SetKeyboardCallback(OnKeyboard);
    window.SetMouseCallback(OnMouse);
    window.SetResizeCallback(OnResize);

    OutputDebugStringW(L"===========================================\n");
    OutputDebugStringW(L"  DX12 Game Engine - Basic Sample\n");
    OutputDebugStringW(L"===========================================\n");
    OutputDebugStringW(L"Controls:\n");
    OutputDebugStringW(L"  ESC - Exit\n");
    OutputDebugStringW(L"  Mouse - Move and click to see events\n");
    OutputDebugStringW(L"  Keyboard - Press keys to see events\n");
    OutputDebugStringW(L"  Resize - Drag window edges\n");
    OutputDebugStringW(L"===========================================\n\n");

    // TODO: #3-10 DX12 초기화 및 렌더링 루프
    // 현재는 윈도우 메시지만 처리

    // 메시지 루프 실행
    return window.Run();
}
