
set (SOURCE_FILES
    c_bt.c
    c_com.c
    c_daisy.c
    c_i2c.c
    c_md5.c
    c_wifi.c
    os_internal.c
    os_unix.c
    wpa_ctrl.c
)

add_library (c_com OBJECT ${SOURCE_FILES})
target_include_directories (c_com PUBLIC
    ${CMAKE_SOURCE_DIR}/lms2012
    ${CMAKE_SOURCE_DIR}/c_input
    ${CMAKE_SOURCE_DIR}/c_memory
    ${CMAKE_SOURCE_DIR}/c_output
    ${LMS2012_DEPS_INCLUDE_DIRS}
)
