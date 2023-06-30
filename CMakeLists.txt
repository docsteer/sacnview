cmake_minimum_required(VERSION 3.20)

set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15 CACHE STRING "Minimum macOS version targeted by sACNView on macOS")

project(sACNView DESCRIPTION "A tool for sending and receiving the ANSI E1.31 Streaming ACN control protocol")

# Workaround for cmake bug
# Ninja generator will often exceed command line limits, but cmake doesn't use the response file by default
SET(CMAKE_NINJA_FORCE_RESPONSE_FILE 1 CACHE INTERNAL "")

# Only do these if this is the main project, and not if it is included through add_subdirectory
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  ### Require out-of-source builds
  file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
  if(EXISTS "${LOC_PATH}")
    message(FATAL_ERROR "You cannot build in a source directory (or any directory with a CMakeLists.txt file). Please make a build subdirectory. Feel free to remove CMakeCache.txt and CMakeFiles.")
  endif()
  
  # Nicer IDE
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)

  # Workaround for CMake bug https://gitlab.kitware.com/cmake/cmake/-/issues/20812
  if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(
      $<$<CONFIG:RELEASE>:/Zi>
    )

    add_link_options(
      $<$<CONFIG:RELEASE>:/DEBUG>
      $<$<CONFIG:RELEASE>:/OPT:REF>
      $<$<CONFIG:RELEASE>:/OPT:ICF>
    )
  endif()

  if (WIN32)
    # Qt5 does not automatically set these, but Qt6 does
    add_compile_definitions(UNICODE _UNICODE)
  endif()
  
  # Docs only available if this is the main app
  find_package(Doxygen)
  if(Doxygen_FOUND)
    add_subdirectory(docs)
  else()
    message(STATUS "Doxygen not found, not building docs")
  endif()
  
endif()

set(SACNVIEW_HEADER_PATHS
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/src/sacn
    ${CMAKE_CURRENT_LIST_DIR}/src/sacn/ACNShare
    ${CMAKE_CURRENT_LIST_DIR}/src/widgets
    ${CMAKE_CURRENT_LIST_DIR}/src/models
    ${CMAKE_CURRENT_LIST_DIR}/src/pcap
    ${CMAKE_CURRENT_LIST_DIR}/src/ui
)

set(SACNVIEW_HEADERS
    src/ui/mdimainwindow.h
    src/sacn/securesacn.h
    src/widgets/monitorspinbox.h
    src/widgets/qpushbutton_rightclick.h
    src/widgets/qspinbox_resizetocontent.h
    src/ui/newversiondialog.h
    src/ui/universeview.h
    src/sacn/sacnsynchronization.h
    src/models/sacnsynclistmodel.h
    src/sacn/ACNShare/CID.h
    src/sacn/ACNShare/defpack.h
    src/sacn/ACNShare/ipaddr.h
    src/sacn/ACNShare/tock.h
    src/sacn/ACNShare/VHD.h
    src/sacn/streamcommon.h
    src/ui/nicselectdialog.h
    src/sacn/streamingacn.h
    src/ui/preferencesdialog.h
    src/preferences.h
    src/sacn/sacnlistener.h
    src/widgets/universedisplay.h
    src/ui/transmitwindow.h
    src/consts.h
    src/sacn/sacnsender.h
    src/ui/configureperchanpriodlg.h
    src/widgets/gridwidget.h
    src/ui/aboutdialog.h
    src/sacn/sacneffectengine.h
    src/models/sacnuniverselistmodel.h
    src/ui/snapshot.h
    src/commandline.h
    src/fontdata.h
    src/ui/multiuniverse.h
    src/ui/flickerfinderinfoform.h
    src/sacn/sacnsocket.h
    src/ui/logwindow.h
    src/firewallcheck.h
    src/ui/bigdisplay.h
    src/ui/addmultidialog.h
    src/sacn/e1_11.h
    src/ipc.h
    src/sacn/sacndiscovery.h
    src/models/sacndiscoveredsourcelistmodel.h
    src/widgets/clssnapshot.h
    src/sacn/fpscounter.h
    src/widgets/grideditwidget.h
    src/ui/multiview.h
    src/models/sacnsourcetablemodel.h
    src/models/csvmodelexport.h
    src/widgets/glscopewidget.h
)

set(SACNVIEW_SOURCES
    sacnview.natvis
    src/commandline.cpp
    src/firewallcheck.cpp
    src/ipc.cpp
    src/preferences.cpp
    src/models/sacnsynclistmodel.cpp
    src/models/sacndiscoveredsourcelistmodel.cpp
    src/models/sacnuniverselistmodel.cpp
    src/sacn/fpscounter.cpp
    src/sacn/sacndiscovery.cpp
    src/sacn/sacneffectengine.cpp
    src/sacn/sacnlistener.cpp
    src/sacn/sacnsender.cpp
    src/sacn/sacnsocket.cpp
    src/sacn/sacnsynchronization.cpp
    src/sacn/securesacn.cpp
    src/sacn/streamcommon.cpp
    src/sacn/streamingacn.cpp
    src/sacn/ACNShare/CID.cpp
    src/sacn/ACNShare/ipaddr.cpp
    src/sacn/ACNShare/tock.cpp
    src/sacn/ACNShare/VHD.cpp
    src/pcap/pcapplayback.cpp
    src/pcap/pcapplaybacksender.cpp
    src/ui/aboutdialog.cpp
    src/ui/configureperchanpriodlg.cpp
    src/ui/newversiondialog.cpp
    src/ui/mdimainwindow.cpp
    src/ui/universeview.cpp
    src/ui/nicselectdialog.cpp
    src/ui/preferencesdialog.cpp
    src/ui/snapshot.cpp
    src/ui/transmitwindow.cpp
    src/ui/multiuniverse.cpp
    src/ui/flickerfinderinfoform.cpp
    src/ui/logwindow.cpp
    src/ui/bigdisplay.cpp
    src/ui/addmultidialog.cpp
    src/ui/multiview.cpp
    src/ui/glscopewindow.cpp
    src/widgets/monitorspinbox.cpp
    src/widgets/qpushbutton_rightclick.cpp
    src/widgets/qspinbox_resizetocontent.cpp
    src/widgets/universedisplay.cpp
    src/widgets/gridwidget.cpp
    src/widgets/clssnapshot.cpp
    src/widgets/grideditwidget.cpp
    src/widgets/glscopewidget.cpp
    src/models/sacnsourcetablemodel.cpp
    src/models/csvmodelexport.cpp
)

