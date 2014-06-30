#!/bin/bash
# mkxorgconfig - Create a working xorg.conf file
# (C) Klaus Knopper 2006
# License: GPL V2

PATH="/bin:/usr/bin:/sbin:/usr/sbin"; export PATH
umask 022

# [ "`id -u`" != "0" ] && echo "WARNING: $0 has to run as root to work properly." 1>&2

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

KNOPPIXSIG="Created by KNOPPIX"

# Only (re-)create /etc/X11/xorg.conf if not autocreated.
if [ -r /etc/X11/xorg.conf ]; then
 if ! egrep -qi "$KNOPPIXSIG" /etc/X11/xorg.conf; then
  echo "${BLUE}$0: Not overwriting existing /etc/X11/xorg.conf.${NORMAL}" >&2
  exit 0
 fi
fi

### Utility Function(s)

KVER="$(uname -r)"

# Reread boot command line; echo last parameter's argument or return false.
CMDLINE="$(cat /proc/cmdline)"

# "modelines" and "no composite" is the safe default
COMPOSITE=""

DDC=""

for i in "$@"; do
 case "$i" in
  -ddc*)        DDC="true";;
  -composite*)  COMPOSITE="true";;
 esac
done

# Override by knoppix-autoconfig
[ -n "$NOCOMPOSITE" ] && COMPOSITE=""

getbootparam(){
case "$CMDLINE" in *\ $1=*) ;; *) return 1; ;; esac
result="${CMDLINE##*$1=}"
result="${result%%[     ]*}"
echo "$result"
return 0
}

checkbootparam(){
case "$CMDLINE" in *\ $1*) return 0;; esac
return 1
}

# Read in what hwsetup has found for X
[ -r /etc/sysconfig/xserver ] && . /etc/sysconfig/xserver

# Check for Xorg
if [ -n "$XSERVER" ]; then
  type -p "$XSERVER" >/dev/null 2>&1 || XSERVER="Xorg"
else
  XSERVER="Xorg"
fi

# Note: This did not work out, some radeonhd work better with "radeon". :-(
#if [ -z "$XMODULE" ]; then
# # Temporary workaround for failing 7.4 radeonhd autodetection
# for i in `lspci -mm | awk '/[Vv][Gg][Aa] compatible [Cc]ontroller/{print $1}'`; do
#  case "$(lspci -k -s "$i")" in
#   *[Rr]adeon\ [Hh][Dd]*) XMODULE="radeonhd"; break ;;
#  esac
# done
#fi

# Handle xorg driver for this graphics card
DRIVER="# Driver (chipset) autodetect"
CHIPSET_TWEAKS=""
if [ -n "$XMODULE" ]; then
 case "$XMODULE" in
  pvr2fb) modprobe "$XMODULE" >/dev/null 2>&1 ; XMODULE="fbdev"; ;;
  psb) CHIPSET_TWEAKS="$CHIPSET_TWEAKS
	Option \"ExaNoComposite\" \"true\"
	Option \"DRI\" \"on\"
        Option \"MigrationHeuristic\" \"greedy\"
        Option \"IgnoreACPI\" \"yes\"
" ;;
 esac
 if [ -r /usr/lib/xorg/modules/drivers/"$XMODULE"_drv.so ]; then
  DRIVER="	# Specified driver (chipset)
	Driver      \"${XMODULE}\""
 else
  XMODULE=""
 fi
fi

# Read default keyboard from config file.
# There seems to be no reliable autoprobe possible.
[ -r /etc/sysconfig/keyboard ] && . /etc/sysconfig/keyboard

DEADKEYS='Option "XkbVariant" "nodeadkeys"'
[ "$XKEYBOARD" = "de" ] || DEADKEYS=''

NODDC=""
if checkbootparam noddc; then
 NODDC="true"
 DDC=""
fi

NORANDR=""
if checkbootparam norandr; then
 NORANDR='Option "RandR" "false"'
fi

# Use commandline vertical refresh rate if specified
if [ -n "$XVREFRESH" ]; then
 XVREFRESH='VertRefresh 49.0 - '"$XVREFRESH"
fi

# Use commandline horizontal refresh rate if specified
if [ -n "$XHREFRESH" ]; then
 XHREFRESH='HorizSync 28.0 - '"$XHREFRESH"
