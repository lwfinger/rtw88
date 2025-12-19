/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2009-2012  Realtek Corporation.*/

#ifndef __RTW_PCI_OLD_H__
#define __RTW_PCI_OLD_H__

#include "rx.h"
#include "tx.h"

#define	REG_BCNQ_DESA				0x0308
#define	REG_HQ_DESA				0x0310
#define	REG_MGQ_DESA				0x0318
#define	REG_VOQ_DESA				0x0320
#define	REG_VIQ_DESA				0x0328
#define	REG_BEQ_DESA				0x0330
#define	REG_BKQ_DESA				0x0338
#define	REG_RX_DESA				0x0340

#define	REG_DBI_CTRL				0x0350

struct rtw_rx_desc_pci_old {
	struct rtw_rx_desc w0_5;
	__le32 w6;
	__le32 w7;
} __packed;

struct rtw_tx_desc_pci_old {
	struct rtw_tx_desc w0_9;
	__le32 w10;
	__le32 w11;
	__le32 w12;
	__le32 w13;
	__le32 w14;
	__le32 w15;
} __packed;

extern const struct rtw_pci_gen rtw_pci_gen_old;

#endif
