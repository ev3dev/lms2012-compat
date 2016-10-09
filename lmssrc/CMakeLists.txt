
find_program (LMSASM lmsasm)
find_program (IMAGEMAGICK_CONVERT convert)
find_program (SOUND_EXCHANGE sox)

set (RAW2RSF ${CMAKE_SOURCE_DIR}/lmssrc/raw2rsf.sh)

macro (add_lms_app TARGET)
    set (options)
    set (one_value_args ARCHIVE INSTALL_PATH ICON)
    set (multi_value_args PROGRAMS PROGRAMS_TASM GRAPHICS SOUNDS)
    cmake_parse_arguments (LMSSRC
        "${options}"
        "${one_value_args}"
        "${multi_value_args}"
        ${ARGN}
    )
    if (LMSSRC_ICON)
        list (APPEND LMSSRC_GRAPHICS icon)
        set (icon_SOURCE ${LMSSRC_ICON})
    endif ()

    foreach (program ${LMSSRC_PROGRAMS})
        set (lms ${CMAKE_CURRENT_SOURCE_DIR}/${program}.lms)
        set (rbf ${CMAKE_CURRENT_BINARY_DIR}/${program}.rbf)

        add_custom_command (OUTPUT ${rbf}
            COMMAND ${LMSASM}
                --support compat
                --output ${rbf}
                ${lms}
            DEPENDS
                ${lms}
        )
        list (APPEND LMSSRC_RBF_LIST ${rbf})
    endforeach ()

    foreach (graphic ${LMSSRC_GRAPHICS})
        # some generated files do not have the same name as the source file
        set (source ${${graphic}_SOURCE})
        if (NOT source)
            set (source ${graphic})
        endif ()

        set (bmp ${CMAKE_CURRENT_SOURCE_DIR}/${source}.bmp)
        set (rgf ${CMAKE_CURRENT_BINARY_DIR}/${graphic}.rgf)

        add_custom_command (OUTPUT ${rgf}
            COMMAND ${IMAGEMAGICK_CONVERT} ${bmp} ${rgf}
            DEPENDS ${bmp}
        )
        list (APPEND LMSSRC_RGF_LIST ${rgf})
    endforeach ()

    foreach (sound ${LMSSRC_SOUNDS})
        set (wav ${CMAKE_CURRENT_SOURCE_DIR}/${sound}.wav)
        set (raw ${CMAKE_CURRENT_BINARY_DIR}/${sound}.raw)
        set (rsf ${CMAKE_CURRENT_BINARY_DIR}/${sound}.rsf)

        add_custom_command (OUTPUT ${rsf}
            COMMAND ${SOUND_EXCHANGE}
                ${wav}
                --bits 8
                --channels 1
                --encoding unsigned-integer
                --rate 8000
                ${raw}
            COMMAND ${RAW2RSF}
                ${raw}
                ${rsf}
            DEPENDS
                ${wav}
                ${RAW2RSF}
        )
        list (APPEND LMSSRC_RSF_LIST ${rsf})
    endforeach ()

    if (LMSSRC_ARCHIVE)
        set (raf ${CMAKE_CURRENT_BINARY_DIR}/${LMSSRC_ARCHIVE}.raf)
        foreach (file ${LMSSRC_RBF_LIST} ${LMSSRC_RGF_LIST} ${LMSSRC_RSF_LIST})
            file (RELATIVE_PATH file ${CMAKE_CURRENT_BINARY_DIR} ${file})
            list (APPEND raf_files ${file})
        endforeach ()

        add_custom_command (OUTPUT ${raf}
            COMMAND tar
                --transform=s,^,${LMSSRC_ARCHIVE}/,
                -czf ${raf}
                ${raf_files}
            WORKING_DIRECTORY
                ${CMAKE_CURRENT_BINARY_DIR}
            DEPENDS
                ${LMSSRC_RBF_LIST}
                ${LMSSRC_RGF_LIST}
                ${LMSSRC_RSF_LIST}
        )

        add_custom_target (${TARGET} ALL DEPENDS ${raf})
        install (FILES ${raf}
            DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/${LMSSRC_INSTALL_PATH})
    else ()
        add_custom_target (${TARGET} ALL
            DEPENDS
                ${LMSSRC_RBF_LIST}
                ${LMSSRC_RGF_LIST}
                ${LMSSRC_RSF_LIST}
        )
        install (FILES ${LMSSRC_RBF_LIST} ${LMSSRC_RGF_LIST} ${LMSSRC_RSF_LIST}
            DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/${LMSSRC_INSTALL_PATH})
    endif ()

endmacro ()

add_subdirectory (Bluetooth)
add_subdirectory ("Brick Datalog")
add_subdirectory ("Brick Info")
add_subdirectory ("Brick Name")
add_subdirectory ("Brick Program")
add_subdirectory (Debug)
add_subdirectory (Demo)
add_subdirectory ("IR Control")
add_subdirectory ("Motor Control")
add_subdirectory ("Port View")
add_subdirectory (SelfTest)
add_subdirectory (Sleep)
add_subdirectory (TEST)
add_subdirectory (Test)
add_subdirectory (Volume)
add_subdirectory (WiFi)
add_subdirectory (tst)
add_subdirectory (ui)
