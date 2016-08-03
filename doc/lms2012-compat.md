% LMS2012-COMPAT(8) | Administrator's Manual
% David Lechner
% August 2016

# NAME

lms2012-compat - LEGO MINDSTORMS EV3 Compatible VM for ev3dev

# SYNOPSIS

lms2012 [*rbf-file*]

# DESCRIPTION

Starts the VM. If no *rbf-file* is specified, the default `ui/ui.rbf` will run.

# OPTIONS

*rbf-file*
: `.rbf` bytecode file to run. Note: The working directory is not the current
directory, so use absolute paths!

# FILES

`/var/lib/lms2012/apps/`
: Contains application directories that are listed on the apps tab of the
user interface.

`/var/lib/lms2012/prjs/`
: Contains project directories that are listed on the projects tab of the
user interface.

`/var/lib/lms2012/prjs/BrkDL_Save/`
: Location where the on-brick data logger stores `.rdf` files.

`/var/lib/lms2012/prjs/BrkProg_Save/`
: Location where the on-brick programmer stores `.rpf` files.

`/var/lib/lms2012/prjs/SD_Card/`
: Used as the "SD Card" directory.

`/var/lib/lms2012/sys/`
: The working directory for `lms2012` (regardless of where it was run).

`/var/lib/lms2012/sys/settings/`
: Contains settings files used by `lms2012`. These files should not be edited
manually!

`/var/lib/lms2012/sys/ui/`
: Contains the default user interface program.

`/var/lib/lms2012/tools/`
: Contains tool directories that are listed on the tools tab of the user
interface.
