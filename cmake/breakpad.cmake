cmake_minimum_required(VERSION 3.14)

include(FetchContent)

set(FETCHCONTENT_QUIET OFF)
FetchContent_Declare(
    breakpad
    GIT_REPOSITORY https://chromium.googlesource.com/breakpad/breakpad
    GIT_TAG        origin/main
)
FetchContent_MakeAvailable(breakpad)

set(BREAKPAD_INCLUDE_DIR ${breakpad_SOURCE_DIR}/src)

add_library(Breakpad STATIC
   ${breakpad_SOURCE_DIR}/src/common/windows/guid_string.h
   ${breakpad_SOURCE_DIR}/src/common/windows/guid_string.cc
   ${breakpad_SOURCE_DIR}/src/client/windows/handler/exception_handler.h
   ${breakpad_SOURCE_DIR}/src/client/windows/handler/exception_handler.cc
   ${breakpad_SOURCE_DIR}/src/client/windows/crash_generation/crash_generation_client.h
   ${breakpad_SOURCE_DIR}/src/client/windows/crash_generation/crash_generation_client.cc
)
target_compile_definitions(Breakpad PRIVATE UNICODE _UNICODE)
target_include_directories(Breakpad PRIVATE ${BREAKPAD_INCLUDE_DIR})
