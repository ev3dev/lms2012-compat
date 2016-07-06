
set (SCRIPTS
    usb-hid-gadget.sh
)

install (FILES ${SCRIPTS}
    DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/scripts)
