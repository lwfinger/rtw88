// SPDX-License-Identifier: GPL-2.0-only
/* Copyright(c) 2007-2017 Realtek Corporation
 * Copyright(c) 2018-2019 Realtek Corporation
 * Copyright 2024 Fiona Klute
 * Copyright(c) 2026 Igor Listopad <dev@listopad.email>
 */

#include <linux/netdevice.h>
#include <linux/slab.h>

#include "main.h"
#include "debug.h"
#include "efuse.h"
#include "fw.h"
#include "mac.h"
#include "phy.h"
#include "ps.h"
#include "reg.h"
#include "rtw8188e.h"
#include "rtw8188e_table.h"
#include "rx.h"
#include "sdio.h"
#include "tx.h"

#define TBTT_PROHIBIT_SETUP_TIME		0x04
#define TBTT_PROHIBIT_HOLD_TIME_STOP_BCN	0x64
#define WLAN_BCN_DMA_TIME			0x02
#define WLAN_BAR_VAL				0x0201ffff
#define WLAN_RF_CTRL_ENABLE			(BIT_RF_EN | BIT_RF_RSTB | \
						 BIT_RF_SDM_RSTB)

#define IQK_DELAY_TIME_8188E		10

static const struct rtw_pwr_seq_cmd trans_carddis_to_cardemu_8188e[] = {
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
	{0x0005,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(3) | BIT(4), 0},
	{ .cmd = RTW_PWR_CMD_END },
};

static const struct rtw_pwr_seq_cmd trans_cardemu_to_act_8188e[] = {
	{0x0006,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_POLLING, BIT(1), BIT(1)},
	{0x0002,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(0) | BIT(1), 0},
	{0x0026,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(7), BIT(7)},
	{0x0005,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(7), 0},
	{0x0005,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(3) | BIT(4), 0},
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
	{0x0023,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(4), 0},
	{ .cmd = RTW_PWR_CMD_END },
};

static const struct rtw_pwr_seq_cmd trans_act_to_lps_8188e[] = {
	{0x0522,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0xff, 0x7f},
	{0x05f8,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_POLLING, 0xff, 0},
	{0x05f9,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_POLLING, 0xff, 0},
	{0x05fa,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_POLLING, 0xff, 0},
	{0x05fb,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_POLLING, 0xff, 0},
	{0x0002,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(0), 0},
	{1,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_DELAY, 0, RTW_PWR_DELAY_US},
	{0x0100,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0xff, 0x3f},
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
	{ .cmd = RTW_PWR_CMD_END },
};

static const struct rtw_pwr_seq_cmd trans_act_to_reset_mcu_8188e[] = {
	{0x0003,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(2), 0},
	{0x0080,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0xff, 0},
	{ .cmd = RTW_PWR_CMD_END },
};

static const struct rtw_pwr_seq_cmd trans_act_to_cardemu_8188e[] = {
	{0x001f,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0xff, 0},
	{0x0023,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(4), BIT(4)},
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
	{ .cmd = RTW_PWR_CMD_END },
};

static const struct rtw_pwr_seq_cmd trans_cardemu_to_carddis_8188e[] = {
	{0x0026,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(7), BIT(7)},
	{0x0005,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(3) | BIT(4), BIT(3)},
	{0x0007,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0xff, 0},
	{0x0041,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, BIT(4), 0},
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
	{0x001c,
	 RTW_PWR_CUT_ALL_MSK,
	 RTW_PWR_INTF_ALL_MSK,
	 RTW_PWR_ADDR_MAC,
	 RTW_PWR_CMD_WRITE, 0xff, 0x0e},
	{ .cmd = RTW_PWR_CMD_END },
};

static const struct rtw_pwr_seq_cmd * const card_enable_flow_8188e[] = {
	trans_carddis_to_cardemu_8188e,
	trans_cardemu_to_act_8188e,
	NULL
};

static const struct rtw_pwr_seq_cmd * const card_disable_flow_8188e[] = {
	trans_act_to_lps_8188e,
	trans_act_to_reset_mcu_8188e,
	trans_act_to_cardemu_8188e,
	trans_cardemu_to_carddis_8188e,
	NULL
};

static const struct rtw_page_table page_table_8188e[] = {
	{12, 0, 9, 0, 1},
};

static const struct rtw_rqpn rqpn_table_8188e[] = {
	{RTW_DMA_MAPPING_NORMAL, RTW_DMA_MAPPING_NORMAL,
	 RTW_DMA_MAPPING_LOW, RTW_DMA_MAPPING_LOW,
	 RTW_DMA_MAPPING_HIGH, RTW_DMA_MAPPING_HIGH},
};

static const u32 rtw8188e_ofdm_swing_table[] = {
	0x0b40002d,	/* 0, -15.0dB */
	0x0c000030,	/* 1, -14.5dB */
	0x0cc00033,	/* 2, -14.0dB */
	0x0d800036,	/* 3, -13.5dB */
	0x0e400039,	/* 4, -13.0dB */
	0x0f00003c,	/* 5, -12.5dB */
	0x10000040,	/* 6, -12.0dB */
	0x11000044,	/* 7, -11.5dB */
	0x12000048,	/* 8, -11.0dB */
	0x1300004c,	/* 9, -10.5dB */
	0x14400051,	/* 10, -10.0dB */
	0x15800056,	/* 11, -9.5dB */
	0x16c0005b,	/* 12, -9.0dB */
	0x18000060,	/* 13, -8.5dB */
	0x19800066,	/* 14, -8.0dB */
	0x1b00006c,	/* 15, -7.5dB */
	0x1c800072,	/* 16, -7.0dB */
	0x1e400079,	/* 17, -6.5dB */
	0x20000080,	/* 18, -6.0dB */
	0x22000088,	/* 19, -5.5dB */
	0x24000090,	/* 20, -5.0dB */
	0x26000098,	/* 21, -4.5dB */
	0x288000a2,	/* 22, -4.0dB */
	0x2ac000ab,	/* 23, -3.5dB */
	0x2d4000b5,	/* 24, -3.0dB */
	0x300000c0,	/* 25, -2.5dB */
	0x32c000cb,	/* 26, -2.0dB */
	0x35c000d7,	/* 27, -1.5dB */
	0x390000e4,	/* 28, -1.0dB */
	0x3c8000f2,	/* 29, -0.5dB */
	0x40000100,	/* 30, +0.0dB */
	0x43c0010f,	/* 31, +0.5dB */
	0x47c0011f,	/* 32, +1.0dB */
	0x4c000130,	/* 33, +1.5dB */
	0x50800142,	/* 34, +2.0dB */
	0x55400155,	/* 35, +2.5dB */
	0x5a400169,	/* 36, +3.0dB */
	0x5fc0017f,	/* 37, +3.5dB */
	0x65400195,	/* 38, +4.0dB */
	0x6b8001ae,	/* 39, +4.5dB */
	0x71c001c7,	/* 40, +5.0dB */
	0x788001e2,	/* 41, +5.5dB */
	0x7f8001fe,	/* 42, +6.0dB */
};

static const u8 rtw8188e_cck_swing_table_ch1_ch13[][8] = {
	{0x09, 0x08, 0x07, 0x06, 0x04, 0x03, 0x01, 0x01},	/* 0, -16.0dB */
	{0x09, 0x09, 0x08, 0x06, 0x05, 0x03, 0x01, 0x01},	/* 1, -15.5dB */
	{0x0a, 0x09, 0x08, 0x07, 0x05, 0x03, 0x02, 0x01},	/* 2, -15.0dB */
	{0x0a, 0x0a, 0x09, 0x07, 0x05, 0x03, 0x02, 0x01},	/* 3, -14.5dB */
	{0x0b, 0x0a, 0x09, 0x08, 0x06, 0x04, 0x02, 0x01},	/* 4, -14.0dB */
	{0x0b, 0x0b, 0x0a, 0x08, 0x06, 0x04, 0x02, 0x01},	/* 5, -13.5dB */
	{0x0c, 0x0c, 0x0a, 0x09, 0x06, 0x04, 0x02, 0x01},	/* 6, -13.0dB */
	{0x0d, 0x0c, 0x0b, 0x09, 0x07, 0x04, 0x02, 0x01},	/* 7, -12.5dB */
	{0x0d, 0x0d, 0x0c, 0x0a, 0x07, 0x05, 0x02, 0x01},	/* 8, -12.0dB */
	{0x0e, 0x0e, 0x0c, 0x0a, 0x08, 0x05, 0x02, 0x01},	/* 9, -11.5dB */
	{0x0f, 0x0f, 0x0d, 0x0b, 0x08, 0x05, 0x03, 0x01},	/* 10, -11.0dB */
	{0x10, 0x10, 0x0e, 0x0b, 0x08, 0x05, 0x03, 0x01},	/* 11, -10.5dB */
	{0x11, 0x11, 0x0f, 0x0c, 0x09, 0x06, 0x03, 0x01},	/* 12, -10.0dB */
	{0x12, 0x12, 0x0f, 0x0c, 0x09, 0x06, 0x03, 0x01},	/* 13, -9.5dB */
	{0x13, 0x13, 0x10, 0x0d, 0x0a, 0x06, 0x03, 0x01},	/* 14, -9.0dB */
	{0x14, 0x14, 0x11, 0x0e, 0x0b, 0x07, 0x03, 0x02},	/* 15, -8.5dB */
	{0x16, 0x15, 0x12, 0x0f, 0x0b, 0x07, 0x04, 0x01},	/* 16, -8.0dB */
	{0x17, 0x16, 0x13, 0x10, 0x0c, 0x08, 0x04, 0x02},	/* 17, -7.5dB */
	{0x18, 0x17, 0x15, 0x11, 0x0c, 0x08, 0x04, 0x02},	/* 18, -7.0dB */
	{0x1a, 0x19, 0x16, 0x12, 0x0d, 0x09, 0x04, 0x02},	/* 19, -6.5dB */
	{0x1b, 0x1a, 0x17, 0x13, 0x0e, 0x09, 0x04, 0x02},	/* 20, -6.0dB */
	{0x1d, 0x1c, 0x18, 0x14, 0x0f, 0x0a, 0x05, 0x02},	/* 21, -5.5dB */
	{0x1f, 0x1e, 0x1a, 0x15, 0x10, 0x0a, 0x05, 0x02},	/* 22, -5.0dB */
	{0x20, 0x20, 0x1b, 0x16, 0x11, 0x08, 0x05, 0x02},	/* 23, -4.5dB */
	{0x22, 0x21, 0x1d, 0x18, 0x11, 0x0b, 0x06, 0x02},	/* 24, -4.0dB */
	{0x24, 0x23, 0x1f, 0x19, 0x13, 0x0c, 0x06, 0x03},	/* 25, -3.5dB */
	{0x26, 0x25, 0x21, 0x1b, 0x14, 0x0d, 0x06, 0x03},	/* 26, -3.0dB */
	{0x28, 0x28, 0x22, 0x1c, 0x15, 0x0d, 0x07, 0x03},	/* 27, -2.5dB */
	{0x2b, 0x2a, 0x25, 0x1e, 0x16, 0x0e, 0x07, 0x03},	/* 28, -2.0dB */
	{0x2d, 0x2d, 0x27, 0x1f, 0x18, 0x0f, 0x08, 0x03},	/* 29, -1.5dB */
	{0x30, 0x2f, 0x29, 0x21, 0x19, 0x10, 0x08, 0x03},	/* 30, -1.0dB */
	{0x33, 0x32, 0x2b, 0x23, 0x1a, 0x11, 0x08, 0x04},	/* 31, -0.5dB */
	{0x36, 0x35, 0x2e, 0x25, 0x1c, 0x12, 0x09, 0x04},	/* 32, +0.0dB */
};

static const u8 rtw8188e_cck_swing_table_ch14[][8] = {
	{0x09, 0x08, 0x07, 0x04, 0x00, 0x00, 0x00, 0x00},	/* 0, -16.0dB */
	{0x09, 0x09, 0x08, 0x05, 0x00, 0x00, 0x00, 0x00},	/* 1, -15.5dB */
	{0x0a, 0x09, 0x08, 0x05, 0x00, 0x00, 0x00, 0x00},	/* 2, -15.0dB */
	{0x0a, 0x0a, 0x09, 0x05, 0x00, 0x00, 0x00, 0x00},	/* 3, -14.5dB */
	{0x0b, 0x0a, 0x09, 0x05, 0x00, 0x00, 0x00, 0x00},	/* 4, -14.0dB */
	{0x0b, 0x0b, 0x0a, 0x06, 0x00, 0x00, 0x00, 0x00},	/* 5, -13.5dB */
	{0x0c, 0x0c, 0x0a, 0x06, 0x00, 0x00, 0x00, 0x00},	/* 6, -13.0dB */
	{0x0d, 0x0c, 0x0b, 0x06, 0x00, 0x00, 0x00, 0x00},	/* 7, -12.5dB */
	{0x0d, 0x0d, 0x0c, 0x07, 0x00, 0x00, 0x00, 0x00},	/* 8, -12.0dB */
	{0x0e, 0x0e, 0x0c, 0x07, 0x00, 0x00, 0x00, 0x00},	/* 9, -11.5dB */
	{0x0f, 0x0f, 0x0d, 0x08, 0x00, 0x00, 0x00, 0x00},	/* 10, -11.0dB */
	{0x10, 0x10, 0x0e, 0x08, 0x00, 0x00, 0x00, 0x00},	/* 11, -10.5dB */
	{0x11, 0x11, 0x0f, 0x09, 0x00, 0x00, 0x00, 0x00},	/* 12, -10.0dB */
	{0x12, 0x12, 0x0f, 0x09, 0x00, 0x00, 0x00, 0x00},	/* 13, -9.5dB */
	{0x13, 0x13, 0x10, 0x0a, 0x00, 0x00, 0x00, 0x00},	/* 14, -9.0dB */
	{0x14, 0x14, 0x11, 0x0a, 0x00, 0x00, 0x00, 0x00},	/* 15, -8.5dB */
	{0x16, 0x15, 0x12, 0x0b, 0x00, 0x00, 0x00, 0x00},	/* 16, -8.0dB */
	{0x17, 0x16, 0x13, 0x0b, 0x00, 0x00, 0x00, 0x00},	/* 17, -7.5dB */
	{0x18, 0x17, 0x15, 0x0c, 0x00, 0x00, 0x00, 0x00},	/* 18, -7.0dB */
	{0x1a, 0x19, 0x16, 0x0d, 0x00, 0x00, 0x00, 0x00},	/* 19, -6.5dB */
	{0x1b, 0x1a, 0x17, 0x0e, 0x00, 0x00, 0x00, 0x00},	/* 20, -6.0dB */
	{0x1d, 0x1c, 0x18, 0x0e, 0x00, 0x00, 0x00, 0x00},	/* 21, -5.5dB */
	{0x1f, 0x1e, 0x1a, 0x0f, 0x00, 0x00, 0x00, 0x00},	/* 22, -5.0dB */
	{0x20, 0x20, 0x1b, 0x10, 0x00, 0x00, 0x00, 0x00},	/* 23, -4.5dB */
	{0x22, 0x21, 0x1d, 0x11, 0x00, 0x00, 0x00, 0x00},	/* 24, -4.0dB */
	{0x24, 0x23, 0x1f, 0x12, 0x00, 0x00, 0x00, 0x00},	/* 25, -3.5dB */
	{0x26, 0x25, 0x21, 0x13, 0x00, 0x00, 0x00, 0x00},	/* 26, -3.0dB */
	{0x28, 0x28, 0x24, 0x14, 0x00, 0x00, 0x00, 0x00},	/* 27, -2.5dB */
	{0x2b, 0x2a, 0x25, 0x15, 0x00, 0x00, 0x00, 0x00},	/* 28, -2.0dB */
	{0x2d, 0x2d, 0x17, 0x17, 0x00, 0x00, 0x00, 0x00},	/* 29, -1.5dB */
	{0x30, 0x2f, 0x29, 0x18, 0x00, 0x00, 0x00, 0x00},	/* 30, -1.0dB */
	{0x33, 0x32, 0x2b, 0x19, 0x00, 0x00, 0x00, 0x00},	/* 31, -0.5dB */
	{0x36, 0x35, 0x2e, 0x1b, 0x00, 0x00, 0x00, 0x00},	/* 32, +0.0dB */
};

#define RTW8188E_OFDM_SWING_TABLE_SIZE ARRAY_SIZE(rtw8188e_ofdm_swing_table)
#define RTW8188E_CCK_SWING_TABLE_SIZE ARRAY_SIZE(rtw8188e_cck_swing_table_ch14)

static_assert(ARRAY_SIZE(rtw8188e_cck_swing_table_ch1_ch13) ==
	      RTW8188E_CCK_SWING_TABLE_SIZE);

static const struct rtw_hw_reg rtw8188e_txagc[] = {
	[DESC_RATE1M]	= { .addr = 0xe08, .mask = 0x0000ff00 },
	[DESC_RATE2M]	= { .addr = 0x86c, .mask = 0x0000ff00 },
	[DESC_RATE5_5M]	= { .addr = 0x86c, .mask = 0x00ff0000 },
	[DESC_RATE11M]	= { .addr = 0x86c, .mask = 0xff000000 },
	[DESC_RATE6M]	= { .addr = 0xe00, .mask = 0x000000ff },
	[DESC_RATE9M]	= { .addr = 0xe00, .mask = 0x0000ff00 },
	[DESC_RATE12M]	= { .addr = 0xe00, .mask = 0x00ff0000 },
	[DESC_RATE18M]	= { .addr = 0xe00, .mask = 0xff000000 },
	[DESC_RATE24M]	= { .addr = 0xe04, .mask = 0x000000ff },
	[DESC_RATE36M]	= { .addr = 0xe04, .mask = 0x0000ff00 },
	[DESC_RATE48M]	= { .addr = 0xe04, .mask = 0x00ff0000 },
	[DESC_RATE54M]	= { .addr = 0xe04, .mask = 0xff000000 },
	[DESC_RATEMCS0]	= { .addr = 0xe10, .mask = 0x000000ff },
	[DESC_RATEMCS1]	= { .addr = 0xe10, .mask = 0x0000ff00 },
	[DESC_RATEMCS2]	= { .addr = 0xe10, .mask = 0x00ff0000 },
	[DESC_RATEMCS3]	= { .addr = 0xe10, .mask = 0xff000000 },
	[DESC_RATEMCS4]	= { .addr = 0xe14, .mask = 0x000000ff },
	[DESC_RATEMCS5]	= { .addr = 0xe14, .mask = 0x0000ff00 },
	[DESC_RATEMCS6]	= { .addr = 0xe14, .mask = 0x00ff0000 },
	[DESC_RATEMCS7]	= { .addr = 0xe14, .mask = 0xff000000 },
};

