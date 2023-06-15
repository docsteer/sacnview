include(FetchContent)
FetchContent_Declare(
  googletest
  # Specify the googletest commit to download. Update this regularly.
  URL https://github.com/google/googletest/archive/refs/tags/v1.13.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP true
)

if (WIN32)
  # For Windows: Prevent overriding the parent project's compiler/linker settings
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
endif()

add_executable(unit_test_sACNView
  main.cpp
  ${SACNVIEW_SOURCES}
)

target_include_directories(unit_test_sACNView PRIVATE
    ${SACNVIEW_HEADER_PATHS}
)

target_compile_features(unit_test_sACNView PRIVATE cxx_std_17)

set_target_properties(unit_test_sACNView PROPERTIES
  CXX_EXTENSIONS OFF
  AUTOUIC_SEARCH_PATHS ../ui
)

target_compile_definitions(unit_test_sACNView PRIVATE
  "GIT_CURRENT_SHA1=\"${GIT_VERSION}\""
  "GIT_DATE_DAY=\"${GIT_DATE_DAY}\""
  "GIT_DATE_DATE=\"${GIT_DATE_DATE}\""
  "GIT_DATE_MONTH=\"${GIT_DATE_MONTH}\""
  "GIT_DATE_YEAR=\"${GIT_DATE_YEAR}\""
  "VERSION=\"${GIT_TAG}\""
)

# Blake2
target_compile_definitions(unit_test_sACNView PRIVATE ${BLAKE2_DEFINES})
target_sources(unit_test_sACNView PRIVATE ${BLAKE2_SOURCES})
target_include_directories(unit_test_sACNView PRIVATE ${BLAKE2_PATH})

target_link_libraries(unit_test_sACNView
  gtest
  ${SACNVIEW_QT_LIBRARIES}
  ${PCAP_LIBS}
)

if(WIN32)
  # Copy WinPCap DLLs
  foreach(DLLFILE IN LISTS PCAP_LIBS)
    add_custom_command (TARGET unit_test_sACNView POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:${DLLFILE}> $<TARGET_FILE_DIR:unit_test_sACNView>)
  endforeach()
endif()

add_test(NAME unit_test_sACNView COMMAND unit_test_sACNView)