fi

MONITOR=""
COMPLETE=""
RC=""

if [ -n "$DDC" -a -z "$NODDC" ] && type ddcxinfo-knoppix >/dev/null 2>&1 ; then
 MONITOR="$(ddcxinfo-knoppix -monitor)"
 RC="$?"
 COMPLETE="$(awk '/EndSection/{print}' <<EOF
$MONITOR
EOF
)"
fi

if [ "$RC" != "0" -o -z "$MONITOR" -o -z "$COMPLETE" ]; then
MONITOR='
Section "Monitor"
	Identifier   "Monitor0"
	ModelName    "Generic Monitor"
#	HorizSync    28.0 - 78.0 # Warning: This may fry very old Monitors
#	HorizSync    28.0 - 96.0 # Warning: This may fry old Monitors
#	VertRefresh  50.0 - 76.0 # Very conservative. May flicker.
#	VertRefresh  50.0 - 60.0 # Extreme conservative. Will flicker. TFT default.'"
        $XHREFRESH
	$XVREFRESH"'
	Option "MonitorLayout" "LVDS,AUTO"
'

# If we are here, yet DDC is set, it has found no modelines. Add some.
if [ -n "$DDC" ]; then
 MONITOR="$MONITOR"'

	#  Default modes distilled from
	#      "VESA and Industry Standards and Guide for Computer Display Monitor
	#       Timing", version 1.0, revision 0.8, adopted September 17, 1998.
	#  $XFree86: xc/programs/Xserver/hw/xfree86/etc/vesamodes,v 1.4 1999/11/18 16:52:17 tsi Exp $
	# 640x350 @ 85Hz (VESA) hsync: 37.9kHz
	ModeLine "640x350"    31.5  640  672  736  832    350  382  385  445 +hsync -vsync
	# 640x400 @ 85Hz (VESA) hsync: 37.9kHz
	ModeLine "640x400"    31.5  640  672  736  832    400  401  404  445 -hsync +vsync
	# 720x400 @ 85Hz (VESA) hsync: 37.9kHz
	ModeLine "720x400"    35.5  720  756  828  936    400  401  404  446 -hsync +vsync
	# 640x480 @ 60Hz (Industry standard) hsync: 31.5kHz
	ModeLine "640x480"    25.2  640  656  752  800    480  490  492  525 -hsync -vsync
	# 640x480 @ 72Hz (VESA) hsync: 37.9kHz
	ModeLine "640x480"    31.5  640  664  704  832    480  489  491  520 -hsync -vsync
	# 640x480 @ 75Hz (VESA) hsync: 37.5kHz
	ModeLine "640x480"    31.5  640  656  720  840    480  481  484  500 -hsync -vsync
	# 640x480 @ 85Hz (VESA) hsync: 43.3kHz
	ModeLine "640x480"    36.0  640  696  752  832    480  481  484  509 -hsync -vsync
	# 800x600 @ 56Hz (VESA) hsync: 35.2kHz
	ModeLine "800x600"    36.0  800  824  896 1024    600  601  603  625 +hsync +vsync
	# 800x600 @ 60Hz (VESA) hsync: 37.9kHz
	ModeLine "800x600"    40.0  800  840  968 1056    600  601  605  628 +hsync +vsync
	# 800x600 @ 72Hz (VESA) hsync: 48.1kHz
	ModeLine "800x600"    50.0  800  856  976 1040    600  637  643  666 +hsync +vsync
	# 800x600 @ 75Hz (VESA) hsync: 46.9kHz
	ModeLine "800x600"    49.5  800  816  896 1056    600  601  604  625 +hsync +vsync
	# 800x600 @ 85Hz (VESA) hsync: 53.7kHz
	ModeLine "800x600"    56.3  800  832  896 1048    600  601  604  631 +hsync +vsync
	# 1024x768i @ 43Hz (industry standard) hsync: 35.5kHz
	ModeLine "1024x768"   44.9 1024 1032 1208 1264    768  768  776  817 +hsync +vsync Interlace
	# 1024x768 @ 60Hz (VESA) hsync: 48.4kHz
	ModeLine "1024x768"   65.0 1024 1048 1184 1344    768  771  777  806 -hsync -vsync
	# 1024x768 @ 70Hz (VESA) hsync: 56.5kHz
	ModeLine "1024x768"   75.0 1024 1048 1184 1328    768  771  777  806 -hsync -vsync
	# 1024x768 @ 75Hz (VESA) hsync: 60.0kHz
	ModeLine "1024x768"   78.8 1024 1040 1136 1312    768  769  772  800 +hsync +vsync
	# 1024x768 @ 85Hz (VESA) hsync: 68.7kHz
	ModeLine "1024x768"   94.5 1024 1072 1168 1376    768  769  772  808 +hsync +vsync
	# 1152x864 @ 75Hz (VESA) hsync: 67.5kHz
	ModeLine "1152x864"  108.0 1152 1216 1344 1600    864  865  868  900 +hsync +vsync
	# 1280x960 @ 60Hz (VESA) hsync: 60.0kHz
	ModeLine "1280x960"  108.0 1280 1376 1488 1800    960  961  964 1000 +hsync +vsync
	# 1280x960 @ 85Hz (VESA) hsync: 85.9kHz
	ModeLine "1280x960"  148.5 1280 1344 1504 1728    960  961  964 1011 +hsync +vsync
	# 1280x1024 @ 60Hz (VESA) hsync: 64.0kHz
	ModeLine "1280x1024" 108.0 1280 1328 1440 1688   1024 1025 1028 1066 +hsync +vsync
	# 1280x1024 @ 75Hz (VESA) hsync: 80.0kHz
	ModeLine "1280x1024" 135.0 1280 1296 1440 1688   1024 1025 1028 1066 +hsync +vsync
	# 1280x1024 @ 85Hz (VESA) hsync: 91.1kHz
	# ModeLine "1280x1024" 157.5 1280 1344 1504 1728   1024 1025 1028 1072 +hsync +vsync
	# (Better for ILIAMA Monitors)
	ModeLine "1280x1024" 157.50 1280 1328 1488 1728 1024 1025 1028 1072 +hsync +vsync
	# 1600x1200 @ 60Hz (VESA) hsync: 75.0kHz
	ModeLine "1600x1200" 162.0 1600 1664 1856 2160   1200 1201 1204 1250 +hsync +vsync
	# 1600x1200 @ 65Hz (VESA) hsync: 81.3kHz
	ModeLine "1600x1200" 175.5 1600 1664 1856 2160   1200 1201 1204 1250 +hsync +vsync
	# 1600x1200 @ 70Hz (VESA) hsync: 87.5kHz
	ModeLine "1600x1200" 189.0 1600 1664 1856 2160   1200 1201 1204 1250 +hsync +vsync
	# 1600x1200 @ 75Hz (VESA) hsync: 93.8kHz
	ModeLine "1600x1200" 202.5 1600 1664 1856 2160   1200 1201 1204 1250 +hsync +vsync
	# 1600x1200 @ 85Hz (VESA) hsync: 106.3kHz
	ModeLine "1600x1200" 229.5 1600 1664 1856 2160   1200 1201 1204 1250 +hsync +vsync
	# 1792x1344 @ 60Hz (VESA) hsync: 83.6kHz
	ModeLine "1792x1344" 204.8 1792 1920 2120 2448   1344 1345 1348 1394 -hsync +vsync
	# 1792x1344 @ 75Hz (VESA) hsync: 106.3kHz
	ModeLine "1792x1344" 261.0 1792 1888 2104 2456   1344 1345 1348 1417 -hsync +vsync
	# 1856x1392 @ 60Hz (VESA) hsync: 86.3kHz
	ModeLine "1856x1392" 218.3 1856 1952 2176 2528   1392 1393 1396 1439 -hsync +vsync
	# 1856x1392 @ 75Hz (VESA) hsync: 112.5kHz
	ModeLine "1856x1392" 288.0 1856 1984 2208 2560   1392 1393 1396 1500 -hsync +vsync
	# 1920x1440 @ 60Hz (VESA) hsync: 90.0kHz
	ModeLine "1920x1440" 234.0 1920 2048 2256 2600   1440 1441 1444 1500 -hsync +vsync
	# 1920x1440 @ 75Hz (VESA) hsync: 112.5kHz
	ModeLine "1920x1440" 297.0 1920 2064 2288 2640   1440 1441 1444 1500 -hsync +vsync
	# Additional modelines
	ModeLine "1800x1440"  230    1800 1896 2088 2392  1440 1441 1444 1490 +HSync +VSync
	ModeLine "1800x1440"  250    1800 1896 2088 2392  1440 1441 1444 1490 +HSync +VSync
	# Extended modelines with GTF timings
	# 640x480 @ 100.00 Hz (GTF) hsync: 50.90 kHz; pclk: 43.16 MHz
	ModeLine "640x480"  43.16  640 680 744 848  480 481 484 509  -HSync +Vsync
	# 768x576 @ 60.00 Hz (GTF) hsync: 35.82 kHz; pclk: 34.96 MHz
	ModeLine "768x576"  34.96  768 792 872 976  576 577 580 597  -HSync +Vsync
	# 768x576 @ 72.00 Hz (GTF) hsync: 43.27 kHz; pclk: 42.93 MHz
	ModeLine "768x576"  42.93  768 800 880 992  576 577 580 601  -HSync +Vsync
	# 768x576 @ 75.00 Hz (GTF) hsync: 45.15 kHz; pclk: 45.51 MHz
	ModeLine "768x576"  45.51  768 808 888 1008  576 577 580 602  -HSync +Vsync
	# 768x576 @ 85.00 Hz (GTF) hsync: 51.42 kHz; pclk: 51.84 MHz
	ModeLine "768x576"  51.84  768 808 888 1008  576 577 580 605  -HSync +Vsync
	# 768x576 @ 100.00 Hz (GTF) hsync: 61.10 kHz; pclk: 62.57 MHz
	ModeLine "768x576"  62.57  768 816 896 1024  576 577 580 611  -HSync +Vsync
	# 800x600 @ 100.00 Hz (GTF) hsync: 63.60 kHz; pclk: 68.18 MHz
	ModeLine "800x600"  68.18  800 848 936 1072  600 601 604 636  -HSync +Vsync
	# 1024x768 @ 100.00 Hz (GTF) hsync: 81.40 kHz; pclk: 113.31 MHz
	ModeLine "1024x768"  113.31  1024 1096 1208 1392  768 769 772 814  -HSync +Vsync
	# 1152x864 @ 60.00 Hz (GTF) hsync: 53.70 kHz; pclk: 81.62 MHz
	ModeLine "1152x864"  81.62  1152 1216 1336 1520  864 865 868 895  -HSync +Vsync
	# 1152x864 @ 85.00 Hz (GTF) hsync: 77.10 kHz; pclk: 119.65 MHz
	ModeLine "1152x864"  119.65  1152 1224 1352 1552  864 865 868 907  -HSync +Vsync
	# 1152x864 @ 100.00 Hz (GTF) hsync: 91.50 kHz; pclk: 143.47 MHz
	ModeLine "1152x864"  143.47  1152 1232 1360 1568  864 865 868 915  -HSync +Vsync
	# 1280x960 @ 72.00 Hz (GTF) hsync: 72.07 kHz; pclk: 124.54 MHz
	ModeLine "1280x960"  124.54  1280 1368 1504 1728  960 961 964 1001  -HSync +Vsync
	# 1280x960 @ 75.00 Hz (GTF) hsync: 75.15 kHz; pclk: 129.86 MHz
	ModeLine "1280x960"  129.86  1280 1368 1504 1728  960 961 964 1002  -HSync +Vsync
	# 1280x960 @ 100.00 Hz (GTF) hsync: 101.70 kHz; pclk: 178.99 MHz
	ModeLine "1280x960"  178.99  1280 1376 1520 1760  960 961 964 1017  -HSync +Vsync
	# 1280x1024 @ 100.00 Hz (GTF) hsync: 108.50 kHz; pclk: 190.96 MHz
	ModeLine "1280x1024"  190.96  1280 1376 1520 1760  1024 1025 1028 1085  -HSync +Vsync
	# 1400x1050 @ 60.00 Hz (GTF) hsync: 65.22 kHz; pclk: 122.61 MHz
	ModeLine "1400x1050"  122.61  1400 1488 1640 1880  1050 1051 1054 1087  -HSync +Vsync
	# 1400x1050 @ 72.00 Hz (GTF) hsync: 78.77 kHz; pclk: 149.34 MHz
	ModeLine "1400x1050"  149.34  1400 1496 1648 1896  1050 1051 1054 1094  -HSync +Vsync
	# 1400x1050 @ 75.00 Hz (GTF) hsync: 82.20 kHz; pclk: 155.85 MHz
	ModeLine "1400x1050"  155.85  1400 1496 1648 1896  1050 1051 1054 1096  -HSync +Vsync
	# 1400x1050 @ 85.00 Hz (GTF) hsync: 93.76 kHz; pclk: 179.26 MHz
	ModeLine "1400x1050"  179.26  1400 1504 1656 1912  1050 1051 1054 1103  -HSync +Vsync
	# 1400x1050 @ 100.00 Hz (GTF) hsync: 111.20 kHz; pclk: 214.39 MHz
	ModeLine "1400x1050"  214.39  1400 1512 1664 1928  1050 1051 1054 1112  -HSync +Vsync
	# 1600x1200 @ 100.00 Hz (GTF) hsync: 127.10 kHz; pclk: 280.64 MHz
	ModeLine "1600x1200"  280.64  1600 1728 1904 2208  1200 1201 1204 1271  -HSync +Vsync
	# 1680x1050 (Notebook)
	ModeLine "1680x1050"  154.20  1680 1712 2296 2328  1050 1071 1081 1103