static const u32 rtw8188e_iqk_adda_regs[] = {
	0x85c, 0xe6c, 0xe70, 0xe74, 0xe78, 0xe7c, 0xe80, 0xe84,
	0xe88, 0xe8c, 0xed0, 0xed4, 0xed8, 0xedc, 0xee0, 0xeec
};

static const u32 rtw8188e_iqk_mac8_regs[] = {
	0x522, 0x550, 0x551
};

static const u32 rtw8188e_iqk_mac32_regs[] = {
	0x40
};

static const u32 rtw8188e_iqk_bb_regs[] = {
	0xc04, 0xc08, 0x874, 0xb68, 0xb6c, 0x870, 0x860, 0x864, 0xa04
};

static const struct rtw_hw_reg rtw8188e_dig[] = {
	{ .addr = 0xc50, .mask = 0x7f },
};

static const struct rtw_hw_reg_offset rtw8188e_edcca_th[] = {
	[EDCCA_TH_L2H_IDX] = {{.addr = 0xc4c, .mask = MASKBYTE0}, .offset = 0},
	[EDCCA_TH_H2L_IDX] = {{.addr = 0xc4c, .mask = MASKBYTE2}, .offset = 0},
};

static const struct rtw_rf_sipi_addr rtw8188e_rf_sipi_addr[] = {
	[RF_PATH_A] = { .hssi_1 = 0x820, .hssi_2 = 0x824,
			.lssi_read = 0x8a0, .lssi_read_pi = 0x8b8 },
};

static const struct rtw_prioq_addrs rtw8188e_prioq_addrs = {
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

static const u8 rtw8188e_pwrtrk_2ga_n[] = {
	0, 0, 1, 1, 2, 2, 2, 3, 3, 3, 3, 4, 5, 5, 5, 6,
	6, 7, 7, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9
};

static const u8 rtw8188e_pwrtrk_2ga_p[] = {
	0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3,
	3, 3, 4, 4, 4, 5, 5, 5, 6, 7, 8, 8, 8, 8
};

static const u8 rtw8188e_pwrtrk_2g_cck_a_n[] = {
	0, 1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 6, 6, 6,
	6, 7, 7, 7, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9
};

static const u8 rtw8188e_pwrtrk_2g_cck_a_p[] = {
	0, 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2,
	3, 3, 3, 3, 4, 4, 4, 5, 6, 6, 6, 6, 7, 7
};

static const struct rtw_pwr_track_tbl rtw8188e_rtw_pwr_track_tbl = {
	.pwrtrk_2ga_n = rtw8188e_pwrtrk_2ga_n,
	.pwrtrk_2ga_p = rtw8188e_pwrtrk_2ga_p,
	.pwrtrk_2g_ccka_n = rtw8188e_pwrtrk_2g_cck_a_n,
	.pwrtrk_2g_ccka_p = rtw8188e_pwrtrk_2g_cck_a_p,
};

static const struct rtw_rfe_def rtw8188e_rfe_defs[] = {
	{
		.phy_pg_tbl = &rtw8188e_bb_pg_tbl,
		.txpwr_lmt_tbl = &rtw8188e_txpwr_lmt_tbl,
		.pwr_track_tbl = &rtw8188e_rtw_pwr_track_tbl,
	},
};

static int rtw8188e_chip_info_setup(struct rtw_dev *rtwdev)
{
	/* I-cut requires different firmware and PHY tables. */
	if (rtwdev->hal.cut_version >= RTW8188E_CHIP_VER_CUT_I) {
		rtw_err(rtwdev, "unsupported RTL8188E cut %u (T-series only)\n",
			rtwdev->hal.cut_version);
		return -EOPNOTSUPP;
	}

	rtwdev->chip_priv = devm_kzalloc(rtwdev->dev,
					 sizeof(struct rtw8188e_priv), GFP_KERNEL);
	if (!rtwdev->chip_priv)
		return -ENOMEM;

	return 0;
}

static void rtw8188e_efuse_grant(struct rtw_dev *rtwdev, bool on)
{
	if (on) {
		rtw_write8(rtwdev, REG_EFUSE_ACCESS, EFUSE_ACCESS_ON);

		rtw_write16_set(rtwdev, REG_SYS_FUNC_EN, BIT_FEN_ELDR);
		rtw_write16_set(rtwdev, REG_SYS_CLKR, BIT_LOADER_CLK_EN | BIT_ANA8M);
	} else {
		rtw_write8(rtwdev, REG_EFUSE_ACCESS, EFUSE_ACCESS_OFF);
	}
}

static void rtw8188e_cfg_ldo25(struct rtw_dev *rtwdev, bool enable)
{
	u8 ldo_pwr;

	ldo_pwr = rtw_read8(rtwdev, REG_LDO_EFUSE_CTRL + 3);
	if (enable) {
		ldo_pwr &= ~BIT_MASK_LDO25_VOLTAGE;
		ldo_pwr |= (BIT_LDO25_VOLTAGE_V25 << 4) | BIT_LDO25_EN;
	} else {
		ldo_pwr &= ~BIT_LDO25_EN;
	}
	rtw_write8(rtwdev, REG_LDO_EFUSE_CTRL + 3, ldo_pwr);
}

static void rtw8188e_set_tx_power_index_by_rate(struct rtw_dev *rtwdev,
						u8 path, u8 rs)
{
	struct rtw_hal *hal = &rtwdev->hal;
	const struct rtw_hw_reg *txagc;
	u8 rate, pwr_index;
	int j;

	for (j = 0; j < rtw_rate_size[rs]; j++) {
		rate = rtw_rate_section[rs][j];
		pwr_index = hal->tx_pwr_tbl[path][rate];

		if (rate >= ARRAY_SIZE(rtw8188e_txagc)) {
			rtw_warn(rtwdev, "rate 0x%x isn't supported\n", rate);
			continue;
		}
		txagc = &rtw8188e_txagc[rate];
		if (!txagc->addr) {
			rtw_warn(rtwdev, "rate 0x%x isn't defined\n", rate);
			continue;
		}

		rtw_write32_mask(rtwdev, txagc->addr, txagc->mask, pwr_index);
	}
}

static void rtw8188e_set_tx_power_index(struct rtw_dev *rtwdev)
{
	struct rtw_hal *hal = &rtwdev->hal;
	int rs, path;

	for (path = 0; path < hal->rf_path_num; path++) {
		for (rs = 0; rs <= RTW_RATE_SECTION_HT_1S; rs++)
			rtw8188e_set_tx_power_index_by_rate(rtwdev, path, rs);
	}
}

static void rtw8188e_set_ampdu_factor(struct rtw_dev *rtwdev, u8 factor)
{
	static const u8 agg_limits[] = {0x41, 0xa8, 0x72, 0xb9};
	u8 limit = min_t(u8, BIT(factor + 2), 0xf);
	u8 high, low;
	int i;

	for (i = 0; i < ARRAY_SIZE(agg_limits); i++) {
		high = min_t(u8, agg_limits[i] >> 4, limit);
		low = min_t(u8, agg_limits[i] & 0xf, limit);
		rtw_write8(rtwdev, REG_AGGLEN_LMT + i, (high << 4) | low);
	}
}

static void rtw8188e_false_alarm_statistics(struct rtw_dev *rtwdev)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	u32 cck_fa_cnt;
	u32 ofdm_fa_cnt;
	u32 crc32_cnt;
	u32 val32;

	/* Hold the counters while reading them. */
	rtw_write32_mask(rtwdev, REG_OFDM_FA_HOLDC_11N, BIT_MASK_OFDM_FA_KEEP, 1);
	rtw_write32_mask(rtwdev, REG_OFDM_FA_RSTD_11N, BIT_MASK_OFDM_FA_KEEP1, 1);
	rtw_write32_mask(rtwdev, REG_CCK_FA_RST_11N, BIT_MASK_CCK_CNT_KEEP, 1);
	rtw_write32_mask(rtwdev, REG_CCK_FA_RST_11N, BIT_MASK_CCK_FA_KEEP, 1);

	cck_fa_cnt = rtw_read32_mask(rtwdev, REG_CCK_FA_LSB_11N, MASKBYTE0);
	cck_fa_cnt += rtw_read32_mask(rtwdev, REG_CCK_FA_MSB_11N, MASKBYTE3) << 8;

	val32 = rtw_read32(rtwdev, REG_OFDM_FA_TYPE1_11N);
	ofdm_fa_cnt = u32_get_bits(val32, BIT_MASK_OFDM_FF_CNT);
	ofdm_fa_cnt += u32_get_bits(val32, BIT_MASK_OFDM_SF_CNT);
	val32 = rtw_read32(rtwdev, REG_OFDM_FA_TYPE2_11N);
	dm_info->ofdm_cca_cnt = u32_get_bits(val32, BIT_MASK_OFDM_CCA_CNT);
	ofdm_fa_cnt += u32_get_bits(val32, BIT_MASK_OFDM_PF_CNT);
	val32 = rtw_read32(rtwdev, REG_OFDM_FA_TYPE3_11N);
	ofdm_fa_cnt += u32_get_bits(val32, BIT_MASK_OFDM_RI_CNT);
	ofdm_fa_cnt += u32_get_bits(val32, BIT_MASK_OFDM_CRC_CNT);
	val32 = rtw_read32(rtwdev, REG_OFDM_FA_TYPE4_11N);
	ofdm_fa_cnt += u32_get_bits(val32, BIT_MASK_OFDM_MNS_CNT);

	dm_info->cck_fa_cnt = cck_fa_cnt;
	dm_info->ofdm_fa_cnt = ofdm_fa_cnt;
	dm_info->total_fa_cnt = cck_fa_cnt + ofdm_fa_cnt;

	dm_info->cck_err_cnt = rtw_read32(rtwdev, REG_CCK_CRC32_ERR_11N);
	dm_info->cck_ok_cnt = rtw_read32(rtwdev, REG_CCK_CRC32_OK_11N);
	crc32_cnt = rtw_read32(rtwdev, REG_OFDM_CRC32_CNT_11N);
	dm_info->ofdm_err_cnt = u32_get_bits(crc32_cnt, BIT_MASK_OFDM_LCRC_ERR);
	dm_info->ofdm_ok_cnt = u32_get_bits(crc32_cnt, BIT_MASK_OFDM_LCRC_OK);
	crc32_cnt = rtw_read32(rtwdev, REG_HT_CRC32_CNT_11N);
	dm_info->ht_err_cnt = u32_get_bits(crc32_cnt, BIT_MASK_HT_CRC_ERR);
	dm_info->ht_ok_cnt = u32_get_bits(crc32_cnt, BIT_MASK_HT_CRC_OK);
	dm_info->vht_err_cnt = 0;
	dm_info->vht_ok_cnt = 0;

	val32 = rtw_read32(rtwdev, REG_CCK_CCA_CNT_11N);
	dm_info->cck_cca_cnt = (u32_get_bits(val32, BIT_MASK_CCK_FA_MSB) << 8) |
			       u32_get_bits(val32, BIT_MASK_CCK_FA_LSB);
	dm_info->total_cca_cnt = dm_info->cck_cca_cnt + dm_info->ofdm_cca_cnt;

	/* Reset the counters and resume counting. */
	rtw_write32_mask(rtwdev, REG_OFDM_FA_RSTC_11N, BIT_MASK_OFDM_FA_RST, 1);
	rtw_write32_mask(rtwdev, REG_OFDM_FA_RSTC_11N, BIT_MASK_OFDM_FA_RST, 0);
	rtw_write32_mask(rtwdev, REG_OFDM_FA_RSTD_11N, BIT_MASK_OFDM_FA_RST1, 1);
	rtw_write32_mask(rtwdev, REG_OFDM_FA_RSTD_11N, BIT_MASK_OFDM_FA_RST1, 0);
	rtw_write32_mask(rtwdev, REG_OFDM_FA_HOLDC_11N, BIT_MASK_OFDM_FA_KEEP, 0);
	rtw_write32_mask(rtwdev, REG_OFDM_FA_RSTD_11N, BIT_MASK_OFDM_FA_KEEP1, 0);
	rtw_write32_mask(rtwdev, REG_CCK_FA_RST_11N, BIT_MASK_CCK_CNT_KPEN, 0);
	rtw_write32_mask(rtwdev, REG_CCK_FA_RST_11N, BIT_MASK_CCK_CNT_KPEN, 2);
	rtw_write32_mask(rtwdev, REG_CCK_FA_RST_11N, BIT_MASK_CCK_FA_KPEN, 0);
	rtw_write32_mask(rtwdev, REG_CCK_FA_RST_11N, BIT_MASK_CCK_FA_KPEN, 2);
	rtw_write32_mask(rtwdev, REG_PAGE_F_RST_11N, BIT_MASK_F_RST_ALL, 1);
	rtw_write32_mask(rtwdev, REG_PAGE_F_RST_11N, BIT_MASK_F_RST_ALL, 0);
}

static int rtw8188e_llt_write(struct rtw_dev *rtwdev, u8 address, u8 next)
{
	u32 value;

	rtw_write32(rtwdev, REG_LLT_INIT, BIT(30) | (address << 8) | next);
	return read_poll_timeout(rtw_read32, value, !(value & GENMASK(31, 30)),
				10, 20000, false, rtwdev, REG_LLT_INIT);
}

static int rtw8188e_llt_init(struct rtw_dev *rtwdev)
{
	u16 boundary = rtwdev->fifo.rsvd_boundary;
	u16 pages = rtwdev->fifo.txff_pg_num;
	unsigned int i;
	int ret;

	for (i = 0; i < pages; i++) {
		u8 next = i + 1;

		if (i == boundary - 1)
			next = 0xff;
		else if (i == pages - 1)
			next = boundary;
		ret = rtw8188e_llt_write(rtwdev, i, next);
		if (ret)
			return ret;
	}

	return 0;
}

static int rtw8188e_mac_init(struct rtw_dev *rtwdev)
{
	/* Management reception requires RCR.AMF as well as RXFLTMAP0. */
	rtwdev->hal.rcr |= BIT_AMF;
	rtwdev->hal.rcr &= ~(BIT_VHT_DACK | BIT_PKTCTL_DLEN);
	rtw_write16(rtwdev, REG_TRXFF_BNDY + 2,
		    rtwdev->chip->rxff_size - RTW8188E_RX_DMA_RESERVED_SIZE - 1);
	rtw_write16_set(rtwdev, REG_CR, BIT_MAC_SEC_EN |
			BIT_32K_CAL_TMR_EN);
	rtw_write16(rtwdev, REG_RXFLTMAP0, 0xffff);
	rtwdev->hal.rxfltmap1 = 0x400;
	rtw_write16(rtwdev, REG_RXFLTMAP1, rtwdev->hal.rxfltmap1);
	rtw_write16(rtwdev, REG_RXFLTMAP2, 0xffff);
	rtw_write32(rtwdev, REG_RCR, rtwdev->hal.rcr);
	rtw_write32(rtwdev, REG_MAR, 0xffffffff);
	rtw_write32(rtwdev, REG_MAR + 4, 0xffffffff);
	rtw_write32(rtwdev, REG_MACID_NO_LINK_0, 0xffffffff);
	rtw_write32(rtwdev, REG_MACID_NO_LINK_1, 0xffffffff);

	return 0;
}

static int rtw8188e_mac_postinit(struct rtw_dev *rtwdev)
{
	/* Enable CCX reports and the TX Report 2 timer for all hardware MACIDs. */
	rtw_write32_set(rtwdev, REG_FWHW_TXQ_CTRL, BIT(12));
	rtw_write8(rtwdev, REG_TX_RPT_CTRL + 1, RTW_MAX_MAC_ID_NUM);
	rtw_write16(rtwdev, REG_TX_RPT_TIME, RTW8188E_RA_RPT_TIME_MAX);
	rtw_write8_set(rtwdev, REG_TX_RPT_CTRL,
		       RTW8188E_TX_RPT_CTRL_REPORT_EN | RTW8188E_TX_RPT_CTRL_TIMER_EN);

	return 0;
}

static void rtw8188e_set_default_port(struct rtw_dev *rtwdev,
				      struct rtw_vif *rtwvif)
{
	struct ieee80211_vif *vif = rtwvif_to_vif(rtwvif);

	/* LPS uses port0/MACID0; T firmware has no port-select command 0x2c. */
	rtwvif->bcn_ctrl |= BIT_DIS_ATIM;
	rtw_write8_set(rtwdev, rtwvif->conf->bcn_ctrl.addr, BIT_DIS_ATIM);
	rtw_write16(rtwdev, rtwvif->conf->aid.addr, 0xc000 | rtwvif->aid);
	if (!rtwdev->ap_active)
		rtw_write16(rtwdev, REG_MBSSID_BCN_SPACE, vif->bss_conf.beacon_int);
}

