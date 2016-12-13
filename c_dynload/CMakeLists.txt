
set (SOURCE_FILES
    c_dynload.c
)

add_library (c_dynload OBJECT ${SOURCE_FILES})
target_include_directories (c_dynload PUBLIC
    ${CMAKE_SOURCE_DIR}/lms2012
    ${CMAKE_BINARY_DIR}/lms2012
)
add_dependencies (c_dynload bytecodes.h)
