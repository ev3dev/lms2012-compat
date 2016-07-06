#!/bin/bash
#
# install-to-working-directory.sh
#
# Copyright (C) 2016 David Lechner <david@lechnology.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#

#
# This script creates or updates the working directory for lms2012
#

dest_dir=/var/lib/lms2012

# This script depends on all source files being installed relative to the script
cd $(dirname $0)/..


# these files and directories are always copied
for d in \
    'apps/Brick Program'            \
    'apps/IR Control'               \
    'apps/Motor Control'            \
    'apps/Port View'                \
    'sys/settings/typedata.rcf'     \
    'sys/settings/typedata50.rcf'   \
    'sys/ui'                        \
    'tools/Bluetooth'               \
    'tools/Brick Info'              \
    'tools/Brick Name'              \
    'tools/Sleep'                   \
    'tools/Volume'                  \
    'tools/WiFi'                    \

do
    rm -rf "$dest_dir/$d"
    mkdir -p "$dest_dir/$(dirname "$d")"
    cp -r "$d" "$dest_dir/$(dirname "$d")/"
done


# everything else is only created if it does not already exist

if [ ! -f "$dest_dir/sys/settings/BrickName" ]; then
    echo -n $(hostname) > "$dest_dir/sys/settings/BrickName"
fi

mkdir -p "$dest_dir/prjs/SD_Card"
