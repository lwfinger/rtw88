// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* Copyright(c) 2018-2019  Realtek Corporation
 */

#include <linux/usb.h>
#include "main.h"
#include "coex.h"
#include "fw.h"
#include "tx.h"
#include "rx.h"
#include "phy.h"
#include "rtw8821a.h"
#include "rtw8821a_table.h"
#include "mac.h"
#include "reg.h"
#include "sec.h"
#include "debug.h"
#include "bf.h"
#include "regd.h"
#include "efuse.h"
#include "usb.h"

static struct rtw_pwr_seq_cmd trans_carddis_to_cardemu_8821a[] = {
	{0x0005,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(3) | BIT(7), 0},
	{0x0086,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_SDIO_MSK,
	 RTW_PWR_ADDR_SDIO,
	 RTW_PWR_CMD_WRITE, BIT(0), 0},
	{0x0086,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_SDIO_MSK,
	 RTW_PWR_ADDR_SDIO,
	 RTW_PWR_CMD_POLLING, BIT(1), BIT(1)},
	{0x004A,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_USB_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(0), 0},
	{0x0005,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(3) | BIT(4), 0},
	{0x0023,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_SDIO_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(4), 0},
	{0x0301,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_PCI_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0xFF, 0},
	{0xFFFF,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 0,
	 RTW_PWR_CMD_END, 0, 0},
};

static struct rtw_pwr_seq_cmd trans_cardemu_to_act_8821a[] = {
	{0x0020,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_USB_MSK | RTW_PWR_INTF_SDIO_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(0), BIT(0)},
	{0x0067,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_USB_MSK | RTW_PWR_INTF_SDIO_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(4), 0},
	{0x0001,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_USB_MSK | RTW_PWR_INTF_SDIO_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_DELAY, 1, RTW_PWR_DELAY_MS},
	{0x0000,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_USB_MSK | RTW_PWR_INTF_SDIO_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(5), 0},
	{0x0005,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(4) | BIT(3) | BIT(2), 0},
	{0x0075,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_PCI_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(0) , BIT(0)},
	{0x0006,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_POLLING, BIT(1), BIT(1)},
	{0x0075,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_PCI_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(0) , 0},
	{0x0006,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(0), BIT(0)},
	{0x0005,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(7), 0},
	{0x0005,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(4) | BIT(3), 0},
	{0x0005,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(0), BIT(0)},
	{0x0005,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_POLLING, BIT(0), 0},
	{0x004F,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(0), BIT(0)},
	{0x0067,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(5) | BIT(4), BIT(5) | BIT(4)},
	{0x0025,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(6), 0},
	{0x0049,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(1), BIT(1)},
	{0x0063,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(1), BIT(1)},
	{0x0062,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(1), 0},
	{0x0058,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(0), BIT(0)},
	{0x005A,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(1), BIT(1)},
	{0x002E,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0xFF, 0x82},
	{0x0010,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(6), BIT(6)},
	{0xFFFF,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 0,
	 RTW_PWR_CMD_END, 0, 0},
};

static const struct rtw_pwr_seq_cmd trans_act_to_lps_8821a[] = {
	{0x0301,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_PCI_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0xFF, 0xFF},
	{0x0522,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0xFF, 0xFF},
	{0x05F8,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_POLLING, 0xFF, 0},
	{0x05F9,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_POLLING, 0xFF, 0},
	{0x05FA,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_POLLING, 0xFF, 0},
	{0x05FB,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_POLLING, 0xFF, 0},
	{0x0002,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(0), 0},
	{0x0002,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_DELAY, 0, RTW_PWR_DELAY_US},
	{0x0002,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(1), 0},
	{0x0100,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0xFF, 0x03},
	{0x0101,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(1), 0},
	{0x0093,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_SDIO_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0xFF, 0x00},
	{0x0553,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(5), BIT(5)},
	{0xFFFF,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 0,
	 RTW_PWR_CMD_END, 0, 0},
};

static struct rtw_pwr_seq_cmd trans_act_to_cardemu_8821a[] = {
	{0x001F,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0xFF, 0},
	{0x004F,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(0), 0},
	{0x0049,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(1), 0},
	{0x0006,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_USB_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(0), BIT(0)},
	{0x0005,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(1), BIT(1)},
	{0x0005,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_POLLING, BIT(1), 0},
	{0x0000,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_USB_MSK | RTW_PWR_INTF_SDIO_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(5), BIT(5)},
	{0x0020,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_USB_MSK | RTW_PWR_INTF_SDIO_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(0), 0},
	{0xFFFF,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 0,
	 RTW_PWR_CMD_END, 0, 0},
};

static struct rtw_pwr_seq_cmd trans_cardemu_to_carddis_8821a[] = {
	{0x0007,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_SDIO_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0xFF, 0x20},
	{0x0005,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_USB_MSK | RTW_PWR_INTF_SDIO_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(3) | BIT(4), BIT(3)},
	{0x0005,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_PCI_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(2), BIT(2)},
        {0x004A,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_USB_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(0), 1},
	{0x0023,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_SDIO_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(4), BIT(4)},
	{0x0086,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_SDIO_MSK,
	 RTW_PWR_ADDR_SDIO,
	 RTW_PWR_CMD_WRITE, BIT(0), BIT(0)},
	{0x0086,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_SDIO_MSK,
	 RTW_PWR_ADDR_SDIO,
	 RTW_PWR_CMD_POLLING, BIT(1), 0},
	{0xFFFF,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 0,
	 RTW_PWR_CMD_END, 0, 0},
};

static const struct rtw_pwr_seq_cmd *card_enable_flow_8821a[] = {
	trans_carddis_to_cardemu_8821a,
	trans_cardemu_to_act_8821a,
	NULL
};

static const struct rtw_pwr_seq_cmd *enter_lps_flow_8821a[] = {
	trans_act_to_lps_8821a,
	NULL
};

static const struct rtw_pwr_seq_cmd *card_disable_flow_8821a[] = {
	trans_act_to_cardemu_8821a,
	trans_cardemu_to_carddis_8821a,
	NULL
};

static struct rtw_pwr_seq_cmd trans_carddis_to_cardemu_8812a[] = {
	{0x0012,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(0), BIT(0)},
	{0x0014,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0x80, 0},
	{0x0015,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0x01, 0},
	{0x0023,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0x10, 0},
	{0x0046,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0xFF, 0x00},
	{0x0043,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0xFF, 0x00},
	{0x0005,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_PCI_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(2), 0},
	{0x0005,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(3), 0},
	{0x0003,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(2), BIT(2)},
	{0x0301,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_PCI_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0xFF, 0},
	{0x0024,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_USB_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(1), BIT(1)},
	{0x0028,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_USB_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(3), BIT(3)},
	{0xFFFF,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 0,
	 RTW_PWR_CMD_END, 0, 0},
};

static struct rtw_pwr_seq_cmd trans_cardemu_to_act_8812a[] = {
	{0x0005,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(2), 0},
	{0x0006,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_POLLING, BIT(1), BIT(1)},
	{0x0005,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_PCI_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(7), 0},
	{0x0005,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(3), 0},
	{0x0005,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(0), BIT(0)},
	{0x0005,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_POLLING, BIT(0), 0},
	{0x0024,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_USB_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(1), 0},
	{0x0028,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_USB_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(3), 0},
	{0xFFFF,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 0,
	 RTW_PWR_CMD_END, 0, 0},
};

static struct rtw_pwr_seq_cmd trans_act_to_lps_8812a[] = {
	{0x0301,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_PCI_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0xFF, 0xFF},
	{0x0522,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0xFF, 0x7F},
	{0x05F8,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_POLLING, 0xFF, 0},
	{0x05F9,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_POLLING, 0xFF, 0},
	{0x05FA,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_POLLING, 0xFF, 0},
	{0x05FB,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_POLLING, 0xFF, 0},
	{0x0c00,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0xFF, 0x04},
	{0x0e00,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0xFF, 0x04},
	{0x0002,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(0), 0},
	{0x0002,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_DELAY, 0, RTW_PWR_DELAY_US},
	{0x0002,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_USB_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(1), 0},
	{0x0100,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0xFF, 0x03},
	{0x0101,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(1), 0},
	{0x0553,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(5), BIT(5)},
	{0xFFFF,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 0,
	 RTW_PWR_CMD_END, 0, 0},
};

static struct rtw_pwr_seq_cmd trans_act_to_cardemu_8812a[] = {
	{0x0c00,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0xFF, 0x04},
	{0x0e00,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0xFF, 0x04},
	{0x0002,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(0), 0},
	{0x0002,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_DELAY, 0, RTW_PWR_DELAY_US},
	{0x0002,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_PCI_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(1), 0},
	{0x0007,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0xFF, 0x2A},
	{0x0008,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_USB_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0x02, 0},
	{0x0005,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(1), BIT(1)},
	{0x0005,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_POLLING, BIT(1), 0},
	{0xFFFF,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 0,
	 RTW_PWR_CMD_END, 0, 0},
};

static struct rtw_pwr_seq_cmd trans_cardemu_to_carddis_8812a[] = {
	{0x0003,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(2), 0},
	{0x0080,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0xFF, 0x05},
	{0x0042,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_USB_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0xF0, 0xcc},
	{0x0042,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_PCI_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0xF0, 0xEC},
	{0x0043,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0xFF, 0x07},
	{0x0045,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0xFF, 0x00},
	{0x0046,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0xFF, 0xff},
	{0x0047,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0xFF, 0},
	{0x0014,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0x80, BIT(7)},
	{0x0015,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0x01, BIT(0)},
	{0x0012,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0x01, 0},
	{0x0023,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0x10, BIT(4)},
	{0x0008,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_USB_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0x02, 0},
	{0x0007,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_USB_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0xFF, 0x20},
	{0x001f,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_USB_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(1), 0},
	{0x0076,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_USB_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(1), 0},
	{0x0005,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(3), BIT(3)},
	{0xFFFF,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 0,
	 RTW_PWR_CMD_END, 0, 0},
};

static const struct rtw_pwr_seq_cmd *card_enable_flow_8812a[] = {
	trans_carddis_to_cardemu_8812a,
	trans_cardemu_to_act_8812a,
	NULL
};

static const struct rtw_pwr_seq_cmd *enter_lps_flow_8812a[] = {
	trans_act_to_lps_8812a,
	NULL
};

static const struct rtw_pwr_seq_cmd *card_disable_flow_8812a[] = {
	trans_act_to_cardemu_8812a,
	trans_cardemu_to_carddis_8812a,
	NULL
};

static void rtw8821a_efuse_grant(struct rtw_dev *rtwdev, bool on)
{
	if (on) {
		rtw_write8(rtwdev, REG_EFUSE_ACCESS, EFUSE_ACCESS_ON);

		rtw_write16_set(rtwdev, REG_SYS_FUNC_EN, BIT_FEN_ELDR);
		rtw_write16_set(rtwdev, REG_SYS_CLKR, BIT_LOADER_CLK_EN | BIT_ANA8M);
	} else {
		rtw_write8(rtwdev, REG_EFUSE_ACCESS, EFUSE_ACCESS_OFF);
	}
}

static void rtw8812a_read_amplifier_type(struct rtw_dev *rtwdev)
{
	struct rtw_efuse *efuse = &rtwdev->efuse;

	efuse->ext_pa_2g = (efuse->pa_type_2g & BIT(5)) &&
			   (efuse->pa_type_2g & BIT(4));
	efuse->ext_lna_2g = (efuse->lna_type_2g & BIT(7)) &&
			    (efuse->lna_type_2g & BIT(3));

	efuse->ext_pa_5g = (efuse->pa_type_5g & BIT(1)) &&
			   (efuse->pa_type_5g & BIT(0));
	efuse->ext_lna_5g = (efuse->lna_type_5g & BIT(7)) &&
			    (efuse->lna_type_5g & BIT(3));

	/* For rtw_phy_cond2: */
	if (efuse->ext_pa_2g) {
		u8 ext_type_pa_2g_a = u8_get_bits(efuse->lna_type_2g, BIT(2));
		u8 ext_type_pa_2g_b = u8_get_bits(efuse->lna_type_2g, BIT(6));

		efuse->gpa_type = (ext_type_pa_2g_b << 2) | ext_type_pa_2g_a;
	}

	if (efuse->ext_pa_5g) {
		u8 ext_type_pa_5g_a = u8_get_bits(efuse->lna_type_5g, BIT(2));
		u8 ext_type_pa_5g_b = u8_get_bits(efuse->lna_type_5g, BIT(6));

		efuse->apa_type = (ext_type_pa_5g_b << 2) | ext_type_pa_5g_a;
	}

	if (efuse->ext_lna_2g) {
		u8 ext_type_lna_2g_a = u8_get_bits(efuse->lna_type_2g,
						   BIT(1) | BIT(0));
		u8 ext_type_lna_2g_b = u8_get_bits(efuse->lna_type_2g,
						   BIT(5) | BIT(4));

		efuse->glna_type = (ext_type_lna_2g_b << 2) | ext_type_lna_2g_a;
	}

	if (efuse->ext_lna_5g) {
		u8 ext_type_lna_5g_a = u8_get_bits(efuse->lna_type_5g,
						   BIT(1) | BIT(0));
		u8 ext_type_lna_5g_b = u8_get_bits(efuse->lna_type_5g,
						   BIT(5) | BIT(4));

		efuse->alna_type = (ext_type_lna_5g_b << 2) | ext_type_lna_5g_a;
	}
}

static void rtw8812a_read_rfe_type(struct rtw_dev *rtwdev, struct rtw8821a_efuse *map)
{
	struct rtw_efuse *efuse = &rtwdev->efuse;

	if (map->rfe_option & BIT(7)) {
		if (efuse->ext_lna_5g) {
			if (efuse->ext_pa_5g) {
				if (efuse->ext_lna_2g && efuse->ext_pa_2g)
					efuse->rfe_option = 3;
				else
					efuse->rfe_option = 0;
			} else {
				efuse->rfe_option = 2;
			}
		} else {
			efuse->rfe_option = 4;
		}
	} else {
		efuse->rfe_option = map->rfe_option & 0x3f;

		/* Due to other customer already use incorrect EFUSE map for
		 * their product. We need to add workaround to prevent to
		 * modify spec and notify all customer to revise the IC 0xca
		 * content.
		 */
		if (efuse->rfe_option == 4 &&
		    (efuse->ext_pa_5g || efuse->ext_pa_2g ||
		     efuse->ext_lna_5g || efuse->ext_lna_2g)) {
			if (rtwdev->hci.type == RTW_HCI_TYPE_USB)
				efuse->rfe_option = 0;
			else if (rtwdev->hci.type == RTW_HCI_TYPE_PCIE)
				efuse->rfe_option = 2;
		}
	}
}

static void rtw8812a_read_usb_type(struct rtw_dev *rtwdev)
{
	struct rtw_efuse *efuse = &rtwdev->efuse;
	struct rtw_hal *hal = &rtwdev->hal;
	u8 antenna = 0;
	u8 wmode = 0;
	u8 val8, i;

	efuse->hw_cap.bw = BIT(RTW_CHANNEL_WIDTH_20) |
			   BIT(RTW_CHANNEL_WIDTH_40) |
			   BIT(RTW_CHANNEL_WIDTH_80);
	efuse->hw_cap.ptcl = EFUSE_HW_CAP_PTCL_VHT;

	if (rtwdev->chip->id == RTW_CHIP_TYPE_8821A)
		efuse->hw_cap.nss = 1;
	else
		efuse->hw_cap.nss = 2;

	if (rtwdev->chip->id == RTW_CHIP_TYPE_8821A)
		return;

	for (i = 0; i < 2; i++) {
		rtw_read8_physical_efuse(rtwdev, 1019 - i, &val8);

		antenna = u8_get_bits(val8, GENMASK(7, 5));
		if (antenna)
			break;
		antenna = u8_get_bits(val8, GENMASK(3, 1));
		if (antenna)
			break;
	}

	for (i = 0; i < 2; i++) {
		rtw_read8_physical_efuse(rtwdev, 1021 - i, &val8);

		wmode = u8_get_bits(val8, GENMASK(3, 2));
		if (wmode)
			break;
	}

	if (antenna == 1) {
		rtw_info(rtwdev, "This RTL8812AU says it is 1T1R.\n");

		efuse->hw_cap.nss = 1;
		hal->rf_type = RF_1T1R;
		hal->rf_path_num = 1;
		hal->rf_phy_num = 1;
		hal->antenna_tx = BB_PATH_A;
		hal->antenna_rx = BB_PATH_A;
	} else {
		/* Override rtw_chip_parameter_setup(). It detects 8812au as 1T1R. */
		efuse->hw_cap.nss = 2;
		hal->rf_type = RF_2T2R;
		hal->rf_path_num = 2;
		hal->rf_phy_num = 2;
		hal->antenna_tx = BB_PATH_AB;
		hal->antenna_rx = BB_PATH_AB;

		if (antenna == 2 && wmode == 2) {
			rtw_info(rtwdev, "This RTL8812AU says it can't do VHT.\n");

			/* Can't be EFUSE_HW_CAP_IGNORE and can't be
			* EFUSE_HW_CAP_PTCL_VHT, so make it 1.
			*/
			efuse->hw_cap.ptcl = 1;
			efuse->hw_cap.bw &= ~BIT(RTW_CHANNEL_WIDTH_80);
		}
	}
}

static int rtw8821a_read_efuse(struct rtw_dev *rtwdev, u8 *log_map)
{
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_efuse *efuse = &rtwdev->efuse;
	struct rtw8821a_efuse *map;
	int i;

	print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 16, 1,
		       log_map, chip->log_efuse_size, true);

	map = (struct rtw8821a_efuse *)log_map;

	efuse->rf_board_option = map->rf_board_option;
	efuse->crystal_cap = map->xtal_k;
	if (efuse->crystal_cap == 0xff)
		efuse->crystal_cap = 0x20;
	efuse->pa_type_2g = map->pa_type;
	efuse->pa_type_5g = map->pa_type;
	efuse->lna_type_2g = map->lna_type_2g;
	efuse->lna_type_5g = map->lna_type_5g;
	if (chip->id == RTW_CHIP_TYPE_8812A) {
		rtw8812a_read_amplifier_type(rtwdev);
		rtw8812a_read_rfe_type(rtwdev, map);

		efuse->usb_mode_switch = u8_get_bits(map->usb_mode, BIT(1));
	}
	efuse->channel_plan = map->channel_plan;
	efuse->country_code[0] = map->country_code[0];
	efuse->country_code[1] = map->country_code[1];
	efuse->bt_setting = map->rf_bt_setting;
	efuse->regd = map->rf_board_option & 0x7;
	efuse->thermal_meter[0] = map->thermal_meter;
	efuse->thermal_meter[1] = map->thermal_meter;
	efuse->thermal_meter_k = map->thermal_meter;
	efuse->tx_bb_swing_setting_2g = map->tx_bb_swing_setting_2g;
	efuse->tx_bb_swing_setting_5g = map->tx_bb_swing_setting_5g;

	rtw8812a_read_usb_type(rtwdev);

	if (chip->id == RTW_CHIP_TYPE_8821A)
		efuse->btcoex = rtw_read32_mask(rtwdev, REG_WL_BT_PWR_CTRL,
						BIT_BT_FUNC_EN);
	else
		efuse->btcoex = (map->rf_board_option & 0xe0) == 0x20;
	efuse->share_ant = !!(efuse->bt_setting & BIT(0));

	// if (map->rf_board_option != 0xff)
	// 	efuse->ant_div_cfg = u8_get_bits(map->rf_board_option, BIT(3));
	// if (efuse->btcoex && efuse->share_ant)
	// 	efuse->ant_div_cfg = 0;

	/* No antenna diversity because it's disabled in the vendor driver */
	efuse->ant_div_cfg = 0;

	efuse->ant_div_type = map->rf_antenna_option;
	if (efuse->ant_div_type == 0xff)
		efuse->ant_div_type = 0x3;

	for (i = 0; i < 4; i++)
		efuse->txpwr_idx_table[i] = map->txpwr_idx_table[i];

	switch (rtw_hci_type(rtwdev)) {
	case RTW_HCI_TYPE_USB:
		if (chip->id == RTW_CHIP_TYPE_8821A)
			ether_addr_copy(efuse->addr, map->rtw8821au.mac_addr);
		else
			ether_addr_copy(efuse->addr, map->rtw8812au.mac_addr);
		break;
	case RTW_HCI_TYPE_PCIE:
	case RTW_HCI_TYPE_SDIO:
	default:
		/* unsupported now */
		return -EOPNOTSUPP;
	}

	return 0;
}

static void rtw8821a_reset_8051(struct rtw_dev *rtwdev)
{
	const struct rtw_chip_info *chip = rtwdev->chip;
	u8 val8;

	/* Reset MCU IO Wrapper */
	rtw_write8_clr(rtwdev, REG_RSV_CTRL, BIT(1));
	if (chip->id == RTW_CHIP_TYPE_8812A)
		rtw_write8_clr(rtwdev, REG_RSV_CTRL + 1, BIT(3));
	else
		rtw_write8_clr(rtwdev, REG_RSV_CTRL + 1, BIT(0));

	val8 = rtw_read8(rtwdev, REG_SYS_FUNC_EN + 1);
	rtw_write8(rtwdev, REG_SYS_FUNC_EN + 1, val8 & ~BIT(2));

	/* Enable MCU IO Wrapper */
	rtw_write8_clr(rtwdev, REG_RSV_CTRL, BIT(1));
	if (chip->id == RTW_CHIP_TYPE_8812A)
		rtw_write8_set(rtwdev, REG_RSV_CTRL + 1, BIT(3));
	else
		rtw_write8_set(rtwdev, REG_RSV_CTRL + 1, BIT(0));

	rtw_write8(rtwdev, REG_SYS_FUNC_EN + 1, val8 | BIT(2));
}

/* A lightweight deinit function */
static void rtw8812au_hw_reset(struct rtw_dev *rtwdev)
{
	u8 val8;

	if (!(rtw_read8(rtwdev, REG_MCUFW_CTRL) & BIT_RAM_DL_SEL))
		return;

	rtw8821a_reset_8051(rtwdev);
	rtw_write8(rtwdev, REG_MCUFW_CTRL, 0x00);

	/* before BB reset should do clock gated */
	rtw_write32_set(rtwdev, REG_FPGA0_XCD_RF_PARA, BIT(6));

	/* reset BB */
	rtw_write8_clr(rtwdev, REG_SYS_FUNC_EN, BIT(0) | BIT(1));

	/* reset RF */
	rtw_write8(rtwdev, REG_RF_CTRL, 0);

	/* reset TRX path */
	rtw_write16(rtwdev, REG_CR, 0);

	/* reset MAC, reg0x5[1], auto FSM off */
	rtw_write8_set(rtwdev, REG_APS_FSMCO + 1, APS_FSMCO_MAC_OFF >> 8);

	/* check if reg0x5[1] auto cleared */
	if (read_poll_timeout_atomic(rtw_read8, val8,
				     !(val8 & (APS_FSMCO_MAC_OFF >> 8)),
				     1, 5000, false,
				     rtwdev, REG_APS_FSMCO + 1))
		rtw_err(rtwdev, "%s: timed out waiting for 0x5[1]\n", __func__);

	/* reg0x5[0], auto FSM on */
	val8 |= APS_FSMCO_MAC_ENABLE >> 8;
	rtw_write8(rtwdev, REG_APS_FSMCO + 1, val8);

	rtw_write8_clr(rtwdev, REG_SYS_FUNC_EN + 1, BIT(4) | BIT(7));
	rtw_write8_set(rtwdev, REG_SYS_FUNC_EN + 1, BIT(4) | BIT(7));
}