static int rtw8188e_lps_port(struct rtw_dev *rtwdev, struct rtw_vif *rtwvif)
{
	struct ieee80211_vif *vif = rtwvif_to_vif(rtwvif);
	struct rtw_sdio *rtwsdio = (struct rtw_sdio *)rtwdev->priv;
	const struct rtw_vif_port *old_conf = rtwvif->conf;
	u8 port = rtwvif->port;
	u8 mac_id = rtwvif->mac_id;
	struct ieee80211_sta *peer;
	struct rtw_sta_info *si;
	struct rtw8188e_sta *sta;
	u32 tsf_low, tsf_high, high;
	int i, ret = 0;

	lockdep_assert_held(&rtwdev->mutex);

	/* The device mutex protects the peer after the RCU lookup. */
	rcu_read_lock();
	peer = ieee80211_find_sta(vif, rtwvif->bssid);
	rcu_read_unlock();
	if (!peer)
		return -ENOENT;
	si = (struct rtw_sta_info *)peer->drv_priv;
	sta = (struct rtw8188e_sta *)si->priv;

	if (port == RTW_PORT_0 && mac_id == 0)
		goto download_rsvd_page;
	if ((port != RTW_PORT_0 && test_bit(RTW_PORT_0, rtwdev->hw_port)) ||
	    (mac_id != 0 && test_bit(0, rtwdev->mac_id_map)))
		return -EBUSY;

	/* A STA can remain on port 1 after the AP is removed. Move it after
	 * queued descriptors using the old MACID and port have been sent.
	 */
	ieee80211_stop_queues(rtwdev->hw);
	synchronize_net();
	flush_work(&rtwdev->tx_work);
	flush_delayed_work(&rtwsdio->tx_handler_data->work);
	for (i = 0; i < RTK_MAX_TX_QUEUE_NUM; i++) {
		if (!skb_queue_empty(&rtwsdio->tx_queue[i])) {
			ret = -EAGAIN;
			goto wake_queues;
		}
	}
	for (i = 0; i < RTW_DMA_MAPPING_MAX; i++) {
		const struct rtw_prioq_addr *addr = &rtwdev->chip->prioq_addrs->prio[i];

		if (rtw_read8(rtwdev, addr->rsvd) != rtw_read8(rtwdev, addr->avail)) {
			ret = -EAGAIN;
			goto wake_queues;
		}
	}

	if (port != RTW_PORT_0) {
		rtw_write8(rtwdev, old_conf->bcn_ctrl.addr, BIT_DIS_TSF_UDT);
		rtw_write8(rtwdev, REG_BCN_CTRL, BIT_DIS_TSF_UDT);
		tsf_high = rtw_read32(rtwdev, REG_TSFTR1 + 4);
		tsf_low = rtw_read32(rtwdev, REG_TSFTR1);
		high = rtw_read32(rtwdev, REG_TSFTR1 + 4);
		if (tsf_high != high) {
			tsf_high = high;
			tsf_low = rtw_read32(rtwdev, REG_TSFTR1);
		}
		rtw_write32_mask(rtwdev, old_conf->net_type.addr,
				 old_conf->net_type.mask, RTW_NET_NO_LINK);
		for (i = 0; i < ETH_ALEN; i++) {
			rtw_write8(rtwdev, old_conf->mac_addr.addr + i, 0);
			rtw_write8(rtwdev, old_conf->bssid.addr + i, 0);
		}
		clear_bit(port, rtwdev->hw_port);
		set_bit(RTW_PORT_0, rtwdev->hw_port);
	}

	if (mac_id != 0) {
		rtw_fw_media_status_report(rtwdev, mac_id, false);
		rtw_release_macid(rtwdev, mac_id);
		set_bit(0, rtwdev->mac_id_map);
	}

	spin_lock_bh(&rtwdev->txq_lock);
	rtw_vif_port_select(rtwvif, RTW_PORT_0);
	rtwvif->mac_id = 0;
	si->mac_id = 0;
	spin_unlock_bh(&rtwdev->txq_lock);
	if (port != RTW_PORT_0) {
		rtw_vif_port_config(rtwdev, rtwvif, PORT_SET_MAC_ADDR |
				    PORT_SET_BSSID | PORT_SET_NET_TYPE |
				    PORT_SET_AID);
		rtw_write32(rtwdev, REG_TSFTR, tsf_low);
		rtw_write32(rtwdev, REG_TSFTR + 4, tsf_high);
		rtw_vif_port_config(rtwdev, rtwvif, PORT_SET_BCN_CTRL);
	}
	if (mac_id != 0)
		rtw_fw_media_status_report(rtwdev, 0, true);
	rtwdev->lps_conf.port_id = RTW_PORT_0;
	sta->rsvd_pending = true;
	rtw_dbg(rtwdev, RTW_DBG_PS, "LPS: STA port %u MACID %u -> port 0 MACID 0\n",
		port, mac_id);
wake_queues:
	ieee80211_wake_queues(rtwdev->hw);
	if (ret)
		return ret;
download_rsvd_page:
	if (!sta->rsvd_pending)
		return 0;
	ret = rtw_fw_download_rsvd_page(rtwdev);
	if (ret)
		return ret;
	rtw_send_rsvd_page_h2c(rtwdev);
	sta->rsvd_pending = false;
	return 0;
}

static int rtw8188e_set_pwr_mode(struct rtw_dev *rtwdev)
{
	struct rtw_lps_conf *conf = &rtwdev->lps_conf;
	struct rtw_rsvd_page *rsvd_pkt;
	struct rtw_vif *rtwvif = NULL;
	u8 h2c[H2C_PKT_SIZE] = {};
	u8 awake_interval = conf->awake_interval;
	int ret;

	if (conf->mode == RTW_MODE_LPS) {
		/* T firmware subtracts one for the user-defined listen interval. */
		awake_interval = min_t(u16, awake_interval + 1, U8_MAX);
		list_for_each_entry(rsvd_pkt, &rtwdev->rsvd_page_list, build_list) {
			if (rsvd_pkt->type != RSVD_NULL)
				continue;
			rtwvif = rsvd_pkt->rtwvif;
			break;
		}
		if (!rtwvif)
			return -ENOENT;
		ret = rtw8188e_lps_port(rtwdev, rtwvif);
		if (ret)
			return ret;
		/* Restore the STA interval after an AP has been removed. */
		rtw8188e_set_default_port(rtwdev, rtwvif);
	}

	SET_H2C_CMD_ID_CLASS(h2c, H2C_CMD_SET_PWR_MODE);
	SET_PWR_MODE_SET_MODE(h2c, conf->mode);
	SET_PWR_MODE_SET_RLBM(h2c, conf->rlbm);
	SET_PWR_MODE_SET_SMART_PS(h2c, conf->smart_ps);
	SET_PWR_MODE_SET_AWAKE_INTERVAL(h2c, awake_interval);
	/* Leave byte 4 (AllQueueUAPSD) clear; it is not a port ID. */
	SET_PWR_MODE_SET_PWR_STATE(h2c, conf->state);
	rtw_fw_send_h2c_command(rtwdev, h2c);
	return 0;
}

static void rtw8188e_config_rx_aggregation(struct rtw_dev *rtwdev)
{
	rtw_write8(rtwdev, REG_RXDMA_AGG_PG_TH, 0x24);
	rtw_write8(rtwdev, REG_RXDMA_AGG_PG_TH + 1, 0x06);
	rtw_write8_set(rtwdev, REG_TXDMA_PQ_MAP, BIT_RXDMA_AGG_EN);
}

static int rtw8188e_write_page(struct rtw_dev *rtwdev, u16 page, u8 *buf,
			       u32 size, bool beacon)
{
	const u32 valid_addr = REG_DWBCN0_CTRL + 2;
	const u8 valid_mask = BIT_BCN_VALID >> 16;
	u8 cr, bcn_ctrl, txq_ctrl = 0, head;
	u8 valid;
	int ret;

	lockdep_assert_held(&rtwdev->mutex);

	cr = rtw_read8(rtwdev, REG_CR + 1);
	bcn_ctrl = rtw_read8(rtwdev, REG_BCN_CTRL);
	if (!beacon)
		txq_ctrl = rtw_read8(rtwdev, REG_FWHW_TXQ_CTRL + 2);
	head = rtw_read8(rtwdev, REG_DWBCN0_CTRL + 1);
	rtw_write8(rtwdev, REG_DWBCN0_CTRL + 1, page);
	/* BCN_VALID is write-one-to-clear. Leave the other bytes alone. */
	rtw_write8_set(rtwdev, valid_addr, valid_mask);
	rtw_write8(rtwdev, REG_CR + 1, cr | (BIT_ENSWBCN >> 8));
	rtw_write8(rtwdev, REG_BCN_CTRL,
		   (bcn_ctrl & ~BIT_EN_BCN_FUNCTION) | BIT_DIS_TSF_UDT);
	/* Clearing EN_BCNQ_DL marks the upload as a reserved-page packet. */
	if (!beacon)
		rtw_write8(rtwdev, REG_FWHW_TXQ_CTRL + 2,
			   txq_ctrl & ~(BIT_EN_BCNQ_DL >> 16));

	ret = rtw_hci_write_data_rsvd_page(rtwdev, buf, size);
	if (!ret && read_poll_timeout(rtw_read8, valid, valid & valid_mask,
				      100, 10000, false, rtwdev, valid_addr))
		ret = -EBUSY;

	rtw_write8(rtwdev, REG_DWBCN0_CTRL + 1, head);
	if (!beacon)
		rtw_write8(rtwdev, REG_FWHW_TXQ_CTRL + 2, txq_ctrl);
	rtw_write8(rtwdev, REG_BCN_CTRL, bcn_ctrl);
	rtw_write8(rtwdev, REG_CR + 1, cr);
	return ret;
}

static int rtw8188e_write_rsvd_page(struct rtw_dev *rtwdev, u16 page,
				    u8 *buf, u32 size)
{
	return rtw8188e_write_page(rtwdev, page, buf, size, false);
}

static void rtw8188e_set_macid_link(struct rtw_dev *rtwdev, u8 mac_id,
				    bool connected)
{
	/* The driver maintains the hardware link bitmap for T-series firmware. */
	rtw_write32_mask(rtwdev, REG_MACID_NO_LINK_0, BIT(mac_id), !connected);
}

static bool rtw8188e_ap_mgmt_tx_done(struct rtw_dev *rtwdev,
				     struct ieee80211_vif *vif)
{
	struct rtw_sdio *rtwsdio = (struct rtw_sdio *)rtwdev->priv;
	struct rtw_tx_report *report = &rtwdev->tx_report;
	const struct ieee80211_hdr_3addr *hdr;
	struct sk_buff *skb;
	unsigned long flags;
	bool empty = true;

	/* A dequeued frame may still be on its way to the hardware queue. */
	flush_delayed_work(&rtwsdio->tx_handler_data->work);

	if (!skb_queue_empty(&rtwsdio->tx_queue[RTW_TX_QUEUE_MGMT]) ||
	    !rtw_read32_mask(rtwdev, REG_BCNQ_INFO, BIT_MGQ_CPU_EMPTY))
		return false;

	/* The CPU queue can be empty while the frame still awaits TX. */
	spin_lock_irqsave(&report->q_lock, flags);
	skb_queue_walk(&report->queue, skb) {
		if (skb->len < sizeof(*hdr))
			continue;

		hdr = (const void *)skb->data;
		if (ieee80211_is_mgmt(hdr->frame_control) &&
		    ether_addr_equal(hdr->addr2, vif->addr)) {
			empty = false;
			break;
		}
	}
	spin_unlock_irqrestore(&report->q_lock, flags);

	return empty;
}

static void rtw8188e_config_bcn(struct rtw_dev *rtwdev,
				struct ieee80211_vif *vif, bool enable)
{
	struct rtw_sdio *rtwsdio = (struct rtw_sdio *)rtwdev->priv;
	struct rtw8188e_priv *priv = rtwdev->chip_priv;
	struct rtw_vif *rtwvif;
	bool empty;

	if (!enable) {
		if (vif) {
			rtwvif = (struct rtw_vif *)vif->drv_priv;
			/* Finish pending deauthentication before blocking the MACID. */
			if (read_poll_timeout(rtw8188e_ap_mgmt_tx_done, empty,
					      empty, 1000, 100000, false, rtwdev,
					      vif))
				rtw_dbg(rtwdev, RTW_DBG_UNEXP,
					"timed out waiting for AP management TX\n");
			rtw8188e_set_macid_link(rtwdev, rtwvif->mac_id, false);
			rtwvif->bcn_ctrl &= ~BIT_EN_BCN_FUNCTION;
			rtw_write8_clr(rtwdev, rtwvif->conf->bcn_ctrl.addr,
				       BIT_EN_BCN_FUNCTION);
		}
		rtwsdio->irq_mask &= ~REG_SDIO_HIMR_ATIMEND;
		rtw_write32(rtwdev, REG_SDIO_HIMR, rtwsdio->irq_mask);
		clear_bit(RTW8188E_FLAG_BEACON_UPDATE, &priv->flags);
		rtw_write16_clr(rtwdev, REG_CR, BIT_ENSWBCN);
		return;
	}

	rtwvif = (struct rtw_vif *)vif->drv_priv;
	rtw8188e_set_macid_link(rtwdev, rtwvif->mac_id, true);
	rtwvif->bcn_ctrl |= BIT_EN_BCN_FUNCTION | BIT_DIS_BCNQ_SUB;
	rtw_write8_set(rtwdev, rtwvif->conf->bcn_ctrl.addr,
		       BIT_EN_BCN_FUNCTION | BIT_DIS_BCNQ_SUB);
	rtw_write16(rtwdev, REG_MBSSID_BCN_SPACE, vif->bss_conf.beacon_int);
	rtw_write8(rtwdev, REG_ATIMWND, 0x0c);
	rtw_write16_set(rtwdev, REG_CR, BIT_ENSWBCN);
	rtw_write8_set(rtwdev, REG_MBID_NUM, BIT(3) | BIT(4));
	rtw_write16(rtwdev, REG_TSFTR_SYN_OFFSET, 0x7fff);
	rtw_write8(rtwdev, REG_DUAL_TSF_RST, BIT(rtwvif->port));
	rtw_write8(rtwdev, REG_TBTT_PROHIBIT + 1, 0x80);
	rtw_write8(rtwdev, REG_RD_CTRL + 1, 0x6f);
	/* Refresh the template after the TBTT beacon download prohibition. */
	rtwsdio->irq_mask |= REG_SDIO_HIMR_ATIMEND;
	rtw_write32(rtwdev, REG_SDIO_HIMR, rtwsdio->irq_mask);
}

static struct sk_buff *rtw8188e_get_rsvd_beacon(struct rtw_dev *rtwdev,
						struct ieee80211_vif *vif,
						u16 *tim_offset)
{
	struct ieee80211_mutable_offsets offsets = {};
	struct sk_buff *skb;

	/* Building reserved pages must preserve the DTIM count. */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
	skb = ieee80211_beacon_get_template(rtwdev->hw, vif, &offsets, 0);
#else
	skb = ieee80211_beacon_get_template(rtwdev->hw, vif, &offsets);
#endif
	*tim_offset = offsets.tim_offset;
	return skb;
}

static u32 rtw8188e_beacon_size(struct rtw_dev *rtwdev)
{
	struct rtw_rsvd_page *rsvd_pkt;
	u16 pages = rtwdev->fifo.rsvd_drv_pg_num;

	list_for_each_entry(rsvd_pkt, &rtwdev->rsvd_page_list, build_list) {
		if (rsvd_pkt->page)
			pages = min_t(u16, pages, rsvd_pkt->page);
	}

	return pages * rtwdev->chip->page_size - rtwdev->chip->tx_pkt_desc_sz;
}

static void rtw8188e_update_beacon_iter(void *data, struct ieee80211_vif *vif)
{
	struct rtw_dev *rtwdev = data;
	struct sk_buff *skb;
	int ret;

	if ((vif->type != NL80211_IFTYPE_AP &&
	     vif->type != NL80211_IFTYPE_ADHOC) || !vif->bss_conf.enable_beacon)
		return;
	rtw_write16(rtwdev, REG_MBSSID_BCN_SPACE, vif->bss_conf.beacon_int);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
	skb = ieee80211_beacon_get_tim(rtwdev->hw, vif, NULL, NULL, 0);
#else
	skb = ieee80211_beacon_get_tim(rtwdev->hw, vif, NULL, NULL);
#endif
	if (!skb)
		return;
	if (skb->len > rtw8188e_beacon_size(rtwdev)) {
		/* A larger TIM can move the STA's reserved packets. */
		if (rtw_fw_download_rsvd_page(rtwdev))
			goto free;
		rtw_send_rsvd_page_h2c(rtwdev);
		if (skb->len > rtw8188e_beacon_size(rtwdev)) {
			rtw_warn(rtwdev, "beacon exceeds the reserved page buffer\n");
			goto free;
		}
	}
	ret = rtw8188e_write_page(rtwdev, rtwdev->fifo.rsvd_drv_addr,
				  skb->data, skb->len, true);
	if (ret)
		rtw_dbg(rtwdev, RTW_DBG_FW, "failed to update beacon: %d\n", ret);

free:
	dev_kfree_skb(skb);
}

static void rtw8188e_update_beacon(struct rtw_dev *rtwdev)
{
	struct rtw8188e_priv *priv = rtwdev->chip_priv;

	/* The next beacon picks up TIM changes and advances the DTIM count. */
	if (test_and_clear_bit(RTW8188E_FLAG_BEACON_UPDATE, &priv->flags) &&
	    test_bit(RTW_FLAG_RUNNING, rtwdev->flags))
		rtw_iterate_vifs(rtwdev, rtw8188e_update_beacon_iter, rtwdev);
}

