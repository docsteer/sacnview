include(GNUInstallDirs)

# Find WindeployQt
find_program(WINDEPLOYQT windeployqt HINTS "${QT_BIN_DIR}" REQUIRED)

foreach(COMPONENT ${CPACK_COMPONENTS_ALL})

    set(COMPONENT_DIR "${CPACK_TEMPORARY_INSTALL_DIRECTORY}/${COMPONENT}")

    # Run WindeployQt
    file(GLOB COMPONENT_EXES "${COMPONENT_DIR}/${CMAKE_INSTALL_BINDIR}/*.exe")
    foreach(COMPONENT_EXE ${COMPONENT_EXES})
        message("Running Windeplpyqt on ${COMPONENT_EXE}")

        # Pull component Qt dependencies
        execute_process(
            COMMAND "${WINDEPLOYQT}"
                --release
                --verbose 1
                --no-compiler-runtime
                --dir ${COMPONENT_DIR}/${CMAKE_INSTALL_BINDIR}
                ${COMPONENT_EXE}
        )
    endforeach()

    if (NOT ${CMAKE_INSTALL_BINDIR} STREQUAL ${CPACK_NSIS_EXECUTABLES_DIRECTORY})
        # Rebase anything in CMAKE_INSTALL_BINDIR, to CPACK_NSIS_EXECUTABLES_DIRECTORY
        message("Rebasing ${COMPONENT_DIR}/${CMAKE_INSTALL_BINDIR} to ${COMPONENT_DIR}/${CPACK_NSIS_EXECUTABLES_DIRECTORY}")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${COMPONENT_DIR}/${CMAKE_INSTALL_BINDIR}/" "${COMPONENT_DIR}/${CPACK_NSIS_EXECUTABLES_DIRECTORY}/"
        )
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E remove_directory "${COMPONENT_DIR}/${CMAKE_INSTALL_BINDIR}"
        )
    endif()
endforeach()