static int rtw8812au_init_power_on(struct rtw_dev *rtwdev)
{
	const struct rtw_chip_info *chip = rtwdev->chip;
	u16 val16;
	int ret;

	if (test_bit(RTW_FLAG_POWERON, rtwdev->flags)) {
		rtw_err(rtwdev, "%s: bailing because RTW_FLAG_POWERON\n", __func__);
		return 0;
	}

	ret = rtw_pwr_seq_parser(rtwdev, chip->pwr_on_seq);
	if (ret) {
		rtw_err(rtwdev, "power on flow failed\n");
		return ret;
	}

	rtw_write16(rtwdev, REG_CR, 0);
	val16 = HCI_TXDMA_EN | HCI_RXDMA_EN | TXDMA_EN | RXDMA_EN |
		PROTOCOL_EN | SCHEDULE_EN | ENSEC | CALTMR_EN;
	rtw_write16_set(rtwdev, REG_CR, val16);

	if (chip->id == RTW_CHIP_TYPE_8821A) {
		if (rtw_read8(rtwdev, REG_SYS_CFG1 + 3) & BIT(0))
			rtw_write8_set(rtwdev, REG_LDO_SWR_CTRL, BIT(6));
	}

	set_bit(RTW_FLAG_POWERON, rtwdev->flags);

	return ret;
}

static void rtw8821au_init_queue_reserved_page(struct rtw_dev *rtwdev)
{
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_fifo_conf *fifo = &rtwdev->fifo;
	const struct rtw_page_table *pg_tbl = NULL;
	u16 pubq_num;
	u32 val32;

	switch (rtw_hci_type(rtwdev)) {
	case RTW_HCI_TYPE_PCIE:
		pg_tbl = &chip->page_table[1];
		break;
	case RTW_HCI_TYPE_USB:
		if (rtwdev->hci.bulkout_num == 2)
			pg_tbl = &chip->page_table[2];
		else if (rtwdev->hci.bulkout_num == 3)
			pg_tbl = &chip->page_table[3];
		else if (rtwdev->hci.bulkout_num == 4)
			pg_tbl = &chip->page_table[4];
		break;
	case RTW_HCI_TYPE_SDIO:
		pg_tbl = &chip->page_table[0];
		break;
	default:
		break;
	}

	pubq_num = fifo->acq_pg_num - pg_tbl->hq_num - pg_tbl->lq_num -
		   pg_tbl->nq_num - pg_tbl->exq_num - pg_tbl->gapq_num;

	val32 = BIT_RQPN_NE(pg_tbl->nq_num, pg_tbl->exq_num);
	rtw_write32(rtwdev, REG_RQPN_NPQ, val32);

	val32 = BIT_RQPN_HLP(pg_tbl->hq_num, pg_tbl->lq_num, pubq_num);
	rtw_write32(rtwdev, REG_RQPN, val32);
}

static void rtw8821au_init_tx_buffer_boundary(struct rtw_dev *rtwdev)
{
	struct rtw_fifo_conf *fifo = &rtwdev->fifo;

	rtw_write8(rtwdev, REG_BCNQ_BDNY, fifo->rsvd_boundary);
	rtw_write8(rtwdev, REG_MGQ_BDNY, fifo->rsvd_boundary);
	rtw_write8(rtwdev, REG_WMAC_LBK_BF_HD, fifo->rsvd_boundary);
	rtw_write8(rtwdev, REG_TRXFF_BNDY, fifo->rsvd_boundary);
	rtw_write8(rtwdev, REG_DWBCN0_CTRL + 1, fifo->rsvd_boundary);
}

static int rtw8821au_init_queue_priority(struct rtw_dev *rtwdev)
{
	const struct rtw_chip_info *chip = rtwdev->chip;
	const struct rtw_rqpn *rqpn = NULL;
	u16 txdma_pq_map;

	switch (rtw_hci_type(rtwdev)) {
	case RTW_HCI_TYPE_PCIE:
		rqpn = &chip->rqpn_table[1];
		break;
	case RTW_HCI_TYPE_USB:
		if (rtwdev->hci.bulkout_num == 2)
			rqpn = &chip->rqpn_table[2];
		else if (rtwdev->hci.bulkout_num == 3)
			rqpn = &chip->rqpn_table[3];
		else if (rtwdev->hci.bulkout_num == 4)
			rqpn = &chip->rqpn_table[4];
		else
			return -EINVAL;
		break;
	case RTW_HCI_TYPE_SDIO:
		rqpn = &chip->rqpn_table[0];
		break;
	default:
		return -EINVAL;
	}

	rtwdev->fifo.rqpn = rqpn;

	txdma_pq_map = rtw_read16(rtwdev, REG_TXDMA_PQ_MAP) & 0x7;
	txdma_pq_map |= BIT_TXDMA_HIQ_MAP(rqpn->dma_map_hi);
	txdma_pq_map |= BIT_TXDMA_MGQ_MAP(rqpn->dma_map_mg);
	txdma_pq_map |= BIT_TXDMA_BKQ_MAP(rqpn->dma_map_bk);
	txdma_pq_map |= BIT_TXDMA_BEQ_MAP(rqpn->dma_map_be);
	txdma_pq_map |= BIT_TXDMA_VIQ_MAP(rqpn->dma_map_vi);
	txdma_pq_map |= BIT_TXDMA_VOQ_MAP(rqpn->dma_map_vo);
	rtw_write16(rtwdev, REG_TXDMA_PQ_MAP, txdma_pq_map);

	/* Packet in Hi Queue Tx immediately (No constraint for ATIM Period). */
	if (rtwdev->hci.type == RTW_HCI_TYPE_USB &&
	    rtwdev->hci.bulkout_num == 4)
		rtw_write8(rtwdev, REG_HIQ_NO_LMT_EN, 0xff);

	return 0;
}

static void rtw8821a_init_wmac_setting(struct rtw_dev *rtwdev)
{
	rtw_write16(rtwdev, REG_RXFLTMAP0, 0xffff);
	rtw_write16(rtwdev, REG_RXFLTMAP1, 0x0400);
	rtw_write16(rtwdev, REG_RXFLTMAP2, 0xffff);

	rtw_write32(rtwdev, REG_MAR, 0xffffffff);
	rtw_write32(rtwdev, REG_MAR + 4, 0xffffffff);
}

static void rtw8821a_init_adaptive_ctrl(struct rtw_dev *rtwdev)
{
	rtw_write32_mask(rtwdev, REG_RRSR, 0xfffff, 0xffff1);
	rtw_write16(rtwdev, REG_RETRY_LIMIT, 0x3030);
}

static void rtw8821a_init_edca(struct rtw_dev *rtwdev)
{
	rtw_write16(rtwdev, REG_SPEC_SIFS, 0x100a);
	rtw_write16(rtwdev, REG_MAC_SPEC_SIFS, 0x100a);

	rtw_write16(rtwdev, REG_SIFS, 0x100a);
	rtw_write16(rtwdev, REG_SIFS + 2, 0x100a);

	rtw_write32(rtwdev, REG_EDCA_BE_PARAM, 0x005EA42B);
	rtw_write32(rtwdev, REG_EDCA_BK_PARAM, 0x0000A44F);
	rtw_write32(rtwdev, REG_EDCA_VI_PARAM, 0x005EA324);
	rtw_write32(rtwdev, REG_EDCA_VO_PARAM, 0x002FA226);

	rtw_write8(rtwdev, REG_USTIME_TSF, 0x50);
	rtw_write8(rtwdev, REG_USTIME_EDCA, 0x50);
}

static void rtw8821au_tx_aggregation(struct rtw_dev *rtwdev)
{
	const struct rtw_chip_info *chip = rtwdev->chip;

	rtw_write32_mask(rtwdev, REG_DWBCN0_CTRL, 0xf0,
			 chip->usb_tx_agg_desc_num);

	if (chip->id == RTW_CHIP_TYPE_8821A)
		rtw_write8(rtwdev, REG_DWBCN1_CTRL,
			   chip->usb_tx_agg_desc_num << 1);
}

static void rtw8821au_rx_aggregation(struct rtw_dev *rtwdev)
{
	u8 rxagg_usb_size, rxagg_usb_timeout;
	bool usb3 = false;///TODO
	u16 val16;

	if (usb3) {
		rxagg_usb_size = 0x7;
		rxagg_usb_timeout = 0x1a;
	} else {
		rxagg_usb_size = 0x5;
		rxagg_usb_timeout = 0x20;
	}

	val16 = (rxagg_usb_timeout << 8) | rxagg_usb_size;
	rtw_write16(rtwdev, REG_RXDMA_AGG_PG_TH, val16);

	rtw_write8_set(rtwdev, REG_TXDMA_PQ_MAP, BIT_RXDMA_AGG_EN);
}

static void rtw8821a_init_beacon_parameters(struct rtw_dev *rtwdev)
{
	u16 val16;

	val16 = (BIT_DIS_TSF_UDT << 8) | BIT_DIS_TSF_UDT;
	if (rtwdev->efuse.btcoex)
		val16 |= BIT_EN_BCN_FUNCTION;
	rtw_write16(rtwdev, REG_BCN_CTRL, val16);

	rtw_write32_mask(rtwdev, REG_TBTT_PROHIBIT, 0xfffff, WLAN_TBTT_TIME);
	rtw_write8(rtwdev, REG_DRVERLYINT, 0x05);
	rtw_write8(rtwdev, REG_BCNDMATIM, WLAN_BCN_DMA_TIME);
	rtw_write16(rtwdev, REG_BCNTCFG, 0x4413);
}

static void rtw8821au_init_burst_pkt_len(struct rtw_dev *rtwdev)
{
	const struct rtw_chip_info *chip = rtwdev->chip;
	u8 speedvalue, provalue, temp;

	/* usb3 rx interval */
	rtw_write8(rtwdev, 0xf050, 0x01);

	/* burst lenght=4, set 0x3400 for burst length=2 */
	rtw_write16(rtwdev, REG_RXDMA_STATUS, 0x7400);
	rtw_write8(rtwdev, REG_RXDMA_STATUS + 1, 0xf5);

	/* 0x456 = 0x70, sugguested by Zhilin */
	if (chip->id == RTW_CHIP_TYPE_8821A)
		rtw_write8(rtwdev, REG_AMPDU_MAX_TIME, 0x5e);
	else
		rtw_write8(rtwdev, REG_AMPDU_MAX_TIME, 0x70);

	rtw_write32(rtwdev, REG_AMPDU_MAX_LENGTH, 0xffffffff);
	rtw_write8(rtwdev, REG_USTIME_TSF, 0x50);
	rtw_write8(rtwdev, REG_USTIME_EDCA, 0x50);

	if (chip->id == RTW_CHIP_TYPE_8821A)
		speedvalue = BIT(7);
	else
		/* check device operation speed: SS 0xff bit7 */
		speedvalue = rtw_read8(rtwdev, 0xff);

	if (speedvalue & BIT(7)) { /* USB 2/1.1 Mode */
		temp = rtw_read8(rtwdev, 0xfe17);
		if (((temp >> 4) & 0x03) == 0) {
			// pHalData->UsbBulkOutSize = USB_HIGH_SPEED_BULK_SIZE;
			provalue = rtw_read8(rtwdev, REG_RXDMA_MODE);
			rtw_write8(rtwdev, REG_RXDMA_MODE, ((provalue | BIT(4) | BIT(3) | BIT(2) | BIT(1)) & (~BIT(5)))); /* set burst pkt len=512B */
		} else {
			// pHalData->UsbBulkOutSize = 64;
			provalue = rtw_read8(rtwdev, REG_RXDMA_MODE);
			rtw_write8(rtwdev, REG_RXDMA_MODE, ((provalue | BIT(5) | BIT(3) | BIT(2) | BIT(1)) & (~BIT(4)))); /* set burst pkt len=64B */
		}

		// pHalData->bSupportUSB3 = _FALSE;
	} else { /* USB3 Mode */
		// pHalData->UsbBulkOutSize = USB_SUPER_SPEED_BULK_SIZE;
		provalue = rtw_read8(rtwdev, REG_RXDMA_MODE);
		rtw_write8(rtwdev, REG_RXDMA_MODE, ((provalue | BIT(3) | BIT(2) | BIT(1)) & (~(BIT(5) | BIT(4))))); /* set burst pkt len=1k */

		// pHalData->bSupportUSB3 = _TRUE;

		/* set Reg 0xf008[3:4] to 2'00 to disable U1/U2 Mode to avoid
		 * 2.5G spur in USB3.0.
		 */
		rtw_write8_clr(rtwdev, 0xf008, BIT(4) | BIT(3));
	}

	/* This has no effect: temp is only u8, it doesn't have BIT(10). */
	// rtw_write8_clr(rtwdev, REG_SYS_FUNC_EN, BIT(10)); /* reset 8051 */

	rtw_write8_set(rtwdev, REG_SINGLE_AMPDU_CTRL, BIT_EN_SINGLE_APMDU);

	/* for VHT packet length 11K */
	rtw_write8(rtwdev, REG_RX_PKT_LIMIT, 0x18);

	rtw_write8(rtwdev, REG_PIFS, 0x00);

	if (chip->id == RTW_CHIP_TYPE_8821A) {
		/* 0x0a0a too small , it can't pass AC logo. change to 0x1f1f */
		rtw_write16(rtwdev, REG_MAX_AGGR_NUM, 0x1f1f);
		rtw_write8(rtwdev, REG_FWHW_TXQ_CTRL, 0x80);
		rtw_write32(rtwdev, REG_FAST_EDCA_CTRL, 0x03087777);
	} else {
		rtw_write16(rtwdev, REG_MAX_AGGR_NUM, 0x1f1f);
		rtw_write8_clr(rtwdev, REG_FWHW_TXQ_CTRL, BIT(7));
	}

	 /* to prevent mac is reseted by bus. */
	rtw_write8_set(rtwdev, REG_RSV_CTRL, BIT(5) | BIT(6));

	/* ARFB table 9 for 11ac 5G 2SS */
	rtw_write32(rtwdev, REG_ARFR0, 0x00000010);
	rtw_write32(rtwdev, REG_ARFRH0, 0xfffff000);

	/* ARFB table 10 for 11ac 5G 1SS */
	rtw_write32(rtwdev, REG_ARFR1_V1, 0x00000010);
	rtw_write32(rtwdev, REG_ARFRH1_V1, 0x003ff000);

	/* ARFB table 11 for 11ac 24G 1SS */
	rtw_write32(rtwdev, REG_ARFR2_V1, 0x00000015);
	rtw_write32(rtwdev, REG_ARFRH2_V1, 0x003ff000);

	/* ARFB table 12 for 11ac 24G 2SS */
	rtw_write32(rtwdev, REG_ARFR3_V1, 0x00000015);
	rtw_write32(rtwdev, REG_ARFRH3_V1, 0xffcff000);
}

static void rtw8821a_phy_bb_config(struct rtw_dev *rtwdev)
{
	u8 val8, crystal_cap;

	/* power on BB/RF domain */
	val8 = rtw_read8(rtwdev, REG_SYS_FUNC_EN);
	val8 |= BIT_FEN_USBA;
	rtw_write8(rtwdev, REG_SYS_FUNC_EN, val8);

	/* toggle BB reset */
	val8 |= BIT_FEN_BB_RSTB | BIT_FEN_BB_GLB_RST;
	rtw_write8(rtwdev, REG_SYS_FUNC_EN, val8);

	rtw_write8(rtwdev, REG_RF_CTRL,
		   BIT_RF_EN | BIT_RF_RSTB | BIT_RF_SDM_RSTB);
	rtw_write8(rtwdev, REG_RF_B_CTRL,
		   BIT_RF_EN | BIT_RF_RSTB | BIT_RF_SDM_RSTB);

	rtw_load_table(rtwdev, rtwdev->chip->bb_tbl);
	rtw_load_table(rtwdev, rtwdev->chip->agc_tbl);

	crystal_cap = rtwdev->efuse.crystal_cap & 0x3F;
	if (rtwdev->chip->id == RTW_CHIP_TYPE_8812A)
		rtw_write32_mask(rtwdev, REG_AFE_CTRL3, 0x7FF80000,
				 crystal_cap | (crystal_cap << 6));
	else
		rtw_write32_mask(rtwdev, REG_AFE_CTRL3, 0xFFF000,
				 crystal_cap | (crystal_cap << 6));
}

static void rtw8821a_phy_rf_config(struct rtw_dev *rtwdev)
{
	u8 rf_path;

	for (rf_path = 0; rf_path < rtwdev->hal.rf_path_num; rf_path++)
		rtw_load_table(rtwdev, rtwdev->chip->rf_tbl[rf_path]);
}

static void rtw8812a_config_1t(struct rtw_dev *rtwdev)
{
	/* BB OFDM RX Path_A */
	rtw_write32_mask(rtwdev, 0x808, 0xff, 0x11);

	/* BB OFDM TX Path_A */
	rtw_write32_mask(rtwdev, 0x80c, MASKLWORD, 0x1111);

	/* BB CCK R/Rx Path_A */
	rtw_write32_mask(rtwdev, 0xa04, 0x0c000000, 0x0);

	/* MCS support */
	rtw_write32_mask(rtwdev, 0x8bc, 0xc0000060, 0x4);

	/* RF Path_B HSSI OFF */
	rtw_write32_mask(rtwdev, 0xe00, 0xf, 0x4);

	/* RF Path_B Power Down */
	rtw_write32_mask(rtwdev, 0xe90, MASKDWORD, 0);

	/* ADDA Path_B OFF */
	rtw_write32_mask(rtwdev, 0xe60, MASKDWORD, 0);
	rtw_write32_mask(rtwdev, 0xe64, MASKDWORD, 0);
}

static const u32 rtw8821a_txscale_tbl[] = {
	0x081, 0x088, 0x090, 0x099, 0x0a2, 0x0ac, 0x0b6, 0x0c0, 0x0cc, 0x0d8,
	0x0e5, 0x0f2, 0x101, 0x110, 0x120, 0x131, 0x143, 0x156, 0x16a, 0x180,
	0x197, 0x1af, 0x1c8, 0x1e3, 0x200, 0x21e, 0x23e, 0x261, 0x285, 0x2ab,
	0x2d3, 0x2fe, 0x32b, 0x35c, 0x38e, 0x3c4, 0x3fe
};

static u32 rtw8821a_get_bb_swing(struct rtw_dev *rtwdev, u8 band, u8 path)
{
	static const u32 swing2setting[4] = {0x200, 0x16a, 0x101, 0x0b6};
	struct rtw_efuse efuse = rtwdev->efuse;
	u8 tx_bb_swing;

	if (band == RTW_BAND_2G)
		tx_bb_swing = efuse.tx_bb_swing_setting_2g;
	else
		tx_bb_swing = efuse.tx_bb_swing_setting_5g;

	if (path == RF_PATH_B)
		tx_bb_swing >>= 2;
	tx_bb_swing &= 0x3;

	return swing2setting[tx_bb_swing];
}

static u8 rtw8821a_get_swing_index(struct rtw_dev *rtwdev)
{
	u32 swing, table_value;
	u8 i = 0;

	swing = rtw8821a_get_bb_swing(rtwdev, rtwdev->hal.current_band_type,
				      RF_PATH_A);

	for (i = 0; i < ARRAY_SIZE(rtw8821a_txscale_tbl); i++) {
		table_value = rtw8821a_txscale_tbl[i];
		if (swing == table_value)
			break;
	}

	return i;
}

static void rtw8821a_pwrtrack_init(struct rtw_dev *rtwdev)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	u8 ofdm_swing_idx;
	u8 path;

	ofdm_swing_idx = rtw8821a_get_swing_index(rtwdev);

	if (ofdm_swing_idx >= ARRAY_SIZE(rtw8821a_txscale_tbl))
		dm_info->default_ofdm_index = 24;
	else
		dm_info->default_ofdm_index = ofdm_swing_idx;

	if (rtwdev->chip->id == RTW_CHIP_TYPE_8821A)
		dm_info->default_cck_index = 0;
	else
		dm_info->default_cck_index = 24;

	for (path = RF_PATH_A; path < rtwdev->hal.rf_path_num; path++) {
		ewma_thermal_init(&dm_info->avg_thermal[path]);
		dm_info->delta_power_index[path] = 0;
		dm_info->delta_power_index_last[path] = 0;
	}

	dm_info->pwr_trk_triggered = false;
	dm_info->pwr_trk_init_trigger = true;
	dm_info->thermal_meter_k = rtwdev->efuse.thermal_meter_k;
}

static void rtw8821a_power_off(struct rtw_dev *rtwdev)
{
	u16 ori_fsmc0;

	rtw_hci_stop(rtwdev);

	if (!rtwdev->efuse.btcoex)
		rtw_write16_clr(rtwdev, REG_GPIO_MUXCFG, BIT_EN_SIC);

	// if (pHalData->bSupportUSB3 == _TRUE) {
	// 	/* set Reg 0xf008[3:4] to 2'11 to eable U1/U2 Mode in USB3.0. added by page, 20120712 */
	// 	rtw_write8_set(Adapter, 0xf008, 0x18);
	// }

	rtw_write32(rtwdev, REG_HISR0, 0xffffffff);
	rtw_write32(rtwdev, REG_HISR1, 0xffffffff);
	rtw_write32(rtwdev, REG_HIMR0, 0);
	rtw_write32(rtwdev, REG_HIMR1, 0);

	if (rtwdev->efuse.btcoex)
		rtw_coex_power_off_setting(rtwdev);

	if (!test_bit(RTW_FLAG_POWERON, rtwdev->flags)) {
		rtw_err(rtwdev, "%s: bailing because RTW_FLAG_POWERON\n", __func__);
		return;
	}

	ori_fsmc0 = rtw_read16(rtwdev, REG_APS_FSMCO);
	rtw_write16(rtwdev, REG_APS_FSMCO, ori_fsmc0 & ~APS_FSMCO_HW_POWERDOWN);

	/* Stop Tx Report Timer. */
	rtw_write8_clr(rtwdev, REG_TX_RPT_CTRL, BIT(1));

	/* Stop Rx */
	rtw_write8(rtwdev, REG_CR, 0);

	if (rtwdev->chip->id == RTW_CHIP_TYPE_8821A)
		rtw_pwr_seq_parser(rtwdev, enter_lps_flow_8821a);
	else
		rtw_pwr_seq_parser(rtwdev, enter_lps_flow_8812a);

	if (rtw_read8(rtwdev, REG_MCUFW_CTRL) & BIT_RAM_DL_SEL)
		rtw8821a_reset_8051(rtwdev);

	rtw_write8_clr(rtwdev, REG_SYS_FUNC_EN + 1, BIT(2));
	rtw_write8(rtwdev, REG_MCUFW_CTRL, 0);

	if (rtwdev->chip->id == RTW_CHIP_TYPE_8821A)
		rtw_pwr_seq_parser(rtwdev, card_disable_flow_8821a);
	else
		rtw_pwr_seq_parser(rtwdev, card_disable_flow_8812a);

	clear_bit(RTW_FLAG_POWERON, rtwdev->flags);

	if (ori_fsmc0 & APS_FSMCO_HW_POWERDOWN)
		rtw_write16_set(rtwdev, REG_APS_FSMCO, APS_FSMCO_HW_POWERDOWN);
}

