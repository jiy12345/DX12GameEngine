/**
 * @file Window.h
 * @brief Win32 윈도우 관리 클래스
 *
 * Win32 API를 사용한 윈도우 생성 및 관리를 담당합니다.
 * 이슈 #2: Win32 윈도우 및 입력 시스템
 */

#pragma once

#include <Windows.h>
#include <string>
#include <functional>

namespace DX12GameEngine
{
    /**
     * @brief 윈도우 생성 파라미터
     */
    struct WindowDesc
    {
        std::wstring title = L"DX12 Game Engine";
        int width = 1280;
        int height = 720;
        bool fullscreen = false;
        bool resizable = true;
    };

    /**
     * @brief 키보드 입력 이벤트 데이터
     */
    struct KeyboardEvent
    {
        WPARAM keyCode;
        bool isPressed;
        bool isRepeat;
    };

    /**
     * @brief 마우스 입력 이벤트 데이터
     */
    struct MouseEvent
    {
        enum class Type
        {
            Move,
            LeftButtonDown,
            LeftButtonUp,
            RightButtonDown,
            RightButtonUp,
            MiddleButtonDown,
            MiddleButtonUp,
            Wheel
        };

        Type type;
        int x;
        int y;
        int wheelDelta;
    };

    /**
     * @brief Win32 윈도우 클래스
     *
     * Win32 API를 래핑하여 윈도우 생성, 메시지 처리, 입력 이벤트를 관리합니다.
     */
    class Window
    {
    public:
        /**
         * @brief 기본 생성자
         */
        Window();

        /**
         * @brief 소멸자
         */
        ~Window();

        // 복사 금지
        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        /**
         * @brief 윈도우를 생성하고 표시합니다
         * @param desc 윈도우 생성 파라미터
         * @return 성공 시 true
         */
        bool Create(const WindowDesc& desc);

        /**
         * @brief 윈도우를 종료합니다
         */
        void Destroy();

        /**
         * @brief 메시지 루프를 실행합니다
         * @return 종료 코드
         */
        int Run();

        /**
         * @brief 윈도우 메시지를 처리합니다
         * @return 계속 실행 시 true, 종료 시 false
         */
        bool ProcessMessages();

        /**
         * @brief 윈도우 핸들을 반환합니다
         * @return HWND
         */
        HWND GetHandle() const { return m_hwnd; }

        /**
         * @brief 윈도우 너비를 반환합니다
         * @return 픽셀 단위 너비
         */
        int GetWidth() const { return m_width; }

        /**
         * @brief 윈도우 높이를 반환합니다
         * @return 픽셀 단위 높이
         */
        int GetHeight() const { return m_height; }

        /**
         * @brief 윈도우가 활성 상태인지 확인합니다
         * @return 활성 상태 시 true
         */
        bool IsActive() const { return m_isActive; }

        /**
         * @brief 전체화면 모드인지 확인합니다
         * @return 전체화면 시 true
         */
        bool IsFullscreen() const { return m_isFullscreen; }

        // 이벤트 콜백 설정
        using KeyboardCallback = std::function<void(const KeyboardEvent&)>;
        using MouseCallback = std::function<void(const MouseEvent&)>;
        using ResizeCallback = std::function<void(int width, int height)>;

        /**
         * @brief 키보드 이벤트 콜백을 설정합니다
         */
        void SetKeyboardCallback(KeyboardCallback callback) { m_keyboardCallback = callback; }

        /**
         * @brief 마우스 이벤트 콜백을 설정합니다
         */
        void SetMouseCallback(MouseCallback callback) { m_mouseCallback = callback; }

        /**
         * @brief 윈도우 리사이즈 콜백을 설정합니다
         */
        void SetResizeCallback(ResizeCallback callback) { m_resizeCallback = callback; }

    private:
        /**
         * @brief 윈도우 클래스를 등록합니다
         * @return 성공 시 true
         */
        bool RegisterWindowClass();

        /**
         * @brief Win32 메시지 프로시저
         */
        static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

        /**
         * @brief 실제 메시지 처리 함수
         */
        LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

        /**
         * @brief 키보드 메시지 처리
         */
        void HandleKeyboard(UINT msg, WPARAM wParam, LPARAM lParam);

        /**
         * @brief 마우스 메시지 처리
         */
        void HandleMouse(UINT msg, WPARAM wParam, LPARAM lParam);

        /**
         * @brief 윈도우 리사이즈 처리
         */
        void HandleResize(int width, int height);

    private:
        // 윈도우 정보
        HWND m_hwnd;
        HINSTANCE m_hInstance;
        std::wstring m_title;
        std::wstring m_className;
        int m_width;
        int m_height;
        bool m_isFullscreen;
        bool m_isResizable;
        bool m_isActive;

        // 이벤트 콜백
        KeyboardCallback m_keyboardCallback;
        MouseCallback m_mouseCallback;
        ResizeCallback m_resizeCallback;
    };

} // namespace DX12GameEngine