'
fi
MONITOR="$MONITOR
EndSection
"
fi

# Extract values for display
MODEL="$(awk '/^[	 ]*ModelName/{print;exit}'<<EOF
$MONITOR
EOF
)"

MODEL="${MODEL#*\"}"
MODEL="${MODEL%\"*}"

HREFRESH="$(awk '/^[	 ]*HorizSync/{print $2 $3 $4; exit}'<<EOF
$MONITOR
EOF
)"

VREFRESH="$(awk '/^[	 ]*VertRefresh/{print $2 $3 $4; exit}'<<EOF
$MONITOR
EOF
)"

# Build line of allowed modes
MODES=""
ADDMODE="$(getbootparam screen)"
case "$ADDMODE" in [1-9]*x*[0-9]) MODES="Modes \"$ADDMODE\"";; esac

case "$XMODULE" in
 fbdev) DEPTH="" ;;
     *) DEPTH=""
        # Use commandline colordepth if specified
        if [ -n "$XDEPTH" ]; then
         DEPTH='DefaultColorDepth '"$XDEPTH"
        fi ;;
esac

# Read mouse config
FULLNAME=""
[ -r /etc/sysconfig/mouse ] && . /etc/sysconfig/mouse

SERIALMOUSE='# Serial Mouse auto-probed'
SERIALMOUSEDEV="$(getbootparam serialmouse)"
[ -z "$SERIALMOUSEDEV" ] && checkbootparam serialmouse && SERIALMOUSEDEV="/dev/ttyS0"
if [ -n "$SERIALMOUSEDEV" ]; then
 modprobe serial >/dev/null 2>&1
