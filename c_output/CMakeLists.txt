
set (SOURCE_FILES
    c_output.c
)

add_library (c_output OBJECT ${SOURCE_FILES})
target_include_directories (c_output PUBLIC
    ${CMAKE_SOURCE_DIR}/lms2012
    ${CMAKE_SOURCE_DIR}/c_com
    ${LMS2012_DEPS_INCLUDE_DIRS}
)
