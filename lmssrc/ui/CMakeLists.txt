
# TODO: Would be nice to rename icons so we don't have to have this lookup table
set (screen_SOURCE              89x178_MainMenuBottom)
set (playrecent_screen_SOURCE   178x26_PLAYLAST_TAB)
set (file_screen_SOURCE         178x26_FILE_TAB)
set (apps_screen_SOURCE         178x26_APPS_TAB)
set (settings_screen_SOURCE     178x26_SETTINGS_TAB)

add_lms_app (ui
    INSTALL_PATH
        sys/ui
    PROGRAMS
        ui
    GRAPHICS
        mindstorms
        Ani1x
        screen
        playrecent_screen
        file_screen
        apps_screen
        settings_screen
        144x65_POP3
        144x82_POP4
    SOUNDS
        Click
        DownloadSuccess
        GeneralAlarm
        OverpowerAlert
        PowerDown
        Startup
)
