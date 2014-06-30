#!/bin/bash

attach(){
  set `VBoxManage list usbhost | awk -F: '/'"$ID_SERIAL_SHORT"'/ {getline; print $0}' | cut -c 21-`
  address="$1"
  if [ -n "$address" ]; then
    VM_NAME="$(cat /home/knoppix/.vmname)"
    su - knoppix -c "VBoxManage controlvm <Name der VB> usbattach $address"        
  fi                                                                   
}                                                                           
                                                                             
case "$DEVNAME" in                                                             
  /dev/sd[a-z]) attach;;
esac
