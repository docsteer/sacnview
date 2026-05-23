# Deploy and install

set(SACNVIEW_DEPLOY_DIR "${CMAKE_CURRENT_LIST_DIR}/deploy" CACHE PATH "Folder to use for deployment")

# Windows deployment
if(WIN32)
    set(SACNVIEW_MAKENSIS_FILE "$ENV{ProgramFiles\(x86\)}/NSIS/makensis.exe" CACHE STRING "makensis.exe filepath")

    if(TARGET Qt::windeployqt)
        # Command needs forward slashes or directory separators will be interpreted as escapes.
        cmake_path(CONVERT $ENV{VCINSTALLDIR} TO_CMAKE_PATH_LIST _windeployqt_vcinstalldir)
        # execute windeployqt in deploy directory after build
        add_custom_command(TARGET sACNView
                POST_BUILD
                # Clean deploy directory.
                COMMAND ${CMAKE_COMMAND} -E rm -Rf ${SACNVIEW_DEPLOY_DIR}
                # Run windeployqt with VCINSTALLDIR set so it will install the MSVC runtime.
                COMMAND ${CMAKE_COMMAND} -E env VCINSTALLDIR="${_windeployqt_vcinstalldir}"
                "$<TARGET_FILE:Qt::windeployqt>"
                --release
                --compiler-runtime
                --dir "${SACNVIEW_DEPLOY_DIR}"
                "$<TARGET_FILE:sACNView>"
        )

        # Copy target
        add_custom_command(TARGET sACNView POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:sACNView> ${SACNVIEW_DEPLOY_DIR})

        # Run makensis if is installed in default location
        if(EXISTS ${SACNVIEW_MAKENSIS_FILE})
            if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64" OR ${CMAKE_SYSTEM_PROCESSOR} MATCHES "AMD64")
                set(SACNVIEW_NSIS_FILE "${CMAKE_CURRENT_LIST_DIR}/win64/install.nsi")
            else()
                set(SACNVIEW_NSIS_FILE "${CMAKE_CURRENT_LIST_DIR}/win/install.nsi")
            endif()
            add_custom_command(TARGET sACNView
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
