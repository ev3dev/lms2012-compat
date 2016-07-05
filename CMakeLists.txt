cmake_minimum_required (VERSION 2.8)
project (lms2012-compat C)

# compiler configuration options

option (LMS2012_ENABLE_TERMINAL "Enable debug terminal")
set (LMS2012_DEBUG_UART_PORT 0
     CACHE string "Input port used for debugging (1-4) or 0 to disable")
option (LMS2012_DEBUG_VM "Enable VM debug messages in lms2012")
option (LMS2012_DEBUG_TRACE_TASK "Enable task trace messages in lms2012")
option (LMS2012_DEBUG_C_COM "Enable debug messages in c_com")
option (LMS2012_DEBUG_C_INPUT "Enable debug messages in c_input")
option (LMS2012_DEBUG_C_INPUT_DAISYCHAIN "Enable daisy chain debug messages in c_input")
option (LMS2012_DEBUG_C_INPUT_DATALOG "Enable datalog debug messages in c_input")
option (LMS2012_DEBUG_C_INPUT_FAST_DATALOG "Enable fast datalog debug messages in c_input")
option (LMS2012_DEBUG_C_MEMORY "Enable debug messages in c_memory")
option (LMS2012_DEBUG_C_MEMORY_LOG "Enable log debug messages in c_memory")
option (LMS2012_DEBUG_C_MEMORY_FILE "Enable file debug messages in c_memory")
option (LMS2012_DEBUG_C_MEMORY_LOW "Enable low debug messages in c_memory")
option (LMS2012_DEBUG_C_SOUND "Enable debug messages in c_sound")
option (LMS2012_DEBUG_C_UI "Enable debug messages in c_ui")
option (LMS2012_BUFPRINT "Buffered debug for c_input and c_ui")
option (LMS2012_DEBUG_TRACE_MODE_CHANGE "Enable mode change trace messages in c_input")
option (LMS2012_DEBUG_TRACE_KEY "Enable key trace messages in c_ui")
option (LMS2012_DEBUG_TRACE_VM "Enable vm trace messages in lms2012")
option (LMS2012_DEBUG_TRACE_DAISYCHAIN "Enable daisy chain trace messages in c_input")
option (LMS2012_DEBUG_BYTECODE_TIME "Enable bytecode time debug messages in lms2012")
option (LMS2012_DEBUG_TRACE_FREEZE "Enable freeze trace messages in lms2012")
option (LMS2012_DEBUG_TRACE_FILENAME "Enable file name trace messages in c_memory")
option (LMS2012_DEBUG_SDCARD "Enable SD card debug messages in lms2012")
option (LMS2012_DEBUG_USBSTICK "Enable USB drive debug messages in lms2012")
option (LMS2012_DEBUG_VIRTUAL_BATT_TEMP "Enable virtual battery temperature debug messages in c_ui")
option (LMS2012_DEBUG_TEMP_SHUTDOWN "Enable temperature shutdown debug messages in c_ui")
option (LMS2012_DEBUG_BACK_BLOCKED "Enable back button debug messages in c_ui")
option (LMS2012_DEBUG_MEMORY_USAGE "Enable memory usage debug messages in c_memory")
option (LMS2012_DEBUG_RECHARGABLE "Enable rechargable battery debug messages in c_ui")
option (LMS2012_ENABLE_DEBUG_PULSE "Enable opUI_WRITE(ALLOW_PULSE) bytecode" Yes)

# translate options into defines

if (LMS2012_ENABLE_TERMINAL)
    add_definitions ("-DTERMINAL_ENABLED=1")
else ()
    add_definitions ("-DTERMINAL_ENABLED=0")
endif ()
add_definitions ("-DDEBUG_UART=(${LMS2012_DEBUG_UART_PORT}-1)")
set (LMS2012_DEBUG_OPTIONS
    VM
    TRACE_TASK
    C_COM
    C_INPUT
    C_INPUT_DAISYCHAIN
    C_INPUT_DATALOG
    C_INPUT_FAST_DATALOG
    C_MEMORY
    C_MEMORY_LOG
    C_MEMORY_FILE
    C_MEMORY_LOW
    C_SOUND
    C_UI
    TRACE_MODE_CHANGE
    TRACE_KEY
    TRACE_VM
    TRACE_DAISYCHAIN
    BYTECODE_TIME
    TRACE_FREEZE
    TRACE_FILENAME
    SDCARD
    USBSTICK
    VIRTUAL_BATT_TEMP
    TEMP_SHUTDOWN
    BACK_BLOCKED
    MEMORY_USAGE
    RECHARGABLE
)
foreach (DEBUG_OPTION ${LMS2012_DEBUG_OPTIONS})
    if (LMS2012_DEBUG_${DEBUG_OPTION})
        add_definitions ("-DDEBUG_${DEBUG_OPTION}")
    endif ()
endforeach ()
if (LMS2012_BUFPRINT)
    add_definitions ("-DBUFPRINTSIZE=100000")
endif ()
if (LMS2012_ENABLE_DEBUG_PULSE)
    add_definitions ("-DALLOW_DEBUG_PULSE")
endif ()

# global compiler options

add_compile_options (-Wall -Werror)


# global dependencies

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
