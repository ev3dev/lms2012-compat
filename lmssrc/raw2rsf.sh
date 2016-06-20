#!/bin/bash
#
# convert raw sound file to LEGO MINDSTORMS EV3 robot sound file (.rsf)
#
# $1: The name of the raw file (8-bit, unsigned, 8kHz, mono)
# $2: The name of the rsf file
#

set -e

raw="$1"
rsf="$2"

size=$(printf "%04x" $(stat --printf="%s" "$raw"))

# create the header - all values are 16-bit big endian

# Format: RAW (0x0100)
printf "\x01\x00" > "$rsf"
# Data length
printf "\x${size:0:2}\x${size:2:4}" >> "$rsf"
# Sample rate (8kHz)
printf "\x1f\x40" >> "$rsf"
# Playback type
printf "\x00\x00" >> "$rsf"

# copy the data

dd if="$raw" of="$rsf" obs=8 seek=1 status=none