# Prepend the path to all sources
LIST(TRANSFORM SACNVIEW_SOURCES PREPEND ${CMAKE_CURRENT_LIST_DIR}/)

set(SACNVIEW_FORMS
    ui/mdimainwindow.ui
    ui/universeview.ui
    ui/nicselectdialog.ui
    ui/preferencesdialog.ui
    ui/transmitwindow.ui
    ui/configureperchanpriodlg.ui
    ui/aboutdialog.ui
    ui/snapshot.ui
    ui/multiuniverse.ui
    ui/flickerfinderinfoform.ui
    ui/logwindow.ui
    ui/bigdisplay.ui
    ui/newversiondialog.ui
    ui/addmultidialog.ui
    ui/pcapplayback.ui
    ui/multiview.ui
)

set(SACNVIEW_RCC
    res/resources.qrc
    res/sacnview.rc
)

# Theming
include(themes/sources.cmake)

# Translation
include(translations/sources.cmake)

# 3rd party libraries
include(libs/libs.cmake)

# Enable automatic Qt handling
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find Qt 5.15 or Qt 6
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Gui Network Multimedia Widgets REQUIRED)
set(SACNVIEW_QT_LIBRARIES
   Qt::Core Qt::Gui Qt::Network Qt::Multimedia Qt::Widgets
)

# Qt 6 moved some components
if (Qt6_FOUND)
    find_package(Qt6 COMPONENTS OpenGLWidgets REQUIRED)
    list(APPEND SACNVIEW_QT_LIBRARIES Qt::OpenGLWidgets)
endif()

add_executable(sACNView WIN32
    src/main.cpp
    ${SACNVIEW_HEADERS}
    ${SACNVIEW_SOURCES}
    ${SACNVIEW_FORMS}
    ${SACNVIEW_RCC}
)

target_include_directories(sACNView PRIVATE
    ${SACNVIEW_HEADER_PATHS}
)

target_compile_features(sACNView PRIVATE cxx_std_17)

set_target_properties(sACNView PROPERTIES
  CXX_EXTENSIONS OFF
  AUTOUIC_SEARCH_PATHS ui
)

# Link Qt libraries
target_link_libraries(sACNView PRIVATE ${SACNVIEW_QT_LIBRARIES})

# Link PCap/WinPCap libraries
target_link_libraries(sACNView PRIVATE ${PCAP_LIBS})

if(WIN32)
  # Copy WinPCap DLLs
  foreach(DLLFILE IN LISTS PCAP_LIBS)
    add_custom_command (TARGET sACNView POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:${DLLFILE}> $<TARGET_FILE_DIR:sACNView>)
  endforeach()
endif()

# Blake2
target_compile_definitions(sACNView PRIVATE ${BLAKE2_DEFINES})
target_sources(sACNView PRIVATE ${BLAKE2_SOURCES})
target_include_directories(sACNView PRIVATE ${BLAKE2_PATH})

## Git Version info

# sha or tag
execute_process(
  COMMAND git describe --always --tags
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  OUTPUT_VARIABLE GIT_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# date
execute_process(
  COMMAND git show -1 -s --date=format:"%a" --format="%cd" ${GIT_VERSION}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  OUTPUT_VARIABLE GIT_DATE_DAY
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND git show -1 -s --date=format:"%d" --format="%cd" ${GIT_VERSION}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  OUTPUT_VARIABLE GIT_DATE_DATE
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND git show -1 -s --date=format:"%b" --format="%cd" ${GIT_VERSION}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  OUTPUT_VARIABLE GIT_DATE_MONTH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND git show -1 -s --date=format:"%Y" --format="%cd" ${GIT_VERSION}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  OUTPUT_VARIABLE GIT_DATE_YEAR
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# most recent tag
execute_process(
  COMMAND git describe --abbrev=0 --always --tags
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  OUTPUT_VARIABLE GIT_TAG
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

target_compile_definitions(sACNView PRIVATE
  "GIT_CURRENT_SHA1=\"${GIT_VERSION}\""
  "GIT_DATE_DAY=\"${GIT_DATE_DAY}\""
  "GIT_DATE_DATE=\"${GIT_DATE_DATE}\""
  "GIT_DATE_MONTH=\"${GIT_DATE_MONTH}\""
  "GIT_DATE_YEAR=\"${GIT_DATE_YEAR}\""
  "VERSION=\"${GIT_TAG}\""
)

# GoogleTest
option(SACNVIEW_UNIT_TESTS "Enable unit tests" OFF)
if (SACNVIEW_UNIT_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

# Deploy the build using windeployqt
option(SACNVIEW_CREATE_INSTALLER "Create the sACNView installer" OFF)

if(SACNVIEW_CREATE_INSTALLER)
  include(install/install.cmake)
endif()
