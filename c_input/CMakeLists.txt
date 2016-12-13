
set (SOURCE_FILES
    c_input.c
)

add_library (c_input OBJECT ${SOURCE_FILES})
target_include_directories (c_input PUBLIC
    ${CMAKE_SOURCE_DIR}/lms2012
    ${CMAKE_BINARY_DIR}/lms2012
    ${CMAKE_SOURCE_DIR}/c_com
    ${CMAKE_SOURCE_DIR}/c_memory
    ${CMAKE_SOURCE_DIR}/c_output
    ${CMAKE_SOURCE_DIR}/c_ui
    ${LMS2012_DEPS_INCLUDE_DIRS}
)
add_dependencies (c_input bytecodes.h)
