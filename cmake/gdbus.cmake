
find_program (GDBUS_CODEGEN gdbus-codegen)

function (add_gdbus_codegen GENERATE_C_CODE)
    set (options)
    set (one_value_args INTERFACE_PREFIX C_NAMESPACE)
    set (multi_value_args XML_FILES)
    cmake_parse_arguments (GDBUS
        "${options}"
        "${one_value_args}"
        "${multi_value_args}"
        ${ARGN}
    )
    set (GDBUS_XML_FILES_ABSOLUTE)
    foreach (file ${GDBUS_XML_FILES})
        get_filename_component (file ${file} ABSOLUTE)
        list (APPEND GDBUS_XML_FILES_ABSOLUTE ${file})
    endforeach ()
    add_custom_command (OUTPUT ${GENERATE_C_CODE}.c ${GENERATE_C_CODE}.h
        COMMAND ${GDBUS_CODEGEN}
            --interface-prefix ${GDBUS_INTERFACE_PREFIX}
            --generate-c-code ${GENERATE_C_CODE}
            --c-namespace ${GDBUS_C_NAMESPACE}
            ${GDBUS_XML_FILES_ABSOLUTE}
        DEPENDS
            ${GDBUS_XML_FILES_ABSOLUTE}
    )
endfunction ()
