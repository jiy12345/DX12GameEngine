# Basic 샘플 애플리케이션
add_executable(BasicSample)

# 소스 파일
target_sources(BasicSample PRIVATE
    Basic/Main.cpp
)

# Engine 라이브러리 링크
target_link_libraries(BasicSample
    PRIVATE
        Engine
)

# IDE 폴더 설정
set_target_properties(BasicSample PROPERTIES FOLDER "Samples")

# Windows 서브시스템 설정 (콘솔 윈도우 없이 실행)
if(MSVC)
    set_target_properties(BasicSample PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()
