#!/bin/bash
# /etc/acpi/powerbtn.sh
# Initiates a shutdown when the power button has been
# pressed.

# Skip if we just in the middle of resuming.
test -f /var/lock/acpisleep && exit 0
poweroff &
sleep 30
poweroff -f
exit 0
