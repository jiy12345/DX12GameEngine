/**
 * @file Window.cpp
 * @brief Win32 윈도우 관리 클래스 구현
 */

#include "Window.h"
#include <stdexcept>

namespace DX12GameEngine
{
    Window::Window(const WindowDesc& desc)
        : m_hwnd(nullptr)
        , m_hInstance(GetModuleHandle(nullptr))
        , m_title(desc.title)
        , m_className(L"DX12GameEngineWindowClass")
        , m_width(desc.width)
        , m_height(desc.height)
        , m_isFullscreen(desc.fullscreen)
        , m_isResizable(desc.resizable)
        , m_isActive(false)
        , m_keyboardCallback(nullptr)
        , m_mouseCallback(nullptr)
        , m_resizeCallback(nullptr)
    {
    }

    Window::~Window()
    {
        Destroy();
    }

    bool Window::RegisterWindowClass()
    {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wc.lpfnWndProc = WindowProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = m_hInstance;
        wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr; // DX12로 렌더링하므로 배경 불필요
        wc.lpszMenuName = nullptr;
        wc.lpszClassName = m_className.c_str();
        wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

        if (!RegisterClassExW(&wc))
        {
            return false;
        }

        return true;
    }

    bool Window::Create()
    {
        // 윈도우 클래스 등록
        if (!RegisterWindowClass())
        {
            return false;
        }

        // 윈도우 스타일 설정
        DWORD style = WS_OVERLAPPEDWINDOW;
        if (!m_isResizable)
        {
            style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
        }

        // 클라이언트 영역 크기를 기준으로 윈도우 크기 계산
        RECT rect = { 0, 0, m_width, m_height };
        AdjustWindowRect(&rect, style, FALSE);

        int windowWidth = rect.right - rect.left;
        int windowHeight = rect.bottom - rect.top;

        // 화면 중앙에 배치
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        int posX = (screenWidth - windowWidth) / 2;
        int posY = (screenHeight - windowHeight) / 2;

        // 윈도우 생성
        m_hwnd = CreateWindowExW(
            0,                      // 확장 스타일
            m_className.c_str(),    // 윈도우 클래스 이름
            m_title.c_str(),        // 윈도우 타이틀
            style,                  // 윈도우 스타일
            posX, posY,             // 위치
            windowWidth, windowHeight, // 크기
            nullptr,                // 부모 윈도우
            nullptr,                // 메뉴
            m_hInstance,            // 인스턴스 핸들
            this                    // lpParam (Window 포인터 전달)
        );

        if (!m_hwnd)
        {
            return false;
        }

        // 윈도우 표시
        ShowWindow(m_hwnd, SW_SHOW);
        UpdateWindow(m_hwnd);

        m_isActive = true;

        return true;
    }

    void Window::Destroy()
    {
        if (m_hwnd)
        {
            DestroyWindow(m_hwnd);
            m_hwnd = nullptr;
        }

        UnregisterClassW(m_className.c_str(), m_hInstance);
        m_isActive = false;
    }

    int Window::Run()
    {
        MSG msg = {};

        while (msg.message != WM_QUIT)
        {
            if (!ProcessMessages())
            {
                break;
            }

            // TODO: 여기서 게임 루프 실행 (렌더링, 업데이트 등)
            // 현재는 메시지 처리만 수행
        }

        return static_cast<int>(msg.wParam);
    }

    bool Window::ProcessMessages()
    {
        MSG msg = {};

        // 모든 메시지 처리
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                return false;
            }

            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        return true;
    }

    LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        Window* window = nullptr;

        if (msg == WM_NCCREATE)
        {
            // CreateWindowEx의 lpParam에서 Window 포인터 추출
            CREATESTRUCTW* pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
            window = reinterpret_cast<Window*>(pCreate->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
            window->m_hwnd = hwnd;
        }
        else
        {
            // 저장된 Window 포인터 가져오기
            window = reinterpret_cast<Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        if (window)
        {
            return window->HandleMessage(msg, wParam, lParam);
        }

        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    LRESULT Window::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_CLOSE:
            PostQuitMessage(0);
            return 0;

        case WM_DESTROY:
            m_isActive = false;
            return 0;

        case WM_ACTIVATE:
            m_isActive = (LOWORD(wParam) != WA_INACTIVE);
            return 0;

        // 키보드 메시지 처리
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
            HandleKeyboard(msg, wParam, lParam);
            return 0;

        // 마우스 메시지 처리
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_MOUSEWHEEL:
            HandleMouse(msg, wParam, lParam);
            return 0;

        // 리사이즈는 다음 커밋에서 구현
        case WM_SIZE:
            // TODO: 리사이즈 처리 (다음 커밋)
            break;
        }

        return DefWindowProcW(m_hwnd, msg, wParam, lParam);
    }

    void Window::HandleKeyboard(UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (!m_keyboardCallback)
        {
            return;
        }

        KeyboardEvent event;
        event.keyCode = wParam;
        event.isPressed = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);

        // 비트 30: 이전 키 상태 (1이면 이미 눌려있던 키)
        event.isRepeat = event.isPressed && ((lParam & 0x40000000) != 0);

        m_keyboardCallback(event);
    }

    void Window::HandleMouse(UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (!m_mouseCallback)
        {
            return;
        }

        MouseEvent event;

        // 마우스 위치 추출 (클라이언트 좌표계)
        event.x = static_cast<int>(static_cast<short>(LOWORD(lParam)));
        event.y = static_cast<int>(static_cast<short>(HIWORD(lParam)));
        event.wheelDelta = 0;

        // 이벤트 타입 설정
        switch (msg)
        {
        case WM_MOUSEMOVE:
            event.type = MouseEvent::Type::Move;
            break;

        case WM_LBUTTONDOWN:
            event.type = MouseEvent::Type::LeftButtonDown;
            SetCapture(m_hwnd); // 마우스 캡처 (윈도우 밖에서도 이벤트 수신)
            break;

        case WM_LBUTTONUP:
            event.type = MouseEvent::Type::LeftButtonUp;
            ReleaseCapture();
            break;

        case WM_RBUTTONDOWN:
            event.type = MouseEvent::Type::RightButtonDown;
            SetCapture(m_hwnd);
            break;

        case WM_RBUTTONUP:
            event.type = MouseEvent::Type::RightButtonUp;
            ReleaseCapture();
            break;

        case WM_MBUTTONDOWN:
            event.type = MouseEvent::Type::MiddleButtonDown;
            SetCapture(m_hwnd);
            break;

        case WM_MBUTTONUP:
            event.type = MouseEvent::Type::MiddleButtonUp;
            ReleaseCapture();
            break;

        case WM_MOUSEWHEEL:
            event.type = MouseEvent::Type::Wheel;
            event.wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
            break;

        default:
            return;
        }

        m_mouseCallback(event);
    }

    void Window::HandleResize(int width, int height)
    {
        // TODO: 다음 커밋에서 구현
    }

} // namespace DX12GameEngine
