/**
 * @file Main.cpp
 * @brief Basic Sample Application Entry Point
 *
 * 이 샘플은 DX12GameEngine의 기본 사용법을 보여줍니다.
 * 현재는 최소한의 코드만 포함되어 있으며,
 * 이슈 #2 (Win32 윈도우) 및 #3-10 (DX12 렌더링) 완료 후 확장될 예정입니다.
 */

#include <Windows.h>

/**
 * @brief 애플리케이션 진입점
 */
int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nShowCmd);

    // TODO: #2 Win32 윈도우 생성
    // TODO: #3-10 DX12 초기화 및 렌더링 루프

    MessageBoxW(
        nullptr,
        L"DX12GameEngine Basic Sample\n\n"
        L"현재 프로젝트 설정만 완료되었습니다.\n"
        L"실제 렌더링은 이슈 #2-10 완료 후 구현됩니다.",
        L"Basic Sample",
        MB_OK | MB_ICONINFORMATION
    );

    return 0;
}
