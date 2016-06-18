
set (SOURCE_FILES
    c_memory.c
)

add_library (c_memory OBJECT ${SOURCE_FILES})
target_include_directories (c_memory PUBLIC
    ${CMAKE_SOURCE_DIR}/lms2012/source
    ${CMAKE_SOURCE_DIR}/c_com/source
    ${C_COM_DEPS_INCLUDE_DIRS}
)
