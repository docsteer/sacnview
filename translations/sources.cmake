# Core translations

list(APPEND SACNVIEW_SOURCES
  ${CMAKE_CURRENT_LIST_DIR}/translationdialog.cpp
  ${CMAKE_CURRENT_LIST_DIR}/translations.cpp
)

list(APPEND SACNVIEW_HEADERS
  ${CMAKE_CURRENT_LIST_DIR}/translationdialog.h
  ${CMAKE_CURRENT_LIST_DIR}/translations.h
)

list(APPEND SACNVIEW_FORMS
  ${CMAKE_CURRENT_LIST_DIR}/translationdialog.ui
)

set(SACNVIEW_TS
  ${CMAKE_CURRENT_LIST_DIR}/sACNView_de.ts
  ${CMAKE_CURRENT_LIST_DIR}/sACNView_en.ts
  ${CMAKE_CURRENT_LIST_DIR}/sACNView_es.ts
  ${CMAKE_CURRENT_LIST_DIR}/sACNView_fr.ts
)