# SERIALMOUSE='InputDevice    "Serial Mouse" "SendCoreEvents"' || SERIALMOUSEDEV="/dev/ttyS0"
fi

AUTOMOUSE=""
AUTOKEYBOARD=""
ALLOWEMPTY=""
# xorg 7.5: use evdev and let hardware detection do the work
if ! checkbootparam noevdev; then
 AUTOMOUSE=true
 AUTOKEYBOARD=true
else
 ALLOWEMPTY='	Option "AllowEmptyInput" "false"'
fi

# AlpsPS/2 | SynPS/2 Touchpad | Unlikely case of detected serial mouse
TOUCHPAD='# Touchpad auto-probed'
USBMOUSE='# USB mouse auto-probed'
if [ -z "$AUTOMOUSE" ]; then
 PSMOUSE='	InputDevice    "PS/2 Mouse" "SendCoreEvents"'
 if [ -n "$FULLNAME$XMOUSETYPE" ]; then
   case "$FULLNAME$XMOUSETYPE" in
    *AlpsPS/2*) TOUCHPAD='InputDevice    "AlpsPad" "SendCoreEvents"'; PSMOUSE="" ;;
   *SynPS/2*)  TOUCHPAD='InputDevice    "SynPad"  "SendCoreEvents"'; PSMOUSE="" ;;
   *[Ss][Ee][Rr][Ii][Aa][Ll]*)
     SERIALMOUSE='InputDevice    "Serial Mouse" "SendCoreEvents"'
     SERIALMOUSEDEV="${DEVICE}"
     [ -n "$SERIALMOUSEDEV" ] || SERIALMOUSEDEV="$(ls -l1 /dev/mouse* 2>/dev/null | awk '/ttyS/{print $NF ; exit 0}')"
     [ -n "$SERIALMOUSEDEV" ] || SERIALMOUSEDEV="/dev/ttyS0"
     ;;
  esac
 fi
 USBMOUSE='	InputDevice    "USB Mouse" "SendCoreEvents"'
 MOUSESECTION='Section "InputDevice"
	Identifier  "PS/2 Mouse"
	Driver      "mouse"
	Option      "Protocol" "auto-dev"
	Option      "Device" "/dev/psaux"
	Option      "Emulate3Buttons" "true"
	Option      "Emulate3Timeout" "70"
        Option      "ZAxisMapping" "4 5"
        Option      "SendCoreEvents"  "true"
