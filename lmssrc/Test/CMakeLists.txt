
set (pop3_SOURCE 144x65_POP3)
set (pop6_SOURCE 144x116_POP6)

add_lms_app (Test
    PATH tools
    ICON TestIcon
    PROGRAMS
        Test
    GRAPHICS
        pop3
        pop6
    SOUNDS
        Test
)
