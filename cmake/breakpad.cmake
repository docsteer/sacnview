cmake_minimum_required(VERSION 3.16.9)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(
    breakpad
    GIT_REPOSITORY  https://chromium.googlesource.com/breakpad/breakpad
    GIT_TAG         origin/main
    GIT_SHALLOW     TRUE
    GIT_PROGRESS    TRUE
)
FetchContent_MakeAvailable(breakpad)
set(BREAKPAD_INCLUDE_DIR ${breakpad_SOURCE_DIR}/src)

FetchContent_Declare(
    lss
    GIT_REPOSITORY  https://chromium.googlesource.com/linux-syscall-support
    GIT_TAG         origin/main
    GIT_SHALLOW     TRUE
    GIT_PROGRESS    TRUE
    SOURCE_DIR      ${breakpad_SOURCE_DIR}/src/third_party/lss
)
FetchContent_MakeAvailable(lss)

if (UNIX)
    list(APPEND BREAKPAD_SOURCES
        ${BREAKPAD_INCLUDE_DIR}/client/minidump_file_writer.cc
        ${BREAKPAD_INCLUDE_DIR}/common/convert_UTF.cc
        ${BREAKPAD_INCLUDE_DIR}/common/md5.cc
        ${BREAKPAD_INCLUDE_DIR}/common/string_conversion.cc
    )
endif()

if(APPLE)
    list(APPEND BREAKPAD_HEADERS
        ${BREAKPAD_INCLUDE_DIR}/client/mac/handler/exception_handler.h
    )
    list(APPEND BREAKPAD_SOURCES
        ${BREAKPAD_INCLUDE_DIR}/client/mac/crash_generation/crash_generation_client.cc
        ${BREAKPAD_INCLUDE_DIR}/client/mac/handler/breakpad_nlist_64.cc
        ${BREAKPAD_INCLUDE_DIR}/client/mac/handler/dynamic_images.cc
        ${BREAKPAD_INCLUDE_DIR}/client/mac/handler/exception_handler.cc
        ${BREAKPAD_INCLUDE_DIR}/client/mac/handler/minidump_generator.cc
        ${BREAKPAD_INCLUDE_DIR}/client/mac/handler/protected_memory_allocator.cc
        ${BREAKPAD_INCLUDE_DIR}/common/mac/MachIPC.mm
        ${BREAKPAD_INCLUDE_DIR}/common/mac/arch_utilities.cc
        ${BREAKPAD_INCLUDE_DIR}/common/mac/bootstrap_compat.cc
        ${BREAKPAD_INCLUDE_DIR}/common/mac/dump_syms.cc
        ${BREAKPAD_INCLUDE_DIR}/common/mac/file_id.cc
        ${BREAKPAD_INCLUDE_DIR}/common/mac/launch_reporter.cc
        ${BREAKPAD_INCLUDE_DIR}/common/mac/macho_id.cc
        ${BREAKPAD_INCLUDE_DIR}/common/mac/macho_reader.cc
        ${BREAKPAD_INCLUDE_DIR}/common/mac/macho_utilities.cc
        ${BREAKPAD_INCLUDE_DIR}/common/mac/macho_walker.cc
        ${BREAKPAD_INCLUDE_DIR}/common/mac/string_utilities.cc
    )
endif()

if((UNIX) AND (NOT APPLE))
    list(APPEND BREAKPAD_HEADERS
        ${BREAKPAD_INCLUDE_DIR}/client/linux/handler/exception_handler.h
    )
    list(APPEND BREAKPAD_SOURCES
        ${BREAKPAD_INCLUDE_DIR}/client/linux/crash_generation/crash_generation_client.cc
        ${BREAKPAD_INCLUDE_DIR}/client/linux/dump_writer_common/thread_info.cc
        ${BREAKPAD_INCLUDE_DIR}/client/linux/dump_writer_common/ucontext_reader.cc
        ${BREAKPAD_INCLUDE_DIR}/client/linux/handler/exception_handler.cc
        ${BREAKPAD_INCLUDE_DIR}/client/linux/handler/minidump_descriptor.cc
        ${BREAKPAD_INCLUDE_DIR}/client/linux/log/log.cc
        ${BREAKPAD_INCLUDE_DIR}/client/linux/microdump_writer/microdump_writer.cc
        ${BREAKPAD_INCLUDE_DIR}/client/linux/minidump_writer/linux_core_dumper.cc
        ${BREAKPAD_INCLUDE_DIR}/client/linux/minidump_writer/linux_dumper.cc
        ${BREAKPAD_INCLUDE_DIR}/client/linux/minidump_writer/linux_ptrace_dumper.cc
        ${BREAKPAD_INCLUDE_DIR}/client/linux/minidump_writer/minidump_writer.cc
        ${BREAKPAD_INCLUDE_DIR}/client/linux/minidump_writer/pe_file.cc
        ${BREAKPAD_INCLUDE_DIR}/common/linux/breakpad_getcontext.S
        ${BREAKPAD_INCLUDE_DIR}/common/linux/elfutils.cc
        ${BREAKPAD_INCLUDE_DIR}/common/linux/file_id.cc
        ${BREAKPAD_INCLUDE_DIR}/common/linux/guid_creator.cc
        ${BREAKPAD_INCLUDE_DIR}/common/linux/linux_libc_support.cc
        ${BREAKPAD_INCLUDE_DIR}/common/linux/memory_mapped_file.cc
        ${BREAKPAD_INCLUDE_DIR}/common/linux/safe_readlink.cc
    )
    set_property(SOURCE ${BREAKPAD_INCLUDE_DIR}/common/linux/breakpad_getcontext.S PROPERTY LANGUAGE C)
endif()

if(WIN32)
    list(APPEND BREAKPAD_HEADERS
        ${BREAKPAD_INCLUDE_DIR}/client/windows/handler/exception_handler.h
    )
    list(APPEND BREAKPAD_SOURCES
        ${BREAKPAD_INCLUDE_DIR}/client/windows/crash_generation/crash_generation_client.cc
        ${BREAKPAD_INCLUDE_DIR}/client/windows/handler/exception_handler.cc
        ${BREAKPAD_INCLUDE_DIR}/common/windows/guid_string.cc
    )
endif()

add_library(Breakpad STATIC
   ${BREAKPAD_SOURCES}
   ${BREAKPAD_HEADERS}
)
target_compile_definitions(Breakpad PRIVATE UNICODE _UNICODE)
target_include_directories(Breakpad PRIVATE ${BREAKPAD_INCLUDE_DIR})
