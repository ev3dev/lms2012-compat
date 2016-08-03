
find_program (PANDOC pandoc)
find_program (GZIP gzip)

add_custom_command (OUTPUT lms2012-compat.8.gz
    COMMAND ${PANDOC}
        --standalone
        --to man
        ${CMAKE_CURRENT_SOURCE_DIR}/lms2012-compat.md
        --output ${CMAKE_CURRENT_BINARY_DIR}/lms2012-compat.8
    COMMAND ${GZIP}
        -9
        ${CMAKE_CURRENT_BINARY_DIR}/lms2012-compat.8
    DEPENDS
        lms2012-compat.md
)
add_custom_target (doc ALL
    DEPENDS
        lms2012-compat.8.gz
)

install (FILES ${CMAKE_CURRENT_BINARY_DIR}/lms2012-compat.8.gz
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man8
)