///TODO: chip identification needs to be copied as well

static void rtw8821a_set_channel_bb_swing(struct rtw_dev *rtwdev, u8 band)
{
	rtw_write32_mask(rtwdev, REG_TXSCALE_A, BB_SWING_MASK,
			 rtw8821a_get_bb_swing(rtwdev, band, RF_PATH_A));
	rtw_write32_mask(rtwdev, REG_TXSCALE_B, BB_SWING_MASK,
			 rtw8821a_get_bb_swing(rtwdev, band, RF_PATH_B));
	rtw8821a_pwrtrack_init(rtwdev);
}

static void rtw8821a_set_ext_band_switch(struct rtw_dev *rtwdev, u8 band)
{
	rtw_write32_mask(rtwdev, REG_LED_CFG, BIT_DPDT_SEL_EN, 0);
	rtw_write32_mask(rtwdev, REG_LED_CFG, BIT_DPDT_WL_SEL, 1);
	rtw_write32_mask(rtwdev, REG_RFE_INV_A, 0xf, 7);
	rtw_write32_mask(rtwdev, REG_RFE_INV_A, 0xf0, 7);

	if (band == RTW_BAND_2G)
		rtw_write32_mask(rtwdev, REG_RFE_INV_A, BIT(29) | BIT(28), 1);
	else
		rtw_write32_mask(rtwdev, REG_RFE_INV_A, BIT(29) | BIT(28), 2);
}

static void rtw8821a_phy_set_rfe_reg_24g(struct rtw_dev *rtwdev)
{
	struct rtw_efuse *efuse = &rtwdev->efuse;

	/* Turn off RF PA and LNA */

	/* 0xCB0[15:12] = 0x7 (LNA_On)*/
	rtw_write32_mask(rtwdev, REG_RFE_PINMUX_A, 0xF000, 0x7);
	/* 0xCB0[7:4] = 0x7 (PAPE_A)*/
	rtw_write32_mask(rtwdev, REG_RFE_PINMUX_A, 0xF0, 0x7);

	if (efuse->ext_lna_2g) {
		/* Turn on 2.4G External LNA */
		rtw_write32_mask(rtwdev, REG_RFE_INV_A, BIT(20), 1);
		rtw_write32_mask(rtwdev, REG_RFE_INV_A, BIT(22), 0);
		rtw_write32_mask(rtwdev, REG_RFE_PINMUX_A, BIT(2) | BIT(1) | BIT(0), 0x2);
		rtw_write32_mask(rtwdev, REG_RFE_PINMUX_A, BIT(10) | BIT(9) | BIT(8), 0x2);
	} else {
		/* Bypass 2.4G External LNA */
		rtw_write32_mask(rtwdev, REG_RFE_INV_A, BIT(20), 0);
		rtw_write32_mask(rtwdev, REG_RFE_INV_A, BIT(22), 0);
		rtw_write32_mask(rtwdev, REG_RFE_PINMUX_A, BIT(2) | BIT(1) | BIT(0), 0x7);
		rtw_write32_mask(rtwdev, REG_RFE_PINMUX_A, BIT(10) | BIT(9) | BIT(8), 0x7);
	}
}

static void rtw8821a_phy_set_rfe_reg_5g(struct rtw_dev *rtwdev)
{
	/* Turn ON RF PA and LNA */

	/* 0xCB0[15:12] = 0x7 (LNA_On)*/
	rtw_write32_mask(rtwdev, REG_RFE_PINMUX_A, 0xF000, 0x5);
	/* 0xCB0[7:4] = 0x7 (PAPE_A)*/
	rtw_write32_mask(rtwdev, REG_RFE_PINMUX_A, 0xF0, 0x4);

	/* Bypass 2.4G External LNA */
	rtw_write32_mask(rtwdev, REG_RFE_INV_A, BIT(20), 0);
	rtw_write32_mask(rtwdev, REG_RFE_INV_A, BIT(22), 0);
	rtw_write32_mask(rtwdev, REG_RFE_PINMUX_A, BIT(2) | BIT(1) | BIT(0), 0x7);
	rtw_write32_mask(rtwdev, REG_RFE_PINMUX_A, BIT(10) | BIT(9) | BIT(8), 0x7);
}

static void rtw8812a_phy_set_rfe_reg_24g(struct rtw_dev *rtwdev)
{
	switch (rtwdev->efuse.rfe_option) {
	case 0:
	case 2:
		rtw_write32(rtwdev, REG_RFE_PINMUX_A, 0x77777777);
		rtw_write32(rtwdev, REG_RFE_PINMUX_B, 0x77777777);
		rtw_write32_mask(rtwdev, REG_RFE_INV_A, RFE_INV_MASK, 0x000);
		rtw_write32_mask(rtwdev, REG_RFE_INV_B, RFE_INV_MASK, 0x000);
		break;
	case 1:
		if (rtwdev->efuse.btcoex) {
			rtw_write32_mask(rtwdev, REG_RFE_PINMUX_A, 0xffffff, 0x777777);
			rtw_write32(rtwdev, REG_RFE_PINMUX_B, 0x77777777);
			rtw_write32_mask(rtwdev, REG_RFE_INV_A, 0x33f00000, 0x000);
			rtw_write32_mask(rtwdev, REG_RFE_INV_B, RFE_INV_MASK, 0x000);
		} else {
			rtw_write32(rtwdev, REG_RFE_PINMUX_A, 0x77777777);
			rtw_write32(rtwdev, REG_RFE_PINMUX_B, 0x77777777);
			rtw_write32_mask(rtwdev, REG_RFE_INV_A, RFE_INV_MASK, 0x000);
			rtw_write32_mask(rtwdev, REG_RFE_INV_B, RFE_INV_MASK, 0x000);
		}
		break;
	case 3:
		rtw_write32(rtwdev, REG_RFE_PINMUX_A, 0x54337770);
		rtw_write32(rtwdev, REG_RFE_PINMUX_B, 0x54337770);
		rtw_write32_mask(rtwdev, REG_RFE_INV_A, RFE_INV_MASK, 0x010);
		rtw_write32_mask(rtwdev, REG_RFE_INV_B, RFE_INV_MASK, 0x010);
		rtw_write32_mask(rtwdev, REG_ANTSEL_SW, 0x00000303, 0x1);
		break;
	case 4:
		rtw_write32(rtwdev, REG_RFE_PINMUX_A, 0x77777777);
		rtw_write32(rtwdev, REG_RFE_PINMUX_B, 0x77777777);
		rtw_write32_mask(rtwdev, REG_RFE_INV_A, RFE_INV_MASK, 0x001);
		rtw_write32_mask(rtwdev, REG_RFE_INV_B, RFE_INV_MASK, 0x001);
		break;
	case 5:
		rtw_write8(rtwdev, REG_RFE_PINMUX_A + 2, 0x77);
		rtw_write32(rtwdev, REG_RFE_PINMUX_B, 0x77777777);
		rtw_write8_clr(rtwdev, REG_RFE_INV_A + 3, BIT(0));
		rtw_write32_mask(rtwdev, REG_RFE_INV_B, RFE_INV_MASK, 0x000);
		break;
	case 6:
		rtw_write32(rtwdev, REG_RFE_PINMUX_A, 0x07772770);
		rtw_write32(rtwdev, REG_RFE_PINMUX_B, 0x07772770);
		rtw_write32(rtwdev, REG_RFE_INV_A, 0x00000077);
		rtw_write32(rtwdev, REG_RFE_INV_B, 0x00000077);
		break;
	default:
		break;
	}
}

static void rtw8812a_phy_set_rfe_reg_5g(struct rtw_dev *rtwdev)
{
	switch (rtwdev->efuse.rfe_option) {
	case 0:
		rtw_write32(rtwdev, REG_RFE_PINMUX_A, 0x77337717);
		rtw_write32(rtwdev, REG_RFE_PINMUX_B, 0x77337717);
		rtw_write32_mask(rtwdev, REG_RFE_INV_A, RFE_INV_MASK, 0x010);
		rtw_write32_mask(rtwdev, REG_RFE_INV_B, RFE_INV_MASK, 0x010);
		break;
	case 1:
		if (rtwdev->efuse.btcoex) {
			rtw_write32_mask(rtwdev, REG_RFE_PINMUX_A, 0xffffff, 0x337717);
			rtw_write32(rtwdev, REG_RFE_PINMUX_B, 0x77337717);
			rtw_write32_mask(rtwdev, REG_RFE_INV_A, 0x33f00000, 0x000);
			rtw_write32_mask(rtwdev, REG_RFE_INV_B, RFE_INV_MASK, 0x000);
		} else {
			rtw_write32(rtwdev, REG_RFE_PINMUX_A, 0x77337717);
			rtw_write32(rtwdev, REG_RFE_PINMUX_B, 0x77337717);
			rtw_write32_mask(rtwdev, REG_RFE_INV_A, RFE_INV_MASK, 0x000);
			rtw_write32_mask(rtwdev, REG_RFE_INV_B, RFE_INV_MASK, 0x000);
		}
		break;
	case 2:
	case 4:
		rtw_write32(rtwdev, REG_RFE_PINMUX_A, 0x77337777);
		rtw_write32(rtwdev, REG_RFE_PINMUX_B, 0x77337777);
		rtw_write32_mask(rtwdev, REG_RFE_INV_A, RFE_INV_MASK, 0x010);
		rtw_write32_mask(rtwdev, REG_RFE_INV_B, RFE_INV_MASK, 0x010);
		break;
	case 3:
		rtw_write32(rtwdev, REG_RFE_PINMUX_A, 0x54337717);
		rtw_write32(rtwdev, REG_RFE_PINMUX_B, 0x54337717);
		rtw_write32_mask(rtwdev, REG_RFE_INV_A, RFE_INV_MASK, 0x010);
		rtw_write32_mask(rtwdev, REG_RFE_INV_B, RFE_INV_MASK, 0x010);
		rtw_write32_mask(rtwdev, REG_ANTSEL_SW, 0x00000303, 0x1);
		break;
	case 5:
		rtw_write8(rtwdev, REG_RFE_PINMUX_A + 2, 0x33);
		rtw_write32(rtwdev, REG_RFE_PINMUX_B, 0x77337777);
		rtw_write8_set(rtwdev, REG_RFE_INV_A + 3, BIT(0));
		rtw_write32_mask(rtwdev, REG_RFE_INV_B, RFE_INV_MASK, 0x010);
		break;
	case 6:
		rtw_write32(rtwdev, REG_RFE_PINMUX_A, 0x07737717);
		rtw_write32(rtwdev, REG_RFE_PINMUX_B, 0x07737717);
		rtw_write32(rtwdev, REG_RFE_INV_A, 0x00000077);
		rtw_write32(rtwdev, REG_RFE_INV_B, 0x00000077);
		break;
	default:
		break;
	}
}

static void rtw8821a_switch_band(struct rtw_dev *rtwdev, u8 new_band, u8 bw)
{
	const struct rtw_chip_info *chip = rtwdev->chip;
	u16 basic_rates, reg_41a;

	/* 8811au one antenna module doesn't support antenna div, so driver must
	 * control antenna band, otherwise one of the band will have issue
	 */
	if (chip->id == RTW_CHIP_TYPE_8821A && !rtwdev->efuse.btcoex &&
	    rtwdev->efuse.ant_div_cfg == 0)
		rtw8821a_set_ext_band_switch(rtwdev, new_band);

	if (new_band == RTW_BAND_2G) {
		rtw_write32_set(rtwdev, REG_RXPSEL, BIT_RX_PSEL_RST);

		if (chip->id == RTW_CHIP_TYPE_8821A) {
			rtw8821a_phy_set_rfe_reg_24g(rtwdev);

			rtw_write32_mask(rtwdev, REG_TXSCALE_A, 0xf00, 0);
		} else {
			rtw_write32_mask(rtwdev, REG_BWINDICATION, 0x3, 0x1);
			rtw_write32_mask(rtwdev, REG_PDMFTH, GENMASK(17, 13), 0x17);

			if (bw == RTW_CHANNEL_WIDTH_20 &&
			    rtwdev->hal.rf_type == RF_1T1R &&
			    !rtwdev->efuse.ext_lna_2g)
				rtw_write32_mask(rtwdev, REG_PDMFTH, GENMASK(3, 1), 0x02);
			else
				rtw_write32_mask(rtwdev, REG_PDMFTH, GENMASK(3, 1), 0x04);

			rtw_write32_mask(rtwdev, REG_CCASEL, 0x3, 0);

			rtw8812a_phy_set_rfe_reg_24g(rtwdev);
		}

		rtw_write32_mask(rtwdev, REG_TXPSEL, 0xf0, 0x1);
		rtw_write32_mask(rtwdev, REG_CCK_RX, 0x0f000000, 0x1);

		basic_rates = BIT(DESC_RATE1M) | BIT(DESC_RATE2M) |
			      BIT(DESC_RATE5_5M) | BIT(DESC_RATE11M) |
			      BIT(DESC_RATE6M) | BIT(DESC_RATE12M) |
			      BIT(DESC_RATE24M);
		rtw_write32_mask(rtwdev, REG_RRSR, 0xfffff, basic_rates);

		rtw_write8_clr(rtwdev, REG_CCK_CHECK, BIT_CHECK_CCK_EN);
	} else { /* RTW_BAND_5G */
		if (chip->id == RTW_CHIP_TYPE_8821A)
			rtw8821a_phy_set_rfe_reg_5g(rtwdev);

		rtw_write8_set(rtwdev, REG_CCK_CHECK, BIT_CHECK_CCK_EN);

		read_poll_timeout_atomic(rtw_read16, reg_41a, (reg_41a & 0x30) == 0x30,
					 50, 2500, false, rtwdev, REG_TXPKT_EMPTY);

		rtw_write32_set(rtwdev, REG_RXPSEL, BIT_RX_PSEL_RST);

		if (chip->id == RTW_CHIP_TYPE_8821A) {
			rtw_write32_mask(rtwdev, REG_TXSCALE_A, 0xf00, 1);
		} else {
			rtw_write32_mask(rtwdev, REG_BWINDICATION, 0x3, 0x2);
			rtw_write32_mask(rtwdev, REG_PDMFTH, GENMASK(17, 13), 0x15);
			rtw_write32_mask(rtwdev, REG_PDMFTH, GENMASK(3, 1), 0x04);

			rtw_write32_mask(rtwdev, REG_CCASEL, 0x3, 1);

			rtw8812a_phy_set_rfe_reg_5g(rtwdev);
		}

		rtw_write32_mask(rtwdev, REG_TXPSEL, 0xf0, 0);
		rtw_write32_mask(rtwdev, REG_CCK_RX, 0x0f000000, 0xf);

		basic_rates = BIT(DESC_RATE6M) | BIT(DESC_RATE12M) |
			      BIT(DESC_RATE24M);
		rtw_write32_mask(rtwdev, REG_RRSR, 0xfffff, basic_rates);
	}

	rtw8821a_set_channel_bb_swing(rtwdev, new_band);
}

static int rtw8821a_power_on(struct rtw_dev *rtwdev)
{
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_efuse *efuse = &rtwdev->efuse;
	struct rtw_hal *hal = &rtwdev->hal;
	bool wifi_only;
	int ret;
	u8 val8;

	/* Override rtw_chip_efuse_info_setup() */
	if (chip->id == RTW_CHIP_TYPE_8821A)
		efuse->btcoex = rtw_read32_mask(rtwdev, REG_WL_BT_PWR_CTRL,
						BIT_BT_FUNC_EN);

	/* Override rtw_chip_efuse_info_setup() */
	if (chip->id == RTW_CHIP_TYPE_8812A)
		rtw8812a_read_amplifier_type(rtwdev);

	ret = rtw_hci_setup(rtwdev);
	if (ret) {
		rtw_err(rtwdev, "failed to setup hci\n");
		goto err;
	}

	///TODO: change to rtw_dbg
	val8 = rtw_read8(rtwdev, REG_CR);
	if (val8 != 0 && val8 != 0xEA &&
	    (rtw_read8(rtwdev, REG_SYS_CLKR + 1) & BIT(3)))
		rtw_info(rtwdev, "MAC has already power on\n");
	else
		rtw_info(rtwdev, "MAC has not been powered on yet\n");

	/* Revise for U2/U3 switch we can not update RF-A/B reset.
	 * Reset after MAC power on to prevent RF R/W error.
	 * Is it a right method?
	 */
	if (chip->id == RTW_CHIP_TYPE_8812A) {
		rtw_write8(rtwdev, REG_RF_CTRL, 5);
		rtw_write8(rtwdev, REG_RF_CTRL, 7);
		rtw_write8(rtwdev, REG_RF_B_CTRL, 5);
		rtw_write8(rtwdev, REG_RF_B_CTRL, 7);
	}

	/* If HW didn't go through a complete de-initial procedure,
	 * it probably occurs some problem for double initial
	 * procedure.
	 */
	rtw8812au_hw_reset(rtwdev);

	ret = rtw8812au_init_power_on(rtwdev);
	if (ret) {
		rtw_err(rtwdev, "failed to power on\n");
		goto err;
	}

	if (efuse->btcoex)
		rtw_coex_power_on_setting(rtwdev);

	ret = set_trx_fifo_info(rtwdev);
	if (ret) {
		rtw_err(rtwdev, "failed to set trx fifo info\n");
		goto err;
	}

	///TODO: undo that commit which introduced this function. move it here
	ret = rtw_llt_init_legacy_old(rtwdev, rtwdev->fifo.rsvd_boundary);
	if (ret) {
		rtw_err(rtwdev, "failed to init llt\n");
		goto err;
	}

	rtw_write32_set(rtwdev, REG_TXDMA_OFFSET_CHK, BIT_DROP_DATA_EN);

	ret = rtw_wait_firmware_completion(rtwdev);
	if (ret) {
		rtw_err(rtwdev, "failed to wait firmware completion\n");
		goto err_off;
	}

	ret = rtw_download_firmware(rtwdev, &rtwdev->fw);
	if (ret) {
		rtw_err(rtwdev, "failed to download firmware\n");
		goto err_off;
	}

	rtw_write8(rtwdev, REG_HMETFR, 0xf);

	rtw_load_table(rtwdev, chip->mac_tbl);

	rtw8821au_init_queue_reserved_page(rtwdev);
	rtw8821au_init_tx_buffer_boundary(rtwdev);
	rtw8821au_init_queue_priority(rtwdev);

	rtw_write16(rtwdev, REG_TRXFF_BNDY + 2,
		    chip->rxff_size - REPORT_BUF - 1);

	if (chip->id == RTW_CHIP_TYPE_8812A)
		rtw_write8(rtwdev, REG_PBP,
			   u8_encode_bits(PBP_512, PBP_TX_MASK) |
			   u8_encode_bits(PBP_64, PBP_RX_MASK));

	rtw_write8(rtwdev, REG_RX_DRVINFO_SZ, PHY_STATUS_SIZE);

	rtw_write32(rtwdev, REG_HIMR0, 0);
	rtw_write32(rtwdev, REG_HIMR1, 0);

	rtw_write32_mask(rtwdev, REG_CR, 0x30000, 0x2);

	rtw8821a_init_wmac_setting(rtwdev);
	rtw8821a_init_adaptive_ctrl(rtwdev);
	rtw8821a_init_edca(rtwdev);

	rtw_write8_set(rtwdev, REG_FWHW_TXQ_CTRL, BIT(7));
	rtw_write8(rtwdev, REG_ACKTO, 0x80);

	rtw8821au_tx_aggregation(rtwdev);
	rtw8821au_rx_aggregation(rtwdev);

	rtw8821a_init_beacon_parameters(rtwdev);
	rtw_write8(rtwdev, REG_BCN_MAX_ERR, 0xff);

	if (rtwdev->hci.type == RTW_HCI_TYPE_USB)
		rtw8821au_init_burst_pkt_len(rtwdev);

	rtw_write8_set(rtwdev, REG_CR, MACTXEN | MACRXEN);

	///TODO: blinking controlled by mac80211
	if (rtwdev->chip->id == RTW_CHIP_TYPE_8821A)
		rtw_write8(rtwdev, REG_LED_CFG + 2, BIT(1) | BIT(5));
	else
		rtw_write8(rtwdev, REG_LED_CFG, BIT(1) | BIT(5));

	rtw8821a_phy_bb_config(rtwdev);
	rtw8821a_phy_rf_config(rtwdev);

	if (chip->id == RTW_CHIP_TYPE_8812A && hal->rf_path_num == 1)
		rtw8812a_config_1t(rtwdev);

	rtw8821a_switch_band(rtwdev, RTW_BAND_2G, RTW_CHANNEL_WIDTH_20);

	rtw_write32(rtwdev, RTW_SEC_CMD_REG, BIT(31) | BIT(30));

	rtw_write8(rtwdev, REG_HWSEQ_CTRL, 0xff);
	rtw_write32(rtwdev, REG_BAR_MODE_CTRL, 0x0201ffff);
	rtw_write8(rtwdev, REG_NAV_CTRL + 2, 0);

	rtw_phy_init(rtwdev);

	rtw8821a_pwrtrack_init(rtwdev);

	/* 0x4c6[3] 1: RTS BW = Data BW
	 * 0: RTS BW depends on CCA / secondary CCA result.
	 */
	rtw_write8_clr(rtwdev, REG_QUEUE_CTRL, BIT(3));

	/* enable Tx report. */
	rtw_write8(rtwdev, REG_FWHW_TXQ_CTRL + 1, 0x0f);

	/* Pretx_en, for WEP/TKIP SEC */
	rtw_write8(rtwdev, REG_EARLY_MODE_CONTROL + 3, 0x01);

	rtw_write16(rtwdev, REG_TX_RPT_TIME, 0x3df0);

	/* Reset USB mode switch setting */
	rtw_write8(rtwdev, REG_SYS_SDIO_CTRL, 0x0);
	rtw_write8(rtwdev, REG_ACLK_MON, 0x0);

	rtw_write8(rtwdev, REG_USB_HRPWM, 0);

	/* ack for xmit mgmt frames. */
	rtw_write32_set(rtwdev, REG_FWHW_TXQ_CTRL, BIT(12));

	/// From Hal_PatchwithJaguar_8812:
	rtw_write8(rtwdev, 0x8c3, 0x3f);

	hal->cck_high_power = rtw_read32_mask(rtwdev, REG_CCK_RPT_FORMAT,
					      BIT_CCK_RPT_FORMAT);

	// rtw8821a_phy_bf_init(rtwdev);

	ret = rtw_hci_start(rtwdev);
	if (ret) {
		rtw_err(rtwdev, "failed to start hci\n");
		goto err_off;
	}

	wifi_only = !efuse->btcoex;
	if (efuse->btcoex) {
		rtw_coex_power_on_setting(rtwdev);
		rtw_coex_init_hw_config(rtwdev, wifi_only);
	}

	return 0;

err_off:
	rtw8821a_power_off(rtwdev);

err:
	return ret;
}

// static void rtw8821a_phy_bf_init(struct rtw_dev *rtwdev)
// {
// 	rtw_bf_phy_init(rtwdev);
// 	/* Grouping bitmap parameters */
// 	rtw_write32(rtwdev, 0x1C94, 0xAFFFAFFF);
// }

