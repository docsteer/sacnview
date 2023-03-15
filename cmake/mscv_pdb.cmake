cmake_minimum_required(VERSION 3.0)

# PDB files for release
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC" AND CMAKE_BUILD_TYPE MATCHES "Release")
    message("Enforcing PDB for release build")

    target_compile_options(${PROJECT_NAME} PRIVATE /Zi)

    # Tell linker to include symbol data
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "/INCREMENTAL:NO /DEBUG /OPT:REF /OPT:ICF"
    )

    # Set file name & location
    set_target_properties(${PROJECT_NAME} PROPERTIES
        COMPILE_PDB_NAME ${PROJECT_NAME}
        COMPILE_PDB_OUTPUT_DIR ${CMAKE_BINARY_DIR}
    )
endif()