static void rtw8188e_sdio_irq_handler(struct rtw_dev *rtwdev, u32 status)
{
	struct rtw_sdio *rtwsdio = (struct rtw_sdio *)rtwdev->priv;
	struct rtw8188e_priv *priv = rtwdev->chip_priv;

	if ((status & REG_SDIO_HISR_ATIMEND) &&
	    (READ_ONCE(rtwsdio->irq_mask) & REG_SDIO_HIMR_ATIMEND) &&
	    test_bit(RTW_FLAG_RUNNING, rtwdev->flags)) {
		set_bit(RTW8188E_FLAG_BEACON_UPDATE, &priv->flags);
		ieee80211_queue_work(rtwdev->hw, &rtwdev->update_beacon_work);
	}
}

static void rtw8188e_set_crystal_cap(struct rtw_dev *rtwdev, u8 crystal_cap)
{
	struct rtw_cfo_track *cfo = &rtwdev->dm_info.cfo_track;

	crystal_cap &= XCAP_MASK;
	if (cfo->crystal_cap == crystal_cap)
		return;

	cfo->crystal_cap = crystal_cap;
	rtw_write32_mask(rtwdev, REG_AFE_CTRL1, GENMASK(22, 11),
			 crystal_cap | (crystal_cap << 6));
}

static void rtw8188e_cfo_init(struct rtw_dev *rtwdev)
{
	struct rtw_cfo_track *cfo = &rtwdev->dm_info.cfo_track;

	memset(cfo, 0, sizeof(*cfo));
	cfo->crystal_cap = rtwdev->efuse.crystal_cap & XCAP_MASK;
	cfo->is_adjust = true;
}

static void rtw8188e_cfo_track(struct rtw_dev *rtwdev)
{
	struct rtw_cfo_track *cfo = &rtwdev->dm_info.cfo_track;
	u8 default_cap = rtwdev->efuse.crystal_cap & XCAP_MASK;
	s8 crystal_cap = cfo->crystal_cap;
	s32 cfo_avg;

	if (rtwdev->sta_cnt != 1) {
		cfo->is_adjust = true;
		cfo->cfo_tail[RF_PATH_A] = 0;
		cfo->cfo_cnt[RF_PATH_A] = 0;
		cfo->packet_count_pre = cfo->packet_count;
		if (crystal_cap > default_cap)
			rtw8188e_set_crystal_cap(rtwdev, crystal_cap - 1);
		else if (crystal_cap < default_cap)
			rtw8188e_set_crystal_cap(rtwdev, crystal_cap + 1);
		rtw_write32_set(rtwdev, REG_OFDM1_CFOTRK, BIT_EN_ATC);
		return;
	}

	if (cfo->packet_count == cfo->packet_count_pre)
		return;
	cfo->packet_count_pre = cfo->packet_count;
	if (!cfo->cfo_cnt[RF_PATH_A])
		return;

	/* Each signed CFO report unit is 2.5 kHz. */
	cfo_avg = cfo->cfo_tail[RF_PATH_A] * 5 / 2 / cfo->cfo_cnt[RF_PATH_A];
	cfo->cfo_tail[RF_PATH_A] = 0;
	cfo->cfo_cnt[RF_PATH_A] = 0;
	if (abs(cfo_avg) > CFO_TRK_ENABLE_TH)
		cfo->is_adjust = true;
	else if (abs(cfo_avg) <= CFO_TRK_STOP_TH)
		cfo->is_adjust = false;

	if (cfo->is_adjust) {
		if (cfo_avg > CFO_TRK_STOP_TH)
			crystal_cap++;
		else if (cfo_avg < -CFO_TRK_STOP_TH)
			crystal_cap--;
		crystal_cap = clamp_t(s8, crystal_cap, 0, XCAP_MASK);
		rtw8188e_set_crystal_cap(rtwdev, crystal_cap);
	}

	rtw_write32_mask(rtwdev, REG_OFDM1_CFOTRK, BIT_EN_ATC,
			 abs(cfo_avg) >= CFO_TH_ATC);
}

static void rtw8188e_set_lna(struct rtw_dev *rtwdev, bool enable)
{
	rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWE, BIT(19), 1);
	rtw_write_rf(rtwdev, RF_PATH_A, RF_MODE_TABLE_ADDR, RFREG_MASK, 0x18000);
	rtw_write_rf(rtwdev, RF_PATH_A, RF_MODE_TABLE_DATA0, RFREG_MASK, 0x0000f);
	rtw_write_rf(rtwdev, RF_PATH_A, RF_MODE_TABLE_DATA1, RFREG_MASK,
		     enable ? 0x77f82 : 0x37f82);
	rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWE, BIT(19), 0);
}

static void rtw8188e_adaptivity_calibrate(struct rtw_dev *rtwdev)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	struct rtw8188e_priv *priv = rtwdev->chip_priv;
	s8 igi = EDCCA_IGI_BASE + 30 + dm_info->l2h_th_ini -
		 EDCCA_L2H_H2L_DIFF;
	s8 l2h = min_t(s8, dm_info->l2h_th_ini + EDCCA_IGI_BASE - igi, 10);
	s8 h2l = l2h - EDCCA_L2H_H2L_DIFF;
	u32 selector = rtw_read32(rtwdev, REG_DBGPORT_SEL_11N);
	u32 report;
	int busy, i;

	rtw8188e_set_lna(rtwdev, false);
	rtw_phy_set_edcca_th(rtwdev, l2h, h2l);
	msleep(30);
	for (;;) {
		rtw_write32(rtwdev, REG_DBGPORT_SEL_11N, 0);
		for (i = 0; i < 3; i++) {
			if (!(rtw_read32(rtwdev, REG_DBGPORT_RPT_11N) & BIT(3)))
				break;
			msleep(3);
		}
		rtw_write32(rtwdev, REG_DBGPORT_SEL_11N, 0x208);
		busy = 0;
		for (i = 0; i < 20; i++) {
			report = rtw_read32(rtwdev, REG_DBGPORT_RPT_11N);
			if (report & (BIT(30) | BIT(29)))
				busy++;
		}
		if (busy <= 1 || l2h == 10)
			break;
		igi--;
		l2h++;
		h2l = l2h - EDCCA_L2H_H2L_DIFF;
		rtw_phy_set_edcca_th(rtwdev, l2h, h2l);
		if (l2h == 10)
			break;
	}
	priv->adaptivity_igi_upper = igi - EDCCA_DC_BACKOFF;
	priv->l2h_th_min = l2h + EDCCA_DC_BACKOFF;
	priv->h2l_th_min = h2l + EDCCA_DC_BACKOFF;
	rtw8188e_set_lna(rtwdev, true);
	rtw_write32(rtwdev, REG_DBGPORT_SEL_11N, selector);
	rtw_phy_set_edcca_th(rtwdev, 0x7f, 0x7f);
}

static void rtw8188e_adaptivity_init(struct rtw_dev *rtwdev)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	struct rtw8188e_priv *priv = rtwdev->chip_priv;

	if (!dm_info->l2h_th_ini)
		dm_info->l2h_th_ini = rtwdev->chip->l2h_th_ini_ad;
	rtw8188e_adaptivity_calibrate(rtwdev);
	if (priv->adaptivity_igi_upper + EDCCA_DC_BACKOFF <= 0x26)
		rtw8188e_adaptivity_calibrate(rtwdev);
	rtw_write32_clr(rtwdev, REG_TX_PTCL_CTRL, BIT_DIS_EDCCA);
	rtw_write32_set(rtwdev, REG_RD_CTRL, BIT_EDCCA_MSK_CNTDOWN_EN);
}

static void rtw8188e_adaptivity(struct rtw_dev *rtwdev)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	struct rtw8188e_priv *priv = rtwdev->chip_priv;
	u8 igi = dm_info->igi_history[0];
	s8 l2h, h2l;

	priv->adaptivity_igi_upper = dm_info->l2h_th_ini + EDCCA_IGI_BASE -
				      priv->l2h_th_min;
	if (dm_info->edcca_mode == RTW_EDCCA_ADAPTIVITY) {
		l2h = min_t(s8, dm_info->l2h_th_ini + EDCCA_IGI_BASE - igi, 10);
		h2l = l2h - EDCCA_L2H_H2L_DIFF;
	} else {
		l2h = 70 - igi;
		h2l = l2h - EDCCA_L2H_H2L_DIFF_NORMAL;
	}
	l2h = max(l2h, priv->l2h_th_min);
	h2l = max(h2l, priv->h2l_th_min);
	rtw_phy_set_edcca_th(rtwdev, l2h, h2l);
}

static void rtw8188e_dig_get_boundary(struct rtw_dev *rtwdev, u8 *upper,
				      u8 *lower)
{
	struct rtw8188e_priv *priv = rtwdev->chip_priv;

	if (rtwdev->dm_info.edcca_mode != RTW_EDCCA_ADAPTIVITY ||
	    !priv->adaptivity_igi_upper)
		return;

	*upper = min(*upper, priv->adaptivity_igi_upper);
	*lower = min(*lower, *upper);
}

static void rtw8188e_phy_bb_config(struct rtw_dev *rtwdev)
{
	rtw_write16_set(rtwdev, REG_SYS_FUNC_EN,
			BIT_FEN_EN_25_1 | BIT_FEN_BB_GLB_RST | BIT_FEN_BB_RSTB);
	rtw_write8(rtwdev, REG_RF_CTRL, WLAN_RF_CTRL_ENABLE);
}

static void rtw8188e_phy_load_bb_tables(struct rtw_dev *rtwdev)
{
	const struct rtw_chip_info *chip = rtwdev->chip;
	u8 xtal_cap = rtwdev->efuse.crystal_cap & 0x3f;

	rtw_load_table(rtwdev, chip->bb_tbl);
	rtw_load_table(rtwdev, chip->agc_tbl);
	rtw_write32_mask(rtwdev, REG_AFE_CTRL1, GENMASK(22, 11),
			 xtal_cap | (xtal_cap << 6));
}

static void rtw8188e_phy_rf6052_config(struct rtw_dev *rtwdev)
{
	u32 val32;

	val32 = rtw_read32_mask(rtwdev, REG_FPGA0_XA_RF_SW_CTRL, RFSI_RFENV);
	rtw_write32_set(rtwdev, REG_FPGA0_XA_RF_INT_OE, RFSI_RFENV << 16);
	udelay(1);
	rtw_write32_set(rtwdev, REG_FPGA0_XA_RF_INT_OE, RFSI_RFENV);
	udelay(1);
	rtw_write32_clr(rtwdev, REG_FPGA0_XA_HSSI_PARM2,
			HSSI_3WIRE_ADDR_LEN | HSSI_3WIRE_DATA_LEN);
	udelay(1);
	rtw_load_table(rtwdev, rtwdev->chip->rf_tbl[RF_PATH_A]);
	rtw_write32_mask(rtwdev, REG_FPGA0_XA_RF_SW_CTRL, RFSI_RFENV, val32);
}

static void rtw8188e_init_transfer_page_size(struct rtw_dev *rtwdev)
{
	rtw_write8(rtwdev, REG_PBP,
		   u8_encode_bits(PBP_128, PBP_RX_MASK) |
		   u8_encode_bits(PBP_128, PBP_TX_MASK));
}

static void rtw8188e_init_adaptive_ctrl(struct rtw_dev *rtwdev)
{
	rtw_write8_set(rtwdev, REG_FWHW_TXQ_CTRL, BIT(7));
	rtw_write8(rtwdev, REG_ACKTO, 0x40);
	rtw_write16(rtwdev, REG_RETRY_LIMIT, 0x3030);
}

static void rtw8188e_init_beacon_parameters(struct rtw_dev *rtwdev)
{
	rtw_write16(rtwdev, REG_BCN_CTRL,
		    BIT_DIS_TSF_UDT | (BIT_DIS_TSF_UDT << 8));
	rtw_write8(rtwdev, REG_TBTT_PROHIBIT, TBTT_PROHIBIT_SETUP_TIME);
	rtw_write8(rtwdev, REG_TBTT_PROHIBIT + 1,
		   TBTT_PROHIBIT_HOLD_TIME_STOP_BCN);
	rtw_write8_clr(rtwdev, REG_TBTT_PROHIBIT + 2, GENMASK(3, 0));
	rtw_write8(rtwdev, REG_BCNDMATIM, WLAN_BCN_DMA_TIME);
	rtw_write8(rtwdev, REG_DRVERLYINT, 0x05);
	rtw_write16(rtwdev, REG_BCNTCFG, 0x4413);
}

static void rtw8188e_init_pa_bias(struct rtw_dev *rtwdev)
{
	u8 pa;
	int ret;

	rtw_chip_efuse_grant_on(rtwdev);
	ret = rtw_read8_physical_efuse(rtwdev, 0x1fa, &pa);
	rtw_chip_efuse_grant_off(rtwdev);
	if (ret) {
		rtw_warn(rtwdev, "failed to read PA bias efuse: %d\n", ret);
		return;
	}
	if (!(pa & BIT(0))) {
		rtw_write_rf(rtwdev, RF_PATH_A, 0x15, RFREG_MASK, 0x0f406);
		rtw_write_rf(rtwdev, RF_PATH_A, 0x15, RFREG_MASK, 0x4f406);
		rtw_write_rf(rtwdev, RF_PATH_A, 0x15, RFREG_MASK, 0x8f406);
		rtw_write_rf(rtwdev, RF_PATH_A, 0x15, RFREG_MASK, 0xcf406);
	}
	if (!(pa & BIT(4))) {
		u8 value = rtw_read8(rtwdev, 0x16) & 0x0f;

		rtw_write8(rtwdev, 0x16, value | 0x80);
		rtw_write8(rtwdev, 0x16, value | 0x90);
	}
}

static u8 rtw8188e_default_ofdm_index(struct rtw_dev *rtwdev)
{
	u32 swing;
	u8 i;

	swing = rtw_read32_mask(rtwdev, REG_OFDM_0_XA_TX_IQ_IMBALANCE,
				GENMASK(31, 22));
	for (i = 0; i < RTW8188E_OFDM_SWING_TABLE_SIZE; i++) {
		if (u32_get_bits(rtw8188e_ofdm_swing_table[i],
				 GENMASK(31, 22)) == swing)
			return i;
	}

	return 30;
}

static u8 rtw8188e_default_cck_index(struct rtw_dev *rtwdev)
{
	u8 swing = rtw_read8(rtwdev, REG_CCK_SWING);
	u8 i;

	for (i = 0; i < RTW8188E_CCK_SWING_TABLE_SIZE; i++) {
		if (rtw8188e_cck_swing_table_ch1_ch13[i][0] == swing)
			return i;
	}

	return 20;
}

static void rtw8188e_pwrtrack_init(struct rtw_dev *rtwdev)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	u8 thermal = rtwdev->efuse.thermal_meter[RF_PATH_A];

	dm_info->default_ofdm_index = rtw8188e_default_ofdm_index(rtwdev);
	dm_info->default_cck_index = rtw8188e_default_cck_index(rtwdev);
	dm_info->delta_power_index[RF_PATH_A] = 0;
	dm_info->txagc_remnant_ofdm[RF_PATH_A] = 0;
	dm_info->txagc_remnant_cck = 0;
	dm_info->pwr_trk_triggered = false;
	dm_info->pwr_trk_init_trigger = true;
	dm_info->thermal_meter_k = thermal;
	dm_info->thermal_meter_lck = thermal;
	ewma_thermal_init(&dm_info->avg_thermal[RF_PATH_A]);
}

static void rtw8188e_set_iqk_matrix(struct rtw_dev *rtwdev, u8 index)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	u32 swing = rtw8188e_ofdm_swing_table[index];
	s32 x, y, a, c, d;

	if (!dm_info->iqk.done) {
		rtw_write32(rtwdev, REG_OFDM_0_XA_TX_IQ_IMBALANCE, swing);
		rtw_write32_mask(rtwdev, REG_TXIQK_MATRIXA_LSB2_11N,
				 GENMASK(31, 28), 0);
		rtw_write32_clr(rtwdev, REG_OFDM_0_ECCA_THRESHOLD, BIT(24));
		return;
	}

	x = sign_extend32(dm_info->iqk.result.s1_x, 9);
	y = sign_extend32(dm_info->iqk.result.s1_y, 9);
	d = u32_get_bits(swing, GENMASK(31, 22));
	a = (x * d) >> 8;
	c = (y * d) >> 8;
	rtw_write32(rtwdev, REG_OFDM_0_XA_TX_IQ_IMBALANCE,
		    u32_encode_bits(d, GENMASK(31, 22)) |
		    u32_encode_bits(c & 0x3f, GENMASK(21, 16)) |
		    u32_encode_bits(a & 0x3ff, GENMASK(9, 0)));
	rtw_write32_mask(rtwdev, REG_TXIQK_MATRIXA_LSB2_11N,
			 GENMASK(31, 28), (c >> 6) & 0xf);
	rtw_write32_mask(rtwdev, REG_OFDM_0_ECCA_THRESHOLD, BIT(24),
			 ((x * d) >> 7) & 1);
}

static void rtw8188e_pwrtrack_set_cck_pwr(struct rtw_dev *rtwdev, u8 channel)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	const u8 *table;
	s8 index, swing;
	int i;

	index = dm_info->default_cck_index +
		dm_info->delta_power_index[RF_PATH_A];
	/* Limit BB swing to -2 dB; apply the remainder through TXAGC. */
	swing = clamp_t(s8, index, 0, 28);
	dm_info->txagc_remnant_cck = index - swing;
	table = channel == 14 ? rtw8188e_cck_swing_table_ch14[swing] :
			       rtw8188e_cck_swing_table_ch1_ch13[swing];
	for (i = 0; i < ARRAY_SIZE(rtw8188e_cck_swing_table_ch14[0]); i++)
		rtw_write8(rtwdev, REG_CCK_SWING + i, table[i]);
}

