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