static int rtw8821a_mac_init(struct rtw_dev *rtwdev)
{
	rtw_err(rtwdev, "%s: useless function\n", __func__);
	return 0;
}

static u32 rtw8821a_phy_read_rf(struct rtw_dev *rtwdev,
				enum rtw_rf_path rf_path, u32 addr, u32 mask)
{
	static const u32 pi_addr[2] = { 0xc00, 0xe00 };
	static const u32 read_addr[2][2] = {
		{ REG_SI_READ_A, REG_SI_READ_B },
		{ REG_PI_READ_A, REG_PI_READ_B }
	};
	const struct rtw_chip_info *chip = rtwdev->chip;
	const struct rtw_hal *hal = &rtwdev->hal;
	bool set_cca, pi_mode;
	u32 val;

	if (rf_path >= hal->rf_phy_num) {
		rtw_err(rtwdev, "8821a unsupported rf path (%d)\n", rf_path);
		return INV_RF_DATA;
	}

	/* CCA off to avoid reading the wrong value.
	 * Toggling CCA would affect RF 0x0, skip it.
	 */
	set_cca = addr != 0x0 && chip->id == RTW_CHIP_TYPE_8812A &&
		  hal->cut_version != RTW_CHIP_VER_CUT_C;

	if (set_cca)
		rtw_write32_set(rtwdev, REG_CCA2ND, BIT(3));

	addr &= 0xff;

	pi_mode = rtw_read32_mask(rtwdev, pi_addr[rf_path], 0x4);

	rtw_write32_mask(rtwdev, REG_HSSI_READ, MASKBYTE0, addr);

	if (chip->id == RTW_CHIP_TYPE_8821A || hal->cut_version == RTW_CHIP_VER_CUT_C)
		udelay(20);

	val = rtw_read32_mask(rtwdev, read_addr[pi_mode][rf_path], mask);

	/* CCA on */
	if (set_cca)
		rtw_write32_clr(rtwdev, REG_CCA2ND, BIT(3));

	return val;
}

static void rtw8821a_cfg_ldo25(struct rtw_dev *rtwdev, bool enable)
{
	/* Nothing to do. */
}

static void rtw8821a_phy_set_param(struct rtw_dev *rtwdev)
{
	rtw_err(rtwdev, "%s: useless function\n", __func__);
}

static void rtw8812a_phy_fix_spur(struct rtw_dev *rtwdev, u8 channel, u8 bw)
{
	/* C cut Item12 ADC FIFO CLOCK */
	if (rtwdev->hal.cut_version == RTW_CHIP_VER_CUT_C) {
		if (bw == RTW_CHANNEL_WIDTH_40 && channel == 11)
			rtw_write32_mask(rtwdev, REG_ADCCLK, 0xC00, 0x3);
		else
			rtw_write32_mask(rtwdev, REG_ADCCLK, 0xC00, 0x2);

		/* A workarould to resolve 2480Mhz spur by setting ADC clock
		 * as 160M.
		 */
		if (bw == RTW_CHANNEL_WIDTH_20 && (channel == 13 || channel == 14)) {
			rtw_write32_mask(rtwdev, REG_ADCCLK, 0x300, 0x3);
			rtw_write32_mask(rtwdev, REG_ADC160, BIT(30), 1);
		} else if (bw == RTW_CHANNEL_WIDTH_40 && channel == 11) {
			rtw_write32_mask(rtwdev, REG_ADC160, BIT(30), 1);
		} else if (bw != RTW_CHANNEL_WIDTH_80) {
			rtw_write32_mask(rtwdev, REG_ADCCLK, 0x300, 0x2);
			rtw_write32_mask(rtwdev, REG_ADC160, BIT(30), 0);
		}
	} else {
		/* A workarould to resolve 2480Mhz spur by setting ADC clock
		 * as 160M.
		 */
		if (bw == RTW_CHANNEL_WIDTH_20 && (channel == 13 || channel == 14))
			rtw_write32_mask(rtwdev, REG_ADCCLK, 0x300, 0x3);
		else if (channel <= 14) /* 2.4G only */
			rtw_write32_mask(rtwdev, REG_ADCCLK, 0x300, 0x2);
	}
}

static void rtw8821a_switch_channel(struct rtw_dev *rtwdev, u8 channel, u8 bw)
{
	struct rtw_hal *hal = &rtwdev->hal;
	u32 fc_area, rf_mod_ag;
	u8 path;

	switch (channel) {
	case 36 ... 48:
		fc_area = 0x494;
		break;
	case 50 ... 64:
		fc_area = 0x453;
		break;
	case 100 ... 116:
		fc_area = 0x452;
		break;
	default:
		if (channel >= 118)
			fc_area = 0x412;
		else
			fc_area = 0x96a;
		break;
	}

	rtw_write32_mask(rtwdev, REG_CLKTRK, 0x1ffe0000, fc_area);

	for (path = 0; path < hal->rf_path_num; path++) {
		switch (channel) {
		case 36 ... 64:
			rf_mod_ag = 0x101;
			break;
		case 100 ... 140:
			rf_mod_ag = 0x301;
			break;
		default:
			if (channel > 140)
				rf_mod_ag = 0x501;
			else
				rf_mod_ag = 0x000;
			break;
		}

		rtw_write_rf(rtwdev, path, 0x18,
			     RF18_RFSI_MASK | RF18_BAND_MASK, rf_mod_ag);

		if (rtwdev->chip->id == RTW_CHIP_TYPE_8812A)
			rtw8812a_phy_fix_spur(rtwdev, channel, bw);

		rtw_write_rf(rtwdev, path, 0x18, RF18_CHANNEL_MASK, channel);
	}
}

static void rtw8821a_set_reg_bw(struct rtw_dev *rtwdev, u8 bw)
{
	u16 val16 = rtw_read16(rtwdev, REG_WMAC_TRXPTCL_CTL);

	val16 &= ~BIT_RFMOD;
	if (bw == RTW_CHANNEL_WIDTH_80)
		val16 |= BIT_RFMOD_80M;
	else if (bw == RTW_CHANNEL_WIDTH_40)
		val16 |= BIT_RFMOD_40M;

	rtw_write16(rtwdev, REG_WMAC_TRXPTCL_CTL, val16);
}

static void rtw8821a_post_set_bw_mode(struct rtw_dev *rtwdev, u8 channel, u8 bw,
				      u8 primary_chan_idx)
{
	struct rtw_hal *hal = &rtwdev->hal;
	u8 txsc40 = 0, txsc20, txsc;
	u8 reg_837, l1pkval;

	rtw8821a_set_reg_bw(rtwdev, bw);

	txsc20 = primary_chan_idx;
	if (bw == RTW_CHANNEL_WIDTH_80) {
		if (txsc20 == RTW_SC_20_UPPER || txsc20 == RTW_SC_20_UPMOST)
			txsc40 = RTW_SC_40_UPPER;
		else
			txsc40 = RTW_SC_40_LOWER;
	}

	txsc = BIT_TXSC_20M(txsc20) | BIT_TXSC_40M(txsc40);
	rtw_write8(rtwdev, REG_DATA_SC, txsc);

	reg_837 = rtw_read8(rtwdev, REG_BWINDICATION + 3);

	switch (bw) {
	default:
	case RTW_CHANNEL_WIDTH_20:
		rtw_write32_mask(rtwdev, REG_ADCCLK, 0x003003C3, 0x00300200);
		rtw_write32_mask(rtwdev, REG_ADC160, BIT(30), 0);

		if (hal->rf_type == RF_2T2R)
			rtw_write32_mask(rtwdev, REG_L1PKTH, 0x03C00000, 7);
		else
			rtw_write32_mask(rtwdev, REG_L1PKTH, 0x03C00000, 8);

		break;
	case RTW_CHANNEL_WIDTH_40:
		rtw_write32_mask(rtwdev, REG_ADCCLK, 0x003003C3, 0x00300201);
		rtw_write32_mask(rtwdev, REG_ADC160, BIT(30), 0);
		rtw_write32_mask(rtwdev, REG_ADCCLK, 0x3C, txsc);
		rtw_write32_mask(rtwdev, REG_CCA2ND, 0xf0000000, txsc);

		if (reg_837 & BIT(2)) {
			l1pkval = 6;
		} else {
			if (hal->rf_type == RF_2T2R)
				l1pkval = 7;
			else
				l1pkval = 8;
		}

		rtw_write32_mask(rtwdev, REG_L1PKTH, 0x03C00000, l1pkval);

		if (txsc == RTW_SC_20_UPPER)
			rtw_write32_set(rtwdev, REG_RXSB, BIT(4));
		else
			rtw_write32_clr(rtwdev, REG_RXSB, BIT(4));

		break;
	case RTW_CHANNEL_WIDTH_80:
		rtw_write32_mask(rtwdev, REG_ADCCLK, 0x003003C3, 0x00300202);
		rtw_write32_mask(rtwdev, REG_ADC160, BIT(30), 1);
		rtw_write32_mask(rtwdev, REG_ADCCLK, 0x3C, txsc);
		rtw_write32_mask(rtwdev, REG_CCA2ND, 0xf0000000, txsc);

		if (reg_837 & BIT(2)) {
			l1pkval = 5;
		} else {
			if (hal->rf_type == RF_2T2R)
				l1pkval = 6;
			else
				l1pkval = 7;
		}

		rtw_write32_mask(rtwdev, REG_L1PKTH, 0x03C00000, l1pkval);

		break;
	}
}

static void rtw8821a_set_channel_rf(struct rtw_dev *rtwdev, u8 channel, u8 bw)
{
	u8 path;

	for (path = RF_PATH_A; path < rtwdev->hal.rf_path_num; path++) {
		switch (bw) {
		case RTW_CHANNEL_WIDTH_5:
		case RTW_CHANNEL_WIDTH_10:
		case RTW_CHANNEL_WIDTH_20:
		default:
			rtw_write_rf(rtwdev, path, 0x18, RF18_BW_MASK, 3);
			break;
		case RTW_CHANNEL_WIDTH_40:
			rtw_write_rf(rtwdev, path, 0x18, RF18_BW_MASK, 1);
			break;
		case RTW_CHANNEL_WIDTH_80:
			rtw_write_rf(rtwdev, path, 0x18, RF18_BW_MASK, 0);
			break;
		}
	}
}

static void rtw8821a_set_channel(struct rtw_dev *rtwdev, u8 channel, u8 bw,
				 u8 primary_chan_idx)
{
	u8 old_band, new_band;

	if (rtw_read8(rtwdev, REG_CCK_CHECK) & BIT_CHECK_CCK_EN)
		old_band = RTW_BAND_5G;
	else
		old_band = RTW_BAND_2G;

	if (channel > 14)
		new_band = RTW_BAND_5G;
	else
		new_band = RTW_BAND_2G;

	if (new_band != old_band)
		rtw8821a_switch_band(rtwdev, new_band, bw);

	rtw8821a_switch_channel(rtwdev, channel, bw);

	rtw8821a_post_set_bw_mode(rtwdev, channel, bw, primary_chan_idx);

	if (rtwdev->chip->id == RTW_CHIP_TYPE_8812A)
		rtw8812a_phy_fix_spur(rtwdev, channel, bw);

	rtw8821a_set_channel_rf(rtwdev, channel, bw);
}

static s8 rtw8821a_cck_rx_pwr(struct rtw_dev *rtwdev, u8 lna_idx, u8 vga_idx)
{
	static const s8 lna_gain_table[] = {15, -1, -17, 0, -30, -38};
	s8 rx_pwr_all = 0;
	s8 lna_gain;

	switch (lna_idx) {
	case 5:
	case 4:
	case 2:
	case 1:
	case 0:
		lna_gain = lna_gain_table[lna_idx];
		rx_pwr_all = lna_gain - 2 * vga_idx;
		break;
	default:
		break;
	}

	return rx_pwr_all;
}

static s8 rtw8812a_cck_rx_pwr(struct rtw_dev *rtwdev, u8 lna_idx, u8 vga_idx)
{
	s8 rx_pwr_all = 0;

	switch (lna_idx) {
	case 7:
		if (vga_idx <= 27)
			rx_pwr_all = -94 + 2 * (27 - vga_idx);
		else
			rx_pwr_all = -94;
		break;
	case 6:
		rx_pwr_all = -42 + 2 * (2 - vga_idx);
		break;
	case 5:
		rx_pwr_all = -36 + 2 * (7 - vga_idx);
		break;
	case 4:
		rx_pwr_all = -30 + 2 * (7 - vga_idx);
		break;
	case 3:
		rx_pwr_all = -18 + 2 * (7 - vga_idx);
		break;
	case 2:
		rx_pwr_all = 2 * (5 - vga_idx);
		break;
	case 1:
		rx_pwr_all = 14 - 2 * vga_idx;
		break;
	case 0:
		rx_pwr_all = 20 - 2 * vga_idx;
		break;
	default:
		break;
	}

	return rx_pwr_all;
}

static void rtw8821a_query_phy_status(struct rtw_dev *rtwdev, u8 *phy_status,
				      struct rtw_rx_pkt_stat *pkt_stat)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	struct rtw8821a_phy_status_rpt *phy_sts = NULL;
	u8 lna_idx, vga_idx, cck_agc_rpt;
	const s8 min_rx_power = -120;
	u8 rssi, val, i;
	s8 rx_pwr_db, power_a, power_b;

	phy_sts = (struct rtw8821a_phy_status_rpt *)phy_status;

	if (pkt_stat->rate <= DESC_RATE11M) {
		cck_agc_rpt = phy_sts->cfosho[0];
		lna_idx = (cck_agc_rpt & 0xE0) >> 5;
		vga_idx = cck_agc_rpt & 0x1F;

		if (rtwdev->chip->id == RTW_CHIP_TYPE_8821A)
			rx_pwr_db = rtw8821a_cck_rx_pwr(rtwdev, lna_idx, vga_idx);
		else
			rx_pwr_db = rtw8812a_cck_rx_pwr(rtwdev, lna_idx, vga_idx);

		pkt_stat->rx_power[RF_PATH_A] = rx_pwr_db;
		pkt_stat->rssi = rtw_phy_rf_power_2_rssi(pkt_stat->rx_power, 1);
		dm_info->rssi[RF_PATH_A] = pkt_stat->rssi;
		pkt_stat->bw = RTW_CHANNEL_WIDTH_20;
		pkt_stat->signal_power = rx_pwr_db;

		if (rtwdev->chip->id == RTW_CHIP_TYPE_8812A &&
		    !rtwdev->hal.cck_high_power) {
			if (pkt_stat->rssi >= 80)
				pkt_stat->rssi = ((pkt_stat->rssi - 80) << 1) +
						 ((pkt_stat->rssi - 80) >> 1) + 80;
			else if (pkt_stat->rssi <= 78 && pkt_stat->rssi >= 20)
				pkt_stat->rssi += 3;
		}
	} else { /* OFDM rate */
		if (pkt_stat->rate >= DESC_RATEMCS0) {
			switch (phy_sts->r_rfmod) {
			case 1:
				if (phy_sts->sub_chnl == 0)
					pkt_stat->bw = 1;
				else
					pkt_stat->bw = 0;
				break;
			case 2:
				if (phy_sts->sub_chnl == 0)
					pkt_stat->bw = 2;
				else if (phy_sts->sub_chnl == 9 ||
					 phy_sts->sub_chnl == 10)
					pkt_stat->bw = 1;
				else
					pkt_stat->bw = 0;
				break;
			default:
			case 0:
				pkt_stat->bw = 0;
				break;
			}
		}

		for (i = RF_PATH_A; i < rtwdev->hal.rf_path_num; i++) {
			val = phy_sts->gain_trsw[i];
			pkt_stat->rx_power[i] = (val & 0x7F) - 110;
			rssi = rtw_phy_rf_power_2_rssi(&pkt_stat->rx_power[i], 1);
			dm_info->rssi[i] = rssi;
		}

		pkt_stat->rssi = rtw_phy_rf_power_2_rssi(pkt_stat->rx_power,
							 rtwdev->hal.rf_path_num);

		power_a = pkt_stat->rx_power[RF_PATH_A];
		power_b = pkt_stat->rx_power[RF_PATH_B];
		if (rtwdev->hal.rf_path_num == 1)
			power_b = power_a;

		pkt_stat->signal_power = max3(power_a, power_b, min_rx_power);
	}
}

static void rtw8821a_query_rx_desc(struct rtw_dev *rtwdev, u8 *rx_desc,
				   struct rtw_rx_pkt_stat *pkt_stat,
				   struct ieee80211_rx_status *rx_status)
{
	struct ieee80211_hdr *hdr;
	u32 desc_sz = rtwdev->chip->rx_pkt_desc_sz;
	u8 *phy_status = NULL;

	memset(pkt_stat, 0, sizeof(*pkt_stat));

	pkt_stat->phy_status = GET_RX_DESC_PHYST(rx_desc);
	pkt_stat->icv_err = GET_RX_DESC_ICV_ERR(rx_desc);
	pkt_stat->crc_err = GET_RX_DESC_CRC32(rx_desc);
	pkt_stat->decrypted = !GET_RX_DESC_SWDEC(rx_desc) &&
			      GET_RX_DESC_ENC_TYPE(rx_desc) != RX_DESC_ENC_NONE;
	pkt_stat->is_c2h = GET_RX_DESC_C2H(rx_desc);
	pkt_stat->pkt_len = GET_RX_DESC_PKT_LEN(rx_desc);
	pkt_stat->drv_info_sz = GET_RX_DESC_DRV_INFO_SIZE(rx_desc);
	pkt_stat->shift = GET_RX_DESC_SHIFT(rx_desc);
	pkt_stat->rate = GET_RX_DESC_RX_RATE(rx_desc);
	pkt_stat->cam_id = GET_RX_DESC_MACID(rx_desc);
	pkt_stat->ppdu_cnt = 0;
	pkt_stat->tsf_low = GET_RX_DESC_TSFL(rx_desc);

	/* drv_info_sz is in unit of 8-bytes */
	pkt_stat->drv_info_sz *= 8;

	/* c2h cmd pkt's rx/phy status is not interested */
	if (pkt_stat->is_c2h)
		return;

	hdr = (struct ieee80211_hdr *)(rx_desc + desc_sz + pkt_stat->shift +
				       pkt_stat->drv_info_sz);
	if (pkt_stat->phy_status) {
		phy_status = rx_desc + desc_sz + pkt_stat->shift;
		rtw8821a_query_phy_status(rtwdev, phy_status, pkt_stat);
	}

	rtw_rx_fill_rx_status(rtwdev, pkt_stat, hdr, rx_status, phy_status);
}

static void
rtw8821a_set_tx_power_index_by_rate(struct rtw_dev *rtwdev, u8 path,
				    u8 rs, u32 *phy_pwr_idx)
{
	struct rtw_hal *hal = &rtwdev->hal;
	static const u32 offset_txagc[2] = {0xc20, 0xe20};
	u8 rate, rate_idx, pwr_index, shift;
	bool write_1ss_mcs9;
	u32 mask;
	int j;

	for (j = 0; j < rtw_rate_size[rs]; j++) {
		rate = rtw_rate_section[rs][j];

		pwr_index = hal->tx_pwr_tbl[path][rate];

		shift = rate & 0x3;
		*phy_pwr_idx |= ((u32)pwr_index << (shift * 8));

		write_1ss_mcs9 = rate == DESC_RATEVHT1SS_MCS9 &&
				 hal->rf_path_num == 1;

		if (write_1ss_mcs9)
			mask = MASKLWORD;
		else
			mask = MASKDWORD;

		if (shift == 0x3 || write_1ss_mcs9) {
			rate_idx = rate & 0xfc;
			if (rate >= DESC_RATEVHT1SS_MCS0)
				rate_idx -= 0x10;

			rtw_write32_mask(rtwdev, offset_txagc[path] + rate_idx,
					 mask, *phy_pwr_idx);

			*phy_pwr_idx = 0;
		}
	}
}

static void rtw8821a_tx_power_training(struct rtw_dev *rtwdev, u8 bw,
				       u8 channel, u8 path)
{
	static const u32 write_offset[] = {
		REG_TX_PWR_TRAINING_A, REG_TX_PWR_TRAINING_B,
	};
	u32 power_level, write_data;
	u8 i;

	power_level = rtwdev->hal.tx_pwr_tbl[path][DESC_RATEMCS7];
	write_data = 0;

	for (i = 0; i < 3; i++) {
		if (i == 0)
			power_level -= 10;
		else if (i == 1)
			power_level -= 8;
		else
			power_level -= 6;

		write_data |= max_t(u32, power_level, 2) << (i * 8);
	}

	rtw_write32_mask(rtwdev, write_offset[path], 0xffffff, write_data);
}

static void rtw8821a_set_tx_power_index(struct rtw_dev *rtwdev)
{
	struct rtw_hal *hal = &rtwdev->hal;
	u32 phy_pwr_idx = 0;
	int rs, path;

	for (path = 0; path < hal->rf_path_num; path++) {
		for (rs = 0; rs < RTW_RATE_SECTION_MAX; rs++) {
			if (hal->rf_path_num == 1 &&
			    (rs == RTW_RATE_SECTION_HT_2S ||
			     rs == RTW_RATE_SECTION_VHT_2S))
				continue;

			if (test_bit(RTW_FLAG_SCANNING, rtwdev->flags) &&
			    rs > RTW_RATE_SECTION_OFDM)
				continue;

			if (hal->current_band_type == RTW_BAND_5G &&
			    rs == RTW_RATE_SECTION_CCK)
				continue;

			rtw8821a_set_tx_power_index_by_rate(rtwdev, path, rs,
							    &phy_pwr_idx);
		}

		rtw8821a_tx_power_training(rtwdev, hal->current_band_width,
					   hal->current_channel, path);
	}
}

