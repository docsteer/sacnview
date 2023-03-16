cmake_minimum_required(VERSION 3.14)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# DarkStyle is used for the dark theme
FetchContent_Declare(
    DarkStyle
    GIT_REPOSITORY  https://github.com/Jorgen-VikingGod/Qt-Frameless-Window-DarkStyle.git
    GIT_TAG         origin/master
    GIT_SHALLOW     TRUE
    GIT_PROGRESS    TRUE
)
FetchContent_MakeAvailable(DarkStyle)

add_library(DarkStyle STATIC
    ${darkstyle_SOURCE_DIR}/DarkStyle.h
    ${darkstyle_SOURCE_DIR}/DarkStyle.cpp
    ${darkstyle_SOURCE_DIR}/darkstyle.qrc
)
target_include_directories(DarkStyle PRIVATE ${darkstyle_SOURCE_DIR})

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
target_link_libraries(DarkStyle PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

target_include_directories(${PROJECT_NAME} PRIVATE ${darkstyle_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE DarkStyle)
