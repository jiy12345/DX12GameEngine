cmake_minimum_required(VERSION 3.20)
project(DX12GameEngine VERSION 0.1.0 LANGUAGES CXX)

# C++ 표준 설정
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 출력 디렉토리 설정
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Lib)

# Configuration별 출력 디렉토리
foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/Bin/${CONFIG})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/Lib/${CONFIG})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/Lib/${CONFIG})
endforeach()

# Windows 플랫폼 확인
if(NOT WIN32)
    message(FATAL_ERROR "This project requires Windows (DirectX 12)")
endif()

# Visual Studio 설정
if(MSVC)
    # 경고 레벨
    add_compile_options(/W4)

    # 멀티프로세서 컴파일
    add_compile_options(/MP)

    # UTF-8 소스 코드 인코딩 (C4819 경고 해결)
    add_compile_options(/utf-8)

    # Windows 매크로
    add_compile_definitions(
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        UNICODE
        _UNICODE
    )

    # Configuration별 설정
    # Debug
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd /Zi /Od")
    add_compile_definitions($<$<CONFIG:Debug>:_DEBUG DX12_DEBUG>)

    # Release
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD /O2 /Ob2")
    add_compile_definitions($<$<CONFIG:Release>:NDEBUG>)

    # Profile (Release 기반)
    set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAGS_RELEASE}")
    set(CMAKE_EXE_LINKER_FLAGS_PROFILE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
    set(CMAKE_SHARED_LINKER_FLAGS_PROFILE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE}")
    add_compile_definitions($<$<CONFIG:Profile>:NDEBUG PROFILE_BUILD>)

    # Configuration 타입에 Profile 추가
    if(NOT "Profile" IN_LIST CMAKE_CONFIGURATION_TYPES)
        list(APPEND CMAKE_CONFIGURATION_TYPES Profile)
        set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING
            "Available build configurations"
            FORCE)
    endif()
endif()

# DirectX 12 라이브러리
set(DX12_LIBRARIES
    d3d12.lib
    dxgi.lib
    d3dcompiler.lib
    dxguid.lib
)

# 서브 프로젝트
add_subdirectory(Source)
add_subdirectory(Samples)
add_subdirectory(Benchmarks)

# IDE에서 폴더 구조 사용
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
