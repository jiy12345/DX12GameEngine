/**
 * @file Main.cpp
 * @brief Basic Sample Application Entry Point
 *
 * DX12GameEngine을 사용한 간단한 샘플 애플리케이션입니다.
 * Engine 클래스를 통해 모든 서브시스템이 자동으로 관리됩니다.
 */

#include <Windows.h>
#include <Core/Engine.h>

using namespace DX12GameEngine;

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

    // 엔진 생성
    Engine engine;

    // 엔진 설정 - 빌드 구성에 맞는 기본값 자동 적용
    EngineDesc desc;  // Debug 빌드면 Debug 기본값, Release면 Release 기본값
    desc.window.title = L"DX12 Game Engine - Basic Sample";

    // 필요시 개별 설정 오버라이드 가능
    // desc.renderer.vsync = false;  // VSync 끄기
    // desc.renderer.msaaSamples = 4;  // MSAA 4x

    // 또는 명시적으로 특정 구성의 기본값 사용:
    // EngineDesc desc = EngineDesc::ForDebug();    // 항상 Debug 설정
    // EngineDesc desc = EngineDesc::ForRelease();  // 항상 Release 설정
    // EngineDesc desc = EngineDesc::ForProfile();  // 항상 Profile 설정

    // 엔진 초기화
    if (!engine.Initialize(desc))
    {
        MessageBoxW(nullptr, L"엔진 초기화 실패!", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    // 게임 루프 실행
    return engine.Run();
}