EndSection

Section "InputDevice"
        Identifier      "USB Mouse"
        Driver          "mouse"
        Option          "Device"                "/dev/input/mice"
        Option          "Protocol"              "auto"
        Option          "ZAxisMapping"          "4 5"
        Option          "Buttons"               "5"
        Option          "SendCoreEvents"        "true"
EndSection

Section "InputDevice"
	Driver		"synaptics"
	Identifier	"AlpsPad"
	Option		"Device"		"/dev/psaux"
	Option		"Protocol"		"auto-dev"
#	Option		"LeftEdge"		"130"
#	Option		"RightEdge"		"840"
#	Option		"TopEdge"		"130"
#	Option		"BottomEdge"		"640"
#	Option		"FingerLow"		"7"
#	Option		"FingerHigh"		"8"
#	Option		"MaxTapTime"		"180"
#	Option		"MaxTapMove"		"110"
#	Option		"EmulateMidButtonTime"	"75"
#	Option		"VertScrollDelta"	"20"
#	Option		"HorizScrollDelta"	"20"
#	Option		"MinSpeed"		"0.60"
#	Option		"MaxSpeed"		"1.10"
#	Option		"AccelFactor"		"0.030"
#	Option		"EdgeMotionMinSpeed"	"200"
#	Option		"EdgeMotionMaxSpeed"	"200"
#	Option		"UpDownScrolling"	"1"
#	Option		"CircularScrolling"	"1"
#	Option		"CircScrollDelta"	"0.1"
#	Option		"CircScrollTrigger"	"2"
	Option		"SHMConfig"		"on"
	Option		"Emulate3Buttons"	"on"
        Option          "SendCoreEvents"        "true"