static void rtw8821a_false_alarm_statistics(struct rtw_dev *rtwdev)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	u32 cck_enable;
	u32 cck_fa_cnt;
	u32 ofdm_fa_cnt;
	u32 crc32_cnt;
	u32 cca32_cnt;

	cck_enable = rtw_read32(rtwdev, REG_RXPSEL) & BIT(28);
	cck_fa_cnt = rtw_read16(rtwdev, REG_FA_CCK);
	ofdm_fa_cnt = rtw_read16(rtwdev, REG_FA_OFDM);

	dm_info->cck_fa_cnt = cck_fa_cnt;
	dm_info->ofdm_fa_cnt = ofdm_fa_cnt;
	dm_info->total_fa_cnt = ofdm_fa_cnt;
	if (cck_enable)
		dm_info->total_fa_cnt += cck_fa_cnt;

	crc32_cnt = rtw_read32(rtwdev, REG_CRC_CCK);
	dm_info->cck_ok_cnt = FIELD_GET(GENMASK(15, 0), crc32_cnt);
	dm_info->cck_err_cnt = FIELD_GET(GENMASK(31, 16), crc32_cnt);

	crc32_cnt = rtw_read32(rtwdev, REG_CRC_OFDM);
	dm_info->ofdm_ok_cnt = FIELD_GET(GENMASK(15, 0), crc32_cnt);
	dm_info->ofdm_err_cnt = FIELD_GET(GENMASK(31, 16), crc32_cnt);

	crc32_cnt = rtw_read32(rtwdev, REG_CRC_HT);
	dm_info->ht_ok_cnt = FIELD_GET(GENMASK(15, 0), crc32_cnt);
	dm_info->ht_err_cnt = FIELD_GET(GENMASK(31, 16), crc32_cnt);

	crc32_cnt = rtw_read32(rtwdev, REG_CRC_VHT);
	dm_info->vht_ok_cnt = FIELD_GET(GENMASK(15, 0), crc32_cnt);
	dm_info->vht_err_cnt = FIELD_GET(GENMASK(31, 16), crc32_cnt);

	cca32_cnt = rtw_read32(rtwdev, REG_CCA_OFDM);
	dm_info->ofdm_cca_cnt = FIELD_GET(GENMASK(31, 16), cca32_cnt);
	dm_info->total_cca_cnt = dm_info->ofdm_cca_cnt;
	if (cck_enable) {
		cca32_cnt = rtw_read32(rtwdev, REG_CCA_CCK);
		dm_info->cck_cca_cnt = FIELD_GET(GENMASK(15, 0), cca32_cnt);
		dm_info->total_cca_cnt += dm_info->cck_cca_cnt;
	}

	rtw_write32_set(rtwdev, REG_FAS, BIT(17));
	rtw_write32_clr(rtwdev, REG_FAS, BIT(17));
	rtw_write32_clr(rtwdev, REG_RXDESC, BIT(15));
	rtw_write32_set(rtwdev, REG_RXDESC, BIT(15));
	rtw_write32_set(rtwdev, REG_CNTRST, BIT(0));
	rtw_write32_clr(rtwdev, REG_CNTRST, BIT(0));
}

#define CAL_NUM_8821A 3
#define MACBB_REG_NUM_8821A 8
#define AFE_REG_NUM_8821A 4
#define RF_REG_NUM_8821A 3

static void rtw8821a_iqk_backup_mac_bb(struct rtw_dev *rtwdev,
				       u32 *macbb_backup,
				       const u32 *backup_macbb_reg,
				       u32 macbb_num)
{
	u32 i;

	/* [31] = 0 --> Page C */
	rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x0);

	/* save MACBB default value */
	for (i = 0; i < macbb_num; i++)
		macbb_backup[i] = rtw_read32(rtwdev, backup_macbb_reg[i]);
}

static void rtw8821a_iqk_backup_afe(struct rtw_dev *rtwdev, u32 *afe_backup,
				    const u32 *backup_afe_reg, u32 afe_num)
{
	u32 i;

	/* [31] = 0 --> Page C */
	rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x0);

	/* Save AFE Parameters */
	for (i = 0; i < afe_num; i++)
		afe_backup[i] = rtw_read32(rtwdev, backup_afe_reg[i]);
}

static void rtw8821a_iqk_backup_rf(struct rtw_dev *rtwdev, u32 *rfa_backup,
				   const u32 *backup_rf_reg, u32 rf_num)
{
	u32 i;

	/* [31] = 0 --> Page C */
	rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x0);

	/* Save RF Parameters */
	for (i = 0; i < rf_num; i++)
		rfa_backup[i] = rtw_read_rf(rtwdev, RF_PATH_A,
					    backup_rf_reg[i], MASKDWORD);
}

static void rtw8821a_iqk_restore_rf(struct rtw_dev *rtwdev,
				    const u32 *backup_rf_reg,
				    u32 *RF_backup, u32 rf_reg_num)
{
	u32 i;

	/* [31] = 0 --> Page C */
	rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x0);

	for (i = 0; i < rf_reg_num; i++)
		rtw_write_rf(rtwdev, RF_PATH_A, backup_rf_reg[i],
			     RFREG_MASK, RF_backup[i]);
}

static void rtw8821a_iqk_restore_afe(struct rtw_dev *rtwdev, u32 *afe_backup,
				     const u32 *backup_afe_reg, u32 afe_num)
{
	u32 i;

	/* [31] = 0 --> Page C */
	rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x0);

	/* Reload AFE Parameters */
	for (i = 0; i < afe_num; i++)
		rtw_write32(rtwdev, backup_afe_reg[i], afe_backup[i]);

	/* [31] = 1 --> Page C1 */
	rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x1);

	rtw_write32(rtwdev, 0xc80, 0x0);
	rtw_write32(rtwdev, 0xc84, 0x0);
	rtw_write32(rtwdev, 0xc88, 0x0);
	rtw_write32(rtwdev, 0xc8c, 0x3c000000);
	rtw_write32(rtwdev, REG_LSSI_WRITE_A, 0x00000080);
	rtw_write32(rtwdev, 0xc94, 0x00000000);
	rtw_write32(rtwdev, 0xcc4, 0x20040000);
	rtw_write32(rtwdev, 0xcc8, 0x20000000);
	rtw_write32(rtwdev, REG_RFECTL, 0x0);
}

static void rtw8821a_iqk_restore_mac_bb(struct rtw_dev *rtwdev,
					u32 *macbb_backup,
					const u32 *backup_macbb_reg,
					u32 macbb_num)
{
	u32 i;

	/* [31] = 0 --> Page C */
	rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x0);

	/* Reload MacBB Parameters */
	for (i = 0; i < macbb_num; i++)
		rtw_write32(rtwdev, backup_macbb_reg[i], macbb_backup[i]);
}

static void rtw8821a_iqk_configure_mac(struct rtw_dev *rtwdev)
{
	/* [31] = 0 --> Page C */
	rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x0);

	rtw_write8(rtwdev, REG_TXPAUSE, 0x3f);
	rtw_write32_mask(rtwdev, REG_BCN_CTRL,
			 (BIT_EN_BCN_FUNCTION << 8) | BIT_EN_BCN_FUNCTION, 0x0);

	/* RX ante off */
	rtw_write8(rtwdev, REG_RXPSEL, 0x00);

	/* CCA off */
	rtw_write32_mask(rtwdev, REG_CCA2ND, 0xf, 0xc);

	/* CCK RX path off */
	rtw_write8(rtwdev, REG_CCK_RX + 3, 0xf);
}

static void rtw8821a_iqk_rx_fill(struct rtw_dev *rtwdev, u8 path,
				 unsigned int rx_x, unsigned int rx_y)
{
	if (path != RF_PATH_A)
		return;

	/* [31] = 0 --> Page C */
	rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x0);

	rtw_write32_mask(rtwdev, REG_RX_IQC_AB_A,
			 0x000003ff, rx_x >> 1);
	rtw_write32_mask(rtwdev, REG_RX_IQC_AB_A,
			 0x03ff0000, (rx_y >> 1) & 0x3ff);
}

static void rtw8821a_iqk_tx_fill(struct rtw_dev *rtwdev, u8 path,
				 unsigned int tx_x, unsigned int tx_y)
{
	if (path != RF_PATH_A)
		return;

	/* [31] = 1 --> Page C1 */
	rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x1);

	rtw_write32(rtwdev, REG_LSSI_WRITE_A, 0x00000080);
	rtw_write32(rtwdev, 0xcc4, 0x20040000);
	rtw_write32(rtwdev, 0xcc8, 0x20000000);
	rtw_write32_mask(rtwdev, 0xccc, 0x000007ff, tx_y);
	rtw_write32_mask(rtwdev, 0xcd4, 0x000007ff, tx_x);
}

static void rtw8821a_iqk(struct rtw_dev *rtwdev)
{
	const struct rtw_efuse *efuse = &rtwdev->efuse;
	const struct rtw_hal *hal = &rtwdev->hal;
	u32 tx_fail, rx_fail, delay_count, iqk_ready, cal_retry, cal = 0;
	int tx_x = 0, tx_y = 0, rx_x = 0, rx_y = 0;
	int tx_average = 0, rx_average = 0, rx_iqk_loop = 0;
	int rx_x_temp = 0, rx_y_temp = 0;
	int rx_x0[2][CAL_NUM_8821A];
	int rx_y0[2][CAL_NUM_8821A];
	int tx_x0[CAL_NUM_8821A];
	int tx_y0[CAL_NUM_8821A];
	bool tx0iqkok = false, rx0iqkok = false;
	bool vdf_enable;
	int i, k, vdf_Y[3], vdf_X[3], tx_dt[3], ii, dx = 0, dy = 0;
	int tx_finish = 0, rx_finish1 = 0, rx_finish2 = 0;
	const u8 path = RF_PATH_A;

	rtw_dbg(rtwdev, RTW_DBG_RFK,
		"band_width = %d, ext_pa = %d, ext_pa_5g = %d\n",
		hal->current_band_width, efuse->ext_pa_2g, efuse->ext_pa_5g);

	vdf_enable = hal->current_band_width == RTW_CHANNEL_WIDTH_80;

	while (cal < CAL_NUM_8821A) {
		/* path-A LOK */
		/* [31] = 0 --> Page C */
		rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x0);

		/* ========path-A AFE all on======== */
		/* Port 0 DAC/ADC on */
		rtw_write32(rtwdev, REG_AFE_PWR1_A, 0x77777777);
		rtw_write32(rtwdev, REG_AFE_PWR2_A, 0x77777777);

		rtw_write32(rtwdev, REG_RX_WAIT_CCA_TX_CCK_RFON_A, 0x19791979);

		/* hardware 3-wire off */
		rtw_write32_mask(rtwdev, 0xc00, 0xf, 0x4);

		/* LOK setting */
		/* ====== LOK ====== */
		/* 1. DAC/ADC sampling rate (160 MHz) */
		rtw_write32_mask(rtwdev, 0xc5c, GENMASK(26, 24), 0x7);

		/* 2. LoK RF setting (at BW = 20M) */
		rtw_write_rf(rtwdev, path, RF_LUTWE, RFREG_MASK, 0x80002);
		rtw_write_rf(rtwdev, path, RF_CFGCH, 0x00c00, 0x3);
		rtw_write_rf(rtwdev, path, RF_MODE_TABLE_ADDR, RFREG_MASK, 0x20000);
		rtw_write_rf(rtwdev, path, RF_MODE_TABLE_DATA0, RFREG_MASK, 0x0003f);
		rtw_write_rf(rtwdev, path, RF_MODE_TABLE_DATA1, RFREG_MASK, 0xf3fc3);

		rtw_write_rf(rtwdev, path, 0x65, RFREG_MASK, 0x931d5);
		rtw_write_rf(rtwdev, path, 0x8f, RFREG_MASK, 0x8a001);
		rtw_write32(rtwdev, 0x90c, 0x00008000);
		rtw_write32_mask(rtwdev, 0xc94, BIT(0), 0x1);
		/* TX (X,Y) */
		rtw_write32(rtwdev, 0x978, 0x29002000);
		/* RX (X,Y) */
		rtw_write32(rtwdev, 0x97c, 0xa9002000);
		/* [0]:AGC_en, [15]:idac_K_Mask */
		rtw_write32(rtwdev, 0x984, 0x00462910);

		/* [31] = 1 --> Page C1 */
		rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x1);

		if (efuse->ext_pa_5g)
			rtw_write32(rtwdev, 0xc88, 0x821403f7);
		else
			rtw_write32(rtwdev, 0xc88, 0x821403f4);

		if (hal->current_band_type == RTW_BAND_5G)
			rtw_write32(rtwdev, 0xc8c, 0x68163e96);
		else
			rtw_write32(rtwdev, 0xc8c, 0x28163e96);

		/* TX_Tone_idx[9:0], TxK_Mask[29] TX_Tone = 16 */
		rtw_write32(rtwdev, 0xc80, 0x18008c10);
		/* RX_Tone_idx[9:0], RxK_Mask[29] */
		rtw_write32(rtwdev, 0xc84, 0x38008c10);
		rtw_write32(rtwdev, REG_RFECTL, 0x00100000);
		rtw_write32(rtwdev, 0x980, 0xfa000000);
		rtw_write32(rtwdev, 0x980, 0xf8000000);

		mdelay(10); /* delay 10ms */
		rtw_write32(rtwdev, REG_RFECTL, 0x00000000);

		/* [31] = 0 --> Page C */
		rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x0);
		rtw_write_rf(rtwdev, path, 0x58, 0x7fe00,
			     rtw_read_rf(rtwdev, path, RF_DTXLOK, 0xffc00));

		if (hal->current_band_width == RTW_CHANNEL_WIDTH_40)
			rtw_write_rf(rtwdev, path, RF_CFGCH, RF18_BW_MASK, 0x1);
		else if (hal->current_band_width == RTW_CHANNEL_WIDTH_80)
			rtw_write_rf(rtwdev, path, RF_CFGCH, RF18_BW_MASK, 0x0);

		/* [31] = 1 --> Page C1 */
		rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x1);

		/* 3. TX RF setting */
		/* [31] = 0 --> Page C */
		rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x0);
		rtw_write_rf(rtwdev, path, RF_LUTWE, RFREG_MASK, 0x80000);
		rtw_write_rf(rtwdev, path, RF_MODE_TABLE_ADDR, RFREG_MASK, 0x20000);
		rtw_write_rf(rtwdev, path, RF_MODE_TABLE_DATA0, RFREG_MASK, 0x0003f);
		rtw_write_rf(rtwdev, path, RF_MODE_TABLE_DATA1, RFREG_MASK, 0xf3fc3);

		rtw_write_rf(rtwdev, path, 0x65, RFREG_MASK, 0x931d5);
		rtw_write_rf(rtwdev, path, 0x8f, RFREG_MASK, 0x8a001);
		rtw_write_rf(rtwdev, path, RF_LUTWE, RFREG_MASK, 0x00000);
		rtw_write32(rtwdev, 0x90c, 0x00008000);
		rtw_write32_mask(rtwdev, 0xc94, BIT(0), 0x1);
		/* TX (X,Y) */
		rtw_write32(rtwdev, 0x978, 0x29002000);
		/* RX (X,Y) */
		rtw_write32(rtwdev, 0x97c, 0xa9002000);
		/* [0]:AGC_en, [15]:idac_K_Mask */
		rtw_write32(rtwdev, 0x984, 0x0046a910);

		/* [31] = 1 --> Page C1 */
		rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x1);

		if (efuse->ext_pa_5g)
			rtw_write32(rtwdev, 0xc88, 0x821403f7);
		else
			rtw_write32(rtwdev, 0xc88, 0x821403e3);

		if (hal->current_band_type == RTW_BAND_5G)
			rtw_write32(rtwdev, 0xc8c, 0x40163e96);
		else
			rtw_write32(rtwdev, 0xc8c, 0x00163e96);

		if (vdf_enable == 1) {
			for (k = 0; k <= 2; k++) {
				switch (k) {
				case 0:
					/* TX_Tone_idx[9:0], TxK_Mask[29] TX_Tone = 16 */
					rtw_write32(rtwdev, 0xc80, 0x18008c38);
					/* RX_Tone_idx[9:0], RxK_Mask[29] */
					rtw_write32(rtwdev, 0xc84, 0x38008c38);
					rtw_write32_mask(rtwdev, 0xce8, BIT(31), 0x0);
					break;
				case 1:
					rtw_write32_mask(rtwdev, 0xc80, BIT(28), 0x0);
					rtw_write32_mask(rtwdev, 0xc84, BIT(28), 0x0);
					rtw_write32_mask(rtwdev, 0xce8, BIT(31), 0x0);
					break;
				case 2:
					rtw_dbg(rtwdev, RTW_DBG_RFK, "vdf_Y[1] = %x;;;vdf_Y[0] = %x\n", vdf_Y[1] >> 21 & 0x00007ff, vdf_Y[0] >> 21 & 0x00007ff);

					rtw_dbg(rtwdev, RTW_DBG_RFK, "vdf_X[1] = %x;;;vdf_X[0] = %x\n", vdf_X[1] >> 21 & 0x00007ff, vdf_X[0] >> 21 & 0x00007ff);

					tx_dt[cal] = (vdf_Y[1] >> 20) - (vdf_Y[0] >> 20);
					tx_dt[cal] = ((16 * tx_dt[cal]) * 10000 / 15708);
					tx_dt[cal] = (tx_dt[cal] >> 1) + (tx_dt[cal] & BIT(0));

					/* TX_Tone_idx[9:0], TxK_Mask[29] TX_Tone = 16 */
					rtw_write32(rtwdev, 0xc80, 0x18008c20);
					/* RX_Tone_idx[9:0], RxK_Mask[29] */
					rtw_write32(rtwdev, 0xc84, 0x38008c20);
					rtw_write32_mask(rtwdev, 0xce8, BIT(31), 0x1);
					rtw_write32_mask(rtwdev, 0xce8, 0x3fff0000, tx_dt[cal] & 0x00003fff);
					break;
				}

				rtw_write32(rtwdev, REG_RFECTL, 0x00100000);
				cal_retry = 0;
				while (1) {
					/* one shot */
					rtw_write32(rtwdev, 0x980, 0xfa000000);
					rtw_write32(rtwdev, 0x980, 0xf8000000);

					mdelay(10);

					rtw_write32(rtwdev, REG_RFECTL, 0x00000000);

					delay_count = 0;
					while (1) {
						iqk_ready = rtw_read32_mask(rtwdev, 0xd00, BIT(10));

						/* Originally: if (~iqk_ready || delay_count > 20)
						 * that looks like a typo so make it more explicit
						 */
						iqk_ready = true;

						if (iqk_ready || delay_count > 20)
							break;

						mdelay(1);
						delay_count++;
					}

					if (delay_count < 20) {
						/* ============TXIQK Check============== */
						tx_fail = rtw_read32_mask(rtwdev, 0xd00, BIT(12));

						/* Originally: if (~tx_fail) {
						 * It looks like a typo, so make it more explicit.
						 */
						tx_fail = false;

						if (!tx_fail) {
							rtw_write32(rtwdev, REG_RFECTL, 0x02000000);
							vdf_X[k] = rtw_read32_mask(rtwdev, 0xd00, 0x07ff0000) << 21;

							rtw_write32(rtwdev, REG_RFECTL, 0x04000000);
							vdf_Y[k] = rtw_read32_mask(rtwdev, 0xd00, 0x07ff0000) << 21;

							tx0iqkok = true;
							break;
						}

						rtw_write32_mask(rtwdev, 0xccc, 0x000007ff, 0x0);
						rtw_write32_mask(rtwdev, 0xcd4, 0x000007ff, 0x200);

						tx0iqkok = false;
						cal_retry++;
						if (cal_retry == 10)
							break;

					} else { /* If 20ms No Result, then cal_retry++ */
						tx0iqkok = false;
						cal_retry++;
						if (cal_retry == 10)
							break;
					}
				}
			}

			if (k == 3) {
				tx_x0[cal] = vdf_X[k - 1];
				tx_y0[cal] = vdf_Y[k - 1];
			}
		} else {
			/* TX_Tone_idx[9:0], TxK_Mask[29] TX_Tone = 16 */
			rtw_write32(rtwdev, 0xc80, 0x18008c10);
			/* RX_Tone_idx[9:0], RxK_Mask[29] */
			rtw_write32(rtwdev, 0xc84, 0x38008c10);
			rtw_write32(rtwdev, 0xcb8, 0x00100000);

			cal_retry = 0;
			while (1) {
				/* one shot */
				rtw_write32(rtwdev, 0x980, 0xfa000000);
				rtw_write32(rtwdev, 0x980, 0xf8000000);

				mdelay(10);
				rtw_write32(rtwdev, REG_RFECTL, 0x00000000);

				delay_count = 0;
				while (1) {
					iqk_ready = rtw_read32_mask(rtwdev, 0xd00, BIT(10));

					/* Originally: if (~iqk_ready || delay_count > 20)
					 * that looks like a typo so make it more explicit
					 */
					iqk_ready = true;

					if (iqk_ready || delay_count > 20)
						break;

					mdelay(1);
					delay_count++;
				}

				if (delay_count < 20) {
					/* ============TXIQK Check============== */
					tx_fail = rtw_read32_mask(rtwdev, 0xd00, BIT(12));

					/* Originally: if (~tx_fail) {
					 * It looks like a typo, so make it more explicit.
					 */
					tx_fail = false;

					if (!tx_fail) {
						rtw_write32(rtwdev, REG_RFECTL, 0x02000000);
						tx_x0[cal] = rtw_read32_mask(rtwdev, 0xd00, 0x07ff0000) << 21;

						rtw_write32(rtwdev, REG_RFECTL, 0x04000000);
						tx_y0[cal] = rtw_read32_mask(rtwdev, 0xd00, 0x07ff0000) << 21;

						tx0iqkok = true;
						break;
					}

					rtw_write32_mask(rtwdev, 0xccc, 0x000007ff, 0x0);
					rtw_write32_mask(rtwdev, 0xcd4, 0x000007ff, 0x200);

					tx0iqkok = false;
					cal_retry++;
					if (cal_retry == 10)
						break;
				} else { /* If 20ms No Result, then cal_retry++ */
					tx0iqkok = false;
					cal_retry++;
					if (cal_retry == 10)
						break;
				}
			}
		}

		if (tx0iqkok == false)
			break; /* TXK fail, Don't do RXK */

		/* ====== RX IQK ====== */
		/* [31] = 0 --> Page C */
		rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x0);
		/* 1. RX RF setting */
		rtw_write_rf(rtwdev, path, RF_LUTWE, RFREG_MASK, 0x80000);
		rtw_write_rf(rtwdev, path, RF_MODE_TABLE_ADDR, RFREG_MASK, 0x30000);
		rtw_write_rf(rtwdev, path, RF_MODE_TABLE_DATA0, RFREG_MASK, 0x0002f);
		rtw_write_rf(rtwdev, path, RF_MODE_TABLE_DATA1, RFREG_MASK, 0xfffbb);
		rtw_write_rf(rtwdev, path, 0x8f, RFREG_MASK, 0x88001);
		rtw_write_rf(rtwdev, path, 0x65, RFREG_MASK, 0x931d8);
		rtw_write_rf(rtwdev, path, RF_LUTWE, RFREG_MASK, 0x00000);
