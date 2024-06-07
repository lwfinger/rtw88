/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/* Copyright(c) 2018-2019  Realtek Corporation
 */

#ifndef __RTW8821A_H__
#define __RTW8821A_H__

#include <asm/byteorder.h>

struct rtw8821au_efuse {
	u8 res4[48];			/* 0xd0 */
	u8 vid[2];			/* 0x100 */
	u8 pid[2];
	u8 res8[3];
	u8 mac_addr[ETH_ALEN];		/* 0x107 */
	u8 res9[243];
};

struct rtw8812au_efuse {
	u8 vid[2];			/* 0xd0 */
	u8 pid[2];			/* 0xd2 */
	u8 res0[3];
	u8 mac_addr[ETH_ALEN];		/* 0xd7 */
	u8 res1[291];
};

struct rtw8821a_efuse {
	__le16 rtl_id;
	u8 res0[6];			/* 0x02 */
	u8 usb_mode;			/* 0x08 */
	u8 res1[7];			/* 0x09 */

	/* power index for four RF paths */
	struct rtw_txpwr_idx txpwr_idx_table[4];

	u8 channel_plan;		/* 0xb8 */
	u8 xtal_k;
	u8 thermal_meter;
	u8 iqk_lck;
	u8 pa_type;			/* 0xbc */
	u8 lna_type_2g;			/* 0xbd */
	u8 res2;
	u8 lna_type_5g;			/* 0xbf */
	u8 res3;
	u8 rf_board_option;		/* 0xc1 */
	u8 rf_feature_option;
	u8 rf_bt_setting;
	u8 eeprom_version;
	u8 eeprom_customer_id;		/* 0xc5 */
	u8 tx_bb_swing_setting_2g;
	u8 tx_bb_swing_setting_5g;
	u8 tx_pwr_calibrate_rate;
	u8 rf_antenna_option;		/* 0xc9 */
	u8 rfe_option;
	u8 country_code[2];
	u8 res4[3];
	union {
		struct rtw8821au_efuse rtw8821au;
		struct rtw8812au_efuse rtw8812au;
	};
} __packed;

static_assert(sizeof(struct rtw8821a_efuse) == 512);

extern const struct rtw_chip_info rtw8821a_hw_spec;
extern const struct rtw_chip_info rtw8812a_hw_spec;

#define BIT_FEN_USBA				BIT(2)
#define BIT_FEN_PCIEA				BIT(6)
#define WLAN_SLOT_TIME				0x09
#define WLAN_PIFS_TIME				0x19
#define WLAN_SIFS_CCK_CONT_TX			0xA
#define WLAN_SIFS_OFDM_CONT_TX			0xE
#define WLAN_SIFS_CCK_TRX			0x10
#define WLAN_SIFS_OFDM_TRX			0x10
#define WLAN_VO_TXOP_LIMIT			0x186
#define WLAN_VI_TXOP_LIMIT			0x3BC
#define WLAN_RDG_NAV				0x05
#define WLAN_TXOP_NAV				0x1B
#define WLAN_CCK_RX_TSF				0x30
#define WLAN_OFDM_RX_TSF			0x30
#define WLAN_TBTT_PROHIBIT			0x04
#define WLAN_TBTT_HOLD_TIME			0x064
#define WLAN_DRV_EARLY_INT			0x04
#define WLAN_BCN_DMA_TIME			0x02

#define WLAN_RX_FILTER0				0x0FFFFFFF
#define WLAN_RX_FILTER2				0xFFFF
#define WLAN_RCR_CFG				0xE400220E
#define WLAN_RXPKT_MAX_SZ			12288
#define WLAN_RXPKT_MAX_SZ_512			(WLAN_RXPKT_MAX_SZ >> 9)

#define WLAN_AMPDU_MAX_TIME			0x70
#define WLAN_RTS_LEN_TH				0xFF
#define WLAN_RTS_TX_TIME_TH			0x08
#define WLAN_MAX_AGG_PKT_LIMIT			0x20
#define WLAN_RTS_MAX_AGG_PKT_LIMIT		0x20
#define FAST_EDCA_VO_TH				0x06
#define FAST_EDCA_VI_TH				0x06
#define FAST_EDCA_BE_TH				0x06
#define FAST_EDCA_BK_TH				0x06
#define WLAN_BAR_RETRY_LIMIT			0x01
#define WLAN_RA_TRY_RATE_AGG_LIMIT		0x08

#define WLAN_TX_FUNC_CFG1			0x30
#define WLAN_TX_FUNC_CFG2			0x30
#define WLAN_MAC_OPT_NORM_FUNC1			0x98
#define WLAN_MAC_OPT_LB_FUNC1			0x80
#define WLAN_MAC_OPT_FUNC2			0xb0810041

