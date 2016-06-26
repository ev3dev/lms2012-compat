#!/bin/bash
#
# usb-hid-gadget.sh
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

set -e

cmd="$1"
udc="$2"

usb_gadget_dir="/sys/kernel/config/usb_gadget"
usb_gadget_name="ev3-hid"
vendor_id="0x0694"  # LEGO Group
product_id="0x0005" # EV3
manufacturer="ev3dev"
product="lms2012-compat"
serial="012345678912"

# 0x06, 0x00, 0xFF, Usage page (vendor defined)
# 0x09, 0x01,       Usage ID (vendor defined)
# 0xA1, 0x01,       Collection (application)
#                   The Input report
# 0x15, 0x00,       Logical Minimum (0)
# 0x26, 0xFF, 0x00, Logical Maximum (255)
# 0x75, 0x08,       Report Size (8 bits)
# 0x96, 0x00, 0x04, Report Count (1024 fields)
# 0x09, 0x01,       USAGE (vendor usage 1)
# 0x81, 0x02,       Input (Data, Variable, Absolute)
# 0x96, 0x00, 0x04, Report Count (1024 fields)
# 0x09, 0x01,       USAGE (vendor usage 1)
# 0x91, 0x02,       Output (Data, Variable, Absolute)
# 0xc0              END_COLLECTION

report_desc="\x06\x00\xFF\x09\x01\xA1\x01\x15\x01\x15\x00\x26\xFF\x00\x75\x08\x96\x00\x04\x09\x01\x81\x02\x96\x00\x04\x09\x01\x91\x02\xc0"
report_length=1024

# 0x06, 0x00, 0xFF, Usage page (vendor defined)
# 0x09, 0x01,       Usage ID (vendor defined)
# 0xA1, 0x01,       Collection (application)
#                   The Input report
# 0x15, 0x00,       Logical Minimum (0)
# 0x26, 0xFF, 0x00, Logical Maximum (255)
# 0x75, 0x08,       Report Size (8 bits)
# 0x96, 0x40, 0x00, Report Count (64 fields)
# 0x09, 0x01,       USAGE (vendor usage 1)
# 0x81, 0x02,       Input (Data, Variable, Absolute)
# 0x96, 0x40, 0x00, Report Count (64 fields)
# 0x09, 0x01,       USAGE (vendor usage 1)
# 0x91, 0x02,       Output (Data, Variable, Absolute)
# 0xc0              END_COLLECTION

# TODO: Need to create a "low speed" mode for daisy chaining that uses the
#       descriptor above. It will have report_length=64


self=$(readlink -f $0)

case $cmd in
start)
    if [ ! -d $usb_gadget_dir ]; then
        modprobe libcomposite
        count=0
        while [ ! -d $usb_gadget_dir ]; do
            sleep 0.1
            count=$(( $count + 1 ))
            if [ $count -ge 10 ]; then
                >&2 echo "starting libcomposite timed out"
                exit 1
            fi
        done
    fi
    cd $usb_gadget_dir
    mkdir $usb_gadget_name
    cd $usb_gadget_name

    echo $vendor_id > idVendor
    echo $product_id > idProduct

    mkdir strings/0x409
    echo $manufacturer > strings/0x409/manufacturer
    echo $product > strings/0x409/product
    echo $serial > strings/0x409/serialnumber

    mkdir configs/c.1
    echo 0xC0 > configs/c.1/bmAttributes # self powered
    echo 1 > configs/c.1/MaxPower        # 2mA

    mkdir configs/c.1/strings/0x409
    echo "lms2012-compat-hid" > configs/c.1/strings/0x409/configuration

    mkdir functions/hid.usb0
    echo 0 > functions/hid.usb0/protocol # use class code info from interfaces
    echo 0 > functions/hid.usb0/subclass
    echo $report_length > functions/hid.usb0/report_length
    echo -en $report_desc > functions/hid.usb0/report_desc

    ln -s functions/hid.usb0 configs/c.1
    echo $udc > UDC || { $self stop; exit 1; }
    ;;
stop)
    if [ ! -d $usb_gadget_dir/$usb_gadget_name ]; then
        # nothing to stop
        exit 0
    fi
    cd $usb_gadget_dir/$usb_gadget_name
    if [ -e configs/c.1/hid.usb0 ]; then
        rm configs/c.1/hid.usb0
    fi
    if [ -d configs/c.1/strings/0x409 ]; then
        rmdir configs/c.1/strings/0x409
    fi
    if [ -d strings/0x409 ]; then
        rmdir strings/0x409
    fi
    if [ -d functions/hid.usb0 ]; then
        rmdir functions/hid.usb0
    fi
    if [ -d configs/c.1 ]; then
        rmdir configs/c.1
    fi
    cd $usb_gadget_dir
    rmdir $usb_gadget_name
    ;;
esac

exit 0
