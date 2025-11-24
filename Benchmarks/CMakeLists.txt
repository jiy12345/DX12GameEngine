# 벤치마크 프로젝트
add_executable(EngineBenchmark)

# 소스 파일
target_sources(EngineBenchmark PRIVATE
    Main.cpp
)

# Engine 라이브러리 링크
target_link_libraries(EngineBenchmark
    PRIVATE
        Engine
)

# IDE 폴더 설정
set_target_properties(EngineBenchmark PROPERTIES FOLDER "Benchmarks")

# 콘솔 애플리케이션 (벤치마크 결과 출력용)
if(MSVC)
    set_target_properties(EngineBenchmark PROPERTIES
        WIN32_EXECUTABLE FALSE
    )
endif()
