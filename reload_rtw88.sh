#! /bin/sh
#
# Script to unload/reload the rtw88 driver on suspend/resume
#

# A full list of the rtw88 drivers
FULLMOD="rtw_8723cs rtw_8723de rtw_8723ds rtw_8723du \
	 rtw_8812au rtw_8814ae rtw_8814au rtw_8821au rtw_8821ce rtw_8821cs rtw_8821cu \
	 rtw_8822be rtw_8822bs rtw_8822bu rtw_8822ce rtw_8822cs rtw_8822cu \
	 rtw_8703b rtw_8723d rtw_8821a rtw_8812a rtw_8814a rtw_8821c rtw_8822b rtw_8822c \
	 rtw_8723x rtw_88xxa rtw_pci rtw_sdio rtw_usb rtw_core"

# Before sleep/hibernation, save a list of the loaded rtw88 drivers to /RTW88_LOADEDMOD
# and unload all the rtw88 drivers.
if [ "${1}" == "pre" ]; then
	LOADEDMOD=$(lsmod | grep "^rtw_.....[a-z]" | cut -d ' ' -f 1)
	if [ ! -z "$LOADEDMOD" -a "$LOADEDMOD" != " " ]; then
		echo $LOADEDMOD > /RTW88_LOADEDMOD
	fi
	
	for mod in $FULLMOD
	do
		rmmod -s $mod || true
	done
 
# When Resuming, load the modules saved in /RTW88_LOADEDMOD and then remove /RTW88_LOADEDMOD
# Not sure why Larry makes the system sleep for 15s before loading the modules.
elif [ "${1}" == "post" ]; then
	if [ -r /RTW88_LOADEDMOD ]; then
		sleep 15
		LOADEDMOD=$(cat /RTW88_LOADEDMOD)
		for mod in $LOADEDMOD
		do
			modprobe $mod
		done
		rm -f /RTW88_LOADEDMOD
	fi

else
	echo "Usage: ./reload_rtw88.sh [pre|post]"
fi
