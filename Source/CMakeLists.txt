# Engine 라이브러리
add_library(Engine STATIC)

# 소스 파일 수집
file(GLOB_RECURSE ENGINE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/Core/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Graphics/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Platform/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Math/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Utils/*.cpp"
)

file(GLOB_RECURSE ENGINE_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/Core/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Graphics/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Platform/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Math/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Utils/*.h"
)

target_sources(Engine PRIVATE
    ${ENGINE_SOURCES}
    ${ENGINE_HEADERS}
)

# Include 디렉토리
target_include_directories(Engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# DirectX 12 라이브러리 링크
target_link_libraries(Engine
    PUBLIC
        ${DX12_LIBRARIES}
)

# IDE 폴더 구조
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ENGINE_SOURCES} ${ENGINE_HEADERS})

# 폴더 설정
set_target_properties(Engine PROPERTIES FOLDER "Engine")