EndSection

Section "InputDevice"
	Driver		"synaptics"
	Identifier	"SynPad"
	Option		"Device"		"/dev/psaux"
	Option		"Protocol"		"auto-dev"
#	Option		"LeftEdge"		"1700"
#	Option		"RightEdge"		"5300"
#	Option		"TopEdge"		"1700"
#	Option		"BottomEdge"		"4200"
#	Option		"FingerLow"		"25"
#	Option		"FingerHigh"		"30"
#	Option		"MaxTapTime"		"180"
#	Option		"MaxTapMove"		"220"
#	Option		"VertScrollDelta"	"100"
#	Option		"MinSpeed"		"0.09"
#	Option		"MaxSpeed"		"0.18"
#	Option		"AccelFactor"		"0.0015"
	Option		"SHMConfig"		"on"
        Option          "SendCoreEvents"        "true"
EndSection

Section "InputDevice"
	Identifier  "Serial Mouse"
	Driver      "mouse"
	Option      "Protocol" "Microsoft"
	Option      "Device" "'$SERIALMOUSEDEV'"
	Option      "Emulate3Buttons" "true"
	Option      "Emulate3Timeout" "70"
        Option      "SendCoreEvents"        "true"
EndSection'
fi

KEYBOARD='# Keyboard auto-probed'
if [ -z "$AUTOKEYBOARD" ]; then
 KEYBOARD='	InputDevice    "Keyboard0" "CoreKeyboard"'
 KEYBOARDSECTION='Section "InputDevice"
	Identifier  "Keyboard0"
	Driver      "kbd"
        Option      "CoreKeyboard"
	Option "XkbRules" "xorg"
	Option "XkbModel" "pc105"
	Option "XkbLayout" "'${XKEYBOARD}'"
	'${DEADKEYS}'
