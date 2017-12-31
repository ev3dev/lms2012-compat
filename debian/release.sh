#!/bin/bash
#
# Maintainer script for publishing releases.

set -e

source=$(dpkg-parsechangelog -S Source)
version=$(dpkg-parsechangelog -S Version)
distribution=$(dpkg-parsechangelog -S Distribution)
codename=$(debian-distro-info --codename --${distribution})
arm_target="robot@ev3dev-rpi3"

gbp buildpackage -S -us -uc

ssh ${arm_target} "mkdir -p ~/pbuilder-ev3dev/source"
scp ../${source}_${version}.dsc ${arm_target}:~/pbuilder-ev3dev/source
scp ../${source}_${version%-*}.tar.xz ${arm_target}:~/pbuilder-ev3dev/source

ssh -t ${arm_target} "OS=debian DIST=${codename} ARCH=armel pbuilder-ev3dev dsc-build \
    ~/pbuilder-ev3dev/source/${source}_${version}.dsc"
mkdir -p ~/pbuilder-ev3dev/debian/${codename}-armel
scp ${arm_target}:~/pbuilder-ev3dev/debian/${codename}-armel/${source}_${version}.dsc \
    ~/pbuilder-ev3dev/debian/${codename}-armel/
scp ${arm_target}:~/pbuilder-ev3dev/debian/${codename}-armel/${source}_${version%-*}.tar.xz \
    ~/pbuilder-ev3dev/debian/${codename}-armel/
scp ${arm_target}:~/pbuilder-ev3dev/debian/${codename}-armel/lms2012-compat_${version}_armel.deb \
    ~/pbuilder-ev3dev/debian/${codename}-armel/
scp ${arm_target}:~/pbuilder-ev3dev/debian/${codename}-armel/lms2012-compat-dbgsym_${version}_armel.deb \
    ~/pbuilder-ev3dev/debian/${codename}-armel/
scp ${arm_target}:~/pbuilder-ev3dev/debian/${codename}-armel/${source}_${version}_armel.changes \
    ~/pbuilder-ev3dev/debian/${codename}-armel/
scp ${arm_target}:~/pbuilder-ev3dev/debian/${codename}-armel/${source}_${version}_armel.buildinfo \
    ~/pbuilder-ev3dev/debian/${codename}-armel/

ssh -t ${arm_target} "OS=debian DIST=${codename} ARCH=armhf PBUILDER_OPTIONS=\"--binary-arch\" pbuilder-ev3dev dsc-build \
    ~/pbuilder-ev3dev/source/${source}_${version}.dsc"
mkdir -p ~/pbuilder-ev3dev/debian/${codename}-armhf
scp ${arm_target}:~/pbuilder-ev3dev/debian/${codename}-armhf/lms2012-compat_${version}_armhf.deb \
    ~/pbuilder-ev3dev/debian/${codename}-armhf/
scp ${arm_target}:~/pbuilder-ev3dev/debian/${codename}-armhf/lms2012-compat-dbgsym_${version}_armhf.deb \
    ~/pbuilder-ev3dev/debian/${codename}-armhf/
scp ${arm_target}:~/pbuilder-ev3dev/debian/${codename}-armhf/${source}_${version}_armhf.changes \
    ~/pbuilder-ev3dev/debian/${codename}-armhf/
scp ${arm_target}:~/pbuilder-ev3dev/debian/${codename}-armhf/${source}_${version}_armhf.buildinfo \
    ~/pbuilder-ev3dev/debian/${codename}-armhf/

debsign ~/pbuilder-ev3dev/debian/${codename}-armel/${source}_${version}_armel.changes
debsign ~/pbuilder-ev3dev/debian/${codename}-armhf/${source}_${version}_armhf.changes

dput ev3dev-debian ~/pbuilder-ev3dev/debian/${codename}-armel/${source}_${version}_armel.changes
dput ev3dev-debian ~/pbuilder-ev3dev/debian/${codename}-armhf/${source}_${version}_armhf.changes

gbp buildpackage --git-tag-only
