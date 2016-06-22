
set (SOURCE_FILES
    c_sound.c
)

add_library (c_sound OBJECT ${SOURCE_FILES})
target_include_directories (c_sound PUBLIC
    ${CMAKE_SOURCE_DIR}/lms2012
)
