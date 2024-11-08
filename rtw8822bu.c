// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* Copyright(c) 2018-2024  Realtek Corporation
 */

#include <linux/module.h>
#include <linux/usb.h>
#include "main.h"
#include "rtw8822b.h"
#include "usb.h"

static const struct usb_device_id rtw_8822bu_id_table[] = {
	{ USB_DEVICE_AND_INTERFACE_INFO(RTW_USB_VENDOR_ID_REALTEK, 0xB812, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8822b_hw_spec) }, /* Default ID for USB Single-function, WiFi only */
	{ USB_DEVICE_AND_INTERFACE_INFO(RTW_USB_VENDOR_ID_REALTEK, 0xB81A, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8822b_hw_spec) }, /* Default ID */
	{ USB_DEVICE_AND_INTERFACE_INFO(RTW_USB_VENDOR_ID_REALTEK, 0xB82C, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8822b_hw_spec) }, /* Default ID for USB multi-function */
	{ USB_DEVICE_AND_INTERFACE_INFO(RTW_USB_VENDOR_ID_REALTEK, 0x2102, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8822b_hw_spec) }, /* CCNC */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x04CA, 0x8602, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8822b_hw_spec) }, /* LiteOn */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x056E, 0x4011, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8822b_hw_spec) }, /* Elecom */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x0846, 0x9055, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8822b_hw_spec) }, /* Netgear A6150 */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x0B05, 0x1841, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8822b_hw_spec) }, /* ASUS AC1300 USB-AC55 B1 */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x0B05, 0x184C, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8822b_hw_spec) }, /* ASUS U2 */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x0B05, 0x1870, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8822b_hw_spec) }, /* ASUS */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x0B05, 0x1874, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8822b_hw_spec) }, /* ASUS */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x0B05, 0x19AA, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8822b_hw_spec) }, /* ASUS - USB-AC58 rev A1 */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x0E66, 0x0025, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8822b_hw_spec) }, /* Hawking HW12ACU */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x13B1, 0x0043, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8822b_hw_spec) }, /* Linksys WUSB6400M */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x13B1, 0x0045, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8822b_hw_spec) }, /* Linksys WUSB3600 v2 */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x2001, 0x331C, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8822b_hw_spec) }, /* Dlink - DWA-182 - D1 */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x2001, 0x331E, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8822b_hw_spec) }, /* Dlink - DWA-181 */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x2001, 0x331F, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8822b_hw_spec) }, /* Dlink - DWA-183 - D */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x2001, 0x3322, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8822b_hw_spec) }, /* Dlink - DWA-T185 rev. A1 */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x20F4, 0x805A, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8822b_hw_spec) }, /* TRENDnet TEW-805UBH */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x20F4, 0x808A, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8822b_hw_spec) }, /* TRENDnet TEW-808UBM */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x2357, 0x0115, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8822b_hw_spec) }, /* TP-Link Archer T4U V3 */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x2357, 0x0116, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8822b_hw_spec) }, /* TP-LINK */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x2357, 0x0117, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8822b_hw_spec) }, /* TP-LINK */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x2357, 0x012D, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8822b_hw_spec) }, /* TP-Link Archer T3U v1 */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x2357, 0x012E, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8822b_hw_spec) }, /* TP-LINK */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x2357, 0x0138, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8822b_hw_spec) }, /* TP-Link Archer T3U Plus v1 */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x2C4E, 0x0107, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8822b_hw_spec) }, /* Mercusys MA30H */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x2C4E, 0x010a, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8822b_hw_spec) }, /* Mercusys MA30N */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x7392, 0xB822, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8822b_hw_spec) }, /* Edimax EW-7822ULC */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x7392, 0xC822, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8822b_hw_spec) }, /* Edimax EW-7822UTC */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x7392, 0xD822, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8822b_hw_spec) }, /* Edimax */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x7392, 0xE822, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8822b_hw_spec) }, /* Edimax */
	{ USB_DEVICE_AND_INTERFACE_INFO(0x7392, 0xF822, 0xff, 0xff, 0xff),
	  .driver_info = (kernel_ulong_t)&(rtw8822b_hw_spec) }, /* Edimax EW-7822UAD */
	{},
};
MODULE_DEVICE_TABLE(usb, rtw_8822bu_id_table);

static int rtw8822bu_probe(struct usb_interface *intf,
			   const struct usb_device_id *id)
{
	return rtw_usb_probe(intf, id);
}

static struct usb_driver rtw_8822bu_driver = {
	.name = "rtw_8822bu",
	.id_table = rtw_8822bu_id_table,
	.probe = rtw8822bu_probe,
	.disconnect = rtw_usb_disconnect,
};
module_usb_driver(rtw_8822bu_driver);

MODULE_AUTHOR("Realtek Corporation");
MODULE_DESCRIPTION("Realtek 802.11ac wireless 8822bu driver");
MODULE_LICENSE("Dual BSD/GPL");