///TODO: it makes no sense to shift this left by 21 every time you want to use it
/// just do it once
		rtw_write32_mask(rtwdev, 0x978, 0x03FF8000, (tx_x0[cal]) >> 21 & 0x000007ff);
		rtw_write32_mask(rtwdev, 0x978, 0x000007FF, (tx_y0[cal]) >> 21 & 0x000007ff);
		rtw_write32_mask(rtwdev, 0x978, BIT(31), 0x1);
		rtw_write32_mask(rtwdev, 0x97c, BIT(31), 0x0);
		rtw_write32(rtwdev, 0x90c, 0x00008000);
		rtw_write32(rtwdev, 0x984, 0x0046a911);

		/* [31] = 1 --> Page C1 */
		rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x1);
		/* TX_Tone_idx[9:0], TxK_Mask[29] TX_Tone = 16 */
		rtw_write32(rtwdev, 0xc80, 0x38008c10);
		/* RX_Tone_idx[9:0], RxK_Mask[29] */
		rtw_write32(rtwdev, 0xc84, 0x18008c10);
		rtw_write32(rtwdev, 0xc88, 0x02140119);

		if (rtw_hci_type(rtwdev) == RTW_HCI_TYPE_PCIE) {
			rx_iqk_loop = 2; /* for 2% fail; */
		} else
			rx_iqk_loop = 1;

		for (i = 0; i < rx_iqk_loop; i++) {
			if (rtw_hci_type(rtwdev) == RTW_HCI_TYPE_PCIE) {
				if (i == 0)
					rtw_write32(rtwdev, 0xc8c, 0x28161100); /* Good */
				else
					rtw_write32(rtwdev, 0xc8c, 0x28160d00);
			} else {
				rtw_write32(rtwdev, 0xc8c, 0x28160d00);
			}

			rtw_write32(rtwdev, REG_RFECTL, 0x00100000);

			cal_retry = 0;
			while (1) {
				/* one shot */
				rtw_write32(rtwdev, 0x980, 0xfa000000);
				rtw_write32(rtwdev, 0x980, 0xf8000000);

				mdelay(10);

				rtw_write32(rtwdev, REG_RFECTL, 0x00000000);

				delay_count = 0;
				while (1) {
					iqk_ready = rtw_read32_mask(rtwdev, 0xd00, BIT(10));

					/* Originally: if (~iqk_ready || delay_count > 20)
					 * that looks like a typo so make it more explicit
					 */
					iqk_ready = true;

					if (iqk_ready || delay_count > 20)
						break;

					mdelay(1);
					delay_count++;
				}

				if (delay_count < 20) {
					/* ============RXIQK Check============== */
					rx_fail = rtw_read32_mask(rtwdev, 0xd00, BIT(11));
					if (rx_fail == 0) {
						rtw_write32(rtwdev, REG_RFECTL, 0x06000000);
						rx_x0[i][cal] = rtw_read32_mask(rtwdev, 0xd00, 0x07ff0000) << 21;

						rtw_write32(rtwdev, REG_RFECTL, 0x08000000);
						rx_y0[i][cal] = rtw_read32_mask(rtwdev, 0xd00, 0x07ff0000) << 21;

						rx0iqkok = true;
						break;
					}

					rtw_write32_mask(rtwdev, REG_RX_IQC_AB_A, 0x000003ff, 0x200 >> 1);
					rtw_write32_mask(rtwdev, REG_RX_IQC_AB_A, 0x03ff0000, 0x0 >> 1);

					rx0iqkok = false;
					cal_retry++;
					if (cal_retry == 10)
						break;
				} else { /* If 20ms No Result, then cal_retry++ */
					rx0iqkok = false;
					cal_retry++;
					if (cal_retry == 10)
						break;
				}
			}
		}

		if (tx0iqkok)
			tx_average++;
		if (rx0iqkok)
			rx_average++;

		cal++;
	}

	/* FillIQK Result */

	if (tx_average == 0)
		return;

	for (i = 0; i < tx_average; i++)
		rtw_dbg(rtwdev, RTW_DBG_RFK,
			"tx_x0[%d] = %x ;; tx_y0[%d] = %x\n", i,
			tx_x0[i] >> 21 & 0x000007ff, i,
			tx_y0[i] >> 21 & 0x000007ff);

	for (i = 0; i < tx_average; i++) {
		for (ii = i + 1; ii < tx_average; ii++) {
			dx = (tx_x0[i] >> 21) - (tx_x0[ii] >> 21);

			if (dx < 3 && dx > -3) {
				dy = (tx_y0[i] >> 21) - (tx_y0[ii] >> 21);

				if (dy < 3 && dy > -3) {
					tx_x = ((tx_x0[i] >> 21) + (tx_x0[ii] >> 21)) / 2;
					tx_y = ((tx_y0[i] >> 21) + (tx_y0[ii] >> 21)) / 2;

					tx_finish = 1;
					break;
				}
			}
		}

		if (tx_finish == 1)
			break;
	}

	if (tx_finish == 1)
		rtw8821a_iqk_tx_fill(rtwdev, path, tx_x, tx_y);
	else
		rtw8821a_iqk_tx_fill(rtwdev, path, 0x200, 0x0);

	if (rx_average == 0)
		return;

	for (i = 0; i < rx_average; i++) {
		rtw_dbg(rtwdev, RTW_DBG_RFK,
			"rx_x0[0][%d] = %x ;; rx_y0[0][%d] = %x\n", i,
			rx_x0[0][i] >> 21 & 0x000007ff, i,
			rx_y0[0][i] >> 21 & 0x000007ff);

		if (rx_iqk_loop == 2)
			rtw_dbg(rtwdev, RTW_DBG_RFK,
				"rx_x0[1][%d] = %x ;; rx_y0[1][%d] = %x\n",
				i, rx_x0[1][i] >> 21 & 0x000007ff, i,
				rx_y0[1][i] >> 21 & 0x000007ff);
	}

	for (i = 0; i < rx_average; i++) {
		for (ii = i + 1; ii < rx_average; ii++) {
			dx = (rx_x0[0][i] >> 21) - (rx_x0[0][ii] >> 21);

			if (dx < 4 && dx > -4) {
				dy = (rx_y0[0][i] >> 21) - (rx_y0[0][ii] >> 21);

				if (dy < 4 && dy > -4) {
					rx_x_temp = ((rx_x0[0][i] >> 21) + (rx_x0[0][ii] >> 21)) / 2;
					rx_y_temp = ((rx_y0[0][i] >> 21) + (rx_y0[0][ii] >> 21)) / 2;

					rx_finish1 = 1;
					break;
				}
			}
		}

		if (rx_finish1 == 1) {
			rx_x = rx_x_temp;
			rx_y = rx_y_temp;
			break;
		}
	}

	if (rx_iqk_loop == 2) {
		for (i = 0; i < rx_average; i++) {
			for (ii = i + 1; ii < rx_average; ii++) {
				dx = (rx_x0[1][i] >> 21) - (rx_x0[1][ii] >> 21);

				if (dx < 4 && dx > -4) {
					dy = (rx_y0[1][i] >> 21) - (rx_y0[1][ii] >> 21);

					if (dy < 4 && dy > -4) {
						rx_x = ((rx_x0[1][i] >> 21) + (rx_x0[1][ii] >> 21)) / 2;
						rx_y = ((rx_y0[1][i] >> 21) + (rx_y0[1][ii] >> 21)) / 2;

						rx_finish2 = 1;
						break;
					}
				}
			}

			if (rx_finish2 == 1)
				break;
		}

		if (rx_finish1 && rx_finish2) {
			rx_x = (rx_x + rx_x_temp) / 2;
			rx_y = (rx_y + rx_y_temp) / 2;
		}
	}

	if (rx_finish1 || rx_finish2)
		rtw8821a_iqk_rx_fill(rtwdev, path, rx_x, rx_y);
	else
		rtw8821a_iqk_rx_fill(rtwdev, path, 0x200, 0x0);
}

static void rtw8821a_do_iqk(struct rtw_dev *rtwdev)
{
	static const u32 backup_macbb_reg[MACBB_REG_NUM_8821A] = {
		0x520, 0x550, 0x808, 0xa04, 0x90c, 0xc00, 0x838, 0x82c
	};
	static const u32 backup_afe_reg[AFE_REG_NUM_8821A] = {
		0xc5c, 0xc60, 0xc64, 0xc68
	};
	static const u32 backup_rf_reg[RF_REG_NUM_8821A] = {
		0x65, 0x8f, 0x0
	};
	u32 macbb_backup[MACBB_REG_NUM_8821A];
	u32 afe_backup[AFE_REG_NUM_8821A];
	u32 rfa_backup[RF_REG_NUM_8821A];

	rtw8821a_iqk_backup_mac_bb(rtwdev, macbb_backup,
				   backup_macbb_reg, MACBB_REG_NUM_8821A);
	rtw8821a_iqk_backup_afe(rtwdev, afe_backup,
				backup_afe_reg, AFE_REG_NUM_8821A);
	rtw8821a_iqk_backup_rf(rtwdev, rfa_backup,
			       backup_rf_reg, RF_REG_NUM_8821A);

	rtw8821a_iqk_configure_mac(rtwdev);

	rtw8821a_iqk(rtwdev);

	rtw8821a_iqk_restore_rf(rtwdev, backup_rf_reg,
				rfa_backup, RF_REG_NUM_8821A);
	rtw8821a_iqk_restore_afe(rtwdev, afe_backup,
				 backup_afe_reg, AFE_REG_NUM_8821A);
	rtw8821a_iqk_restore_mac_bb(rtwdev, macbb_backup,
				    backup_macbb_reg, MACBB_REG_NUM_8821A);
}

static void rtw8812a_do_iqk(struct rtw_dev *rtwdev)
{

}

static void rtw8821a_phy_calibration(struct rtw_dev *rtwdev)
{
	if (rtwdev->chip->id == RTW_CHIP_TYPE_8821A)
		rtw8821a_do_iqk(rtwdev);
	else
		rtw8812a_do_iqk(rtwdev);
}

static void rtw8812a_do_lck(struct rtw_dev *rtwdev)
{
	u32 cont_tx, lc_cal, i;

	cont_tx = rtw_read32_mask(rtwdev, REG_SINGLE_TONE_CONT_TX, 0x70000);

	lc_cal = rtw_read_rf(rtwdev, RF_PATH_A, RF_CFGCH, RFREG_MASK);

	if (!cont_tx)
		rtw_write8(rtwdev, REG_TXPAUSE, 0xff);

	rtw_write_rf(rtwdev, RF_PATH_A, RF_LCK, BIT(14), 1);

	rtw_write_rf(rtwdev, RF_PATH_A, RF_CFGCH, 0x08000, 1);

	mdelay(150);

	for (i = 0; i < 5; i++) {
		if (rtw_read_rf(rtwdev, RF_PATH_A, RF_CFGCH, 0x08000) != 1)
			break;

		mdelay(10);
	}

	if (i == 5)
		rtw_dbg(rtwdev, RTW_DBG_RFK, "LCK timed out\n");

	rtw_write_rf(rtwdev, RF_PATH_A, RF_CFGCH, RFREG_MASK, lc_cal);

	rtw_write_rf(rtwdev, RF_PATH_A, RF_LCK, BIT(14), 0);

	if (!cont_tx)
		rtw_write8(rtwdev, REG_TXPAUSE, 0);

	rtw_write_rf(rtwdev, RF_PATH_A, RF_CFGCH, RFREG_MASK, lc_cal);
}

/* for coex */
static void rtw8821a_coex_cfg_init(struct rtw_dev *rtwdev)
{
// 	/* enable TBTT nterrupt */
// 	rtw_write8_set(rtwdev, REG_BCN_CTRL, BIT_EN_BCN_FUNCTION);
//
// 	/* BT report packet sample rate */
// 	rtw_write8_mask(rtwdev, REG_BT_TDMA_TIME, BIT_MASK_SAMPLE_RATE, 0x5);
//
// 	/* enable BT counter statistics */
// 	rtw_write8(rtwdev, REG_BT_STAT_CTRL, BT_CNT_ENABLE);
//
// 	/* enable PTA (3-wire function form BT side) */
// 	rtw_write32_set(rtwdev, REG_GPIO_MUXCFG, BIT_BT_PTA_EN);
// 	rtw_write32_set(rtwdev, REG_GPIO_MUXCFG, BIT_PO_BT_PTA_PINS);
//
// 	/* enable PTA (tx/rx signal form WiFi side) */
// 	rtw_write8_set(rtwdev, REG_QUEUE_CTRL, BIT_PTA_WL_TX_EN);
// 	/* wl tx signal to PTA not case EDCCA */
// 	rtw_write8_clr(rtwdev, REG_QUEUE_CTRL, BIT_PTA_EDCCA_EN);
// 	/* GNT_BT=1 while select both */
// 	rtw_write16_set(rtwdev, REG_BT_COEX_V2, BIT_GNT_BT_POLARITY);
//
// 	/* beacon queue always hi-pri  */
// 	rtw_write8_mask(rtwdev, REG_BT_COEX_TABLE_H + 3, BIT_BCN_QUEUE,
// 			BCN_PRI_EN);
}

static void rtw8821a_coex_cfg_ant_switch(struct rtw_dev *rtwdev, u8 ctrl_type,
					 u8 pos_type)
{
// 	struct rtw_coex *coex = &rtwdev->coex;
// 	struct rtw_coex_dm *coex_dm = &coex->dm;
// 	struct rtw_coex_rfe *coex_rfe = &coex->rfe;
// 	u32 switch_status = FIELD_PREP(CTRL_TYPE_MASK, ctrl_type) | pos_type;
// 	bool polarity_inverse;
// 	u8 regval = 0;
//
// 	if (switch_status == coex_dm->cur_switch_status)
// 		return;
//
// 	if (coex_rfe->wlg_at_btg) {
// 		ctrl_type = COEX_SWITCH_CTRL_BY_BBSW;
//
// 		if (coex_rfe->ant_switch_polarity)
// 			pos_type = COEX_SWITCH_TO_WLA;
// 		else
// 			pos_type = COEX_SWITCH_TO_WLG_BT;
// 	}
//
// 	coex_dm->cur_switch_status = switch_status;
//
// 	if (coex_rfe->ant_switch_diversity &&
// 	    ctrl_type == COEX_SWITCH_CTRL_BY_BBSW)
// 		ctrl_type = COEX_SWITCH_CTRL_BY_ANTDIV;
//
// 	polarity_inverse = (coex_rfe->ant_switch_polarity == 1);
//
// 	switch (ctrl_type) {
// 	default:
// 	case COEX_SWITCH_CTRL_BY_BBSW:
// 		rtw_write32_clr(rtwdev, REG_LED_CFG, BIT_DPDT_SEL_EN);
// 		rtw_write32_set(rtwdev, REG_LED_CFG, BIT_DPDT_WL_SEL);
// 		/* BB SW, DPDT use RFE_ctrl8 and RFE_ctrl9 as ctrl pin */
// 		rtw_write8_mask(rtwdev, REG_RFE_CTRL8, BIT_MASK_RFE_SEL89,
// 				DPDT_CTRL_PIN);
//
// 		if (pos_type == COEX_SWITCH_TO_WLG_BT) {
// 			if (coex_rfe->rfe_module_type != 0x4 &&
// 			    coex_rfe->rfe_module_type != 0x2)
// 				regval = 0x3;
// 			else
// 				regval = (!polarity_inverse ? 0x2 : 0x1);
// 		} else if (pos_type == COEX_SWITCH_TO_WLG) {
// 			regval = (!polarity_inverse ? 0x2 : 0x1);
// 		} else {
// 			regval = (!polarity_inverse ? 0x1 : 0x2);
// 		}
//
// 		rtw_write32_mask(rtwdev, REG_RFE_CTRL8, BIT_MASK_R_RFE_SEL_15,
// 				 regval);
// 		break;
// 	case COEX_SWITCH_CTRL_BY_PTA:
// 		rtw_write32_clr(rtwdev, REG_LED_CFG, BIT_DPDT_SEL_EN);
// 		rtw_write32_set(rtwdev, REG_LED_CFG, BIT_DPDT_WL_SEL);
// 		/* PTA,  DPDT use RFE_ctrl8 and RFE_ctrl9 as ctrl pin */
// 		rtw_write8_mask(rtwdev, REG_RFE_CTRL8, BIT_MASK_RFE_SEL89,
// 				PTA_CTRL_PIN);
//
// 		regval = (!polarity_inverse ? 0x2 : 0x1);
// 		rtw_write32_mask(rtwdev, REG_RFE_CTRL8, BIT_MASK_R_RFE_SEL_15,
// 				 regval);
// 		break;
// 	case COEX_SWITCH_CTRL_BY_ANTDIV:
// 		rtw_write32_clr(rtwdev, REG_LED_CFG, BIT_DPDT_SEL_EN);
// 		rtw_write32_set(rtwdev, REG_LED_CFG, BIT_DPDT_WL_SEL);
// 		rtw_write8_mask(rtwdev, REG_RFE_CTRL8, BIT_MASK_RFE_SEL89,
// 				ANTDIC_CTRL_PIN);
// 		break;
// 	case COEX_SWITCH_CTRL_BY_MAC:
// 		rtw_write32_set(rtwdev, REG_LED_CFG, BIT_DPDT_SEL_EN);
//
// 		regval = (!polarity_inverse ? 0x0 : 0x1);
// 		rtw_write8_mask(rtwdev, REG_PAD_CTRL1, BIT_SW_DPDT_SEL_DATA,
// 				regval);
// 		break;
// 	case COEX_SWITCH_CTRL_BY_FW:
// 		rtw_write32_clr(rtwdev, REG_LED_CFG, BIT_DPDT_SEL_EN);
// 		rtw_write32_set(rtwdev, REG_LED_CFG, BIT_DPDT_WL_SEL);
// 		break;
// 	case COEX_SWITCH_CTRL_BY_BT:
// 		rtw_write32_clr(rtwdev, REG_LED_CFG, BIT_DPDT_SEL_EN);
// 		rtw_write32_clr(rtwdev, REG_LED_CFG, BIT_DPDT_WL_SEL);
// 		break;
// 	}
//
// 	if (ctrl_type == COEX_SWITCH_CTRL_BY_BT) {
// 		rtw_write8_clr(rtwdev, REG_CTRL_TYPE, BIT_CTRL_TYPE1);
// 		rtw_write8_clr(rtwdev, REG_CTRL_TYPE, BIT_CTRL_TYPE2);
// 	} else {
// 		rtw_write8_set(rtwdev, REG_CTRL_TYPE, BIT_CTRL_TYPE1);
// 		rtw_write8_set(rtwdev, REG_CTRL_TYPE, BIT_CTRL_TYPE2);
// 	}
}

static void rtw8821a_coex_cfg_gnt_fix(struct rtw_dev *rtwdev)
{}

static void rtw8821a_coex_cfg_gnt_debug(struct rtw_dev *rtwdev)
{
// 	rtw_write32_clr(rtwdev, REG_PAD_CTRL1, BIT_BTGP_SPI_EN);
// 	rtw_write32_clr(rtwdev, REG_PAD_CTRL1, BIT_BTGP_JTAG_EN);
// 	rtw_write32_clr(rtwdev, REG_GPIO_MUXCFG, BIT_FSPI_EN);
// 	rtw_write32_clr(rtwdev, REG_PAD_CTRL1, BIT_LED1DIS);
// 	rtw_write32_clr(rtwdev, REG_SYS_SDIO_CTRL, BIT_SDIO_INT);
// 	rtw_write32_clr(rtwdev, REG_SYS_SDIO_CTRL, BIT_DBG_GNT_WL_BT);
}

static void rtw8821a_coex_cfg_rfe_type(struct rtw_dev *rtwdev)
{
// 	struct rtw_coex *coex = &rtwdev->coex;
// 	struct rtw_coex_rfe *coex_rfe = &coex->rfe;
// 	struct rtw_efuse *efuse = &rtwdev->efuse;
//
// 	coex_rfe->rfe_module_type = efuse->rfe_option;
// 	coex_rfe->ant_switch_polarity = 0;
// 	coex_rfe->ant_switch_exist = true;
// 	coex_rfe->wlg_at_btg = false;
//
// 	switch (coex_rfe->rfe_module_type) {
// 	case 0:
// 	case 8:
// 	case 1:
// 	case 9:  /* 1-Ant, Main, WLG */
// 	default: /* 2-Ant, DPDT, WLG */
// 		break;
// 	case 2:
// 	case 10: /* 1-Ant, Main, BTG */
// 	case 7:
// 	case 15: /* 2-Ant, DPDT, BTG */
// 		coex_rfe->wlg_at_btg = true;
// 		break;
// 	case 3:
// 	case 11: /* 1-Ant, Aux, WLG */
// 		coex_rfe->ant_switch_polarity = 1;
// 		break;
// 	case 4:
// 	case 12: /* 1-Ant, Aux, BTG */
// 		coex_rfe->wlg_at_btg = true;
// 		coex_rfe->ant_switch_polarity = 1;
// 		break;
// 	case 5:
// 	case 13: /* 2-Ant, no switch, WLG */
// 	case 6:
// 	case 14: /* 2-Ant, no antenna switch, WLG */
// 		coex_rfe->ant_switch_exist = false;
// 		break;
// 	}
}

static void rtw8821a_coex_cfg_wl_tx_power(struct rtw_dev *rtwdev, u8 wl_pwr)
{
// 	struct rtw_coex *coex = &rtwdev->coex;
// 	struct rtw_coex_dm *coex_dm = &coex->dm;
// 	struct rtw_efuse *efuse = &rtwdev->efuse;
// 	bool share_ant = efuse->share_ant;
//
// 	if (share_ant)
// 		return;
//
// 	if (wl_pwr == coex_dm->cur_wl_pwr_lvl)
// 		return;
//
// 	coex_dm->cur_wl_pwr_lvl = wl_pwr;
}

static void rtw8821a_coex_cfg_wl_rx_gain(struct rtw_dev *rtwdev, bool low_gain)
{}

static void rtw8821a_pwrtrack_set(struct rtw_dev *rtwdev, u8 tx_rate, u8 path)
{
	static const u32 reg_txscale[2] = { REG_TXSCALE_A, REG_TXSCALE_B };
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	u8 cck_swing_idx, ofdm_swing_idx;
	u8 pwr_tracking_limit;

	switch (tx_rate) {
	case DESC_RATE1M ... DESC_RATE11M:
		pwr_tracking_limit = 32;
		break;
	case DESC_RATE6M ... DESC_RATE48M:
	case DESC_RATEMCS3 ... DESC_RATEMCS4:
	case DESC_RATEMCS11 ... DESC_RATEMCS12:
	case DESC_RATEVHT1SS_MCS3 ... DESC_RATEVHT1SS_MCS4:
	case DESC_RATEVHT2SS_MCS3 ... DESC_RATEVHT2SS_MCS4:
		pwr_tracking_limit = 30;
		break;
	case DESC_RATE54M:
	case DESC_RATEMCS5 ... DESC_RATEMCS7:
	case DESC_RATEMCS13 ... DESC_RATEMCS15:
	case DESC_RATEVHT1SS_MCS5 ... DESC_RATEVHT1SS_MCS6:
	case DESC_RATEVHT2SS_MCS5 ... DESC_RATEVHT2SS_MCS6:
		pwr_tracking_limit = 28;
		break;
	case DESC_RATEMCS0 ... DESC_RATEMCS2:
	case DESC_RATEMCS8 ... DESC_RATEMCS10:
	case DESC_RATEVHT1SS_MCS0 ... DESC_RATEVHT1SS_MCS2:
	case DESC_RATEVHT2SS_MCS0 ... DESC_RATEVHT2SS_MCS2:
		pwr_tracking_limit = 34;
		break;
	case DESC_RATEVHT1SS_MCS7:
	case DESC_RATEVHT2SS_MCS7:
		pwr_tracking_limit = 26;
		break;
	default:
	case DESC_RATEVHT1SS_MCS8:
	case DESC_RATEVHT2SS_MCS8:
		pwr_tracking_limit = 24;
		break;
	case DESC_RATEVHT1SS_MCS9:
	case DESC_RATEVHT2SS_MCS9:
		pwr_tracking_limit = 22;
		break;
	}

	cck_swing_idx = dm_info->delta_power_index[path] + dm_info->default_cck_index;
	ofdm_swing_idx = dm_info->delta_power_index[path] + dm_info->default_ofdm_index;

	if (ofdm_swing_idx > pwr_tracking_limit) {
		if (path == RF_PATH_A)
			dm_info->txagc_remnant_cck = cck_swing_idx - pwr_tracking_limit;
		dm_info->txagc_remnant_ofdm[path] = ofdm_swing_idx - pwr_tracking_limit;

		ofdm_swing_idx = pwr_tracking_limit;
	} else if (ofdm_swing_idx == 0) {
		if (path == RF_PATH_A)
			dm_info->txagc_remnant_cck = cck_swing_idx;
		dm_info->txagc_remnant_ofdm[path] = ofdm_swing_idx;
	} else {
		if (path == RF_PATH_A)
			dm_info->txagc_remnant_cck = 0;
		dm_info->txagc_remnant_ofdm[path] = 0;
	}

	rtw_write32_mask(rtwdev, reg_txscale[path], GENMASK(31, 21),
			 rtw8821a_txscale_tbl[ofdm_swing_idx]);
}