#define WLAN_SIFS_CFG	(WLAN_SIFS_CCK_CONT_TX | \
			(WLAN_SIFS_OFDM_CONT_TX << BIT_SHIFT_SIFS_OFDM_CTX) | \
			(WLAN_SIFS_CCK_TRX << BIT_SHIFT_SIFS_CCK_TRX) | \
			(WLAN_SIFS_OFDM_TRX << BIT_SHIFT_SIFS_OFDM_TRX))

#define WLAN_TBTT_TIME	(WLAN_TBTT_PROHIBIT |\
			(WLAN_TBTT_HOLD_TIME << BIT_SHIFT_TBTT_HOLD_TIME_AP))

#define WLAN_NAV_CFG		(WLAN_RDG_NAV | (WLAN_TXOP_NAV << 16))
#define WLAN_RX_TSF_CFG		(WLAN_CCK_RX_TSF | (WLAN_OFDM_RX_TSF) << 8)
#define WLAN_PRE_TXCNT_TIME_TH			0x1E4


struct rtw8821a_phy_status_rpt {
	/*	DWORD 0*/
	u8 gain_trsw[2]; /*path-A and path-B {TRSW, gain[6:0] }*/
	u8 chl_num_lsb; /*@channel number[7:0]*/
#ifdef __LITTLE_ENDIAN
	u8 chl_num_msb : 2; /*@channel number[9:8]*/
	u8 sub_chnl : 4; /*sub-channel location[3:0]*/
	u8 r_rfmod : 2; /*RF mode[1:0]*/
#else /*@_BIG_ENDIAN_	*/
	u8 r_rfmod : 2;
	u8 sub_chnl : 4;
	u8 chl_num_msb : 2;
#endif

	/*	@DWORD 1*/
	u8 pwdb_all; /*@CCK signal quality / OFDM pwdb all*/
	s8 cfosho[2]; /*@CCK AGC report and CCK_BB_Power*/
		      /*OFDM path-A and path-B short CFO*/
#ifdef __LITTLE_ENDIAN
	u8 resvd_0 : 6;
	u8 bt_rf_ch_msb : 2; /*@8812A:2'b0  8814A: bt rf channel keep[7:6]*/
#else /*@_BIG_ENDIAN_*/
	u8 bt_rf_ch_msb : 2;
	u8 resvd_0 : 6;
#endif

/*	@DWORD 2*/
#ifdef __LITTLE_ENDIAN
	u8 ant_div_sw_a : 1; /*@8812A: ant_div_sw_a    8814A: 1'b0*/
	u8 ant_div_sw_b : 1; /*@8812A: ant_div_sw_b    8814A: 1'b0*/
	u8 bt_rf_ch_lsb : 6; /*@8812A: 6'b0     8814A: bt rf channel keep[5:0]*/
#else /*@_BIG_ENDIAN_	*/
	u8 bt_rf_ch_lsb : 6;
	u8 ant_div_sw_b : 1;
	u8 ant_div_sw_a : 1;
#endif
	s8 cfotail[2]; /*@DW2 byte 1 DW2 byte 2	path-A and path-B CFO tail*/
	u8 pcts_msk_rpt_0; /*PCTS mask report[7:0]*/
	u8 pcts_msk_rpt_1; /*PCTS mask report[15:8]*/

	/*	@DWORD 3*/
	s8 rxevm[2]; /*@DW3 byte 1 DW3 byte 2	stream 1 and stream 2 RX EVM*/
	s8 rxsnr[2]; /*@DW3 byte 3 DW4 byte 0	path-A and path-B RX SNR*/

	/*	@DWORD 4*/
	u8 pcts_msk_rpt_2; /*PCTS mask report[23:16]*/
#ifdef __LITTLE_ENDIAN
	u8 pcts_msk_rpt_3 : 6; /*PCTS mask report[29:24]*/
	u8 pcts_rpt_valid : 1; /*pcts_rpt_valid*/
	u8 resvd_1 : 1; /*@1'b0*/
#else /*@_BIG_ENDIAN_*/
	u8 resvd_1 : 1;
	u8 pcts_rpt_valid : 1;
	u8 pcts_msk_rpt_3 : 6;
#endif
	s8 rxevm_cd[2]; /*@8812A: 16'b0*/
			/*@8814A: stream 3 and stream 4 RX EVM*/
	/*	@DWORD 5*/
	u8 csi_current[2]; /*@8812A: stream 1 and 2 CSI*/
			   /*@8814A:  path-C and path-D RX SNR*/
	u8 gain_trsw_cd[2]; /*path-C and path-D {TRSW, gain[6:0] }*/