EndSection'
fi

if [ -n "$COMPOSITE" -a "$XMODULE" != "vesa" -a "$XMODULE" != "fbdev" -a "$XMODULE" != "nv" ] && \
   ! checkbootparam "no3d" && ! checkbootparam "nocomposite"; then
	COMPOSITE='Option "Composite" "Enable"'
else
	COMPOSITE='Option "Composite" "Disable"'
fi

# VMWare special handling
VMWARE=""
if [ "$XMODULE" = "vmware" ]; then
VMWARE='BusID "PCI:0:15:0"'
DEPTH=''
fi

# Do NOT use a default colordepth setting if we are using the "fbdev" module
if [ "$XMODULE" = "fbdev" ]; then
DEPTH=''
fi

# These drivers need the sw_cursor option
SWCURSOR=""
case "$XMODULE" in trident) SWCURSOR='Option "sw_cursor"';; esac

# We must use NoPM, because some machines freeze if Power management is being activated.

NOPM=""
DPMS=""
checkbootparam noapm && NOPM='Option	"NoPM"	"true"' || DPMS='Option	"DPMS"	"true"'

# This is xorg.conf.in
cat >/etc/X11/xorg.conf <<EOT
# /etc/X11/xorg.conf
# ${KNOPPIXSIG} # Delete this line if you don't want KNOPPIX to overwrite your /etc/X11/xorg.conf

Section "ServerLayout"
	Identifier     "XFree86 Configured"
	Screen      0  "Screen0" 0 0
	${ALLOWEMPTY}
	# Since evdev, manual keyboard/mice entries are mostly ignored:
	${KEYBOARD}
	${PSMOUSE}
	${TOUCHPAD}
	${USBMOUSE}
	${SERIALMOUSE}
### AIGLX for compiz 3D-Support with DRI & Composite
### This option doesn't hurt even if it's not supported by the individual card
        Option         "AIGLX"     "true"
	${NORANDR}
EndSection

Section "ServerFlags"
	Option "AllowMouseOpenFail"  "true"
	${DPMS}
	${NOPM}
EndSection

Section "Files"
	ModulePath   "/usr/lib/xorg/modules"
	FontPath     "/usr/share/fonts/X11/misc:unscaled"
	FontPath     "/usr/share/fonts/X11/75dpi:unscaled"
	FontPath     "/usr/share/fonts/X11/100dpi:unscaled"
	FontPath     "/usr/share/fonts/X11/Type1"
	FontPath     "/usr/share/fonts/X11/Speedo"
	FontPath     "/usr/share/fonts/X11/PEX"
# Additional fonts: Locale, Gimp, TTF...
	FontPath     "/usr/share/fonts/X11/cyrillic"
#	FontPath     "/usr/share/fonts/X11/latin2/75dpi"
#	FontPath     "/usr/share/fonts/X11/latin2/100dpi"
# True type and type1 fonts are also handled via xftlib, see /etc/X11/XftConfig!
	FontPath     "/var/lib/defoma/x-ttcidfont-conf.d/dirs/TrueType"
	FontPath     "/usr/share/fonts/truetype"
	FontPath     "/usr/share/fonts/latex-ttf-fonts"
EndSection

Section "Module"
# Comments: see http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=346408
	Load  "dbe" # Double Buffering Extension, very important.
	Load  "dri" # This shouldn't be available choice if user has selected driver vga, vesa or nv.
	Load  "glx" # GLX Extension.
	Load  "freetype" # Freetype fonts.
	Load  "type1"  # Type 1 fonts
	Load  "record" # Developer extension, usually not needed
	Load  "extmod" # This is okay, but if you look into "man xorg.conf" you'll find option NOT to include DGA extension with extmod, and for a good reason.. DGA causes instability as it accesses videoram without consulting X about it.
	SubSection      "extmod"
		Option          "omit xfree86-dga"
	EndSubSection