static void rtw8821a_phy_pwrtrack(struct rtw_dev *rtwdev)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	struct rtw_swing_table swing_table;
	u8 thermal_value, delta, path;
	bool need_iqk;

	rtw_phy_config_swing_table(rtwdev, &swing_table);

	if (rtwdev->efuse.thermal_meter[0] == 0xff) {
		pr_err_once("efuse thermal meter is 0xff\n");
		return;
	}

	thermal_value = rtw_read_rf(rtwdev, RF_PATH_A, RF_T_METER, 0xfc00);

	rtw_phy_pwrtrack_avg(rtwdev, thermal_value, RF_PATH_A);

	need_iqk = rtw_phy_pwrtrack_need_iqk(rtwdev);

	if (rtwdev->chip->id == RTW_CHIP_TYPE_8812A) {
		if (need_iqk)
			rtw8812a_do_lck(rtwdev);
	}

	if (dm_info->pwr_trk_init_trigger)
		dm_info->pwr_trk_init_trigger = false;
	else if (!rtw_phy_pwrtrack_thermal_changed(rtwdev, thermal_value,
						   RF_PATH_A))
		goto iqk;

	delta = rtw_phy_pwrtrack_get_delta(rtwdev, RF_PATH_A);

	for (path = RF_PATH_A; path < rtwdev->hal.rf_path_num; path++) {
		dm_info->delta_power_index[path] =
			rtw_phy_pwrtrack_get_pwridx(rtwdev, &swing_table, path,
						    RF_PATH_A, delta);

		if (dm_info->delta_power_index[path] !=
				dm_info->delta_power_index_last[path]) {
			dm_info->delta_power_index_last[path] =
				dm_info->delta_power_index[path];

			rtw8821a_pwrtrack_set(rtwdev, dm_info->tx_rate, path);
		}
	}

	rtw_phy_set_tx_power_level(rtwdev, rtwdev->hal.current_channel);

iqk:
	if (need_iqk)
		rtw8821a_do_iqk(rtwdev);
}

static void rtw8821a_pwr_track(struct rtw_dev *rtwdev)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;

	if (!dm_info->pwr_trk_triggered) {
		rtw_write_rf(rtwdev, RF_PATH_A, RF_T_METER,
			     GENMASK(17, 16), 0x03);
		dm_info->pwr_trk_triggered = true;
		return;
	}

	rtw8821a_phy_pwrtrack(rtwdev);
	dm_info->pwr_trk_triggered = false;
}

// static void rtw8821a_bf_config_bfee_su(struct rtw_dev *rtwdev,
// 				       struct rtw_vif *vif,
// 				       struct rtw_bfee *bfee, bool enable)
// {
// 	if (enable)
// 		rtw_bf_enable_bfee_su(rtwdev, vif, bfee);
// 	else
// 		rtw_bf_remove_bfee_su(rtwdev, bfee);
// }

// static void rtw8821a_bf_config_bfee_mu(struct rtw_dev *rtwdev,
// 				       struct rtw_vif *vif,
// 				       struct rtw_bfee *bfee, bool enable)
// {
// 	if (enable)
// 		rtw_bf_enable_bfee_mu(rtwdev, vif, bfee);
// 	else
// 		rtw_bf_remove_bfee_mu(rtwdev, bfee);
// }

static void rtw8821a_bf_config_bfee(struct rtw_dev *rtwdev, struct rtw_vif *vif,
				    struct rtw_bfee *bfee, bool enable)
{
// 	if (bfee->role == RTW_BFEE_SU)
// 		rtw8821a_bf_config_bfee_su(rtwdev, vif, bfee, enable);
// 	else if (bfee->role == RTW_BFEE_MU)
// 		rtw8821a_bf_config_bfee_mu(rtwdev, vif, bfee, enable);
// 	else
// 		rtw_warn(rtwdev, "wrong bfee role\n");
}

static void rtw8821a_phy_cck_pd_set(struct rtw_dev *rtwdev, u8 new_lvl)
{
	static const u8 pd[CCK_PD_LV_MAX] = {0x40, 0x83, 0xcd, 0xdd, 0xed};
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;

	/* Override rtw_phy_cck_pd_lv_link(). It implements something
	 * like type 2/3/4. We need type 1 here.
	 */
	if (rtw_is_assoc(rtwdev)) {
		if (dm_info->min_rssi > 60) {
			new_lvl = CCK_PD_LV3;
		} else if (dm_info->min_rssi > 35) {
			new_lvl = CCK_PD_LV2;
		} else if (dm_info->min_rssi > 20) {
			if (dm_info->cck_fa_avg > 500)
				new_lvl = CCK_PD_LV2;
			else if (dm_info->cck_fa_avg < 250)
				new_lvl = CCK_PD_LV1;
			else
				return;
		} else {
			new_lvl = CCK_PD_LV1;
		}
	}

	rtw_dbg(rtwdev, RTW_DBG_PHY, "lv: (%d) -> (%d)\n",
		dm_info->cck_pd_lv[RTW_CHANNEL_WIDTH_20][RF_PATH_A], new_lvl);

	if (dm_info->cck_pd_lv[RTW_CHANNEL_WIDTH_20][RF_PATH_A] == new_lvl)
		return;

	dm_info->cck_fa_avg = CCK_FA_AVG_RESET;
	dm_info->cck_pd_lv[RTW_CHANNEL_WIDTH_20][RF_PATH_A] = new_lvl;

	rtw_write8(rtwdev, 0xa0a, pd[new_lvl]);
}

static void rtw8821a_fill_txdesc_checksum(struct rtw_dev *rtwdev,
					  struct rtw_tx_pkt_info *pkt_info,
					  u8 *txdesc)
{
	fill_txdesc_checksum_common(txdesc, 16);
}

static const struct rtw_intf_phy_para usb2_param_8821a[] = {
	{0xFFFF, 0x00,
	 RTW_IP_SEL_PHY,
	 RTW_INTF_PHY_CUT_ALL,
	 RTW_INTF_PHY_PLATFORM_ALL},
};

static const struct rtw_intf_phy_para usb3_param_8821a[] = {
	{0xFFFF, 0x0000,
	 RTW_IP_SEL_PHY,
	 RTW_INTF_PHY_CUT_ALL,
	 RTW_INTF_PHY_PLATFORM_ALL},
};

static const struct rtw_intf_phy_para_table phy_para_table_8821a = {
	.usb2_para	= usb2_param_8821a,
	.usb3_para	= usb3_param_8821a,
	.n_usb2_para	= ARRAY_SIZE(usb2_param_8821a),
	.n_usb3_para	= ARRAY_SIZE(usb2_param_8821a),
};

static const struct rtw_rfe_def rtw8821a_rfe_defs[] = {
	[0] = { .phy_pg_tbl	= &rtw8821a_bb_pg_tbl,
		.txpwr_lmt_tbl	= &rtw8821a_txpwr_lmt_tbl, },
};

static const struct rtw_rfe_def rtw8812a_rfe_defs[] = {
	[0] = { .phy_pg_tbl	= &rtw8812a_bb_pg_tbl,
		.txpwr_lmt_tbl	= &rtw8812a_txpwr_lmt_tbl, },
};

static struct rtw_hw_reg rtw8821a_dig[] = {
	[0] = { .addr = 0xc50, .mask = 0x7f },
	[1] = { .addr = 0xe50, .mask = 0x7f },
};

static struct rtw_page_table page_table_8821a[] = {
	/* hq_num, nq_num, lq_num, exq_num, gapq_num */
	{0, 0, 0, 0, 0},	// unused by USB
	{0, 0, 0, 0, 0},	// unused by USB
	{8, 0, 0, 0, 1},	// 2 bulk out endpoints
	{8, 0, 8, 0, 1},	// 3 bulk out endpoints
	{8, 0, 8, 4, 1},	// 4 bulk out endpoints
};

static struct rtw_page_table page_table_8812a[] = {
	/* hq_num, nq_num, lq_num, exq_num, gapq_num */
	{0, 0, 0, 0, 0},	// unused by USB
	{0, 0, 0, 0, 0},	// unused by USB
	{16, 0, 0, 0, 1},	// 2 bulk out endpoints
	{16, 0, 16, 0, 1},	// 3 bulk out endpoints
	{16, 0, 16, 0, 1},	// 4 bulk out endpoints
};

static struct rtw_rqpn rqpn_table_8821a[] = {
	{RTW_DMA_MAPPING_NORMAL, RTW_DMA_MAPPING_NORMAL,
	 RTW_DMA_MAPPING_LOW, RTW_DMA_MAPPING_LOW,
	 RTW_DMA_MAPPING_EXTRA, RTW_DMA_MAPPING_HIGH},

	{RTW_DMA_MAPPING_NORMAL, RTW_DMA_MAPPING_NORMAL,
	 RTW_DMA_MAPPING_LOW, RTW_DMA_MAPPING_LOW,
	 RTW_DMA_MAPPING_EXTRA, RTW_DMA_MAPPING_HIGH},

	{RTW_DMA_MAPPING_HIGH, RTW_DMA_MAPPING_HIGH,
	 RTW_DMA_MAPPING_NORMAL, RTW_DMA_MAPPING_NORMAL,
	 RTW_DMA_MAPPING_HIGH, RTW_DMA_MAPPING_HIGH},

	{RTW_DMA_MAPPING_HIGH, RTW_DMA_MAPPING_NORMAL,
	 RTW_DMA_MAPPING_LOW, RTW_DMA_MAPPING_LOW,
	 RTW_DMA_MAPPING_HIGH, RTW_DMA_MAPPING_HIGH},

	{RTW_DMA_MAPPING_NORMAL, RTW_DMA_MAPPING_NORMAL,
	 RTW_DMA_MAPPING_LOW, RTW_DMA_MAPPING_LOW,
	 RTW_DMA_MAPPING_EXTRA, RTW_DMA_MAPPING_HIGH},
};

static struct rtw_prioq_addrs prioq_addrs_8821a = {
	.prio[RTW_DMA_MAPPING_EXTRA] = {
		.rsvd = REG_RQPN_NPQ + 2, .avail = REG_RQPN_NPQ + 3,
	},
	.prio[RTW_DMA_MAPPING_LOW] = {
		.rsvd = REG_RQPN + 1, .avail = REG_FIFOPAGE_CTRL_2 + 1,
	},
	.prio[RTW_DMA_MAPPING_NORMAL] = {
		.rsvd = REG_RQPN_NPQ, .avail = REG_RQPN_NPQ + 1,
	},
	.prio[RTW_DMA_MAPPING_HIGH] = {
		.rsvd = REG_RQPN, .avail = REG_FIFOPAGE_CTRL_2,
	},
	.wsize = false,
};

static struct rtw_chip_ops rtw8821a_ops = {
	.power_on		= rtw8821a_power_on,
	.power_off		= rtw8821a_power_off,
	.phy_set_param		= rtw8821a_phy_set_param,	///
	.read_efuse		= rtw8821a_read_efuse,		///
	.query_rx_desc		= rtw8821a_query_rx_desc,	///
	.set_channel		= rtw8821a_set_channel,		///
	.mac_init		= rtw8821a_mac_init,		///
	.llt_init_legacy	= rtw_llt_init_legacy_old,	///
	.read_rf		= rtw8821a_phy_read_rf,		///
	.write_rf		= rtw_phy_write_rf_reg_sipi,	///
	.set_antenna		= NULL,				///
	.set_tx_power_index	= rtw8821a_set_tx_power_index,
	.cfg_ldo25		= rtw8821a_cfg_ldo25,		///
	.efuse_grant		= rtw8821a_efuse_grant,		///
	.false_alarm_statistics	= rtw8821a_false_alarm_statistics,
	.phy_calibration	= rtw8821a_phy_calibration,
	.cck_pd_set		= rtw8821a_phy_cck_pd_set,
	.pwr_track		= rtw8821a_pwr_track,
	.config_bfee		= rtw8821a_bf_config_bfee,
	.set_gid_table		= rtw_bf_set_gid_table,
	.cfg_csi_rate		= rtw_bf_cfg_csi_rate,
	.fill_txdesc_checksum	= rtw8821a_fill_txdesc_checksum,///

	.coex_set_init		= rtw8821a_coex_cfg_init,
	.coex_set_ant_switch	= rtw8821a_coex_cfg_ant_switch,
	.coex_set_gnt_fix	= rtw8821a_coex_cfg_gnt_fix,
	.coex_set_gnt_debug	= rtw8821a_coex_cfg_gnt_debug,
	.coex_set_rfe_type	= rtw8821a_coex_cfg_rfe_type,
	.coex_set_wl_tx_power	= rtw8821a_coex_cfg_wl_tx_power,
	.coex_set_wl_rx_gain	= rtw8821a_coex_cfg_wl_rx_gain,
};

///TODO
/* rssi in percentage % (dbm = % - 100) */
static const u8 wl_rssi_step_8821a[] = {101, 45, 101, 40};
static const u8 bt_rssi_step_8821a[] = {101, 101, 101, 101};

/* Shared-Antenna Coex Table */
static const struct coex_table_para table_sant_8821a[] = {
///TODO: 8821au driver has only cases 0-8, and 19. What do we do about the rest? Cases 9-18 copied from 8821c.
	{0x55555555, 0x55555555}, /* case-0 */
	{0x55555555, 0x5a5a5a5a},
	{0x5a5a5a5a, 0x5a5a5a5a},
	{0x5a5a5a5a, 0xaaaaaaaa},
	{0x55555555, 0x5a5a5a5a},
	{0x5a5a5a5a, 0xaaaa5a5a}, /* case-5 */
	{0x55555555, 0xaaaa5a5a},
	{0xaaaaaaaa, 0xaaaaaaaa},
	{0x55555555, 0xaaaaaaaa},
	{0x66555555, 0x5a5a5a5a},
	{0x66555555, 0x6a5a5a5a}, /* case-10 */
	{0x66555555, 0xaaaaaaaa},
	{0x66555555, 0x6a5a5aaa},
	{0x66555555, 0x6aaa6aaa},
	{0x66555555, 0x6a5a5aaa},
	{0x66555555, 0xaaaaaaaa}, /* case-15 */
	{0xffff55ff, 0xfafafafa},
	{0xffff55ff, 0x6afa5afa},
	{0xaaffffaa, 0xfafafafa},
	{0xa5555555, 0x5a5a5a5a},
};

/* Non-Shared-Antenna Coex Table */
static const struct coex_table_para table_nsant_8821a[] = {///done
	{0x55555555, 0x55555555}, /* case-100 */
	{0x55555555, 0x5afa5afa},
	{0x5ada5ada, 0x5ada5ada},
	{0xaaaaaaaa, 0xaaaaaaaa},
	{0xffffffff, 0xffffffff},
	{0x5fff5fff, 0x5fff5fff}, /* case-105 */
	{0x55ff55ff, 0x5a5a5a5a},
	{0x55dd55dd, 0x5ada5ada},
	{0x55dd55dd, 0x5ada5ada},
	{0x55dd55dd, 0x5ada5ada},
	{0x55dd55dd, 0x5ada5ada}, /* case-110 */
	{0x55dd55dd, 0x5ada5ada},
	{0x55dd55dd, 0x5ada5ada},
	{0x5fff5fff, 0xaaaaaaaa},
	{0x5fff5fff, 0x5ada5ada},
	{0x55dd55dd, 0xaaaaaaaa}, /* case-115 */
	{0x5fdf5fdf, 0x5fdb5fdb},
	{0xfafafafa, 0xfafafafa},
	{0x5555555f, 0x5ada5ada},
	{0x55555555, 0x5a5a5a5a},
	{0x55555555, 0x5a5a5a5a}, /* case-120 */
	{0xaaffffaa, 0x5a5a5a5a},
	{0xffff55ff, 0xfafafafa},
	{0x55555555, 0x5afa5afa},
};

/* Shared-Antenna TDMA */
static const struct coex_tdma_para tdma_sant_8821a[] = {///done
	{ {0x00, 0x00, 0x00, 0x00, 0x00} }, /* case-0 */
	{ {0x51, 0x3a, 0x03, 0x10, 0x50} },
	{ {0x51, 0x2b, 0x03, 0x10, 0x50} },
	{ {0x51, 0x1d, 0x1d, 0x00, 0x52} },
	{ {0x93, 0x15, 0x03, 0x14, 0x00} },
	{ {0x61, 0x15, 0x03, 0x11, 0x10} },
	{ {0x61, 0x20, 0x03, 0x11, 0x13} },
	{ {0x13, 0x0c, 0x05, 0x00, 0x00} },
	{ {0x93, 0x25, 0x03, 0x10, 0x00} },
	{ {0x51, 0x21, 0x03, 0x10, 0x50} },
	{ {0x51, 0x30, 0x03, 0x10, 0x50} },
	{ {0x51, 0x15, 0x03, 0x10, 0x50} },
	{ {0x51, 0x0a, 0x0a, 0x00, 0x50} },
	{ {0x51, 0x10, 0x07, 0x10, 0x54} },
	{ {0x51, 0x1e, 0x03, 0x10, 0x14} },
	{ {0x13, 0x0a, 0x03, 0x08, 0x00} },
	{ {0x61, 0x10, 0x03, 0x11, 0x15} },
	{ {0x51, 0x1a, 0x1a, 0x00, 0x50} },
	{ {0x93, 0x25, 0x03, 0x10, 0x00} },
	{ {0x51, 0x1a, 0x1a, 0x00, 0x50} },
	{ {0x61, 0x35, 0x03, 0x11, 0x10} },
	{ {0x61, 0x30, 0x03, 0x11, 0x10} },
	{ {0x61, 0x25, 0x03, 0x11, 0x10} },
	{ {0x61, 0x10, 0x03, 0x11, 0x10} },
	{ {0xe3, 0x15, 0x03, 0x31, 0x18} },
	{ {0xe3, 0x0a, 0x03, 0x31, 0x18} },
	{ {0xe3, 0x0a, 0x03, 0x31, 0x18} },
	{ {0xe3, 0x25, 0x03, 0x31, 0x98} },
	{ {0x69, 0x25, 0x03, 0x31, 0x00} },
	{ {0xab, 0x1a, 0x1a, 0x01, 0x10} },
	{ {0x51, 0x30, 0x03, 0x10, 0x10} },
	{ {0xd3, 0x1a, 0x1a, 0x00, 0x58} },
	{ {0x61, 0x35, 0x03, 0x11, 0x11} },
	{ {0xa3, 0x25, 0x03, 0x30, 0x90} },
	{ {0x53, 0x1a, 0x1a, 0x00, 0x10} },
	{ {0x63, 0x1a, 0x1a, 0x00, 0x10} },
	{ {0xd3, 0x12, 0x03, 0x14, 0x50} },
	{ {0x51, 0x10, 0x03, 0x10, 0x54} },
	{ {0x61, 0x10, 0x03, 0x11, 0x54} },
	{ {0x51, 0x1a, 0x1a, 0x00, 0x50} },
	{ {0x23, 0x18, 0x00, 0x10, 0x24} },
	{ {0x51, 0x15, 0x03, 0x11, 0x11} },
	{ {0x51, 0x20, 0x03, 0x11, 0x11} },
	{ {0x51, 0x30, 0x03, 0x10, 0x11} },
	{ {0x51, 0x20, 0x03, 0x10, 0x14} },
	{ {0x51, 0x25, 0x03, 0x10, 0x10} },
	{ {0x51, 0x35, 0x03, 0x10, 0x10} },
};

/* Non-Shared-Antenna TDMA */
static const struct coex_tdma_para tdma_nsant_8821a[] = {///done
	{ {0x00, 0x00, 0x00, 0x40, 0x00} }, /* case-100 */
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x2d, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x1c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x10, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x3c, 0x03, 0x70, 0x90} },
	{ {0xe3, 0x2d, 0x03, 0x70, 0x90} },
	{ {0xe3, 0x1c, 0x03, 0x70, 0x90} },
	{ {0xa3, 0x10, 0x03, 0x70, 0x90} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x2d, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x1c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x10, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x3c, 0x03, 0x70, 0x90} },
	{ {0xe3, 0x2d, 0x03, 0x70, 0x90} },
	{ {0xe3, 0x1c, 0x03, 0x70, 0x90} },
	{ {0xe3, 0x10, 0x03, 0x70, 0x90} },
	{ {0xa3, 0x2f, 0x2f, 0x60, 0x90} },
	{ {0xe3, 0x05, 0x05, 0xe1, 0x90} },
	{ {0xe3, 0x25, 0x25, 0xe1, 0x90} },
	{ {0xe3, 0x25, 0x25, 0x60, 0x90} },
	{ {0xe3, 0x15, 0x03, 0x70, 0x90} },
	{ {0xe3, 0x30, 0x03, 0x71, 0x10} },
	{ {0xe3, 0x1e, 0x03, 0xf1, 0x94} },
	{ {0xe3, 0x25, 0x03, 0x71, 0x11} },
	{ {0xe3, 0x14, 0x03, 0xf1, 0x90} },
	{ {0xd3, 0x30, 0x03, 0x70, 0x50} },
	{ {0xd3, 0x23, 0x03, 0x70, 0x50} },
	{ {0xe3, 0x1e, 0x03, 0xf1, 0x94} },
	{ {0xd3, 0x08, 0x03, 0x70, 0x54} },
	{ {0xd3, 0x08, 0x07, 0x70, 0x54} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xd3, 0x21, 0x03, 0x70, 0x50} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x10, 0x03, 0x71, 0x54} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
	{ {0xe3, 0x3c, 0x03, 0xf1, 0x90} },
};

static const struct coex_5g_afh_map afh_5g_8821a[] = { {0, 0, 0} };

///TODO: vendor driver coex code is terrible
/* wl_tx_dec_power, bt_tx_dec_power, wl_rx_gain, bt_rx_lna_constrain */
static const struct coex_rf_para rf_para_tx_8821a[] = {
	{0, 0, false, 7},  /* for normal */
	{0, 20, false, 7}, /* for WL-CPT */
	{8, 17, true, 4},
	{7, 18, true, 4},
	{6, 19, true, 4},
	{5, 20, true, 4}
};

static const struct coex_rf_para rf_para_rx_8821a[] = {
	{0, 0, false, 7},  /* for normal */
	{0, 20, false, 7}, /* for WL-CPT */
	{3, 24, true, 5},
	{2, 26, true, 5},
	{1, 27, true, 5},
	{0, 28, true, 5}
};

static_assert(ARRAY_SIZE(rf_para_tx_8821a) == ARRAY_SIZE(rf_para_rx_8821a));