static void rtw8188e_phy_set_param(struct rtw_dev *rtwdev)
{
	const struct rtw_chip_info *chip = rtwdev->chip;

	rtw_load_table(rtwdev, chip->mac_tbl);
	rtw8188e_phy_bb_config(rtwdev);
	rtw8188e_phy_load_bb_tables(rtwdev);
	rtw8188e_phy_rf6052_config(rtwdev);

	rtw_write32_set(rtwdev, REG_FPGA0_RFMOD, BIT_CCKEN | BIT_OFDMEN);

	rtw8188e_init_transfer_page_size(rtwdev);
	rtw8188e_init_adaptive_ctrl(rtwdev);
	rtw8188e_init_beacon_parameters(rtwdev);

	rtw_write8(rtwdev, REG_HWSEQ_CTRL, 0xff);
	rtw_write8(rtwdev, REG_AMPDU_MAX_TIME, 0x70);
	rtw_write32(rtwdev, REG_BAR_MODE_CTRL, WLAN_BAR_VAL);
	rtw_write8(rtwdev, REG_SDIO_TX_CTRL, 0x02);
	rtw_write8_set(rtwdev, REG_CR, BIT_MACTXEN | BIT_MACRXEN);

	rtw_phy_init(rtwdev);
	rtw8188e_pwrtrack_init(rtwdev);
	rtw8188e_init_pa_bias(rtwdev);
}

static void rtw8188e_set_channel(struct rtw_dev *rtwdev, u8 channel,
				 u8 bw, u8 primary)
{
	bool wide = bw == RTW_CHANNEL_WIDTH_40;
	bool upper = primary == RTW_SC_20_UPPER;
	u32 rf = rtw_read_rf(rtwdev, RF_PATH_A, RF_CFGCH, RFREG_MASK);

	rtw_write8_mask(rtwdev, REG_BWOPMODE, BIT(2), !wide);
	rtw_write32_mask(rtwdev, REG_FPGA0_RFMOD, BIT(0), wide);
	rtw_write32_mask(rtwdev, REG_FPGA1_RFMOD, BIT(0), wide);
	if (wide) {
		rtw_write8_mask(rtwdev, REG_RRSR + 2, GENMASK(6, 5), upper ? 2 : 1);
		rtw_write32_mask(rtwdev, REG_CCK0_SYS, BIT(4), upper);
		rtw_write32_mask(rtwdev, REG_OFDM_FA_RSTD_11N, GENMASK(11, 10),
				 upper ? 2 : 1);
		rtw_write32_mask(rtwdev, REG_BB_PWR_SAV5_11N, GENMASK(27, 26),
				 upper ? 1 : 2);
	}
	rf &= ~(GENMASK(9, 0) | GENMASK(11, 10));
	rf |= channel | ((wide ? 1 : 3) << 10);
	rtw_write_rf(rtwdev, RF_PATH_A, RF_CFGCH, RFREG_MASK, rf);
	rtw8188e_pwrtrack_set_cck_pwr(rtwdev, channel);
}

static s8 rtw8188e_cck_rx_power(u8 lna_idx, u8 vga_idx)
{
	static const s8 lna_gain_table[] = {29, 20, 12, 3, -6, -15, -24, -33};

	return lna_gain_table[lna_idx] - 2 * vga_idx;
}

static void rtw8188e_query_phy_status_cck(struct rtw_dev *rtwdev, void *phy_raw,
					  struct rtw_rx_pkt_stat *pkt_stat)
{
	struct phy_status_8188e *phy_status = phy_raw;
	u8 agc = phy_status->cck_agc_rpt_ofdm_cfosho_a;
	u8 lna_idx = u8_get_bits(agc, GENMASK(7, 5));
	u8 vga_idx = u8_get_bits(agc, GENMASK(4, 0));
	s8 rx_power = rtw8188e_cck_rx_power(lna_idx, vga_idx);

	pkt_stat->bw = RTW_CHANNEL_WIDTH_20;
	pkt_stat->rx_power[RF_PATH_A] = rx_power;
	pkt_stat->signal_power = rx_power;
	pkt_stat->rssi = rtw_phy_rf_power_2_rssi(pkt_stat->rx_power, 1);
	rtwdev->dm_info.rssi[RF_PATH_A] = pkt_stat->rssi;
}

static void rtw8188e_query_phy_status_ofdm(struct rtw_dev *rtwdev, void *phy_raw,
					   struct rtw_rx_pkt_stat *pkt_stat)
{
	struct phy_status_8188e *phy_status = phy_raw;
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	s8 rx_power, evm;

	rx_power = (phy_status->path_agc[RF_PATH_A] & GENMASK(5, 0)) * 2 - 110;
	pkt_stat->rx_power[RF_PATH_A] = rx_power;
	pkt_stat->rssi = rtw_phy_rf_power_2_rssi(pkt_stat->rx_power, 1);
	pkt_stat->signal_power =
		((phy_status->cck_sig_qual_ofdm_pwdb_all >> 1) & 0x7f) - 110;
	pkt_stat->rx_snr[RF_PATH_A] = phy_status->path_rxsnr[RF_PATH_A] / 2;
	pkt_stat->rx_evm[RF_PATH_A] = phy_status->stream_rxevm[RF_PATH_A];
	pkt_stat->cfo_tail[RF_PATH_A] = phy_status->path_cfotail[RF_PATH_A];
	dm_info->rssi[RF_PATH_A] = pkt_stat->rssi;
	dm_info->rx_snr[RF_PATH_A] = pkt_stat->rx_snr[RF_PATH_A];
	dm_info->cfo_tail[RF_PATH_A] = pkt_stat->cfo_tail[RF_PATH_A] * 5 / 2;
	evm = phy_status->stream_rxevm[RF_PATH_A];
	dm_info->rx_evm_dbm[RF_PATH_A] = clamp_t(s8, -evm / 2, 0, 64) & 0x3f;
	rtw_phy_parsing_cfo(rtwdev, pkt_stat);
}

static void rtw8188e_query_phy_status(struct rtw_dev *rtwdev, u8 *phy_status,
				      struct rtw_rx_pkt_stat *pkt_stat)
{
	/* The PHY status does not include a channel number. */
	pkt_stat->channel_invalid = true;

	if (pkt_stat->rate <= DESC_RATE11M)
		rtw8188e_query_phy_status_cck(rtwdev, phy_status, pkt_stat);
	else
		rtw8188e_query_phy_status_ofdm(rtwdev, phy_status, pkt_stat);
}

static void rtw8188e_lck(struct rtw_dev *rtwdev)
{
	u32 rf18, lc_cal;
	u8 val_ctx, rf_val, tx_pause;
	int ret;

	tx_pause = rtw_read8(rtwdev, REG_TXPAUSE);
	val_ctx = rtw_read8(rtwdev, REG_CTX);
	if (val_ctx & BIT_MASK_CTX_TYPE)
		rtw_write8(rtwdev, REG_CTX, val_ctx & ~BIT_MASK_CTX_TYPE);
	else
		rtw_write8(rtwdev, REG_TXPAUSE, 0xff);
	rf18 = rtw_read_rf(rtwdev, RF_PATH_A, RF_CFGCH, RFREG_MASK);
	/* LCK uses the low 12 bits of the channel configuration. */
	lc_cal = rf18 & GENMASK(11, 0);
	rtw_dbg(rtwdev, RTW_DBG_RFK, "LCK start: ctx=%02x rf18=%05x cal=%03x\n",
		val_ctx, rf18, lc_cal);
	rtw_write_rf(rtwdev, RF_PATH_A, RF_CFGCH, RFREG_MASK, lc_cal | BIT_LCK);
	/* Allow the oscillator to settle before polling for completion. */
	msleep(100);
	ret = read_poll_timeout(rtw_read_rf, rf_val, rf_val != 0x1,
				10000, 1000000, false,
				rtwdev, RF_PATH_A, RF_CFGCH, BIT_LCK);
	if (ret)
		rtw_warn(rtwdev, "failed to poll LCK status bit\n");

	rtw_write_rf(rtwdev, RF_PATH_A, RF_CFGCH, RFREG_MASK, lc_cal);
	if (val_ctx & BIT_MASK_CTX_TYPE)
		rtw_write8(rtwdev, REG_CTX, val_ctx);
	else
		rtw_write8(rtwdev, REG_TXPAUSE, tx_pause);
	rtw_dbg(rtwdev, RTW_DBG_RFK, "LCK done: ret=%d bit15=%u restore=%05x\n",
		ret, rf_val, lc_cal);
}

static bool rtw8188e_iqk_tx_ok(struct rtw_dev *rtwdev, bool rx_imr)
{
	u32 eac = rtw_read32(rtwdev, REG_IQK_RES_RY);
	u32 e94 = rtw_read32(rtwdev, REG_IQK_RES_TX);
	u32 e9c = rtw_read32(rtwdev, REG_IQK_RES_TY);
	bool ok = !(eac & BIT(28)) &&
		  u32_get_bits(e94, BIT_MASK_IQK_RESULT) != IQK_TX_X_ERR &&
		  u32_get_bits(e9c, BIT_MASK_IQK_RESULT) != IQK_TX_Y_ERR;

	rtw_dbg(rtwdev, RTW_DBG_RFK,
		"IQK %s: eac=%08x e94=%08x e9c=%08x ok=%d\n",
		rx_imr ? "TXIMR" : "TX", eac, e94, e9c, ok);
	return ok;
}

static void rtw8188e_iqk_trigger(struct rtw_dev *rtwdev)
{
	rtw_write32(rtwdev, REG_IQK_AGC_PTS_11N, 0xf9000000);
	rtw_write32(rtwdev, REG_IQK_AGC_PTS_11N, 0xf8000000);
	msleep(IQK_DELAY_TIME_8188E);
}

static bool rtw8188e_iqk_tx_path_a(struct rtw_dev *rtwdev, bool rx_imr)
{
	if (rx_imr) {
		rtw_write32_mask(rtwdev, REG_FPGA0_IQK_11N, BIT_MASK_IQK_MODE, 0);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWE, RFREG_MASK, 0x800a0);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_MODE_TABLE_ADDR, RFREG_MASK, 0x30000);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_MODE_TABLE_DATA0, RFREG_MASK, 0x0000f);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_MODE_TABLE_DATA1, RFREG_MASK, 0xf117b);
		rtw_write32_mask(rtwdev, REG_FPGA0_IQK_11N, BIT_MASK_IQK_MODE, 0x808000);
		rtw_write32(rtwdev, REG_TXIQK_11N, 0x01007c00);
		rtw_write32(rtwdev, REG_RXIQK_11N, 0x81004800);
	}
	rtw_write32(rtwdev, REG_TXIQK_TONE_A_11N, 0x10008c1c);
	rtw_write32(rtwdev, REG_RXIQK_TONE_A_11N, 0x30008c1c);
	rtw_write32(rtwdev, REG_TXIQK_PI_A_11N, rx_imr ? 0x82160804 : 0x8214032a);
	rtw_write32(rtwdev, REG_RXIQK_PI_A_11N, 0x28160000);
	rtw_write32(rtwdev, REG_IQK_AGC_RSP_11N, rx_imr ? 0x0046a911 : 0x00462911);
	rtw8188e_iqk_trigger(rtwdev);
	return rtw8188e_iqk_tx_ok(rtwdev, rx_imr);
}

static bool rtw8188e_iqk_rx_path_a(struct rtw_dev *rtwdev)
{
	u32 imr, eac, ea4;
	bool ok;

	if (!rtw8188e_iqk_tx_path_a(rtwdev, true))
		return false;
	imr = 0x80007c00 |
	      (rtw_read32(rtwdev, REG_IQK_RES_TX) & BIT_MASK_IQK_RESULT) |
	      rtw_read32_mask(rtwdev, REG_IQK_RES_TY, BIT_MASK_IQK_RESULT);
	rtw_write32(rtwdev, REG_TXIQK_11N, imr);
	rtw_write32_mask(rtwdev, REG_FPGA0_IQK_11N, BIT_MASK_IQK_MODE, 0);
	rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWE, RFREG_MASK, 0x800a0);
	rtw_write_rf(rtwdev, RF_PATH_A, RF_MODE_TABLE_ADDR, RFREG_MASK, 0x30000);
	rtw_write_rf(rtwdev, RF_PATH_A, RF_MODE_TABLE_DATA0, RFREG_MASK, 0x0000f);
	rtw_write_rf(rtwdev, RF_PATH_A, RF_MODE_TABLE_DATA1, RFREG_MASK, 0xf7ffa);
	rtw_write32_mask(rtwdev, REG_FPGA0_IQK_11N, BIT_MASK_IQK_MODE, 0x808000);
	rtw_write32(rtwdev, REG_RXIQK_11N, 0x01004800);
	rtw_write32(rtwdev, REG_TXIQK_TONE_A_11N, 0x30008c1c);
	rtw_write32(rtwdev, REG_RXIQK_TONE_A_11N, 0x10008c1c);
	rtw_write32(rtwdev, REG_TXIQK_PI_A_11N, 0x82160c05);
	rtw_write32(rtwdev, REG_RXIQK_PI_A_11N, 0x28160c05);
	rtw_write32(rtwdev, REG_IQK_AGC_RSP_11N, 0x0046a911);
	rtw8188e_iqk_trigger(rtwdev);
	eac = rtw_read32(rtwdev, REG_IQK_RES_RY);
	ea4 = rtw_read32(rtwdev, REG_IQK_RES_RX);
	ok = !(eac & BIT(27)) &&
	     u32_get_bits(ea4, BIT_MASK_IQK_RESULT) != IQK_RX_X_ERR &&
	     u32_get_bits(eac, BIT_MASK_IQK_RESULT) != IQK_RX_Y_ERR;
	rtw_dbg(rtwdev, RTW_DBG_RFK, "IQK RX: eac=%08x ea4=%08x ok=%d\n",
		eac, ea4, ok);
	return ok;
}

static void rtw8188e_iqk_backup_regs(struct rtw_dev *rtwdev,
				     struct rtw8188e_iqk_backup_regs *backup)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(rtw8188e_iqk_adda_regs); i++)
		backup->adda[i] = rtw_read32(rtwdev, rtw8188e_iqk_adda_regs[i]);
	for (i = 0; i < ARRAY_SIZE(rtw8188e_iqk_mac8_regs); i++)
		backup->mac8[i] = rtw_read8(rtwdev, rtw8188e_iqk_mac8_regs[i]);
	for (i = 0; i < ARRAY_SIZE(rtw8188e_iqk_mac32_regs); i++)
		backup->mac32[i] = rtw_read32(rtwdev, rtw8188e_iqk_mac32_regs[i]);
	for (i = 0; i < ARRAY_SIZE(rtw8188e_iqk_bb_regs); i++)
		backup->bb[i] = rtw_read32(rtwdev, rtw8188e_iqk_bb_regs[i]);
}

static void rtw8188e_iqk_restore_regs(struct rtw_dev *rtwdev,
				      const struct rtw8188e_iqk_backup_regs *backup)
{
	int i;

	for (i = 0; i < RTW8188E_IQK_ADDA_REG_NUM; i++)
		rtw_write32(rtwdev, rtw8188e_iqk_adda_regs[i], backup->adda[i]);
	for (i = 0; i < RTW8188E_IQK_MAC8_REG_NUM; i++)
		rtw_write8(rtwdev, rtw8188e_iqk_mac8_regs[i], backup->mac8[i]);
	for (i = 0; i < RTW8188E_IQK_MAC32_REG_NUM; i++)
		rtw_write32(rtwdev, rtw8188e_iqk_mac32_regs[i], backup->mac32[i]);
	for (i = 0; i < RTW8188E_IQK_BB_REG_NUM; i++)
		rtw_write32(rtwdev, rtw8188e_iqk_bb_regs[i], backup->bb[i]);

	/* Restore RX gain through RF mode. */
	rtw_write_rf(rtwdev, RF_PATH_A, RF_MODE, RFREG_MASK, 0x32ed3);
	rtw_write32(rtwdev, REG_TXIQK_TONE_A_11N, 0x01008c00);
	rtw_write32(rtwdev, REG_RXIQK_TONE_A_11N, 0x01008c00);
}

static bool rtw8188e_iqk_similarity_cmp(s32 result[][IQK_NR], u8 c1, u8 c2)
{
	u8 candidate = IQK_ROUND_INVALID;
	u32 bitmap = 0;
	s32 x, y;
	int i;

	for (i = 0; i < IQK_NR; i++) {
		x = sign_extend32(result[c1][i], 9);
		y = sign_extend32(result[c2][i], 9);
		if (abs(x - y) <= MAX_TOLERANCE)
			continue;

		if (i == IQK_RX_X && !bitmap) {
			if (!result[c1][IQK_RX_X] && !result[c1][IQK_RX_Y]) {
				candidate = c2;
				continue;
			}
			if (!result[c2][IQK_RX_X] && !result[c2][IQK_RX_Y]) {
				candidate = c1;
				continue;
			}
		}
		bitmap |= BIT(i);
	}

	if (!bitmap) {
		if (candidate == IQK_ROUND_INVALID)
			return true;

		result[IQK_ROUND_HYBRID][IQK_TX_X] = result[candidate][IQK_TX_X];
		result[IQK_ROUND_HYBRID][IQK_TX_Y] = result[candidate][IQK_TX_Y];
		return false;
	}

	for (i = 0; i < IQK_NR; i += 2) {
		if (bitmap & GENMASK(i + 1, i))
			continue;

		result[IQK_ROUND_HYBRID][i] = result[c1][i];
		result[IQK_ROUND_HYBRID][i + 1] = result[c1][i + 1];
	}

	return false;
}

