#!/bin/bash
# hwsetup - minimalistic Hardware configuration tool for KNOPPIX
# (C) 2010 Klaus Knopper
# License: GPL V2
#
# Surprise! This used to be a C program, now it's a shell script, which
# looks up everything in /sys and /proc.
#
# Changes in behaviour of hwsetup 1.4 vs. 1.3:
# - hwinfo is no onger used, all information is being 
#   read from /proc and /sys directly.
#
# Changes in behaviour of hwsetup 1.3 vs. 1.2:
# - modules are not loaded anymore (apart from those thatr
#   hwinfo atomatically loads), this is now udevs job.
# - Mouse type not probed - it was guessed in 1.2 anyways.
#

# ANSI COLORS
CRE="[K"
NORMAL="[0;39m"
# RED: Failure or error message
RED="[1;31m"
# GREEN: Success message
GREEN="[1;32m"
# YELLOW: Descriptions
YELLOW="[1;33m"
# BLUE: System messages
BLUE="[1;34m"
# MAGENTA: Found devices or drivers
MAGENTA="[1;35m"
# CYAN: Questions
CYAN="[1;36m"
# BOLD WHITE: Hint
WHITE="[1;37m"

TMP="/tmp/hwsetup.$$.tmp"

trap bailout 1 2 3 10 15 

bailout(){
 rm -f "$TMP" "$TMP".progress
 exit $1
}

# Same options as old hwsetup 1.2, some newer
for arg in "$@"; do
 case "$arg" in
  -v) VERBOSE=1;;
  -p) FUNNYPROMPT=1;;
  -a) NOAUDIO=1;;
  -s) NOSCSI=1;;
  -x) NOX=1;;
  -n) DONOTHING=1;;
 esac
done

progress(){
 trap bailout 1 2 3 10 15 
 local prompt=( "/" "-" "\\" "|" )
 local green="\033[42;32m \033[0m"
 local count=0
 echo -n "Autoconfiguring devices...  "
 while [ -r "$TMP".progress ]; do
  if [ "$count" -ge 44 ]; then
   echo -n -e "\b${prompt[$((count%4))]}"
  else
   echo -n -e "\b$green$green"
  fi
  let count++
  usleep 25000
 done
 echo -e "\b$green$green"
}

# writeconfig string file
writeconfig(){
 local key="${1%%=*}"
 [ -n "$VERBOSE" ]   && echo -e "$1"
 if [ -z "$DONOTHING" ]; then
  grep -q "^$key=" "$2" 2>/dev/null && sed -i -e "s|^$key=.*\$|$1|g" "$2" || echo -e "$1" >> "$2" 2>/dev/null
 fi
}

# Check boot commandline for specified option
read CMDLINE </proc/cmdline
checkbootparam(){
 for i in $CMDLINE; do
  case "$i" in ($1|$1=*) return 0;; esac
 done
 return 1
}

# Check boot commandline for specified option,
# echo last found argument, or return false.
getbootparam(){
 result=""
 for i in $CMDLINE; do
  case "$i" in ($1=*) result="${i#*=}" ;; esac
 done
 [ -n "$result" ] || return 1
 echo "$result"
 return 0
}

# MAIN

[ -n "$FUNNYPROMPT" ] && touch "$TMP".progress && progress &

