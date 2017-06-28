lms2012-compat
==============

This is an unofficial fork of the official software that runs on the LEGO
MINDSTORMS EV3 programmable brick.

At a high level, the goal is to remain compatible with the official software
so that any programming tool that works with the official software will work
with `lms2012-compat`. At a low level, it has been modified to run on top of
[ev3dev], so that it will be able to run on hardware other than the EV3.

[ev3dev]: http://www.ev3dev.org


Status
------

The current goal of this project is to work with a new BeagleBone cape that
is being developed. As it stands, [ev3dev lacks some key features for motors
and sensors][issue28] that are needed for general use.

[issue28]: https://github.com/ev3dev/lms2012-compat/issues/28


Usage
-----

*   Grab the latest ev3dev-stretch image from [here][snapshot].

*   For FatcatLab EVB and Quest Institute QuestCape, Edit `uEnv.txt` and
    uncomment the appropriate `cape=...` line in the `EV3DEV_BOOT` partition
    before booting the first time.

*   For LEGO MINDSTORMS EV3 boot and run...

        sudo FK_MACHINE="LEGO MINDSTORMS EV3 + lms2012-compat" flash-kernel
        sudo reboot
        # to revert to standard ev3dev kernel...
        sudo FK_MACHINE="LEGO MINDSTORMS EV3" flash-kernel
        sudo reboot

*   After (re)boot, open a remote terminal and run...

        sudo lms2012

[snapshot]: https://oss.jfrog.org/list/oss-snapshot-local/org/ev3dev/brickstrap/


Hacking
-------

There is information on how to build the source code in the [docker](docker/)
folder.
