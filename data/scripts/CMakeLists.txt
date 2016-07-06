
set (SCRIPTS
    usb-hid-gadget.sh
    install-to-working-directory.sh
)

install (PROGRAMS ${SCRIPTS}
    DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/scripts)