	/*	@DWORD 6*/
	s8 sigevm; /*signal field EVM*/
#ifdef __LITTLE_ENDIAN
	u8 antidx_antc : 3;	/*@8812A: 3'b0	8814A: antidx_antc[2:0]*/
	u8 antidx_antd : 3;	/*@8812A: 3'b0	8814A: antidx_antd[2:0]*/
	u8 dpdt_ctrl_keep : 1;	/*@8812A: 1'b0	8814A: dpdt_ctrl_keep*/
	u8 gnt_bt_keep : 1;	/*@8812A: 1'b0	8814A: GNT_BT_keep*/
#else /*@_BIG_ENDIAN_*/
	u8 gnt_bt_keep : 1;
	u8 dpdt_ctrl_keep : 1;
	u8 antidx_antd : 3;
	u8 antidx_antc : 3;
#endif
#ifdef __LITTLE_ENDIAN
	u8 antidx_anta : 3; /*@antidx_anta[2:0]*/
	u8 antidx_antb : 3; /*@antidx_antb[2:0]*/
	u8 hw_antsw_occur : 2; /*@1'b0*/
#else /*@_BIG_ENDIAN_*/
	u8 hw_antsw_occur : 2;
	u8 antidx_antb : 3;
	u8 antidx_anta : 3;
#endif
};

