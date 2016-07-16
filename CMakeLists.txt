cmake_minimum_required (VERSION 2.8)
project (lms2012-compat C)

set (CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include (GNUInstallDirs)

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
option (LMS2012_DEBUG_BUFPRINT "Buffered debug for c_input and c_ui")
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
option (LMS2012_DEBUG_WIFI "Enable wifi debug messages in c_wifi")
option (LMS2012_ENABLE_DEBUG_PULSE "Enable opUI_WRITE(ALLOW_PULSE) bytecode" Yes)

option (LMS2012_ENABLE_AD_WORD_PROTECT "Enable A/D word result protection" Yes)
option (LMS2012_ENABLE_BLOCK_ALIAS_LOCALS "Enable change of block locals if sub call alias (parallelism)")
option (LMS2012_ENABLE_BUMPED "Enable touch sensor bumped" Yes)
option (LMS2012_ENABLE_DAISYCHAIN "Enable daisy chaining")
option (LMS2012_ENABLE_DAISYCHAIN_COM_CALL "Enable daisy chain communications call")
option (LMS2012_ENABLE_FAST_DATALOG_BUFFER "Enable fast datalog buffer" Yes)
option (LMS2012_ENABLE_FILENAME_CHECK "Enable c_memory filename check" Yes)
option (LMS2012_ENABLE_HIGH_CURRENT "Enable shut down on high current")
option (LMS2012_ENABLE_LOAD_TEST "Show integrated current in the top line")
option (LMS2012_ENABLE_LOG_ASCII "Enable ASCII log instead of numeric")
option (LMS2012_ENABLE_LOW_MEMORY "Enable check low memory" Yes)
option (LMS2012_ENABLE_LOW_VOLTAGE "Enable shut down on low voltage" Yes)
option (LMS2012_ENABLE_MEMORY_TEST "Show used memory in the top line")
option (LMS2012_ENABLE_NEW_CALL_MUTEX "Enable smart object switching after return from non reentrant sub call (enables blocked thread call)" Yes)
option (LMS2012_ENABLE_OLD_COLOR "Enable support for NXT color sensor" Yes)
option (LMS2012_ENABLE_OLDCALL "Don't use optimised sub calls")
option (LMS2012_ENABLE_PAR_ALIGNMENT "Enable possibility to align sub call parameter types" Yes)
option (LMS2012_ENABLE_PERFORMANCE_TEST "Show performance bar in the top line")
option (LMS2012_ENABLE_SDCARD_SUPPORT "Enable SD card")
option (LMS2012_ENABLE_SOUND "Enable sound" Yes)
option (LMS2012_ENABLE_STATUS_TEST "Enable status test")
option (LMS2012_ENABLE_SYSTEM_BYTECODE "Enable the use of opSYSTEM command" Yes)
option (LMS2012_ENABLE_UART_DATA_ERROR "Enable reset UART sensor if timeout or crc error" Yes)
option (LMS2012_ENABLE_UPDATE_DISASSEMBLY "Enable disassemble of running update commands" Yes)
option (LMS2012_ENABLE_USBSTICK_SUPPORT "Enable USB stick")
option (LMS2012_ENABLE_VIRTUAL_BATT_TEMP "Enable guessing of battery temperature" Yes)


# translate options into defines

if (LMS2012_ENABLE_TERMINAL)
    add_definitions ("-DTERMINAL_ENABLED=1")
else ()
    add_definitions ("-DTERMINAL_ENABLED=0")
endif ()
# subtracting one here converts port number to enum value
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
    WIFI
)
foreach (DEBUG_OPTION ${LMS2012_DEBUG_OPTIONS})
    if (LMS2012_DEBUG_${DEBUG_OPTION})
        add_definitions ("-DDEBUG_${DEBUG_OPTION}")
    endif ()
endforeach ()
if (LMS2012_DEBUG_BUFPRINT)
    add_definitions ("-DBUFPRINTSIZE=100000")
endif ()
if (LMS2012_ENABLE_DEBUG_PULSE)
    add_definitions ("-DALLOW_DEBUG_PULSE")
endif ()
set (LMS2012_ENABLE_OPTIONS
    OLDCALL
    LOG_ASCII
    HIGH_CURRENT
    PERFORMANCE_TEST
    LOAD_TEST
    MEMORY_TEST
    STATUS_TEST
)
foreach (OPTION ${LMS2012_ENABLE_OPTIONS})
    if (LMS2012_ENABLE_${OPTION})
        add_definitions ("-DENABLE_${OPTION}")
    endif ()
endforeach ()
set (LMS2012_DISABLE_OPTIONS
    OLD_COLOR
    DAISYCHAIN
    DAISYCHAIN_COM_CALL
    FAST_DATALOG_BUFFER
    BUMPED
    LOW_VOLTAGE
    LOW_MEMORY
    UART_DATA_ERROR
    SDCARD_SUPPORT
    USBSTICK_SUPPORT
    VIRTUAL_BATT_TEMP
    SOUND
    PAR_ALIGNMENT
    NEW_CALL_MUTEX
    SYSTEM_BYTECODE
    FILENAME_CHECK
    AD_WORD_PROTECT
    UPDATE_DISASSEMBLY
    BLOCK_ALIAS_LOCALS
)
foreach (OPTION ${LMS2012_DISABLE_OPTIONS})
    if (NOT LMS2012_ENABLE_${OPTION})
        add_definitions ("-DDISABLE_${OPTION}")
    endif ()
endforeach ()

# global compiler options

add_compile_options (-Wall -Werror)


# global dependencies

find_package (PkgConfig REQUIRED)
pkg_check_modules (LMS2012_DEPS REQUIRED
    alsa
    bluez
    dbus-1
    gio-2.0
    gio-unix-2.0
    glib-2.0
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
add_subdirectory (data)
add_subdirectory (lms2012)
add_subdirectory (lmssrc)
