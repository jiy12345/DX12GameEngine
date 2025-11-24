/**
 * @file Main.cpp
 * @brief DX12GameEngine Benchmark Tool
 *
 * 엔진의 성능을 측정하고 결과를 출력하는 벤치마크 도구입니다.
 * 현재는 프로젝트 구조만 생성되어 있으며,
 * 이슈 #14-15 (벤치마크 프레임워크) 완료 후 실제 측정 기능이 추가됩니다.
 */

#include <Windows.h>
#include <iostream>
#include <string>

/**
 * @brief 벤치마크 정보 출력
 */
void PrintBenchmarkInfo()
{
    std::cout << "=================================================\n";
    std::cout << "   DX12GameEngine Benchmark Tool v0.1.0\n";
    std::cout << "=================================================\n\n";

    std::cout << "현재 사용 가능한 벤치마크:\n";
    std::cout << "  (이슈 #14-15 완료 후 추가 예정)\n\n";

    std::cout << "계획된 벤치마크:\n";
    std::cout << "  - FPS 측정\n";
    std::cout << "  - 프레임 타임 측정\n";
    std::cout << "  - GPU 타이밍 (Timestamp Query)\n";
    std::cout << "  - PSO 생성 시간\n";
    std::cout << "  - 디스크립터 할당 성능\n\n";
}

/**
 * @brief 벤치마크 도구 진입점
 */
int main(int argc, char* argv[])
{
    // TODO: #14 기본 벤치마크 프레임워크 구현
    // TODO: #15 GPU 타이밍 구현

    PrintBenchmarkInfo();

    std::cout << "벤치마크 도구가 준비되면 다음과 같이 실행할 수 있습니다:\n";
    std::cout << "  EngineBenchmark.exe --all\n";
    std::cout << "  EngineBenchmark.exe --category <category_name>\n";
    std::cout << "  EngineBenchmark.exe --output <json|csv>\n\n";

    std::cout << "결과는 Benchmarks/Results/ 폴더에 저장됩니다.\n";

    return 0;
}
