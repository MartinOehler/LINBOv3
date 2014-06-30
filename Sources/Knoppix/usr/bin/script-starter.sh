#!/bin/bash
export PATH=".:/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin"

echo "Content-Type: text/plain"
echo ""

xterm -T LINBO -e bash -c "echo \"$@\"; \"$@\"; sleep 5"
