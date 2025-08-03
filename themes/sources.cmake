# Qt-Frameless-Window-DarkStyle
list(APPEND SACNVIEW_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/themes.cpp
    ${CMAKE_CURRENT_LIST_DIR}/darktheme.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Qt-Frameless-Window-DarkStyle/DarkStyle.cpp
)

list(APPEND SACNVIEW_RCC
    ${CMAKE_CURRENT_LIST_DIR}/Qt-Frameless-Window-DarkStyle/darkstyle.qrc
)

list(APPEND SACNVIEW_HEADER_PATHS
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/Qt-Frameless-Window-DarkStyle
)
