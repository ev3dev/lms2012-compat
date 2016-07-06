
set (RCF_FILES
    typedata.rcf
    typedata50.rcf
)

install (FILES ${RCF_FILES}
    DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/sys/settings)
