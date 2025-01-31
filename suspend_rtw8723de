#!/bin/sh
#
# Script to unload/reload driver for rtw8723de on suspend/resume
#

if [ "${1}" == "pre" ]; then
# shutting down - isolate name of driver, if any
  MODNAME=$(lsmod | grep 8723de | awk '{split($0,a," "); print a[1]}' | grep 8723de)
  if [ ! -z "$MODNAME" -a "$MODNAME" != " " ]; then
# here if rtw88_8723de or rtw_8873 is currently loaded - actual name in MODNAME
# Save name in file
    echo $MODNAME > /root/module_name
# unload the module
    modprobe -rv $MODNAME
  fi
elif [ "${1}" == "post" ]; then
# Resuming - check if file saving name exists
  if [ -f /root/module_name ]; then
# it does - sleep a while, recover MODNAME, load the driver, and delete the file that saved MODNAME
    sleep 15
    MODNAME=$(cat /root/module_name) 
    modprobe -v $MODNAME
    rm /root/module_name
  fi
fi

