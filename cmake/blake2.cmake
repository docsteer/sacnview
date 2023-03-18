cmake_minimum_required(VERSION 3.14)
include(FetchContent)

set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(
    blake2
    GIT_REPOSITORY  https://github.com/BLAKE2/BLAKE2.git
    GIT_TAG         ed1974ea83433eba7b2d95c5dcd9ac33cb847913
    GIT_SHALLOW     TRUE
    GIT_PROGRESS    TRUE
    PATCH_COMMAND   git reset --hard && git apply --ignore-space-change --ignore-whitespace ${PROJECT_SOURCE_DIR}/patch/blake2.patch
    UPDATE_COMMAND  ""
)
FetchContent_MakeAvailable(blake2)

# Check if SSE2 is supported by target
include(CheckCCompilerFlag)
if (CMAKE_C_COMPILER_ID MATCHES "MSVC")
    if(MSVC_C_ARCHITECTURE_ID MATCHES 64)
        set(SSE2_SUPPORTED 1)
    elseif()
        check_c_compiler_flag("/arch:SSE2" SSE2_SUPPORTED)
    endif()
else()
    check_c_compiler_flag("-msse2" SSE2_SUPPORTED)
endif()
if(SSE2_SUPPORTED)
    set(BLAKE2_INCLUDE_DIR ${blake2_SOURCE_DIR}/sse)
else()
    set(BLAKE2_INCLUDE_DIR ${blake2_SOURCE_DIR}/ref)
endif()

file(GLOB_RECURSE BLAKE2_HEADERS CONFIGURE_DEPENDS "${BLAKE2_INCLUDE_DIR}/*.h")
file(GLOB_RECURSE BLAKE2_SOURCES CONFIGURE_DEPENDS "${BLAKE2_INCLUDE_DIR}/*.c")
set_source_files_properties(${BLAKE2_SOURCES} PROPERTIES LANGUAGE C)
add_library(Blake2 STATIC
    ${BLAKE2_HEADERS}
    ${BLAKE2_SOURCES}
)
target_include_directories(Blake2 PRIVATE ${BLAKE2_INCLUDE_DIR})