static void rtw8188e_iqk_fill_matrix(struct rtw_dev *rtwdev, const s32 result[])
{
	s32 x, y, scale, a, c;

	x = sign_extend32(result[IQK_TX_X], 9);
	y = sign_extend32(result[IQK_TX_Y], 9);
	scale = rtw_read32_mask(rtwdev, REG_OFDM_0_XA_TX_IQ_IMBALANCE, GENMASK(31, 22));
	a = (x * scale) >> 8;
	c = (y * scale) >> 8;
	rtw_write32_mask(rtwdev, REG_OFDM_0_XA_TX_IQ_IMBALANCE, GENMASK(9, 0), a & 0x3ff);
	rtw_write32_mask(rtwdev, REG_OFDM_0_ECCA_THRESHOLD, BIT(31), (x * scale >> 7) & 1);
	rtw_write32_mask(rtwdev, REG_TXIQK_MATRIXA_LSB2_11N, GENMASK(31, 28), (c >> 6) & 0xf);
	rtw_write32_mask(rtwdev, REG_OFDM_0_XA_TX_IQ_IMBALANCE, GENMASK(21, 16), c & 0x3f);
	rtw_write32_mask(rtwdev, REG_OFDM_0_ECCA_THRESHOLD, BIT(29), (y * scale >> 7) & 1);

	if (!result[IQK_RX_X])
		return;

	rtw_write32_mask(rtwdev, REG_A_RXIQI, GENMASK(9, 0), result[IQK_RX_X]);
	rtw_write32_mask(rtwdev, REG_A_RXIQI, GENMASK(15, 10), result[IQK_RX_Y] & 0x3f);
	rtw_write32_mask(rtwdev, REG_RXIQK_MATRIX_LSB_11N, GENMASK(31, 28),
			 (result[IQK_RX_Y] >> 6) & 0xf);
}

static void
rtw8188e_iqk_one_round(struct rtw_dev *rtwdev, s32 result[IQK_NR], int round,
		       const struct rtw8188e_iqk_backup_regs *backup, bool pi_enabled)
{
	int i, retry;

	for (i = 0; i < RTW8188E_IQK_ADDA_REG_NUM; i++)
		rtw_write32(rtwdev, rtw8188e_iqk_adda_regs[i],
			    i ? 0x0bdb25a0 : 0x0b1b25a0);
	if (!pi_enabled) {
		rtw_write32(rtwdev, REG_FPGA0_XA_HSSI_PARM1, 0x01000100);
		rtw_write32(rtwdev, REG_FPGA0_XB_HSSI_PARM1, 0x01000100);
	}
	rtw_write8(rtwdev, REG_TXPAUSE, 0x3f);
	rtw_write8(rtwdev, REG_BCN_CTRL, backup->mac8[1] & ~BIT(3));
	rtw_write8(rtwdev, REG_BCN_CTRL + 1, backup->mac8[2] & ~BIT(3));
	rtw_write8(rtwdev, REG_GPIO_MUXCFG, backup->mac32[0] & ~BIT(5));
	rtw_write32_mask(rtwdev, REG_CCK_ANT_SEL_11N, GENMASK(27, 24), 0xf);
	rtw_write32(rtwdev, REG_BB_RX_PATH_11N, 0x03a05600);
	rtw_write32(rtwdev, REG_TRMUX_11N, 0x000800e4);
	rtw_write32(rtwdev, REG_BB_PWR_SAV1_11N, 0x22204000);
	rtw_write32_set(rtwdev, REG_FPGA0_XA_RF_SW_CTRL, BIT(10) | BIT(26));
	rtw_write32_clr(rtwdev, REG_FPGA0_XA_RF_INT_OE, BIT(10));
	rtw_write32_clr(rtwdev, REG_FPGA0_XB_RF_INT_OE, BIT(10));
	rtw_write32(rtwdev, REG_CONFIG_ANT_A_11N, 0x0f600000);
	rtw_write32_mask(rtwdev, REG_FPGA0_IQK_11N, BIT_MASK_IQK_MODE, 0x808000);
	rtw_write32(rtwdev, REG_TXIQK_11N, 0x01007c00);
	rtw_write32(rtwdev, REG_RXIQK_11N, 0x81004800);

	/* Use the neutral matrix for a phase if both attempts fail. */
	result[IQK_TX_X] = IQK_COEFF_UNITY;
	result[IQK_RX_X] = IQK_COEFF_UNITY;

	for (retry = 0; retry < PATH_IQK_RETRY; retry++) {
		rtw_dbg(rtwdev, RTW_DBG_RFK, "IQK round %d TX attempt %d\n",
			round, retry);
		if (!rtw8188e_iqk_tx_path_a(rtwdev, false))
			continue;
		result[IQK_TX_X] = rtw_read32_mask(rtwdev,
						   REG_IQK_RES_TX, BIT_MASK_IQK_RESULT);
		result[IQK_TX_Y] = rtw_read32_mask(rtwdev,
						   REG_IQK_RES_TY, BIT_MASK_IQK_RESULT);
		break;
	}
	for (retry = 0; retry < PATH_IQK_RETRY; retry++) {
		rtw_dbg(rtwdev, RTW_DBG_RFK, "IQK round %d RX attempt %d\n",
			round, retry);
		if (!rtw8188e_iqk_rx_path_a(rtwdev))
			continue;
		result[IQK_RX_X] = rtw_read32_mask(rtwdev,
						   REG_IQK_RES_RX, BIT_MASK_IQK_RESULT);
		result[IQK_RX_Y] = rtw_read32_mask(rtwdev,
						   REG_IQK_RES_RY, BIT_MASK_IQK_RESULT);
		break;
	}
	rtw_dbg(rtwdev, RTW_DBG_RFK, "IQK round %d: TX %x %x RX %x %x\n",
		round, result[IQK_TX_X], result[IQK_TX_Y],
		result[IQK_RX_X], result[IQK_RX_Y]);
	rtw_write32_mask(rtwdev, REG_FPGA0_IQK_11N, BIT_MASK_IQK_MODE, 0);
}

static void rtw8188e_phy_calibration(struct rtw_dev *rtwdev)
{
	struct rtw8188e_iqk_backup_regs backup;
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	s32 result[IQK_ROUND_SIZE][IQK_NR] = {};
	u32 hssi_a, hssi_b;
	u8 candidate = IQK_ROUND_INVALID;
	int round, i, prev;
	s32 hybrid_sum = 0;
	bool similar;

	/* Defer calibration until the scan restores the operating channel. */
	if (test_bit(RTW_FLAG_SCANNING, rtwdev->flags)) {
		rtwdev->need_rfk = true;
		rtw_dbg(rtwdev, RTW_DBG_RFK, "IQK deferred during scan\n");
		return;
	}

	/* Calibration requires the baseband and RF to be awake. */
	rtw_leave_lps(rtwdev);
	hssi_a = rtw_read32(rtwdev, REG_FPGA0_XA_HSSI_PARM1);
	hssi_b = rtw_read32(rtwdev, REG_FPGA0_XB_HSSI_PARM1);
	rtw_dbg(rtwdev, RTW_DBG_RFK, "IQK start: ch=%u bw=%u hssi=%08x/%08x\n",
		rtwdev->hal.current_channel, rtwdev->hal.current_band_width,
		hssi_a, hssi_b);
	rtw8188e_lck(rtwdev);
	rtw8188e_iqk_backup_regs(rtwdev, &backup);
	for (round = IQK_ROUND_0; round <= IQK_ROUND_2; round++) {
		rtw8188e_iqk_one_round(rtwdev, result[round], round, &backup,
				       hssi_a & BIT(8));
		if (round == IQK_ROUND_0)
			continue;

		rtw_write32(rtwdev, REG_FPGA0_XA_HSSI_PARM1, hssi_a);
		rtw_write32(rtwdev, REG_FPGA0_XB_HSSI_PARM1, hssi_b);
		rtw8188e_iqk_restore_regs(rtwdev, &backup);
		for (prev = IQK_ROUND_0; prev < round; prev++) {
			similar = rtw8188e_iqk_similarity_cmp(result, prev, round);
			rtw_dbg(rtwdev, RTW_DBG_RFK, "IQK compare %d/%d: similar=%d\n",
				prev, round, similar);
			if (similar) {
				candidate = prev;
				break;
			}
		}
		if (candidate != IQK_ROUND_INVALID)
			break;
	}
	if (candidate == IQK_ROUND_INVALID) {
		for (i = 0; i < IQK_NR; i++)
			hybrid_sum += result[IQK_ROUND_HYBRID][i];
		if (hybrid_sum)
			candidate = IQK_ROUND_HYBRID;
	}
	if (candidate == IQK_ROUND_INVALID) {
		rtw_dbg(rtwdev, RTW_DBG_RFK,
			"IQK failure: candidate %u hybrid TX %x %x RX %x %x\n",
			candidate, result[IQK_ROUND_HYBRID][IQK_TX_X],
			result[IQK_ROUND_HYBRID][IQK_TX_Y],
			result[IQK_ROUND_HYBRID][IQK_RX_X],
			result[IQK_ROUND_HYBRID][IQK_RX_Y]);
		rtw_warn(rtwdev, "RTL8188E IQ calibration failed\n");
		return;
	}
	rtw_dbg(rtwdev, RTW_DBG_RFK, "IQK candidate %d: TX %x %x RX %x %x\n",
		candidate, result[candidate][IQK_TX_X], result[candidate][IQK_TX_Y],
		result[candidate][IQK_RX_X], result[candidate][IQK_RX_Y]);
	/* Keep the previous TX coefficients for power tracking. */
	if (!result[candidate][IQK_TX_X])
		return;

	rtw8188e_iqk_fill_matrix(rtwdev, result[candidate]);
	dm_info->iqk.result.s1_x = result[candidate][IQK_TX_X];
	dm_info->iqk.result.s1_y = result[candidate][IQK_TX_Y];
	dm_info->iqk.done = true;
	dm_info->pwr_trk_init_trigger = true;
}

static u8 rtw8188e_pwrtrack_get_limit_ofdm(struct rtw_dev *rtwdev)
{
	switch (rtwdev->dm_info.tx_rate) {
	case DESC_RATE6M ... DESC_RATE48M:
		return 36;
	case DESC_RATE54M:
		return 34;
	case DESC_RATEMCS0 ... DESC_RATEMCS2:
		return 38;
	case DESC_RATEMCS3 ... DESC_RATEMCS4:
		return 36;
	case DESC_RATEMCS5 ... DESC_RATEMCS7:
		return 34;
	default:
		return 30;
	}
}

static void rtw8188e_pwrtrack_set(struct rtw_dev *rtwdev, s8 power_index)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	u8 limit = rtw8188e_pwrtrack_get_limit_ofdm(rtwdev);
	s8 index, swing;

	index = dm_info->default_ofdm_index + power_index;
	swing = clamp_t(s8, index, 0, limit);
	if (!dm_info->pwr_trk_init_trigger &&
	    dm_info->delta_power_index[RF_PATH_A] == power_index &&
	    dm_info->txagc_remnant_ofdm[RF_PATH_A] == index - swing)
		return;

	dm_info->pwr_trk_init_trigger = false;
	dm_info->delta_power_index[RF_PATH_A] = power_index;
	dm_info->txagc_remnant_ofdm[RF_PATH_A] = index - swing;
	rtw8188e_set_iqk_matrix(rtwdev, swing);
	rtw8188e_pwrtrack_set_cck_pwr(rtwdev, rtwdev->hal.current_channel);
	rtw_phy_set_tx_power_level(rtwdev, rtwdev->hal.current_channel);
}

static void rtw8188e_pwr_track(struct rtw_dev *rtwdev)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	struct rtw_swing_table swing_table;
	u8 thermal, delta;
	s8 power_index;
	bool do_lck, do_iqk;

	if (!dm_info->pwr_trk_triggered) {
		rtw_write_rf(rtwdev, RF_PATH_A, RF_T_METER, GENMASK(17, 16), 3);
		dm_info->pwr_trk_triggered = true;
		return;
	}
	dm_info->pwr_trk_triggered = false;
	thermal = rtw_read_rf(rtwdev, RF_PATH_A, RF_T_METER, GENMASK(15, 10));
	if (!thermal)
		return;
	rtw_phy_pwrtrack_avg(rtwdev, thermal, RF_PATH_A);
	if (!test_bit(RTW_FLAG_SCANNING, rtwdev->flags)) {
		do_lck = rtw_phy_pwrtrack_need_lck(rtwdev);
		do_iqk = rtw_phy_pwrtrack_need_iqk(rtwdev);
		if (do_iqk)
			rtw8188e_phy_calibration(rtwdev);
		else if (do_lck)
			rtw8188e_lck(rtwdev);
	}

	rtw_phy_config_swing_table(rtwdev, &swing_table);
	delta = rtw_phy_pwrtrack_get_delta(rtwdev, RF_PATH_A);
	power_index = rtw_phy_pwrtrack_get_pwridx(rtwdev, &swing_table,
						  RF_PATH_A, RF_PATH_A, delta);
	rtw8188e_pwrtrack_set(rtwdev, power_index);
}

static int rtw8188e_read_efuse(struct rtw_dev *rtwdev, u8 *log_map)
{
	static const u8 default_txpwr_idx[] = {
		0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d,
		0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x24
	};
	struct rtw8188e_efuse *map = (struct rtw8188e_efuse *)log_map;
	struct rtw_efuse *efuse = &rtwdev->efuse;
	u8 *pwr = (u8 *)&efuse->txpwr_idx_table[RF_PATH_A].pwr_idx_2g;
	u8 thermal = map->thermal_meter == 0xff ? 0x18 : map->thermal_meter;
	u8 rfe = map->rfe_option;
	int i;

	efuse->rfe_option = 0;
	efuse->rf_board_option = map->rf_board_option & GENMASK(4, 0);
	efuse->crystal_cap = map->xtal_k == 0xff ? 0x20 : map->xtal_k;
	efuse->channel_plan = map->channel_plan;
	memcpy(efuse->country_code, map->country_code, sizeof(efuse->country_code));
	efuse->regd = map->rf_board_option == 0xff ? 0 : map->rf_board_option & 0x7;
	efuse->thermal_meter[RF_PATH_A] = thermal;
	efuse->thermal_meter_k = thermal;
	efuse->afe = map->afe;
	ether_addr_copy(efuse->addr, map->mac_addr);

	efuse->txpwr_idx_table[RF_PATH_A].pwr_idx_2g = map->txpwr_idx;
	if (!memchr_inv(pwr, 0xff, sizeof(default_txpwr_idx)))
		memcpy(pwr, default_txpwr_idx, sizeof(default_txpwr_idx));

	/* Preserve valid channel groups when efuse is only partly programmed. */
	for (i = 0; i < offsetof(struct rtw_2g_txpwr_idx, ht_1s_diff); i++) {
		if (pwr[i] > rtwdev->chip->max_power_index)
			pwr[i] = default_txpwr_idx[i];
	}

	/* Encode 8188E's active-low PA/LNA selection for the common parser. */
	efuse->pa_type_2g = rfe & RTW8188E_EFUSE_INT_PA ? 0 : BIT(4);
	efuse->lna_type_2g = rfe & RTW8188E_EFUSE_INT_LNA ? 0 : BIT(3);
	switch (u8_get_bits(rfe, RTW8188E_EFUSE_GLNA_TYPE)) {
	case 0:
		efuse->glna_type = 1;
		break;
	case 2:
		efuse->glna_type = 2;
		break;
	default:
		efuse->glna_type = 0;
		break;
	}
	efuse->bt_setting = 0;
	efuse->hw_cap.nss = 1;
	efuse->hw_cap.ant_num = 1;
	efuse->hw_cap.bw = BIT(RTW_CHANNEL_WIDTH_20) | BIT(RTW_CHANNEL_WIDTH_40);

	return 0;
}

/* Realtek's T-series host RA, driven by the hardware's TX Report 2. */
static const u8 rtw8188e_retry_penalty[][RTW8188E_RA_RETRY_COUNT + 1] = {
	{ 5,  4,  3,  2, 0,  3},
	{ 6,  5,  4,  3, 0,  4},
	{ 6,  5,  4,  2, 0,  4},
	{ 8,  7,  6,  4, 0,  6},
	{10,  9,  8,  6, 0,  8},
	{10,  9,  8,  4, 0,  8},
	{10,  9,  8,  2, 0,  8},
	{10,  9,  8,  0, 0,  8},
	{18, 17, 16,  8, 0, 16},
	{26, 25, 24, 16, 0, 24},
	{34, 33, 32, 24, 0, 32},
	{34, 31, 28, 20, 0, 32},
	{34, 31, 27, 18, 0, 32},
	{34, 31, 26, 16, 0, 32},
	{34, 30, 22, 16, 0, 32},
	{34, 30, 24, 16, 0, 32},
	{49, 46, 40, 16, 0, 48},
	{49, 45, 32,  0, 0, 48},
	{49, 45, 22, 18, 0, 48},
	{49, 40, 24, 16, 0, 48},
	{49, 32, 18, 12, 0, 48},
	{49, 22, 18, 14, 0, 48},
};

static const u8 rtw8188e_retry_penalty_idx[2][RTW8188E_RA_RATE_COUNT] = {
	{ 4,  4,  4,  5,  4,  4,  5,  7,  7,  7,  8, 10,
	  4,  4,  4,  4,  6, 10, 11, 13},
	{10, 10, 11, 12, 10, 10, 11, 12, 13, 16, 19, 19,
	 11, 12, 13, 14, 15, 17, 19, 19},
};

static const u8 rtw8188e_retry_penalty_up_idx[RTW8188E_RA_RATE_COUNT] = {
	12, 13, 13, 15, 13, 14, 15, 15, 16, 18, 19, 20,
	15, 16, 16, 18, 18, 19, 20, 21,
};

static const u8 rtw8188e_rssi_threshold[RTW8188E_RA_RATE_COUNT] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 36, 38, 42,
	24, 26, 29, 31, 33, 39, 41, 42,
};

