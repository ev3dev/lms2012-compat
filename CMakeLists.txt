cmake_minimum_required (VERSION 2.8)
project (lms2012-compat C)

add_compile_options (-Wall -Werror)

find_package (PkgConfig REQUIRED)
pkg_check_modules (LMS2012_DEPS REQUIRED
    alsa
    bluez
    dbus-1
    libudev
    libusb-1.0
)

add_subdirectory (c_com)
add_subdirectory (c_dynload)
add_subdirectory (c_input)
add_subdirectory (c_memory)
add_subdirectory (c_output)
add_subdirectory (c_robotcvm)
add_subdirectory (c_sound)
add_subdirectory (c_ui)
add_subdirectory (lms2012)
add_subdirectory (lmssrc)
