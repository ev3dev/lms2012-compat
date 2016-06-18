
set (SOURCE_FILES
    c_ui.c
    d_lcd.c
    d_terminal.c
)

add_library (c_ui OBJECT ${SOURCE_FILES})
target_include_directories (c_ui PUBLIC
    ${CMAKE_SOURCE_DIR}/lms2012/source
    ${CMAKE_BINARY_DIR}/lmssrc/c_ui
    ${C_COM_DEPS_INCLUDE_DIRS}
)
add_dependencies (c_ui c_ui_xbms)
