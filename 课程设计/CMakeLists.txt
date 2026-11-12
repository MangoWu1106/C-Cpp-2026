cmake_minimum_required(VERSION 3.24)
project(Gobang LANGUAGES C)

if(NOT WIN32)
    message(FATAL_ERROR "Gobang currently targets Windows.")
endif()

include(FetchContent)
# Allow raylib's older CMake policy declaration when configuring with CMake 4.
set(CMAKE_POLICY_VERSION_MINIMUM 3.10)
set(BUILD_EXAMPLES OFF CACHE BOOL "Build raylib examples" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
set(USE_EXTERNAL_GLFW OFF CACHE STRING "Use an installed GLFW" FORCE)
set(CUSTOMIZE_BUILD ON CACHE BOOL "Configure raylib modules" FORCE)
set(SUPPORT_MODULE_RAUDIO OFF CACHE BOOL "Build the raylib audio module" FORCE)
FetchContent_Declare(raylib
    URL https://codeload.github.com/raysan5/raylib/tar.gz/refs/tags/5.5
    URL_HASH SHA256=aea98ecf5bc5c5e0b789a76de0083a21a70457050ea4cc2aec7566935f5e258e
)
FetchContent_MakeAvailable(raylib)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/utf-8 /W4)
else()
    add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8 -Wall -Wextra -Wpedantic)
endif()

add_executable(Gobang WIN32
    src/main.c
    src/game.c
    src/ai.c
    src/search_worker.c
    src/ui.c
    src/assets.c
)
target_link_libraries(Gobang PRIVATE raylib user32)
if(MSVC)
    target_link_options(Gobang PRIVATE /ENTRY:mainCRTStartup)
elseif(MINGW)
    target_link_options(Gobang PRIVATE -static -static-libgcc)
endif()

add_custom_command(TARGET Gobang POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:Gobang>/assets"
    VERBATIM
)
install(TARGETS Gobang RUNTIME DESTINATION . COMPONENT Game)
install(DIRECTORY assets/ DESTINATION assets COMPONENT Game)
install(FILES README.zh-CN.md DESTINATION . COMPONENT Game)

include(CTest)
if(BUILD_TESTING)
    add_executable(test_game tests/test_game.c src/game.c)
    add_executable(test_ai tests/test_ai.c src/game.c)
    add_executable(test_worker tests/test_worker.c src/search_worker.c src/game.c)
    foreach(test_name IN ITEMS game ai worker)
        target_include_directories(test_${test_name} PRIVATE src)
        add_test(NAME ${test_name} COMMAND test_${test_name})
    endforeach()
endif()
