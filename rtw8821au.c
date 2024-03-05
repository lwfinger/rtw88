// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* Copyright(c) 2018-2019  Realtek Corporation
 */

#include <linux/module.h>
#include <linux/usb.h>
#include "main.h"
#include "rtw8821a.h"
#include "usb.h"

static const struct usb_device_id rtw_8821au_id_table[] = {
        /*=== Realtek demoboard ===*/
	{ USB_DEVICE_AND_INTERFACE_INFO(RTW_USB_VENDOR_ID_REALTEK, 0x0811, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8821a_hw_spec) }, /* 8821AU */
	{ USB_DEVICE_AND_INTERFACE_INFO(RTW_USB_VENDOR_ID_REALTEK, 0x0820, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8821a_hw_spec) }, /* 8821AU */
	{ USB_DEVICE_AND_INTERFACE_INFO(RTW_USB_VENDOR_ID_REALTEK, 0x0821, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8821a_hw_spec) }, /* 8821AU */
	{ USB_DEVICE_AND_INTERFACE_INFO(RTW_USB_VENDOR_ID_REALTEK, 0x0822, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8821a_hw_spec) }, /* 8821AU */
	{ USB_DEVICE_AND_INTERFACE_INFO(RTW_USB_VENDOR_ID_REALTEK, 0x0823, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8821a_hw_spec) }, /* 8821AU */
	{ USB_DEVICE_AND_INTERFACE_INFO(RTW_USB_VENDOR_ID_REALTEK, 0xa811, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8821a_hw_spec) }, /* 8821AU */
        /*=== Customer ID ===*/
	{ USB_DEVICE_AND_INTERFACE_INFO(0x0411, 0x0242, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8821a_hw_spec) }, /* Elecom - WDC-433DU2H */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x0411, 0x0298, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8821a_hw_spec) }, /* Buffalo - WI-U2-433DHP */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x0488, 0x0953, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8821a_hw_spec) }, /* I-O DATA - Edimax */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x056e, 0x4007, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8821a_hw_spec) }, /* Elecom WDC-433DU2HBK */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x056e, 0x400e, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8821a_hw_spec) }, /* Elecom */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x056e, 0x400f, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8821a_hw_spec) }, /* Elecom */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x0846, 0x9052, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8821a_hw_spec) }, /* Netgear - A6100 */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x0e66, 0x0023, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8821a_hw_spec) }, /* Hawking - Edimax */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x2001, 0x3314, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8821a_hw_spec) }, /* D-Link - Cameo */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x2001, 0x3318, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8821a_hw_spec) }, /* D-Link - Cameo */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x2019, 0xab32, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8821a_hw_spec) }, /* Planex - GW-450S */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x20f4, 0x804b, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8821a_hw_spec) }, /* TrendNet */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x2357, 0x011e, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8821a_hw_spec) }, /* TP-Link T2U Nano */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x2357, 0x011f, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8821a_hw_spec) }, /* TP-Link Archer AC600 T2U Nano */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x2357, 0x0120, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8821a_hw_spec) }, /* TP-Link T2U Plus and Archer T600U Plus */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x3823, 0x6249, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8821a_hw_spec) }, /* Obihai - OBiWiFi */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x7392, 0xa811, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8821a_hw_spec) }, /* Edimax */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x7392, 0xa812, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8821a_hw_spec) }, /* Edimax EW-7811UTC */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x7392, 0xa813, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8821a_hw_spec) }, /* Edimax EW-7811UAC */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x7392, 0xb611, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8821a_hw_spec) }, /* Edimax EW-7811UCB */
	{},
};
MODULE_DEVICE_TABLE(usb, rtw_8821au_id_table);

static int rtw_8821au_probe(struct usb_interface *intf,
			    const struct usb_device_id *id)
{
	return rtw_usb_probe(intf, id);
}

static struct usb_driver rtw_8821au_driver = {
	.name = "rtw_8821au",
	.id_table = rtw_8821au_id_table,
	.probe = rtw_8821au_probe,
	.disconnect = rtw_usb_disconnect,
};
module_usb_driver(rtw_8821au_driver);

MODULE_AUTHOR("Larry Finger <Larry.Finger@lwfinger.net>");

MODULE_DESCRIPTION("Realtek 802.11ac wireless 8821au driver");
MODULE_LICENSE("Dual BSD/GPL");
