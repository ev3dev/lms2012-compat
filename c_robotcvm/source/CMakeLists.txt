
set (SOURCE_FILES
    robotc.c
)

add_library (c_robotcvm OBJECT ${SOURCE_FILES})
target_include_directories (c_robotcvm PUBLIC
    ${CMAKE_SOURCE_DIR}/lms2012/source
    ${CMAKE_SOURCE_DIR}/c_dynload/source
    ${C_COM_DEPS_INCLUDE_DIRS}
)
