
set (tdef_SOURCE other)
set (t010_SOURCE NXT_device)
set (t020_SOURCE NXT_device)
set (t030_SOURCE NXT_device)
set (t040_SOURCE NXT_device)
set (t050_SOURCE NXT_device)
set (t060_SOURCE temp_c)
set (t061_SOURCE temp_f)
set (t070_SOURCE lrg_motor_d)
set (t071_SOURCE lrg_motor)
set (t080_SOURCE med_motor_d)
set (t081_SOURCE med_motor)
set (t160_SOURCE touch)
set (t290_SOURCE color_ref)
set (t291_SOURCE color_amb)
set (t292_SOURCE color_col)
set (t300_SOURCE us_prox)
set (t320_SOURCE gyro_a)
set (t321_SOURCE gyro_rate)
set (t330_SOURCE ir_prox)
set (t331_SOURCE ir_bpos)
set (t332_SOURCE ir_bbut)

add_lms_app (Brick+Datalog
    ARCHIVE
        "Brick Datalog"
    INSTALL_PATH
        "apps/Brick Datalog"
    ICON
        24x12_DLApp
    PROGRAMS
        "Brick Datalog"
    GRAPHICS
        144x82_POP4
        dl_PrtDetault
        dl_Rec
        dl_Set
        dl_RecH
        dl_SetH
        dl_Prt1
        dl_Prt2
        dl_Prt3
        dl_Prt4
        dl_PrtA
        dl_PrtB
        dl_PrtC
        dl_PrtD
        dl_Prt_All
        dl_Prt_Er
        dl_Prt_Un
        dl_count
        dl_mxave
        dl_NoDetails
        dlSetAccept
        dlSetAcceptH
        dl_GR0_1
        dl_GR0_8
        dl_GR0_10
        dl_GR0_100
        dl_GR0_250
        dl_GRc100
        dl_GRc180
        dl_GRc450
        dl_GR_TC
        dl_Highrate
        tdef
        t010
        t020
        t030
        t040
        t050
        t060
        t061
        t070
        t071
        t080
        t081
        t160
        t290
        t291
        t292
        t300
        t320
        t321
        t330
        t331
        t332
        PE_input
        PE_output
    SOUNDS
        GeneralAlarm
)
