  # Deploy and install

  set(SACNVIEW_DEPLOY_DIR "${CMAKE_CURRENT_LIST_DIR}/deploy" CACHE PATH "Folder to use for deployment")

  # Windows deployment
  if(WIN32)
    set(SACNVIEW_MAKENSIS_FILE "$ENV{ProgramFiles\(x86\)}/NSIS/makensis.exe" CACHE STRING "makensis.exe filepath")

    if(TARGET Qt::qmake AND NOT TARGET Qt::windeployqt)
      get_target_property(_qt_qmake_location Qt::qmake IMPORTED_LOCATION)

      execute_process(
        COMMAND "${_qt_qmake_location}" -query QT_INSTALL_PREFIX
        RESULT_VARIABLE return_code
        OUTPUT_VARIABLE qt_install_prefix
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )

      set(imported_location "${qt_install_prefix}/bin/windeployqt.exe")

      if(EXISTS ${imported_location})
        add_executable(Qt::windeployqt IMPORTED)

        set_target_properties(Qt::windeployqt PROPERTIES
            IMPORTED_LOCATION ${imported_location}
        )
      endif()
    endif()

    if(TARGET Qt::windeployqt)
      # execute windeployqt in deploy directory after build
      add_custom_command(TARGET sACNView
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${SACNVIEW_DEPLOY_DIR}
        COMMAND set PATH=%PATH%$<SEMICOLON>${qt5_install_prefix}/bin
        COMMAND Qt::windeployqt  --release --no-compiler-runtime --dir "${SACNVIEW_DEPLOY_DIR}" "$<TARGET_FILE:sACNView>"
      )

      # Qt5 requires OpenSSL to be added separately
      if (NOT Qt6_FOUND)
        foreach(DLLFILE IN LISTS OPENSSL_LIBS)
          add_custom_command (TARGET sACNView POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DLLFILE} ${SACNVIEW_DEPLOY_DIR})
        endforeach()
      endif()

      # Copy target
        add_custom_command (TARGET sACNView POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
          $<TARGET_FILE:sACNView> ${SACNVIEW_DEPLOY_DIR})

      # Run makensis if is installed in default location
      if(EXISTS ${SACNVIEW_MAKENSIS_FILE})
        if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64" OR ${CMAKE_SYSTEM_PROCESSOR} MATCHES "AMD64")
          set(SACNVIEW_NSIS_FILE "${CMAKE_CURRENT_LIST_DIR}/win64/install.nsi")
        else()
          set(SACNVIEW_NSIS_FILE "${CMAKE_CURRENT_LIST_DIR}/win/install.nsi")
        endif()
        add_custom_command (TARGET sACNView
          POST_BUILD
          COMMAND ${SACNVIEW_MAKENSIS_FILE} /DPRODUCT_VERSION="${GIT_VERSION}" ${SACNVIEW_NSIS_FILE}
        )
      endif()
    endif()
  endif()

if(APPLE)
    if(TARGET Qt::qmake AND NOT TARGET Qt::macdeployqt)
      get_target_property(_qt_qmake_location Qt::qmake IMPORTED_LOCATION)

      execute_process(
        COMMAND "${_qt_qmake_location}" -query QT_INSTALL_PREFIX
        RESULT_VARIABLE return_code
        OUTPUT_VARIABLE qt_install_prefix
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )

      set(imported_location "${qt_install_prefix}/bin/macdeployqt")

      if(EXISTS ${imported_location})
        add_executable(Qt::macdeployqt IMPORTED)

        set_target_properties(Qt::macdeployqt PROPERTIES
            IMPORTED_LOCATION ${imported_location}
        )
      endif()
    endif()

    if(TARGET Qt::macdeployqt)
      # execute macdeployqt in deploy directory after build
      add_custom_command(TARGET sACNView
        POST_BUILD
        COMMAND Qt::macdeployqt "$<TARGET_BUNDLE_DIR:sACNView>"
      )
      # Build the DMG file
      add_custom_command(TARGET sACNView
        POST_BUILD
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMAND /bin/bash "${PROJECT_SOURCE_DIR}/install/mac/create_sacnview_dmg.sh"
      )
    endif()
  endif()

  if(LINUX)
    # Generate the deployment script for the target MyApp.
    qt_generate_deploy_app_script(
      TARGET sACNView
      OUTPUT_SCRIPT deploy_script
    )

    # Call the deployment script on "cmake --install".
    install(SCRIPT ${deploy_script})

    install(TARGETS sACNView 
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

    set(CPACK_PACKAGE_NAME sACNView)
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Application to work with the sACN Lighting Control protocol")
    set(CPACK_PACKAGE_VENDOR "Tom Steer")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})
    set(CPACK_VERBATIM_VARIABLES ON)
    set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/sacnview")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Tom Steer <me@tomsteer.net>")
    set(CPACK_PACKAGE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/package")
    
    include(CPack)
endif()
