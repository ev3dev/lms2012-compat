
set (SOURCE_FILES
    c_input.c
)

add_library (c_input OBJECT ${SOURCE_FILES})
target_include_directories (c_input PUBLIC
    ${CMAKE_SOURCE_DIR}/lms2012/source
    ${CMAKE_SOURCE_DIR}/c_com
    ${CMAKE_SOURCE_DIR}/c_output/source
    ${CMAKE_SOURCE_DIR}/c_ui/source
    ${C_COM_DEPS_INCLUDE_DIRS}
)
