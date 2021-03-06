
include (gdbus)

set (SOURCE_FILES
    c_bt.c
    c_com.c
    c_daisy.c
    c_i2c.c
    c_md5.c
    c_wifi.c
    ${CMAKE_CURRENT_BINARY_DIR}/bluez.c
    ${CMAKE_CURRENT_BINARY_DIR}/connman.c
    ${CMAKE_CURRENT_BINARY_DIR}/systemd.c
)

add_gdbus_codegen(bluez
    INTERFACE_PREFIX org.bluez
    C_NAMESPACE Bluez
    XML_FILES org.bluez.xml
)
add_gdbus_codegen(connman
    INTERFACE_PREFIX net.connman
    C_NAMESPACE Connman
    XML_FILES net.connman.xml
)
add_gdbus_codegen(systemd
    INTERFACE_PREFIX org.freedesktop.systemd1
    C_NAMESPACE Systemd
    XML_FILES org.freedesktop.systemd.xml
)

add_library (c_com OBJECT ${SOURCE_FILES})
target_include_directories (c_com PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/lms2012
    ${CMAKE_BINARY_DIR}/lms2012
    ${CMAKE_SOURCE_DIR}/c_input
    ${CMAKE_SOURCE_DIR}/c_memory
    ${CMAKE_SOURCE_DIR}/c_output
    ${LMS2012_DEPS_INCLUDE_DIRS}
)
add_dependencies (c_com bytecodes.h)