static const u16 rtw8188e_n_threshold_high[RTW8188E_RA_RATE_COUNT] = {
	4, 4, 8, 16, 24, 36, 48, 72, 96, 144, 192, 216,
	60, 80, 100, 160, 240, 400, 600, 800,
};

static const u16 rtw8188e_n_threshold_low[RTW8188E_RA_RATE_COUNT] = {
	2, 2, 4, 8, 12, 18, 24, 36, 48, 72, 96, 108,
	30, 40, 50, 80, 120, 200, 300, 400,
};

static const u8 rtw8188e_dropping_necessary[RTW8188E_RA_RATE_COUNT] = {
	1, 1, 1, 1, 1, 2, 3, 4, 5, 6, 7, 8,
	1, 2, 3, 4, 5, 6, 7, 8,
};

static const u8 rtw8188e_pending_for_rate_up_fail[] = {2, 10, 24, 40, 60};
static const u8 rtw8188e_pt_penalty[] = {34, 31, 30, 24, 0, 32};

/* The report timer is in units of 32 us: 200 ms through 1200 ms. */
static const u16 rtw8188e_tx_rpt_timing[] = {
	0x186a, 0x30d4, 0x493e, 0x61a8, 0x7a12, RTW8188E_RA_RPT_TIME_MAX,
};

enum rtw8188e_ra_timing {
	RTW8188E_RA_TIMING_DEFAULT,
	RTW8188E_RA_TIMING_INCREASE,
	RTW8188E_RA_TIMING_DECREASE,
};

static void rtw8188e_set_tx_rpt_timing(struct rtw8188e_ra_info *ra,
				       enum rtw8188e_ra_timing timing)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(rtw8188e_tx_rpt_timing) - 1; i++)
		if (ra->rpt_time == rtw8188e_tx_rpt_timing[i])
			break;

	if (timing == RTW8188E_RA_TIMING_DEFAULT)
		i = 0;
	else if (timing == RTW8188E_RA_TIMING_INCREASE)
		i = min_t(int, i + 1, ARRAY_SIZE(rtw8188e_tx_rpt_timing) - 1);
	else if (i)
		i--;

	ra->rpt_time = rtw8188e_tx_rpt_timing[i];
}

static void rtw8188e_reset_ra_counter(struct rtw8188e_ra_info *ra)
{
	u8 rate = ra->decision_rate;

	ra->nsc_up = (rtw8188e_n_threshold_high[rate] +
		      rtw8188e_n_threshold_low[rate]) >> 1;
	ra->nsc_down = ra->nsc_up;
}

static void rtw8188e_rate_down(struct rtw8188e_ra_info *ra)
{
	u8 rate = ra->pre_rate;
	int i;

	if (rate > ra->highest_rate) {
		rate = ra->highest_rate;
	} else if (ra->rate_sgi) {
		ra->rate_sgi = false;
	} else if (rate > ra->lowest_rate) {
		for (i = rate - 1; i >= ra->lowest_rate; i--) {
			if (ra->rate_mask & BIT(i)) {
				rate = i;
				break;
			}
		}
	} else {
		rate = ra->lowest_rate;
	}

	if (ra->waiting == 1) {
		ra->waiting++;
		ra->pending++;
	} else if (ra->waiting > 1) {
		ra->waiting = 0;
		ra->pending = 0;
	}

	ra->pending = min_t(u8, ra->pending,
			    ARRAY_SIZE(rtw8188e_pending_for_rate_up_fail) - 1);
	ra->drop_after_down = 1;
	ra->decision_rate = rate;
	rtw8188e_set_tx_rpt_timing(ra, RTW8188E_RA_TIMING_DECREASE);
}

static void rtw8188e_rate_up(struct rtw8188e_ra_info *ra)
{
	u8 rate = ra->pre_rate;
	int i;

	if (ra->waiting == 1) {
		ra->waiting = 0;
		ra->pending = 0;
	} else if (ra->waiting > 1) {
		ra->pre_rssi = ra->rssi;
		goto out;
	}

	rtw8188e_set_tx_rpt_timing(ra, RTW8188E_RA_TIMING_DEFAULT);
	if (rate < ra->highest_rate) {
		for (i = rate + 1; i <= ra->highest_rate; i++) {
			if (ra->rate_mask & BIT(i)) {
				rate = i;
				break;
			}
		}
	} else if (rate == ra->highest_rate) {
		ra->rate_sgi = ra->sgi_enable;
	} else {
		rate = ra->highest_rate;
	}

out:
	if (ra->waiting == 4 + rtw8188e_pending_for_rate_up_fail[ra->pending])
		ra->waiting = 0;
	else
		ra->waiting++;
	ra->decision_rate = rate;
}

static void rtw8188e_rate_decision(struct rtw8188e_ra_info *ra)
{
	u8 rate, rssi_idx, penalty_down, penalty_up;
	int i;

	if (ra->drop_after_down) {
		ra->drop_after_down--;
		rtw8188e_reset_ra_counter(ra);
		return;
	}

	if (abs((int)ra->rssi - ra->pre_rssi) > 3) {
		ra->pre_rssi = ra->rssi;
		ra->waiting = 0;
		ra->pending = 0;
	}

	rate = min(ra->pre_rate, ra->highest_rate);
	rssi_idx = ra->rssi <= rtw8188e_rssi_threshold[rate];
	penalty_down = rtw8188e_retry_penalty_idx[rssi_idx][rate];
	penalty_up = rtw8188e_retry_penalty_up_idx[rate];
	for (i = 0; i < RTW8188E_RA_RETRY_COUNT; i++) {
		ra->nsc_down += ra->retry[i] * rtw8188e_retry_penalty[penalty_down][i];
		ra->nsc_up += ra->retry[i] * rtw8188e_retry_penalty[penalty_up][i];
	}
	ra->nsc_down -= min(ra->nsc_down,
			    ra->total * rtw8188e_retry_penalty[penalty_down][5]);
	ra->nsc_up -= min(ra->nsc_up,
			  ra->total * rtw8188e_retry_penalty[penalty_up][5]);

	if (ra->nsc_down < rtw8188e_n_threshold_low[rate] ||
	    ra->drop > rtw8188e_dropping_necessary[rate])
		rtw8188e_rate_down(ra);
	else if (ra->nsc_up > rtw8188e_n_threshold_high[rate])
		rtw8188e_rate_up(ra);

	if (ra->decision_rate == ra->pre_rate)
		ra->rpt_stable++;
	else
		ra->rpt_stable = 0;
	if (ra->rpt_stable >= 4) {
		rtw8188e_set_tx_rpt_timing(ra, RTW8188E_RA_TIMING_INCREASE);
		ra->rpt_stable = 0;
	}

	ra->pre_rate = ra->decision_rate;
	rtw8188e_reset_ra_counter(ra);
}

static void rtw8188e_power_training_try(struct rtw8188e_ra_info *ra)
{
	bool try_training;

	/* RTL8188E has one spatial stream. */
	if (ra->highest_rate >= DESC_RATEMCS0)
		try_training = ra->decision_rate >= DESC_RATEMCS5;
	else
		try_training = ra->decision_rate >= DESC_RATE11M;

	if (ra->rssi < 48) {
		ra->pt_stage = 0;
	} else if (try_training) {
		if (ra->pt_stop_count >= 10 ||
		    abs((int)ra->pt_pre_rssi - ra->rssi) > 5 ||
		    ra->decision_rate != ra->pt_pre_rate) {
			if (!ra->pt_stage)
				ra->pt_stage = 1;
			else if (ra->pt_stage == 1)
				ra->pt_stage = 3;
			else
				ra->pt_stage = 5;
			ra->pt_pre_rssi = ra->rssi;
			ra->pt_stop_count = 0;
		} else {
			ra->stage = 0;
			ra->pt_stop_count++;
		}
	} else {
		ra->pt_stage = 0;
		ra->stage = 0;
	}
	ra->pt_pre_rate = ra->decision_rate;
}

static void rtw8188e_power_training_decision(struct rtw8188e_ra_info *ra)
{
	u32 score = 0;
	u32 total = ra->total * rtw8188e_pt_penalty[5];
	u8 stage = (ra->pt_stage + 1) >> 1;
	int i;

	for (i = 0; i < RTW8188E_RA_RETRY_COUNT; i++) {
		score += ra->retry[i] * rtw8188e_pt_penalty[i];
		if (score > total)
			break;
	}
	i >>= 1;
	stage = stage > i ? stage - i : 0;
	ra->pt_smooth_factor = min_t(u16, (ra->pt_smooth_factor >> 1) +
				    (ra->pt_smooth_factor >> 2) + stage * 16 + 2,
				    192);
	stage = (ra->pt_smooth_factor >> 6) * 2;
	if (stage)
		stage--;
	ra->pt_stage = ra->drop > 3 ? 0 : stage;
}

static bool rtw8188e_sgi_supported(struct rtw_sta_info *si, u8 bw)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 19, 0)
	const struct ieee80211_sta_ht_cap *ht_cap = &si->sta->deflink.ht_cap;
#else
	const struct ieee80211_sta_ht_cap *ht_cap = &si->sta->ht_cap;
#endif
	u16 cap = bw == RTW_CHANNEL_WIDTH_40 ? IEEE80211_HT_CAP_SGI_40 :
					     IEEE80211_HT_CAP_SGI_20;

	return ht_cap->ht_supported && (ht_cap->cap & cap);
}

static void rtw8188e_sta_add(struct rtw_dev *rtwdev, struct rtw_sta_info *si)
{
	struct rtw8188e_sta *sta = (void *)si->priv;
	struct rtw8188e_ra_info *ra = &sta->ra;

	spin_lock_init(&sta->ra_lock);
	sta->rsvd_pending = false;
	memset(ra, 0, sizeof(*ra));
	ra->rpt_time = RTW8188E_RA_RPT_TIME_MAX;
	ra->pt_stage = 5;
	ra->pt_smooth_factor = 192;
	ra->active = true;
}

static void rtw8188e_sta_remove(struct rtw_dev *rtwdev, struct rtw_sta_info *si)
{
	struct rtw8188e_sta *sta = (void *)si->priv;
	unsigned long flags;

	spin_lock_irqsave(&sta->ra_lock, flags);
	sta->ra.active = false;
	spin_unlock_irqrestore(&sta->ra_lock, flags);
}

static u32 rtw8188e_configured_rate_mask(struct rtw_sta_info *si)
{
	const struct cfg80211_bitrate_mask *cfg = si->mask;
	const enum nl80211_band band = NL80211_BAND_2GHZ;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 19, 0)
	const struct ieee80211_sta_ht_cap *ht_cap = &si->sta->deflink.ht_cap;
	u32 legacy = si->sta->deflink.supp_rates[band];
#else
	const struct ieee80211_sta_ht_cap *ht_cap = &si->sta->ht_cap;
	u32 legacy = si->sta->supp_rates[band];
#endif
	u32 mask;

	mask = legacy & cfg->control[band].legacy;
	if (ht_cap->ht_supported)
		mask |= (u32)(ht_cap->mcs.rx_mask[0] &
			      cfg->control[band].ht_mcs[0]) << DESC_RATEMCS0;

	return mask & GENMASK(DESC_RATEMCS7, DESC_RATE1M);
}

static void rtw8188e_host_ra_update(struct rtw_dev *rtwdev,
				    struct rtw_sta_info *si)
{
	struct rtw8188e_sta *sta = (void *)si->priv;
	struct rtw8188e_ra_info *ra = &sta->ra;
	u32 mask = si->ra_mask & GENMASK(DESC_RATEMCS7, DESC_RATE1M);
	u8 bw = min_t(u8, si->bw_mode, rtwdev->hal.current_band_width);
	unsigned long flags;
	bool changed, sgi;

	/* Keep the configured rates if the common filters removed them all. */
	if (!mask && si->use_cfg_mask)
		mask = rtw8188e_configured_rate_mask(si);
	if (!mask)
		return;

	sgi = (mask & GENMASK(DESC_RATEMCS7, DESC_RATEMCS0)) &&
	      rtw8188e_sgi_supported(si, bw);

	spin_lock_irqsave(&sta->ra_lock, flags);
	changed = mask != ra->rate_mask || bw != ra->bw_mode ||
		  sgi != ra->sgi_enable;
	if (!ra->rate_mask)
		ra->decision_rate = fls(mask) - 1;
	ra->rate_mask = mask;
	ra->highest_rate = fls(mask) - 1;
	ra->lowest_rate = __ffs(mask);
	ra->bw_mode = bw;
	ra->sgi_enable = sgi;
	ra->rssi = ewma_rssi_read(&si->avg_rssi);
	if (!(mask & BIT(ra->decision_rate))) {
		ra->decision_rate = ra->highest_rate;
		ra->rate_sgi = false;
	}
	if (!sgi || ra->decision_rate < DESC_RATEMCS0)
		ra->rate_sgi = false;
	if (changed) {
		ra->pre_rate = ra->decision_rate;
		ra->waiting = 0;
		ra->pending = 0;
		ra->drop_after_down = 0;
		ra->rpt_stable = 0;
		ra->stage = 0;
		rtw8188e_reset_ra_counter(ra);
	}
	rtw_update_sta_ra_report(si, ra->decision_rate, ra->rate_sgi, bw);
	spin_unlock_irqrestore(&sta->ra_lock, flags);
}

static void rtw8188e_tx_data_rate(struct rtw_dev *rtwdev,
				  struct rtw_sta_info *si,
				  struct rtw_tx_pkt_info *pkt_info)
{
	struct rtw8188e_sta *sta = (void *)si->priv;
	struct rtw8188e_ra_info *ra = &sta->ra;
	unsigned long flags;

	spin_lock_irqsave(&sta->ra_lock, flags);
	if (ra->active && ra->rate_mask) {
		pkt_info->rate = ra->decision_rate;
		pkt_info->bw = min_t(u8, pkt_info->bw,
				     rtwdev->hal.current_band_width);
		pkt_info->short_gi = ra->decision_rate >= DESC_RATEMCS0 &&
				     ra->rate_sgi &&
				     rtw8188e_sgi_supported(si, pkt_info->bw);
		pkt_info->tx_power_offset = ra->pt_stage;
		pkt_info->use_rate = true;
	}
	spin_unlock_irqrestore(&sta->ra_lock, flags);
}

static void rtw8188e_tx_prepare(struct rtw_dev *rtwdev,
				struct rtw_tx_pkt_info *pkt_info,
				struct ieee80211_sta *sta, struct sk_buff *skb)
{
	struct ieee80211_tx_info *info = IEEE80211_SKB_CB(skb);
	struct ieee80211_hdr *hdr = (struct ieee80211_hdr *)skb->data;
	struct ieee80211_vif *vif = info->control.vif;
	struct ieee80211_rate *rts_rate;
	struct rtw_vif *rtwvif;
	u8 fix_rate;

	if (ieee80211_is_data(hdr->frame_control) &&
	    !ieee80211_is_any_nullfunc(hdr->frame_control) &&
	    skb->protocol != cpu_to_be16(ETH_P_PAE)) {
		if (!sta || is_multicast_ether_addr(hdr->addr1))
			rtw_tx_pkt_info_update_rate(rtwdev, pkt_info, skb, false);
		else
			rtw8188e_tx_data_rate(rtwdev,
					      (struct rtw_sta_info *)sta->drv_priv,
					      pkt_info);

		/* Preserve the native debug override after host rate selection. */
		fix_rate = rtwdev->dm_info.fix_rate;
		if (fix_rate < DESC_RATE_MAX) {
			pkt_info->rate = fix_rate;
			pkt_info->dis_rate_fallback = true;
			pkt_info->use_rate = true;
		}
	}

	if (pkt_info->rts) {
		rts_rate = ieee80211_get_rts_cts_rate(rtwdev->hw, info);
		pkt_info->rts_rate = rts_rate ? rts_rate->hw_value : DESC_RATE1M;
	}
	if (vif) {
		rtwvif = (struct rtw_vif *)vif->drv_priv;
		pkt_info->port_id = rtwvif->port;
	}
}

struct rtw8188e_ra_report_data {
	struct rtw_dev *rtwdev;
	const struct rtw8188e_tx_report2 *reports;
	u32 macid_valid;
	u16 min_rpt_time;
	u8 items;
};

static void rtw8188e_ra_report_iter(void *data, struct ieee80211_sta *sta)
{
	struct rtw8188e_ra_report_data *rpt = data;
	struct rtw_sta_info *si = (void *)sta->drv_priv;
	struct rtw8188e_sta *rtwsta = (void *)si->priv;
	struct rtw8188e_ra_info *ra = &rtwsta->ra;
	const struct rtw8188e_tx_report2 *report;
	unsigned long flags;
	int i;

	spin_lock_irqsave(&rtwsta->ra_lock, flags);
	if (!ra->active || !ra->rate_mask)
		goto unlock;
	if (si->mac_id >= rpt->items ||
	    !(rpt->macid_valid & BIT(si->mac_id)))
		goto timing;

	report = &rpt->reports[si->mac_id];
	ra->retry[0] = le16_to_cpu(report->retry0);
	ra->total = ra->retry[0];
	for (i = 1; i < RTW8188E_RA_RETRY_COUNT; i++) {
		ra->retry[i] = report->retry[i - 1];
		ra->total += ra->retry[i];
	}
	ra->drop = report->drop;
	ra->total += ra->drop;
	ra->rssi = ewma_rssi_read(&si->avg_rssi);
	if (ra->total) {
		if (ra->stage < 5)
			rtw8188e_rate_decision(ra);
		else if (ra->stage == 5)
			rtw8188e_power_training_try(ra);
		else
			rtw8188e_power_training_decision(ra);
		if (ra->stage <= 5)
			ra->stage++;
		else
			ra->stage = 0;

		rtw_update_sta_ra_report(si, ra->decision_rate,
					 ra->rate_sgi, ra->bw_mode);
		WRITE_ONCE(rpt->rtwdev->dm_info.tx_rate, ra->decision_rate);
	}

timing:
	rpt->min_rpt_time = min(rpt->min_rpt_time, ra->rpt_time);
unlock:
	spin_unlock_irqrestore(&rtwsta->ra_lock, flags);
}