static const u8 rtw8821a_pwrtrk_5gb_n[][RTW_PWR_TRK_TBL_SZ] = {
	{0, 0, 1, 2, 3, 3, 4, 5, 6, 6, 7, 8, 9, 9, 10, 11, 12, 12, 13, 14, 15,
	 15, 16, 16, 16, 16, 16, 16, 16, 16},
	{0, 0, 1, 2, 3, 3, 4, 5, 6, 6, 7, 8, 9, 9, 10, 11, 12, 12, 13, 14, 15,
	 15, 16, 16, 16, 16, 16, 16, 16, 16},
	{0, 0, 1, 2, 3, 3, 4, 5, 6, 6, 7, 8, 9, 9, 10, 11, 12, 12, 13, 14, 15,
	 15, 16, 16, 16, 16, 16, 16, 16, 16},
};

static const u8 rtw8821a_pwrtrk_5gb_p[][RTW_PWR_TRK_TBL_SZ] = {
	{0, 0, 1, 2, 3, 3, 4, 5, 6, 6, 7, 8, 9, 9, 10, 11, 12, 12, 13, 14, 15,
	 15, 16, 16, 16, 16, 16, 16, 16, 16},
	{0, 0, 1, 2, 3, 3, 4, 5, 6, 6, 7, 8, 9, 9, 10, 11, 12, 12, 13, 14, 15,
	 15, 16, 16, 16, 16, 16, 16, 16, 16},
	{0, 0, 1, 2, 3, 3, 4, 5, 6, 6, 7, 8, 9, 9, 10, 11, 12, 12, 13, 14, 15,
	 15, 16, 16, 16, 16, 16, 16, 16, 16},
};

static const u8 rtw8821a_pwrtrk_5ga_n[][RTW_PWR_TRK_TBL_SZ] = {
	{0, 0, 1, 2, 3, 3, 4, 5, 6, 6, 7, 8, 9, 9, 10, 11, 12, 12, 13, 14, 15,
	 15, 16, 16, 16, 16, 16, 16, 16, 16},
	{0, 0, 1, 2, 3, 3, 4, 5, 6, 6, 7, 8, 9, 9, 10, 11, 12, 12, 13, 14, 15,
	 15, 16, 16, 16, 16, 16, 16, 16, 16},
	{0, 0, 1, 2, 3, 3, 4, 5, 6, 6, 7, 8, 9, 9, 10, 11, 12, 12, 13, 14, 15,
	 15, 16, 16, 16, 16, 16, 16, 16, 16},
};

static const u8 rtw8821a_pwrtrk_5ga_p[][RTW_PWR_TRK_TBL_SZ] = {
	{0, 0, 1, 2, 3, 3, 4, 5, 6, 6, 7, 8, 9, 9, 10, 11, 12, 12, 13, 14, 15,
	 15, 16, 16, 16, 16, 16, 16, 16, 16},
	{0, 0, 1, 2, 3, 3, 4, 5, 6, 6, 7, 8, 9, 9, 10, 11, 12, 12, 13, 14, 15,
	 15, 16, 16, 16, 16, 16, 16, 16, 16},
	{0, 0, 1, 2, 3, 3, 4, 5, 6, 6, 7, 8, 9, 9, 10, 11, 12, 12, 13, 14, 15,
	 15, 16, 16, 16, 16, 16, 16, 16, 16},
};

static const u8 rtw8821a_pwrtrk_2gb_n[] = {
	0, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6,
	6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10, 10, 10
};

static const u8 rtw8821a_pwrtrk_2gb_p[] = {
	0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7,
	8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 12, 12, 12, 12
};

static const u8 rtw8821a_pwrtrk_2ga_n[] = {
	0, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6,
	6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10, 10, 10
};

static const u8 rtw8821a_pwrtrk_2ga_p[] = {
	0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7,
	8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 12, 12, 12, 12
};

static const u8 rtw8821a_pwrtrk_2g_cck_b_n[] = {
	0, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6,
	6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10, 10, 10
};

static const u8 rtw8821a_pwrtrk_2g_cck_b_p[] = {
	0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7,
	8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 12, 12, 12, 12
};

static const u8 rtw8821a_pwrtrk_2g_cck_a_n[] = {
	0, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6,
	6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10, 10, 10
};

static const u8 rtw8821a_pwrtrk_2g_cck_a_p[] = {
	0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7,
	8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 12, 12, 12, 12
};

static const struct rtw_pwr_track_tbl rtw8821a_rtw_pwr_track_tbl = {///done
	.pwrtrk_5gb_n[0] = rtw8821a_pwrtrk_5gb_n[0],
	.pwrtrk_5gb_n[1] = rtw8821a_pwrtrk_5gb_n[1],
	.pwrtrk_5gb_n[2] = rtw8821a_pwrtrk_5gb_n[2],
	.pwrtrk_5gb_p[0] = rtw8821a_pwrtrk_5gb_p[0],
	.pwrtrk_5gb_p[1] = rtw8821a_pwrtrk_5gb_p[1],
	.pwrtrk_5gb_p[2] = rtw8821a_pwrtrk_5gb_p[2],
	.pwrtrk_5ga_n[0] = rtw8821a_pwrtrk_5ga_n[0],
	.pwrtrk_5ga_n[1] = rtw8821a_pwrtrk_5ga_n[1],
	.pwrtrk_5ga_n[2] = rtw8821a_pwrtrk_5ga_n[2],
	.pwrtrk_5ga_p[0] = rtw8821a_pwrtrk_5ga_p[0],
	.pwrtrk_5ga_p[1] = rtw8821a_pwrtrk_5ga_p[1],
	.pwrtrk_5ga_p[2] = rtw8821a_pwrtrk_5ga_p[2],
	.pwrtrk_2gb_n = rtw8821a_pwrtrk_2gb_n,
	.pwrtrk_2gb_p = rtw8821a_pwrtrk_2gb_p,
	.pwrtrk_2ga_n = rtw8821a_pwrtrk_2ga_n,
	.pwrtrk_2ga_p = rtw8821a_pwrtrk_2ga_p,
	.pwrtrk_2g_cckb_n = rtw8821a_pwrtrk_2g_cck_b_n,
	.pwrtrk_2g_cckb_p = rtw8821a_pwrtrk_2g_cck_b_p,
	.pwrtrk_2g_ccka_n = rtw8821a_pwrtrk_2g_cck_a_n,
	.pwrtrk_2g_ccka_p = rtw8821a_pwrtrk_2g_cck_a_p,
};


static const u8 rtw8812a_pwrtrk_5gb_n[][RTW_PWR_TRK_TBL_SZ] = {
	{0, 1, 1, 2, 2, 3, 4, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12,
	 12, 13, 13, 14, 14, 14, 14, 14, 14},
	{0, 1, 1, 2, 2, 3, 4, 4, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12,
	 12, 13, 13, 14, 14, 14, 14, 14, 14},
	{0, 1, 1, 2, 2, 3, 4, 5, 6, 6, 7, 8, 8, 9, 10, 10, 11, 11, 12, 12, 13,
	 13, 14, 14, 15, 16, 16, 16, 16, 16},
};

static const u8 rtw8812a_pwrtrk_5gb_p[][RTW_PWR_TRK_TBL_SZ] = {
	{0, 1, 1, 2, 2, 3, 3, 4, 5, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 11,
	 11, 11, 11, 11, 11, 11, 11, 11, 11},
	{0, 1, 1, 2, 3, 3, 4, 5, 5, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 11,
	 11, 11, 11, 11, 11, 11, 11, 11, 11},
	{0, 1, 1, 2, 3, 3, 4, 5, 6, 7, 7, 8, 8, 9, 9, 10, 11, 11, 11, 11, 11,
	 11, 11, 11, 11, 11, 11, 11, 11, 11},
};

static const u8 rtw8812a_pwrtrk_5ga_n[][RTW_PWR_TRK_TBL_SZ] = {
	{0, 1, 1, 2, 2, 3, 4, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12,
	 12, 13, 13, 14, 15, 15, 15, 15, 15},
	{0, 1, 1, 2, 2, 3, 4, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12,
	 12, 13, 13, 14, 15, 15, 15, 15, 15},
	{0, 1, 1, 2, 2, 3, 4, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12,
	 12, 13, 13, 14, 15, 15, 15, 15, 15},
};

static const u8 rtw8812a_pwrtrk_5ga_p[][RTW_PWR_TRK_TBL_SZ] = {
	{0, 1, 1, 2, 2, 3, 4, 5, 6, 7, 7, 8, 8, 9, 10, 11, 11, 11, 11, 11, 11,
	 11, 11, 11, 11, 11, 11, 11, 11, 11},
	{0, 1, 1, 2, 3, 3, 4, 5, 6, 7, 7, 8, 8, 9, 10, 11, 11, 11, 11, 11, 11,
	 11, 11, 11, 11, 11, 11, 11, 11, 11},
	{0, 1, 1, 2, 3, 3, 4, 5, 6, 7, 7, 8, 8, 9, 10, 11, 11, 12, 12, 11, 11,
	 11, 11, 11, 11, 11, 11, 11, 11, 11},
};

static const u8 rtw8812a_pwrtrk_2gb_n[] = {
	0, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 5, 5, 5, 6, 6,
	7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 11, 11, 11, 11
};

static const u8 rtw8812a_pwrtrk_2gb_p[] = {
	0, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6,
	6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
};

static const u8 rtw8812a_pwrtrk_2ga_n[] = {
	0, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6,
	6, 6, 7, 7, 7, 8, 8, 9, 10, 10, 10, 10, 10, 10
};

static const u8 rtw8812a_pwrtrk_2ga_p[] = {
	0, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6,
	6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
};

static const u8 rtw8812a_pwrtrk_2g_cck_b_n[] = {
	0, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 5, 5, 5, 6, 6,
	7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 11, 11, 11, 11
};

static const u8 rtw8812a_pwrtrk_2g_cck_b_p[] = {
	0, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6,
	6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
};

static const u8 rtw8812a_pwrtrk_2g_cck_a_n[] = {
	0, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6,
	6, 6, 7, 7, 7, 8, 8, 9, 10, 10, 10, 10, 10, 10
};

static const u8 rtw8812a_pwrtrk_2g_cck_a_p[] = {
	0, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6,
	6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
};

static const struct rtw_pwr_track_tbl rtw8812a_rtw_pwr_track_tbl = {
	.pwrtrk_5gb_n[0] = rtw8812a_pwrtrk_5gb_n[0],
	.pwrtrk_5gb_n[1] = rtw8812a_pwrtrk_5gb_n[1],
	.pwrtrk_5gb_n[2] = rtw8812a_pwrtrk_5gb_n[2],
	.pwrtrk_5gb_p[0] = rtw8812a_pwrtrk_5gb_p[0],
	.pwrtrk_5gb_p[1] = rtw8812a_pwrtrk_5gb_p[1],
	.pwrtrk_5gb_p[2] = rtw8812a_pwrtrk_5gb_p[2],
	.pwrtrk_5ga_n[0] = rtw8812a_pwrtrk_5ga_n[0],
	.pwrtrk_5ga_n[1] = rtw8812a_pwrtrk_5ga_n[1],
	.pwrtrk_5ga_n[2] = rtw8812a_pwrtrk_5ga_n[2],
	.pwrtrk_5ga_p[0] = rtw8812a_pwrtrk_5ga_p[0],
	.pwrtrk_5ga_p[1] = rtw8812a_pwrtrk_5ga_p[1],
	.pwrtrk_5ga_p[2] = rtw8812a_pwrtrk_5ga_p[2],
	.pwrtrk_2gb_n = rtw8812a_pwrtrk_2gb_n,
	.pwrtrk_2gb_p = rtw8812a_pwrtrk_2gb_p,
	.pwrtrk_2ga_n = rtw8812a_pwrtrk_2ga_n,
	.pwrtrk_2ga_p = rtw8812a_pwrtrk_2ga_p,
	.pwrtrk_2g_cckb_n = rtw8812a_pwrtrk_2g_cck_b_n,
	.pwrtrk_2g_cckb_p = rtw8812a_pwrtrk_2g_cck_b_p,
	.pwrtrk_2g_ccka_n = rtw8812a_pwrtrk_2g_cck_a_n,
	.pwrtrk_2g_ccka_p = rtw8812a_pwrtrk_2g_cck_a_p,
};

static const struct rtw_reg_domain coex_info_hw_regs_8821a[] = {
	{0xCB0, MASKDWORD, RTW_REG_DOMAIN_MAC32},
	{0xCB4, MASKDWORD, RTW_REG_DOMAIN_MAC32},
	{0xCBA, MASKBYTE0, RTW_REG_DOMAIN_MAC8},
	{0, 0, RTW_REG_DOMAIN_NL},
	{0x430, MASKDWORD, RTW_REG_DOMAIN_MAC32},
	{0x434, MASKDWORD, RTW_REG_DOMAIN_MAC32},
	{0x42a, MASKLWORD, RTW_REG_DOMAIN_MAC16},
	{0x426, MASKBYTE0, RTW_REG_DOMAIN_MAC8},
	{0x45e, BIT(3), RTW_REG_DOMAIN_MAC8},
	{0x454, MASKLWORD, RTW_REG_DOMAIN_MAC16},
	{0, 0, RTW_REG_DOMAIN_NL},
	{0x4c, BIT(24) | BIT(23), RTW_REG_DOMAIN_MAC32},
	{0x64, BIT(0), RTW_REG_DOMAIN_MAC8},
	{0x4c6, BIT(4), RTW_REG_DOMAIN_MAC8},
	{0x40, BIT(5), RTW_REG_DOMAIN_MAC8},
	{0x1, RFREG_MASK, RTW_REG_DOMAIN_RF_A},
	{0, 0, RTW_REG_DOMAIN_NL},
	{0x550, MASKDWORD, RTW_REG_DOMAIN_MAC32},
	{0x522, MASKBYTE0, RTW_REG_DOMAIN_MAC8},
	{0x953, BIT(1), RTW_REG_DOMAIN_MAC8},
	{0xc50,  MASKBYTE0, RTW_REG_DOMAIN_MAC8},
	{0x60A, MASKBYTE0, RTW_REG_DOMAIN_MAC8},
};

const struct rtw_chip_info rtw8821a_hw_spec = {
	.ops = &rtw8821a_ops,
	.id = RTW_CHIP_TYPE_8821A,
	.fw_name = "rtw88/rtw8821a_fw.bin",
	.wlan_cpu = RTW_WCPU_11N, ///TODO maybe RTW_WCPU_11N should be renamed to RTW_WCPU_8051 and RTW_WCPU_11AC to RTW_WCPU_3081
	.tx_pkt_desc_sz = 40,
	.tx_buf_desc_sz = 16,
	.rx_pkt_desc_sz = 24,
	.rx_buf_desc_sz = 8,
	.phy_efuse_size = 512,
	.log_efuse_size = 512,
	.ptct_efuse_size = 96 + 1,///TODO or just 18?
	.txff_size = 65536,
	.rxff_size = 16128,
	.rsvd_drv_pg_num = 8,
	.txgi_factor = 1,
	.is_pwr_by_rate_dec = true,
	.max_power_index = 0x3f,
	.csi_buf_pg_num = 0,
	.band = RTW_BAND_2G | RTW_BAND_5G,
	.page_size = 256,
	.dig_min = 0x20,
	.ht_supported = true,
	.vht_supported = true,
	.lps_deep_mode_supported = 0,
	.sys_func_en = 0xFD,
	.pwr_on_seq = card_enable_flow_8821a,
	.pwr_off_seq = card_disable_flow_8821a,
	.page_table = page_table_8821a,
	.rqpn_table = rqpn_table_8821a,
	.prioq_addrs = &prioq_addrs_8821a,
	.intf_table = &phy_para_table_8821a,
	.dig = rtw8821a_dig,
	.rf_sipi_addr = {REG_LSSI_WRITE_A, REG_LSSI_WRITE_B},
	.ltecoex_addr = NULL, /* Apparently not a thing here. */
	.mac_tbl = &rtw8821a_mac_tbl,
	.agc_tbl = &rtw8821a_agc_tbl,
	.bb_tbl = &rtw8821a_bb_tbl,
	.rf_tbl = {&rtw8821a_rf_a_tbl},
	.rfe_defs = rtw8821a_rfe_defs,
	.rfe_defs_size = ARRAY_SIZE(rtw8821a_rfe_defs),
	.rx_ldpc = false,
	.has_hw_feature_report = false,
	.c2h_ra_report_size = 4,
	.old_datarate_fb_limit = true,
	.usb_tx_agg_desc_num = 6,
	.pwr_track_tbl = &rtw8821a_rtw_pwr_track_tbl,
	.iqk_threshold = 8,
	// .bfer_su_max_num = 2,
	// .bfer_mu_max_num = 1,
	.ampdu_density = IEEE80211_HT_MPDU_DENSITY_16,
	.max_scan_ie_len = IEEE80211_MAX_DATA_LEN,

	.coex_para_ver = 20190509, /* glcoex_ver_date_8821a_1ant */
	.bt_desired_ver = 0x62, /* But for 2 ant it's 0x5c */
	.scbd_support = true,///TODO is it okay?
	.new_scbd10_def = false,///TODO is it okay?
	.ble_hid_profile_support = false,
	.wl_mimo_ps_support = false,
	.pstdma_type = COEX_PSTDMA_FORCE_LPSOFF,
	.bt_rssi_type = COEX_BTRSSI_RATIO,
	.ant_isolation = 15,
	.rssi_tolerance = 2,
	.wl_rssi_step = wl_rssi_step_8821a,
	.bt_rssi_step = bt_rssi_step_8821a,
	.table_sant_num = ARRAY_SIZE(table_sant_8821a),
	.table_sant = table_sant_8821a,
	.table_nsant_num = ARRAY_SIZE(table_nsant_8821a),
	.table_nsant = table_nsant_8821a,
	.tdma_sant_num = ARRAY_SIZE(tdma_sant_8821a),
	.tdma_sant = tdma_sant_8821a,
	.tdma_nsant_num = ARRAY_SIZE(tdma_nsant_8821a),
	.tdma_nsant = tdma_nsant_8821a,
	.wl_rf_para_num = ARRAY_SIZE(rf_para_tx_8821a),
	.wl_rf_para_tx = rf_para_tx_8821a,
	.wl_rf_para_rx = rf_para_rx_8821a,
	.bt_afh_span_bw20 = 0x24,
	.bt_afh_span_bw40 = 0x36,
	.afh_5g_num = ARRAY_SIZE(afh_5g_8821a),
	.afh_5g = afh_5g_8821a,

	.coex_info_hw_regs_num = ARRAY_SIZE(coex_info_hw_regs_8821a),
	.coex_info_hw_regs = coex_info_hw_regs_8821a,
};
EXPORT_SYMBOL(rtw8821a_hw_spec);

const struct rtw_chip_info rtw8812a_hw_spec = {
	.ops = &rtw8821a_ops,
	.id = RTW_CHIP_TYPE_8812A,
	.fw_name = "rtw88/rtw8812a_fw.bin",
	.wlan_cpu = RTW_WCPU_11N,
	.tx_pkt_desc_sz = 40,
	.tx_buf_desc_sz = 16,
	.rx_pkt_desc_sz = 24,
	.rx_buf_desc_sz = 8,
	.phy_efuse_size = 512,
	.log_efuse_size = 512,
	.ptct_efuse_size = 96 + 1,///TODO or just 18?
	.txff_size = 131072,
	.rxff_size = 16128,
	.rsvd_drv_pg_num = 9,
	.txgi_factor = 1,
	.is_pwr_by_rate_dec = true,
	.max_power_index = 0x3f,
	.csi_buf_pg_num = 0,
	.band = RTW_BAND_2G | RTW_BAND_5G,
	.page_size = 512,
	.dig_min = 0x20,
	.ht_supported = true,
	.vht_supported = true,
	.lps_deep_mode_supported = 0,
	.sys_func_en = 0xFD,
	.pwr_on_seq = card_enable_flow_8812a,
	.pwr_off_seq = card_disable_flow_8812a,
	.page_table = page_table_8812a,
	.rqpn_table = rqpn_table_8821a,
	.prioq_addrs = &prioq_addrs_8821a,
	.intf_table = &phy_para_table_8821a,
	.dig = rtw8821a_dig,
	.rf_sipi_addr = {REG_LSSI_WRITE_A, REG_LSSI_WRITE_B},
	.ltecoex_addr = NULL,
	.mac_tbl = &rtw8812a_mac_tbl,
	.agc_tbl = &rtw8812a_agc_tbl,
	.bb_tbl = &rtw8812a_bb_tbl,
	.rf_tbl = {&rtw8812a_rf_a_tbl, &rtw8812a_rf_b_tbl},
	.rfe_defs = rtw8812a_rfe_defs,
	.rfe_defs_size = ARRAY_SIZE(rtw8812a_rfe_defs),
	.rx_ldpc = false,
	.has_hw_feature_report = false,
	.c2h_ra_report_size = 4,
	.old_datarate_fb_limit = true,
	.usb_tx_agg_desc_num = 1,
	.pwr_track_tbl = &rtw8812a_rtw_pwr_track_tbl,
	.iqk_threshold = 8,
	// .bfer_su_max_num = 2,
	// .bfer_mu_max_num = 1,
	.ampdu_density = IEEE80211_HT_MPDU_DENSITY_16,
	.max_scan_ie_len = IEEE80211_MAX_DATA_LEN,

	.coex_para_ver = 0, /* no coex code in 8812au driver */
	.bt_desired_ver = 0,
	.scbd_support = true,
	.new_scbd10_def = false,
	.ble_hid_profile_support = false,
	.wl_mimo_ps_support = false,
	.pstdma_type = COEX_PSTDMA_FORCE_LPSOFF,
	.bt_rssi_type = COEX_BTRSSI_RATIO,
	.ant_isolation = 15,
	.rssi_tolerance = 2,
	.wl_rssi_step = wl_rssi_step_8821a,
	.bt_rssi_step = bt_rssi_step_8821a,
	.table_sant_num = ARRAY_SIZE(table_sant_8821a),
	.table_sant = table_sant_8821a,
	.table_nsant_num = ARRAY_SIZE(table_nsant_8821a),
	.table_nsant = table_nsant_8821a,
	.tdma_sant_num = ARRAY_SIZE(tdma_sant_8821a),
	.tdma_sant = tdma_sant_8821a,
	.tdma_nsant_num = ARRAY_SIZE(tdma_nsant_8821a),
	.tdma_nsant = tdma_nsant_8821a,
	.wl_rf_para_num = ARRAY_SIZE(rf_para_tx_8821a),
	.wl_rf_para_tx = rf_para_tx_8821a,
	.wl_rf_para_rx = rf_para_rx_8821a,
	.bt_afh_span_bw20 = 0x24,
	.bt_afh_span_bw40 = 0x36,
	.afh_5g_num = ARRAY_SIZE(afh_5g_8821a),
	.afh_5g = afh_5g_8821a,

	.coex_info_hw_regs_num = ARRAY_SIZE(coex_info_hw_regs_8821a),
	.coex_info_hw_regs = coex_info_hw_regs_8821a,
};
EXPORT_SYMBOL(rtw8812a_hw_spec);

// MODULE_FIRMWARE("rtw88/rtw8811a_fw.bin");
MODULE_FIRMWARE("rtw88/rtw8821a_fw.bin");
MODULE_FIRMWARE("rtw88/rtw8812a_fw.bin");

MODULE_AUTHOR("Realtek Corporation");
MODULE_DESCRIPTION("Realtek 802.11ac wireless 8821a/8811a/8812a driver");
MODULE_LICENSE("Dual BSD/GPL");