#	Load  "speedo" # Speedo fonts, this module doesn't exist in Xorg 7.0.17
# The following are deprecated/unstable/unneeded in Xorg 7.0
#       Load  "ddc"  # ddc probing of monitor, this should be never present, as it gets automatically loaded.
#	Load  "GLcore" # This should be never present, as it gets automatically loaded.
#       Load  "bitmap" # Should be never present, as it gets automatically loaded. This is a font module, and loading it in xorg.conf makes X try to load it twice.
EndSection

Section "Extensions"
	# compiz needs Composite, but it can cause bad (end even softreset-resistant)
	# effects in some graphics cards, especially nv.
	${COMPOSITE}
EndSection

${KEYBOARDSECTION}
${MOUSESECTION}

# Monitor section auto-generated by KNOPPIX mkxorgconfig
${MONITOR}

Section "Device"
	### Available Driver options are:-
# sw_cursor is needed for some ati and radeon cards
        #Option     "sw_cursor"
        #Option     "hw_cursor"
        #Option     "NoAccel"
        #Option     "ShowCache"
        #Option     "ShadowFB"
        #Option     "UseFBDev"
        #Option     "Rotate"
	Identifier  "Card0"
	${DRIVER}
	VendorName  "All"
	BoardName   "All"
#	BusID       "PCI:1:0:0"

# compiz, beryl 3D-Support with DRI & Composite
	Option "XAANoOffscreenPixmaps"
	Option "AllowGLXWithComposite" "true"
#       Some Radeon cards don't handle this well.
#	Option "EnablePageFlip" "true"
	Option "TripleBuffer" "true"

# Tweaks for the xorg 7.4 (otherwise broken) "intel" driver
#	Option "Tiling" "no"
	Option "Legacy3D" "false"
	${CHIPSET_TWEAKS}

# These two lines are (presumably) needed to prevent fonts from being scrambled
        Option  "XaaNoScanlineImageWriteRect" "true"
        Option  "XaaNoScanlineCPUToScreenColorExpandFill" "true"
EndSection

Section "Screen"
	Identifier "Screen0"
	Device     "Card0"
	Monitor    "Monitor0"
	${DEPTH}
	Option "AddARGBGLXVisuals" "true"
	Option "DisableGLXRootClipping" "true"
	SubSection "Display"
		Depth     1
		${MODES}
	EndSubSection
	SubSection "Display"
		Depth     4
		${MODES}
	EndSubSection
	SubSection "Display"
		Depth     8
		${MODES}
	EndSubSection
	SubSection "Display"
		Depth     15
		${MODES}
	EndSubSection
	SubSection "Display"
		Depth     16
		${MODES}
	EndSubSection
	SubSection "Display"
		Depth     24
		${MODES}
	EndSubSection
	SubSection "Display"
		Depth     32
		${MODES}
	EndSubSection
EndSection

Section "DRI"
	Mode 0666
EndSection
EOT

# Print info about selected X-Server
# [ -n "$XDESC" ] || XDESC="(yet) unknown card"
# echo -n " ${GREEN}Video is"
# [ -n "$XDESC" ] && echo -n " ${YELLOW}$XDESC${GREEN},"
# echo -n " using ${YELLOW}${XSERVER:-generic VESA}"
# [ -n "$XMODULE" ] && echo -n "(${MAGENTA}$XMODULE${YELLOW})"
# echo " Server${NORMAL}"
# echo -n " ${GREEN}Monitor is ${YELLOW}${MODEL:-Generic Monitor}${NORMAL}"
# [ -n "$HREFRESH" -a -n "$VREFRESH" ] && echo "${GREEN}, ${GREEN}H:${YELLOW}${HREFRESH}kHz${GREEN}, V:${YELLOW}${VREFRESH}Hz${NORMAL}" || echo ""
# [ -n "$XVREFRESH" ] && echo " ${GREEN}Trying specified vrefresh rate of ${YELLOW}${XVREFRESH}Hz.${NORMAL}"
# [ -n "$MODES" ] && echo " ${GREEN}Using Modes ${YELLOW}${MODES##Modes }${NORMAL}"