static void rtw8188e_ra_report(struct rtw_dev *rtwdev,
			       const struct rtw_rx_desc *desc,
			       const u8 *buf, u32 len)
{
	struct rtw8188e_ra_report_data data = {
		.rtwdev = rtwdev,
		.reports = (const void *)buf,
		.macid_valid = le32_to_cpu(desc->w4),
		.min_rpt_time = RTW8188E_RA_RPT_TIME_MAX,
		.items = min_t(u32, len / sizeof(*data.reports), RTW_MAX_MAC_ID_NUM),
	};

	if (!test_bit(RTW_FLAG_RUNNING, rtwdev->flags))
		return;

	rtw_iterate_stas_atomic(rtwdev, rtw8188e_ra_report_iter, &data);
	/* SDIO register I/O must stay outside the atomic station iterator. */
	if (rtw_read16(rtwdev, REG_TX_RPT_TIME) != data.min_rpt_time)
		rtw_write16(rtwdev, REG_TX_RPT_TIME, data.min_rpt_time);
}

static u8 rtw8188e_rate_id(u8 rate_id)
{
	switch (rate_id) {
	case RTW_RATEID_BGN_40M_1SS:
	case RTW_RATEID_BGN_20M_1SS:
		return RTW8188E_RATEID_NGB;
	case RTW_RATEID_GN_N1SS:
		return RTW8188E_RATEID_NG;
	case RTW_RATEID_BG:
		return RTW8188E_RATEID_GB;
	case RTW_RATEID_G:
		return RTW8188E_RATEID_G;
	case RTW_RATEID_B_20M:
		return RTW8188E_RATEID_B;
	default:
		return RTW8188E_RATEID_NGB;
	}
}

static void rtw8188e_fill_tx_desc(struct rtw_dev *rtwdev,
				  struct rtw_tx_pkt_info *pkt_info,
				  struct rtw_tx_desc *txdesc)
{
	struct rtw_rsvd_page *rsvd_pkt;
	struct rtw_vif *rtwvif;
	bool bw_40 = pkt_info->bw == RTW_CHANNEL_WIDTH_40;
	u8 primary_sc = RTW8188E_SC_DONT_CARE;
	bool agg_break;
	u8 data_sc;

	if (pkt_info->qsel == TX_DESC_QSEL_BEACON) {
		rsvd_pkt = list_first_entry_or_null(&rtwdev->rsvd_page_list,
						    struct rtw_rsvd_page, build_list);
		if (rsvd_pkt && rsvd_pkt->rtwvif) {
			rtwvif = rsvd_pkt->rtwvif;
			pkt_info->mac_id = rtwvif->mac_id;
			pkt_info->port_id = rtwvif->port;
		}
	}

	if (rtwdev->hal.current_band_width == RTW_CHANNEL_WIDTH_40) {
		if (rtwdev->hal.current_primary_channel_index == RTW_SC_20_UPPER)
			primary_sc = RTW8188E_SC_UPPER;
		else if (rtwdev->hal.current_primary_channel_index == RTW_SC_20_LOWER)
			primary_sc = RTW8188E_SC_LOWER;
	}

	data_sc = bw_40 ? RTW8188E_SC_DONT_CARE : primary_sc;
	agg_break = !pkt_info->ampdu_en && pkt_info->qsel < TX_DESC_QSEL_BEACON;

	txdesc->w0 = le32_encode_bits(pkt_info->tx_pkt_size, RTW_TX_DESC_W0_TXPKTSIZE) |
		   le32_encode_bits(pkt_info->offset, RTW_TX_DESC_W0_OFFSET) |
		   le32_encode_bits(pkt_info->bmc, RTW_TX_DESC_W0_BMC) |
		   cpu_to_le32(RTW_TX_DESC_W0_LS | RTW_TX_DESC_W0_FS |
			       RTW_TX_DESC_W0_OWN);
	txdesc->w1 = le32_encode_bits(pkt_info->mac_id, RTW8188E_TX_DESC_W1_MACID) |
		   le32_encode_bits(pkt_info->qsel, RTW_TX_DESC_W1_QSEL) |
		   le32_encode_bits(rtw8188e_rate_id(pkt_info->rate_id),
				    RTW8188E_TX_DESC_W1_RATE_ID) |
		   le32_encode_bits(pkt_info->nav_use_hdr, RTW8188E_TX_DESC_W1_NAVUSEHDR) |
		   le32_encode_bits(pkt_info->sec_type, RTW_TX_DESC_W1_SEC_TYPE) |
		   le32_encode_bits(pkt_info->pkt_offset, RTW8188E_TX_DESC_W1_PKT_OFFSET);
	txdesc->w2 = le32_encode_bits(pkt_info->ampdu_en, RTW_TX_DESC_W2_AGG_EN) |
		   le32_encode_bits(agg_break, RTW8188E_TX_DESC_W2_AGG_BREAK) |
		   le32_encode_bits(pkt_info->report, RTW_TX_DESC_W2_SPE_RPT) |
		   le32_encode_bits(pkt_info->ampdu_density, RTW_TX_DESC_W2_AMPDU_DEN) |
		   le32_encode_bits(pkt_info->bt_null, RTW_TX_DESC_W2_BT_NULL);
	txdesc->w3 = le32_encode_bits(pkt_info->seq, RTW8188E_TX_DESC_W3_SEQ) |
		   le32_encode_bits(pkt_info->en_hwseq, RTW8188E_TX_DESC_W3_EN_HWSEQ);
	txdesc->w4 = le32_encode_bits(pkt_info->hw_ssn_sel, RTW8188E_TX_DESC_W4_HW_SSN_SEL) |
		   le32_encode_bits(pkt_info->use_rate, RTW8188E_TX_DESC_W4_USE_RATE) |
		   le32_encode_bits(pkt_info->port_id, RTW8188E_TX_DESC_W4_PORT_ID) |
		   le32_encode_bits(pkt_info->tx_power_offset,
				    RTW8188E_TX_DESC_W4_PWR_STATUS) |
		   le32_encode_bits(pkt_info->dis_rate_fallback,
				    RTW8188E_TX_DESC_W4_DISDATAFB) |
		   le32_encode_bits(data_sc, RTW8188E_TX_DESC_W4_DATA_SC) |
		   le32_encode_bits(bw_40, RTW8188E_TX_DESC_W4_DATA_BW);
	if (pkt_info->rts) {
		txdesc->w4 |= le32_encode_bits(pkt_info->rts_rate,
					      RTW8188E_TX_DESC_W4_RTS_RATE) |
			    le32_encode_bits(primary_sc, RTW8188E_TX_DESC_W4_RTS_SC) |
			    le32_encode_bits(bw_40, RTW8188E_TX_DESC_W4_RTS_BW) |
			    cpu_to_le32(RTW8188E_TX_DESC_W4_RTS_EN |
					RTW8188E_TX_DESC_W4_HW_RTS_EN);
	}
	txdesc->w5 = le32_encode_bits(pkt_info->rate, RTW8188E_TX_DESC_W5_DATARATE) |
		   le32_encode_bits(pkt_info->short_gi, RTW8188E_TX_DESC_W5_SHORT_GI) |
		   cpu_to_le32(RTW8188E_TX_DESC_W5_DATA_FB_LIMIT |
			       RTW8188E_TX_DESC_W5_RTS_FB_LIMIT);
	txdesc->w6 = le32_encode_bits(pkt_info->ampdu_factor,
				      RTW8188E_TX_DESC_W6_MAX_AGG_NUM);
	txdesc->w7 = le32_encode_bits(pkt_info->sn, RTW8188E_TX_DESC_W7_SW_DEFINE);
}

static void rtw8188e_fill_txdesc_checksum(struct rtw_dev *rtwdev,
					  struct rtw_tx_pkt_info *pkt_info,
					  struct rtw_tx_desc *txdesc)
{
	/* XOR the 16 words of the 32-byte TX descriptor. */
	fill_txdesc_checksum_common(txdesc, 16);
}

static void rtw8188e_fill_rsvd_page_desc(struct rtw_dev *rtwdev,
					 struct rtw_rsvd_page *page,
					 struct sk_buff *skb)
{
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_vif *rtwvif = page->rtwvif;
	struct rtw_tx_pkt_info pkt_info = {};
	struct rtw_tx_desc *txdesc;

	rtw_tx_rsvd_page_pkt_info_update(rtwdev, &pkt_info, skb, page->type);
	pkt_info.mac_id = rtwvif->mac_id;
	pkt_info.port_id = rtwvif->port;
	/* The ordinary QoS Null is not a Bluetooth QoS Null frame. */
	pkt_info.bt_null = false;
	if (pkt_info.en_hwseq)
		pkt_info.hw_ssn_sel = RTW8188E_RSVD_HW_SSN_SEL;
	txdesc = skb_push(skb, chip->tx_pkt_desc_sz);
	memset(txdesc, 0, chip->tx_pkt_desc_sz);
	rtw8188e_fill_tx_desc(rtwdev, &pkt_info, txdesc);
	rtw8188e_fill_txdesc_checksum(rtwdev, &pkt_info, txdesc);
}

static void rtw8188e_query_rx_desc(struct rtw_dev *rtwdev, const void *raw,
				   struct rtw_rx_pkt_stat *pkt)
{
	const struct rtw_rx_desc *desc = raw;

	pkt->is_c2h = false;
	pkt->cam_id = le32_get_bits(desc->w1, RTW8188E_RX_DESC_W1_MACID);
	pkt->ppdu_cnt = 0;
	pkt->rate = le32_get_bits(desc->w3, RTW8188E_RX_DESC_W3_RX_RATE);
	pkt->bw = le32_get_bits(desc->w3, RTW8188E_RX_DESC_W3_BW) ?
		RTW_CHANNEL_WIDTH_40 : RTW_CHANNEL_WIDTH_20;
}

static int rtw8188e_rx_report_handle(struct rtw_dev *rtwdev, const u8 *buf,
				     u32 len)
{
	const struct rtw_rx_desc *desc = (const void *)buf;
	const struct rtw8188e_tx_report *report;
	u32 type, packet_len, offset, consumed;
	u16 sn;
	bool acked;

	if (len < sizeof(*desc))
		return -EINVAL;
	type = le32_get_bits(desc->w3, RTW8188E_RX_DESC_W3_RPT_SEL);
	packet_len = le32_get_bits(desc->w0, RTW_RX_DESC_W0_PKT_LEN);
	offset = sizeof(*desc);
	switch (type) {
	case RTW8188E_RX_NORMAL:
		offset += le32_get_bits(desc->w0, RTW_RX_DESC_W0_DRV_INFO_SIZE) * 8;
		offset += le32_get_bits(desc->w0, RTW_RX_DESC_W0_SHIFT);
		break;
	case RTW8188E_RX_TX_REPORT1:
		packet_len = sizeof(*report);
		break;
	case RTW8188E_RX_TX_REPORT2:
		packet_len &= 0x3ff;
		if (packet_len % sizeof(struct rtw8188e_tx_report2))
			return -EINVAL;
		break;
	}
	if (offset + packet_len > len)
		return -EINVAL;
	if (type == RTW8188E_RX_NORMAL)
		return 0;

	if (type == RTW8188E_RX_TX_REPORT2)
		rtw8188e_ra_report(rtwdev, desc, buf + offset, packet_len);

	consumed = min_t(u32, ALIGN(offset + packet_len,
				    RTW8188E_RX_ALIGN), len);
	if (type != RTW8188E_RX_TX_REPORT1)
		return consumed;

	report = (const void *)(buf + offset);
	if (!le32_get_bits(report->w0, RTW8188E_TX_REPORT_W0_INT_CCX))
		return consumed;

	sn = le32_get_bits(report->w1, RTW8188E_TX_REPORT_W1_SW_LOW) |
	     (le32_get_bits(report->w1, RTW8188E_TX_REPORT_W1_SW_HIGH) << 8);
	acked = le32_get_bits(report->w0, RTW8188E_TX_REPORT_W0_PKT_OK);
	if (sn <= U8_MAX)
		rtw_tx_report_complete(rtwdev, sn, acked);
	return consumed;
}

static const struct rtw_chip_ops rtw8188e_ops = {
	.chip_info_setup	= rtw8188e_chip_info_setup,
	.power_on		= rtw_power_on,
	.power_off		= rtw_power_off,
	.mac_init		= rtw8188e_mac_init,
	.mac_postinit		= rtw8188e_mac_postinit,
	.llt_init		= rtw8188e_llt_init,
	.write_rsvd_page		= rtw8188e_write_rsvd_page,
	.fill_rsvd_page_desc	= rtw8188e_fill_rsvd_page_desc,
	.config_bcn		= rtw8188e_config_bcn,
	.get_rsvd_beacon		= rtw8188e_get_rsvd_beacon,
	.update_beacon		= rtw8188e_update_beacon,
	.set_default_port	= rtw8188e_set_default_port,
	.set_pwr_mode		= rtw8188e_set_pwr_mode,
	.set_macid_link		= rtw8188e_set_macid_link,
	.sta_add		= rtw8188e_sta_add,
	.sta_remove		= rtw8188e_sta_remove,
	.host_ra_update		= rtw8188e_host_ra_update,
	.tx_prepare		= rtw8188e_tx_prepare,
	.read_efuse		= rtw8188e_read_efuse,
	.phy_set_param		= rtw8188e_phy_set_param,
	.set_channel		= rtw8188e_set_channel,
	.query_phy_status	= rtw8188e_query_phy_status,
	.query_rx_desc		= rtw8188e_query_rx_desc,
	.read_rf		= rtw_phy_read_rf_sipi,
	.write_rf		= rtw_phy_write_rf_reg_sipi,
	.set_tx_power_index	= rtw8188e_set_tx_power_index,
	.set_ampdu_factor	= rtw8188e_set_ampdu_factor,
	.cfg_ldo25		= rtw8188e_cfg_ldo25,
	.efuse_grant		= rtw8188e_efuse_grant,
	.false_alarm_statistics	= rtw8188e_false_alarm_statistics,
	.dig_get_boundary	= rtw8188e_dig_get_boundary,
	.phy_calibration	= rtw8188e_phy_calibration,
	.cck_pd_set		= rtw_phy_cck_pd_set,
	.pwr_track		= rtw8188e_pwr_track,
	.cfo_init		= rtw8188e_cfo_init,
	.cfo_track		= rtw8188e_cfo_track,
	.adaptivity_init		= rtw8188e_adaptivity_init,
	.adaptivity		= rtw8188e_adaptivity,
	.fill_tx_desc		= rtw8188e_fill_tx_desc,
	.fill_txdesc_checksum	= rtw8188e_fill_txdesc_checksum,
	.sdio_irq_handler	= rtw8188e_sdio_irq_handler,
	.config_rx_aggregation	= rtw8188e_config_rx_aggregation,
	.rx_report_handle	= rtw8188e_rx_report_handle,
};

const struct rtw_chip_info rtw8188e_hw_spec = {
	.ops = &rtw8188e_ops,
	.sta_priv_size = sizeof(struct rtw8188e_sta),
	.id = RTW_CHIP_TYPE_8188E,
	.fw_name = "rtw88/rtw8188e_fw.bin",
	.wlan_cpu = RTW_WCPU_8051,
	.tx_pkt_desc_sz = 32,
	.rx_pkt_desc_sz = 24,
	.phy_efuse_size = 256,
	.log_efuse_size = 512,
	.ptct_efuse_size = 18,
	.txff_size = 22528,
	.rxff_size = 10240,
	.rsvd_drv_pg_num = 8,
	.txgi_factor = 1,
	.is_pwr_by_rate_dec = true,
	.max_power_index = 0x3f,
	.band = RTW_BAND_2G,
	.page_size = 128,
	.dig_min = 0x20,
	.ht_supported = true,
	.lps_deep_mode_supported = 0,
	.pwr_on_seq = card_enable_flow_8188e,
	.pwr_off_seq = card_disable_flow_8188e,
	.page_table = page_table_8188e,
	.rqpn_table = rqpn_table_8188e,
	.prioq_addrs = &rtw8188e_prioq_addrs,
	.dig = rtw8188e_dig,
	.edcca_th = rtw8188e_edcca_th,
	.l2h_th_ini_ad = -11,
	.l2h_th_ini_cs = 10,
	.rf_sipi_addr = {0x840},
	.rf_sipi_read_addr = rtw8188e_rf_sipi_addr,
	.fix_rf_phy_num = 1,
	.mac_tbl = &rtw8188e_mac_tbl,
	.agc_tbl = &rtw8188e_agc_tbl,
	.bb_tbl = &rtw8188e_bb_tbl,
	.rf_tbl = {&rtw8188e_rf_a_tbl},
	.rfe_defs = rtw8188e_rfe_defs,
	.rfe_defs_size = ARRAY_SIZE(rtw8188e_rfe_defs),
	.iqk_threshold = 8,
	.lck_threshold = 8,
	.ampdu_density = IEEE80211_HT_MPDU_DENSITY_16,
	.max_scan_ie_len = IEEE80211_MAX_DATA_LEN,
};
EXPORT_SYMBOL(rtw8188e_hw_spec);

MODULE_FIRMWARE("rtw88/rtw8188e_fw.bin");

MODULE_AUTHOR("Igor Listopad <dev@listopad.email>");
MODULE_DESCRIPTION("Realtek 802.11n wireless 8188e driver");
MODULE_LICENSE("GPL");
