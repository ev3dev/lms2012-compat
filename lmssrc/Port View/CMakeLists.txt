
set (ViewCore_SOURCE View_Core)
set (ViewTop_SOURCE P0_top)
set (ViewBot_SOURCE P0_bot)
set (ViewP1_SOURCE P1)
set (ViewP2_SOURCE P2)
set (ViewP3_SOURCE P3)
set (ViewP4_SOURCE P4)
set (ViewPA_SOURCE PA)
set (ViewPB_SOURCE PB)
set (ViewPC_SOURCE PC)
set (ViewPD_SOURCE PD)
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

add_lms_app (Port+View
    INSTALL_PATH "apps/Port View"
    ICON 24x12_ViewApp
    PROGRAMS "Port View"
    GRAPHICS
        ViewCore
        ViewTop
        ViewBot
        ViewP1
        ViewP2
        ViewP3
        ViewP4
        ViewPA
        ViewPB
        ViewPC
        ViewPD
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
)