#define REG_SYS_CTRL				0x000
#define BIT_FEN_EN				BIT(26)
#define REG_APS_FSMCO				0x04
#define  APS_FSMCO_MAC_ENABLE			BIT(8)
#define  APS_FSMCO_MAC_OFF			BIT(9)
#define  APS_FSMCO_HW_POWERDOWN			BIT(15)
#define REG_ACLK_MON				0x3e
#define REG_RF_B_CTRL				0x76
#define REG_HIMR0				0xb0
#define REG_HISR0				0xb4
#define REG_HIMR1				0xb8
#define REG_HISR1				0xbc
#define HCI_TXDMA_EN				BIT(0)
#define HCI_RXDMA_EN				BIT(1)
#define TXDMA_EN				BIT(2)
#define RXDMA_EN				BIT(3)
#define PROTOCOL_EN				BIT(4)
#define SCHEDULE_EN				BIT(5)
#define MACTXEN					BIT(6)
#define MACRXEN					BIT(7)
#define ENSWBCN					BIT(8)
#define ENSEC					BIT(9)
#define CALTMR_EN				BIT(10)	/* 32k CAL TMR enable */
#define REG_MSR					0x102
#define REG_PBP					0x104
#define PBP_RX_MASK				0x0f
#define PBP_TX_MASK				0xf0
#define PBP_64					0x0
#define PBP_128					0x1
#define PBP_256					0x2
#define PBP_512					0x3
#define PBP_1024				0x4
#define REG_DWBCN1_CTRL				0x228
#define REG_EARLY_MODE_CONTROL			0x2bc
#define REG_TXPKT_EMPTY				0x41a
#define REG_AMPDU_MAX_LENGTH			0x458
#define REG_FAST_EDCA_CTRL			0x460
#define REG_TX_RPT_CTRL				0x4ec
#define REG_TX_RPT_TIME				0x4f0
#define REG_BCNTCFG				0x510
#define REG_INIRTS_RATE_SEL 			0x0480
#define REG_HTSTFWT				0x800
#define REG_CCK_RPT_FORMAT			0x804
#define BIT_CCK_RPT_FORMAT			BIT(16)
#define REG_RXPSEL				0x808
#define BIT_RX_PSEL_RST				(BIT(28) | BIT(29))
#define REG_TXPSEL				0x80c
#define REG_RXCCAMSK				0x814
#define REG_CCASEL				0x82c
#define REG_PDMFTH				0x830
#define REG_BWINDICATION			0x834
#define REG_CCA2ND				0x838
#define REG_L1WT				0x83c
#define REG_L1PKWT				0x840
#define REG_L1PKTH				0x848
#define REG_MRC					0x850
#define REG_CLKTRK				0x860
#define REG_ADCCLK				0x8ac
#define REG_HSSI_READ				0x8b0
#define REG_FPGA0_XCD_RF_PARA			0x8b4
#define REG_ADC160				0x8c4
#define REG_ADC40				0x8c8
#define REG_CHFIR				0x8f0
#define REG_ANTSEL_SW				0x900
#define REG_SINGLE_TONE_CONT_TX			0x914
#define REG_CDDTXP				0x93c
#define REG_TXPSEL1				0x940
#define REG_ACBB0				0x948
#define REG_ACBBRXFIR				0x94c
#define REG_ACGG2TBL				0x958
#define REG_FAS					0x9a4
#define REG_RXSB				0xa00
#define REG_CCK_RX				0xa04
#define REG_PWRTH				0xa08
#define REG_CCA_FLTR				0xa20
#define REG_TXSF2				0xa24
#define REG_TXSF6				0xa28
#define REG_FA_CCK				0xa5c
#define REG_RXDESC				0xa2c
#define REG_ENTXCCK				0xa80
#define BTG_LNA					0xfc84
#define WLG_LNA					0x7532
#define REG_ENRXCCA				0xa84
#define BTG_CCA					0x0e
#define WLG_CCA					0x12
#define REG_PWRTH2				0xaa8
#define REG_CSRATIO				0xaaa
#define REG_TXFILTER				0xaac
#define REG_CNTRST				0xb58
#define REG_AGCTR_A				0xc08
#define REG_RX_IQC_AB_A				0xc10
#define REG_TXSCALE_A				0xc1c
#define BB_SWING_MASK				GENMASK(31, 21)
#define REG_TXDFIR				0xc20
#define REG_RXIGI_A				0xc50
#define REG_TX_PWR_TRAINING_A			0xc54
#define REG_AFE_PWR1_A				0xc60
#define REG_AFE_PWR2_A				0xc64
#define REG_RX_WAIT_CCA_TX_CCK_RFON_A		0xc68
#define REG_LSSI_WRITE_A			0xc90
#define REG_TXAGCIDX				0xc94
#define REG_TRSW				0xca0
#define REG_RFESEL0				0xcb0
#define REG_RFE_PINMUX_A			0xcb0
#define REG_RFESEL8				0xcb4
#define REG_RFE_INV_A				0xcb4
#define RFE_INV_MASK				0x3ff00000
#define REG_RFECTL				0xcb8
#define B_BTG_SWITCH				BIT(16)
#define B_CTRL_SWITCH				BIT(18)
#define B_WL_SWITCH				(BIT(20) | BIT(22))
#define B_WLG_SWITCH				BIT(21)
#define B_WLA_SWITCH				BIT(23)
#define REG_RFEINV				0xcbc
#define REG_PI_READ_A				0xd04
#define REG_SI_READ_A				0xd08
#define REG_PI_READ_B				0xd44
#define REG_SI_READ_B				0xd48
#define REG_AGCTR_B				0xe08
#define REG_TXSCALE_B				0xe1c
#define REG_RXIGI_B				0xe50
#define REG_TX_PWR_TRAINING_B			0xe54
#define REG_LSSI_WRITE_B			0xe90
#define REG_RFE_PINMUX_B			0xeb0
#define REG_RFE_INV_B				0xeb4
#define REG_CRC_CCK				0xf04
#define REG_CRC_OFDM				0xf14
#define REG_CRC_HT				0xf10
#define REG_CRC_VHT				0xf0c
#define REG_CCA_OFDM				0xf08
#define REG_FA_OFDM				0xf48
#define REG_CCA_CCK				0xfcc
#define REG_DMEM_CTRL				0x1080
#define BIT_WL_RST				BIT(16)
#define REG_ANTWT				0x1904
#define REG_IQKFAILMSK				0x1bf0
#define BIT_MASK_R_RFE_SEL_15			GENMASK(31, 28)
#define BIT_SDIO_INT				BIT(18)
#define BT_CNT_ENABLE				0x1
#define BIT_BCN_QUEUE				BIT(3)
#define BCN_PRI_EN				0x1
#define PTA_CTRL_PIN				0x66
#define DPDT_CTRL_PIN				0x77
#define ANTDIC_CTRL_PIN				0x88
#define REG_CTRL_TYPE				0x67
#define BIT_CTRL_TYPE1				BIT(5)
#define BIT_CTRL_TYPE2				BIT(4)
#define CTRL_TYPE_MASK				GENMASK(15, 8)
#define REG_USB_HRPWM				0xfe58

#define RF18_BAND_MASK				(BIT(16) | BIT(9) | BIT(8))
#define RF18_BAND_2G				(0)
#define RF18_BAND_5G				(BIT(16) | BIT(8))
#define RF18_CHANNEL_MASK			(MASKBYTE0)
#define RF18_RFSI_MASK				(BIT(18) | BIT(17))
#define RF18_RFSI_GE				(BIT(17))
#define RF18_RFSI_GT				(BIT(18))
#define RF18_BW_MASK				(BIT(11) | BIT(10))
#define RF18_BW_20M				(BIT(11) | BIT(10))
#define RF18_BW_40M				(BIT(10))
#define RF18_BW_80M				(0)
#define RF_MODE_TABLE_ADDR			0x30
#define RF_MODE_TABLE_DATA0			0x31
#define RF_MODE_TABLE_DATA1			0x32
#define RF_LCK					0xb4

#endif