# Xorg configuration, including boot opts
if [ -z "$NOX" ] && type -p mkxorgconfig >/dev/null 2>&1 ; then
 XORGOPTIONS="-nomodelines"
 checkbootparam "xmodelines" || XORGOPTIONS=""
 XSERVER="$(getbootparam xserver)" 
 [ -n "$XSERVER" ] && writeconfig "XSERVER=\"$XSERVER\"" /etc/sysconfig/xserver
 XMODULE="$(getbootparam xmodule)" 
 COMPOSITE="-composite"
 # Check for graphics cards that work with special drivers, but which are not autodetected
 if [ -z "$XMODULE" ]; then
  NVPCI="$(lspci -nd 10de:\* | awk '/0300/{print $3; exit}')"
  # Note: NVidia cards seem to get autodetected with nouveau, so this is default,
  # and we just need to handle cases for cards not working with nouveau.
  # 10de:0421 (GeForce 8500 GT) Jan reported it doesn't work with nouveau.
  # Try nvidias proprietary driver first, if present.
  if [ -n "$NVPCI" -a -r /usr/lib/xorg/modules/drivers/nvidia_drv.so ]; then
   XMODULE="nvidia"
  else
   case "$NVPCI" in
    10de:000*|10de:001*|10de:0421)
     if [ -r /usr/lib/xorg/modules/drivers/nv_drv.so ]; then
      XMODULE="nv" # unsupported by nouveau
      COMPOSITE=""
     fi ;;
    # NV40 NV30
    10de:00[3456789abcdef]*|10de:0[3456789abcdef]*|10de:[123456789abcdef]*) # >= NV30: works with 3D
     XMODULE="nouveau";;
    10de:*) # other cards: may work, but no 3D
     XMODULE="nouveau"; COMPOSITE="";;
   esac
  fi
  # Check for poulsbo
  PSBPCI="$(lspci -nd 8086:\* | awk '/0300/{print $3; exit}')"
  case "$PSBPCI" in
   8086:810[89])
    if [ -r /usr/lib/xorg/modules/drivers/psb_drv.so ]; then
     XMODULE="psb"; COMPOSITE="-composite"
    fi
   ;;
  esac
 fi
 [ -n "$XMODULE" ] && writeconfig "XMODULE=\"$XMODULE\"" /etc/sysconfig/xserver
 XSCREEN="$(getbootparam xscreen 2>/dev/null)"
 [ -n "$XSCREEN" ] || XSCREEN="$(getbootparam screen 2>/dev/null)"
 [ -n "$XSCREEN" ] && writeconfig "XSCREEN=\"$XSCREEN\"" /etc/sysconfig/xserver
 XVREFRESH="$(getbootparam xvrefresh 2>/dev/null)"
 [ -n "$XVREFRESH" ] || XVREFRESH="$(getbootparam vrefresh 2>/dev/null)"
 [ -n "$XVREFRESH" ] || XVREFRESH="$(getbootparam xvsync 2>/dev/null)"
 [ -n "$XVREFRESH" ] || XVREFRESH="$(getbootparam vsync 2>/dev/null)"
 [ -n "$XVREFRESH" ] && writeconfig "XVREFRESH=\"$XVREFRESH\"" /etc/sysconfig/xserver
 XHREFRESH="$(getbootparam xhrefresh 2>/dev/null)"
 [ -n "$XHREFRESH" ] || XHREFRESH="$(getbootparam hrefresh 2>/dev/null)"
 [ -n "$XHREFRESH" ] || XHREFRESH="$(getbootparam xhsync 2>/dev/null)"
 [ -n "$XHREFRESH" ] || XHREFRESH="$(getbootparam hsync 2>/dev/null)"
 [ -n "$XHREFRESH" ] && writeconfig "XHREFRESH='$XHREFRESH'" /etc/sysconfig/xserver
 XDEPTH="$(getbootparam xdepth 2>/dev/null)"
 [ -n "$XDEPTH" ] || XDEPTH="$(getbootparam depth 2>/dev/null)"
 [ -n "$XDEPTH" ] && writeconfig "XDEPTH='$XDEPTH'" /etc/sysconfig/xserver

 # Enable Composite in xorg.conf if chosen desktop requires it, or user specified "3d".
 DESKTOP="$(getbootparam desktop)" 
  { checkbootparam "no3d" >/dev/null 2>&1 || \
    checkbootparam "nocomposite" >/dev/null 2>&1; } && \
       COMPOSITE=""
   # Never enable Composite for vesa and fbdev!
 case "$XMODULE" in vesa|fbdev) COMPOSITE="";; esac
 # This ONLY detects the primary (internal) mouse, /sys/class/input/mouse*/device/protocol exists.
 for i in $(ls -1d /sys/class/input/mouse* 2>/dev/null); do
  [ -r "${i}/device/protocol" ] || continue
  p="$(cat ${i}/device/protocol 2>/dev/null)"
  d="$(cat ${i}/device/input*/name 2>/dev/null | tail -1)"
  [ -n "$d" ] && writeconfig "FULLNAME='$d'" /etc/sysconfig/mouse
  [ -n "$p" ] && writeconfig "XMOUSETYPE='$p'" /etc/sysconfig/mouse
  break
 done
 mkxorgconfig $XORGOPTIONS $COMPOSITE
fi

rm -f "$TMP".progress # End prompt subprocess
wait # for subprocesses to end

bailout 0
