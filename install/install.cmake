  # Deploy and install

  set(SACNVIEW_DEPLOY_DIR "${CMAKE_CURRENT_LIST_DIR}/deploy" CACHE PATH "Folder to use for deployment")
  
  # Windows deployment
  if(WIN32)
    set(SACNVIEW_MAKENSIS_FILE "$ENV{ProgramFiles\(x86\)}/NSIS/makensis.exe" CACHE FILE "makensis.exe filepath")

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

      # Copy target
        add_custom_command (TARGET sACNView POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
          $<TARGET_FILE:sACNView> ${SACNVIEW_DEPLOY_DIR})

      # Copy WinPCap DLLs
      foreach(DLLFILE IN LISTS PCAP_LIBS)
        add_custom_command (TARGET sACNView POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
          $<TARGET_FILE:${DLLFILE}> ${SACNVIEW_DEPLOY_DIR})
      endforeach()

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
