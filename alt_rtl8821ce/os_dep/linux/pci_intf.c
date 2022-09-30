/******************************************************************************
 *
 * Copyright(c) 2007 - 2017 Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 *****************************************************************************/
#define _HCI_INTF_C_

#include <drv_types.h>
#include <hal_data.h>

#include <linux/pci_regs.h>

#ifndef CONFIG_PCI_HCI

	#error "CONFIG_PCI_HCI shall be on!\n"

#endif


#ifdef CONFIG_80211N_HT
	extern int rtw_ht_enable;
	extern int rtw_bw_mode;
	extern int rtw_ampdu_enable;/* for enable tx_ampdu */
#endif

#ifdef CONFIG_GLOBAL_UI_PID
int ui_pid[3] = {0, 0, 0};
#endif

extern int pm_netdev_open(struct net_device *pnetdev, u8 bnormal);
int rtw_resume_process(_adapter *padapter);

#ifdef CONFIG_PM
	static int rtw_pci_suspend(struct pci_dev *pdev, pm_message_t state);
	static int rtw_pci_resume(struct pci_dev *pdev);
#endif

static int rtw_drv_init(struct pci_dev *pdev, const struct pci_device_id *pdid);
static void rtw_dev_remove(struct pci_dev *pdev);
static void rtw_dev_shutdown(struct pci_dev *pdev);

static struct specific_device_id specific_device_id_tbl[] = {
	{.idVendor = 0x0b05, .idProduct = 0x1791, .flags = SPEC_DEV_ID_DISABLE_HT},
	{.idVendor = 0x13D3, .idProduct = 0x3311, .flags = SPEC_DEV_ID_DISABLE_HT},
	{}
};

struct pci_device_id rtw_pci_id_tbl[] = {
#ifdef CONFIG_RTL8188E
	{PCI_DEVICE(PCI_VENDER_ID_REALTEK, 0x8179), .driver_data = RTL8188E},
#endif
#ifdef CONFIG_RTL8812A
	{PCI_DEVICE(PCI_VENDER_ID_REALTEK, 0x8812), .driver_data = RTL8812},
#endif
#ifdef CONFIG_RTL8821A
	{PCI_DEVICE(PCI_VENDER_ID_REALTEK, 0x8821), .driver_data = RTL8821},
#endif
#ifdef CONFIG_RTL8192E
	{PCI_DEVICE(PCI_VENDER_ID_REALTEK, 0x818B), .driver_data = RTL8192E},
#endif
#ifdef CONFIG_RTL8192F
	{PCI_DEVICE(PCI_VENDER_ID_REALTEK, 0xf192), .driver_data = RTL8192F},
#endif
#ifdef CONFIG_RTL8723B
	{PCI_DEVICE(PCI_VENDER_ID_REALTEK, 0xb723), .driver_data = RTL8723B},
#endif
#ifdef CONFIG_RTL8723D
	{PCI_DEVICE(PCI_VENDER_ID_REALTEK, 0xd723), .driver_data = RTL8723D},
#endif
#ifdef CONFIG_RTL8814A
	{PCI_DEVICE(PCI_VENDER_ID_REALTEK, 0x8813), .driver_data = RTL8814A},
#endif
#ifdef CONFIG_RTL8822B
	{PCI_DEVICE(PCI_VENDER_ID_REALTEK, 0xB822), .driver_data = RTL8822B},
#endif
#ifdef CONFIG_RTL8821C
	{PCI_DEVICE(PCI_VENDER_ID_REALTEK, 0xC821), .driver_data = RTL8821C},
	{PCI_DEVICE(PCI_VENDER_ID_REALTEK, 0xC82A), .driver_data = RTL8821C},
	{PCI_DEVICE(PCI_VENDER_ID_REALTEK, 0xC82B), .driver_data = RTL8821C},
#endif
#ifdef CONFIG_RTL8822C
	{PCI_DEVICE(PCI_VENDER_ID_REALTEK, 0xC822), .driver_data = RTL8822C},
#endif
#ifdef CONFIG_RTL8814B
	{PCI_DEVICE(PCI_VENDER_ID_REALTEK, 0xB814), .driver_data = RTL8814B},
#endif
	{},
};

struct pci_drv_priv {
	struct pci_driver rtw_pci_drv;
	int drv_registered;
};


static struct pci_drv_priv pci_drvpriv = {
	.rtw_pci_drv.name = (char *)DRV_NAME,
	.rtw_pci_drv.probe = rtw_drv_init,
	.rtw_pci_drv.remove = rtw_dev_remove,
	.rtw_pci_drv.shutdown = rtw_dev_shutdown,
	.rtw_pci_drv.id_table = rtw_pci_id_tbl,
#ifdef CONFIG_PM
	.rtw_pci_drv.suspend = rtw_pci_suspend,
	.rtw_pci_drv.resume = rtw_pci_resume,
#endif
};


MODULE_DEVICE_TABLE(pci, rtw_pci_id_tbl);

void	PlatformClearPciPMEStatus(PADAPTER Adapter)
{
	struct dvobj_priv	*pdvobjpriv = adapter_to_dvobj(Adapter);
	struct pci_dev	*pdev = pdvobjpriv->ppcidev;
	BOOLEAN		PCIClkReq = _FALSE;
	u8	PMCSReg;

	if (pdev->pm_cap) {
		/* Get the PM CSR (Control/Status Register), */
		/* The PME_Status is located at PM Capatibility offset 5, bit 7 */

		pci_read_config_byte(pdev, pdev->pm_cap + 5, &PMCSReg);
		if (PMCSReg & BIT7) {
			/* PME event occurred, clear the PM_Status by write 1 */
			PMCSReg = PMCSReg | BIT7;

			pci_write_config_byte(pdev, pdev->pm_cap + 5, PMCSReg);
			PCIClkReq = _TRUE;
			/* Read it back to check */
			pci_read_config_byte(pdev, pdev->pm_cap + 5, &PMCSReg);
			RTW_INFO("%s(): Clear PME status 0x%2x to 0x%2x\n", __func__, pdev->pm_cap + 5, PMCSReg);
		} else {
			RTW_INFO("%s(): PME status(0x%2x) = 0x%2x\n", __func__, pdev->pm_cap + 5, PMCSReg);
		}
	} else {
		RTW_INFO("%s(): Cannot find PME Capability\n", __func__);
	}

	RTW_INFO("PME, value_offset = %x, PME EN = %x\n", pdev->pm_cap + 5, PCIClkReq);
}

void rtw_pci_aspm_config_clkreql0sl1(_adapter *padapter)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	u8 tmp8 = 0;
	u16 tmp16 = 0;

	/* 0x70f Bit7 for L0s */
	tmp8 = rtw_hal_pci_dbi_read(padapter, 0x70f);

	if (pHalData->pci_backdoor_ctrl & PCI_BC_ASPM_L0s)
		tmp8 |= BIT7;
	else
		tmp8 &= (~BIT7);

	/* Default set L1 entrance latency to 16us */
	/* L0s: b[0-2], L1: b[3-5]*/
	if (pHalData->pci_backdoor_ctrl & PCI_BC_ASPM_L1) {
		tmp8 &= (~0x38);
		tmp8 |= 0x20;
	}

	rtw_hal_pci_dbi_write(padapter, 0x70f, tmp8);


	/* 0x719 Bit 3 for L1 ,  Bit4 for clock req */
	tmp8 = rtw_hal_pci_dbi_read(padapter, 0x719);

	if (pHalData->pci_backdoor_ctrl & PCI_BC_ASPM_L1)
		tmp8 |= BIT3;
	else
		tmp8 &= (~BIT3);

	if (pHalData->pci_backdoor_ctrl & PCI_BC_CLK_REQ)
		tmp8 |= BIT4;
	else
		tmp8 &= (~BIT4);

	rtw_hal_pci_dbi_write(padapter, 0x719, tmp8);

	if (pHalData->pci_backdoor_ctrl & PCI_BC_CLK_REQ) {
		tmp16 = rtw_hal_pci_mdio_read(padapter, 0x10);
		rtw_hal_pci_mdio_write(padapter, 0x10, (tmp16 | BIT2));
	}
}

void rtw_pci_aspm_config_l1off(_adapter *padapter)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);

	u8 enable_l1off = _FALSE;

	if (pHalData->pci_backdoor_ctrl & PCI_BC_ASPM_L1Off)
		enable_l1off = rtw_hal_pci_l1off_nic_support(padapter);

	padapter->hal_func.hal_set_l1ssbackdoor_handler(padapter, enable_l1off);

}

void rtw_pci_aspm_config_l1off_general(_adapter *padapter, u8 enablel1off)
{

	u8 tmp8;
	u16 tmp16;

	if (enablel1off) {
		/* 0x718 Bit5 for L1SS */
		tmp8 = rtw_hal_pci_dbi_read(padapter, 0x718);
		rtw_hal_pci_dbi_write(padapter, 0x718, (tmp8 | BIT5));

		tmp16 = rtw_hal_pci_mdio_read(padapter, 0x1b);
		rtw_hal_pci_mdio_write(padapter, 0x1b, (tmp16 | BIT4));
	} else {
		tmp8 = rtw_hal_pci_dbi_read(padapter, 0x718);
		rtw_hal_pci_dbi_write(padapter, 0x718, (tmp8 & (~BIT5)));
	}

}

#ifdef CONFIG_PCI_DYNAMIC_ASPM_L1_LATENCY
void rtw_pci_set_l1_latency(_adapter *padapter, u8 mode)
{
	HAL_DATA_TYPE *pHalData	= GET_HAL_DATA(padapter);
	u8 tmp8;

	if (!(pHalData->pci_backdoor_ctrl & PCI_BC_ASPM_L1))
		return;

	tmp8 = rtw_hal_pci_dbi_read(padapter, 0x70F);
	tmp8 &= (~0x38);

	switch (mode) {
	case ASPM_MODE_PS:
		/*tmp8 |= 0x10; *//*L1 entrance latency: 4us*/
		/*tmp8 |= 0x18; *//*L1 entrance latency: 8us*/
		tmp8 |= 0x20; /*L1 entrance latency: 16us*/
		rtw_hal_pci_dbi_write(padapter, 0x70F, tmp8);
		break;
	case ASPM_MODE_PERF:
		tmp8 |= 0x28; /*L1 entrance latency: 32us*/
		rtw_hal_pci_dbi_write(padapter, 0x70F, tmp8);
		break;
	}
}
#endif

#ifdef CONFIG_PCI_DYNAMIC_ASPM_LINK_CTRL
static bool _rtw_pci_set_aspm_lnkctl_reg(struct pci_dev *pdev, u8 mask, u8 val)
{
	u8 linkctrl, new_val;

	if (!pdev || !pdev->pcie_cap || !mask)
		return false;

	pci_read_config_byte(pdev, pdev->pcie_cap + PCI_EXP_LNKCTL, &linkctrl);
	new_val = (linkctrl & ~mask) | val;
	if (new_val == linkctrl)
		return false;

	pci_write_config_byte(pdev, pdev->pcie_cap + PCI_EXP_LNKCTL, new_val);

	return true;
}

void rtw_pci_set_aspm_lnkctl(_adapter *padapter, u8 mode)
{
	struct dvobj_priv *pdvobjpriv = adapter_to_dvobj(padapter);
	struct pci_priv	*pcipriv = &(pdvobjpriv->pcipriv);
	struct pci_dev	*pdev = pdvobjpriv->ppcidev;
	struct pci_dev	*br_pdev = pdev->bus->self;
	struct registry_priv  *registry_par = &padapter->registrypriv;
	u32 pci_dynamic_aspm_linkctrl = registry_par->pci_dynamic_aspm_linkctrl;
	u8 lnkctl_val, lnkctl_mask;
	u8 dev_lnkctl_val, br_lnkctl_val;

	if (!pci_dynamic_aspm_linkctrl)
		return;

	switch (mode) {
	case ASPM_MODE_PERF:
		lnkctl_val = pci_dynamic_aspm_linkctrl & GENMASK(1, 0);
		lnkctl_mask = (pci_dynamic_aspm_linkctrl & GENMASK(5, 4)) >> 4;
		break;
	case ASPM_MODE_PS:
		lnkctl_val = (pci_dynamic_aspm_linkctrl & GENMASK(9, 8)) >> 8;
		lnkctl_mask = (pci_dynamic_aspm_linkctrl & GENMASK(13, 12)) >> 12;
		break;
	case ASPM_MODE_DEF:
		lnkctl_val = 0x0; /* fill val to make checker happy */
		lnkctl_mask = 0x0;
		break;
	default:
		return;
	}

	/* if certain mask==0x0, we restore the default value with mask 0x03 */
	if (lnkctl_mask == 0x0) {
		lnkctl_mask = PCI_EXP_LNKCTL_ASPMC;
		dev_lnkctl_val = pcipriv->linkctrl_reg;
		br_lnkctl_val = pcipriv->pcibridge_linkctrlreg;
	} else {
		dev_lnkctl_val = lnkctl_val;
		br_lnkctl_val = lnkctl_val;
	}

	if (_rtw_pci_set_aspm_lnkctl_reg(pdev, lnkctl_mask, dev_lnkctl_val))
		rtw_udelay_os(50);
	_rtw_pci_set_aspm_lnkctl_reg(br_pdev, lnkctl_mask, br_lnkctl_val);
}
#endif

void rtw_pci_dump_aspm_info(_adapter *padapter)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	struct dvobj_priv	*pdvobjpriv = adapter_to_dvobj(padapter);
	struct pci_priv	*pcipriv = &(pdvobjpriv->pcipriv);
	u8 tmp8 = 0;
	u16 tmp16 = 0;
	u32 tmp32 = 0;
	u8 l1_idle = 0;


	RTW_INFO("***** ASPM Capability *****\n");

	pci_read_config_dword(pdvobjpriv->ppcidev, pcipriv->pciehdr_offset + PCI_EXP_LNKCAP, &tmp32);

	RTW_INFO("CLK REQ:	%s\n", (tmp32&PCI_EXP_LNKCAP_CLKPM) ? "Enable" : "Disable");

	RTW_INFO("ASPM L0s:	%s\n", (tmp32&BIT10) ? "Enable" : "Disable");
	RTW_INFO("ASPM L1:	%s\n", (tmp32&BIT11) ? "Enable" : "Disable");

	tmp8 = rtw_hal_pci_l1off_capability(padapter);
	RTW_INFO("ASPM L1OFF:%s\n", tmp8 ? "Enable" : "Disable");

	RTW_INFO("***** ASPM CTRL Reg *****\n");

	pci_read_config_word(pdvobjpriv->ppcidev, pcipriv->pciehdr_offset + PCI_EXP_LNKCTL, &tmp16);

	RTW_INFO("CLK REQ:	%s\n", (tmp16&PCI_EXP_LNKCTL_CLKREQ_EN) ? "Enable" : "Disable");
	RTW_INFO("ASPM L0s:	%s\n", (tmp16&BIT0) ? "Enable" : "Disable");
	RTW_INFO("ASPM L1:	%s\n", (tmp16&BIT1) ? "Enable" : "Disable");

	tmp8 = rtw_hal_pci_l1off_nic_support(padapter);
	RTW_INFO("ASPM L1OFF:%s\n", tmp8 ? "Enable" : "Disable");

	RTW_INFO("***** ASPM Backdoor *****\n");

	tmp8 = rtw_hal_pci_dbi_read(padapter, 0x719);
	RTW_INFO("CLK REQ:	%s\n", (tmp8 & BIT4) ? "Enable" : "Disable");

	tmp8 = rtw_hal_pci_dbi_read(padapter, 0x70f);
	l1_idle = tmp8 & 0x38;
	RTW_INFO("ASPM L0s:	%s\n", (tmp8&BIT7) ? "Enable" : "Disable");

	tmp8 = rtw_hal_pci_dbi_read(padapter, 0x719);
	RTW_INFO("ASPM L1:	%s\n", (tmp8 & BIT3) ? "Enable" : "Disable");

	tmp8 = rtw_hal_pci_dbi_read(padapter, 0x718);
	RTW_INFO("ASPM L1OFF:%s\n", (tmp8 & BIT5) ? "Enable" : "Disable");
	
	RTW_INFO("********* MISC **********\n");
	RTW_INFO("ASPM L1 Idel Time: 0x%x\n", l1_idle>>3);
	RTW_INFO("*************************\n");
}

void rtw_pci_aspm_config(_adapter *padapter)
{
	rtw_pci_aspm_config_clkreql0sl1(padapter);
	rtw_pci_aspm_config_l1off(padapter);
	rtw_pci_dynamic_aspm_set_mode(padapter, ASPM_MODE_PERF);
	rtw_pci_dump_aspm_info(padapter);
}

static u8 rtw_pci_get_amd_l1_patch(struct dvobj_priv *pdvobjpriv, struct pci_dev *pdev)
{
	u8	status = _FALSE;
	u8	offset_e0;
	u32	offset_e4;

	pci_write_config_byte(pdev, 0xE0, 0xA0);
	pci_read_config_byte(pdev, 0xE0, &offset_e0);

	if (offset_e0 == 0xA0) {
		pci_read_config_dword(pdev, 0xE4, &offset_e4);
		if (offset_e4 & BIT(23))
			status = _TRUE;
	}

	return status;
}

static s32	rtw_set_pci_cache_line_size(struct pci_dev *pdev, u8 CacheLineSizeToSet)
{
	u8	ucPciCacheLineSize;
	s32	Result;

	/* ucPciCacheLineSize  = pPciConfig->CacheLineSize; */
	pci_read_config_byte(pdev, PCI_CACHE_LINE_SIZE, &ucPciCacheLineSize);

	if (ucPciCacheLineSize < 8 || ucPciCacheLineSize > 16) {
		RTW_INFO("Driver Sets default Cache Line Size...\n");

		ucPciCacheLineSize = CacheLineSizeToSet;

		Result = pci_write_config_byte(pdev, PCI_CACHE_LINE_SIZE, ucPciCacheLineSize);

		if (Result != 0) {
			RTW_INFO("pci_write_config_byte (CacheLineSize) Result=%d\n", Result);
			goto _SET_CACHELINE_SIZE_FAIL;
		}

		Result = pci_read_config_byte(pdev, PCI_CACHE_LINE_SIZE, &ucPciCacheLineSize);
		if (Result != 0) {
			RTW_INFO("pci_read_config_byte (PciCacheLineSize) Result=%d\n", Result);
			goto _SET_CACHELINE_SIZE_FAIL;
		}

		if (ucPciCacheLineSize != CacheLineSizeToSet) {
			RTW_INFO("Failed to set Cache Line Size to 0x%x! ucPciCacheLineSize=%x\n", CacheLineSizeToSet, ucPciCacheLineSize);
			goto _SET_CACHELINE_SIZE_FAIL;
		}
	}

	return _SUCCESS;

_SET_CACHELINE_SIZE_FAIL:

	return _FAIL;
}


#define PCI_CMD_ENABLE_BUS_MASTER		BIT(2)
#define PCI_CMD_DISABLE_INTERRUPT		BIT(10)
#define CMD_BUS_MASTER				BIT(2)

static s32 rtw_pci_parse_configuration(struct pci_dev *pdev, struct dvobj_priv *pdvobjpriv)
{
	struct pci_priv	*pcipriv = &(pdvobjpriv->pcipriv);
	/* PPCI_COMMON_CONFIG pPciConfig = (PPCI_COMMON_CONFIG) pucBuffer; */
	/* u16	usPciCommand = pPciConfig->Command; */
	u16	usPciCommand = 0;
	int	Result, ret = _FAIL;
	u8	LinkCtrlReg;
	u8	ClkReqReg;

	/* RTW_INFO("%s==>\n", __func__); */

	pci_read_config_word(pdev, PCI_COMMAND, &usPciCommand);

	do {
		/* 3 Enable bus matering if it isn't enabled by the BIOS */
		if (!(usPciCommand & PCI_CMD_ENABLE_BUS_MASTER)) {
			RTW_INFO("Bus master is not enabled by BIOS! usPciCommand=%x\n", usPciCommand);

			usPciCommand |= CMD_BUS_MASTER;

			Result = pci_write_config_word(pdev, PCI_COMMAND, usPciCommand);
			if (Result != 0) {
				RTW_INFO("pci_write_config_word (Command) Result=%d\n", Result);
				ret = _FAIL;
				break;
			}

			Result = pci_read_config_word(pdev, PCI_COMMAND, &usPciCommand);
			if (Result != 0) {
				RTW_INFO("pci_read_config_word (Command) Result=%d\n", Result);
				ret = _FAIL;
				break;
			}

			if (!(usPciCommand & PCI_CMD_ENABLE_BUS_MASTER)) {
				RTW_INFO("Failed to enable bus master! usPciCommand=%x\n", usPciCommand);
				ret = _FAIL;
				break;
			}
		}
		RTW_INFO("Bus master is enabled. usPciCommand=%x\n", usPciCommand);

		/* 3 Enable interrupt */
		if ((usPciCommand & PCI_CMD_DISABLE_INTERRUPT)) {
			RTW_INFO("INTDIS==1 usPciCommand=%x\n", usPciCommand);

			usPciCommand &= (~PCI_CMD_DISABLE_INTERRUPT);

			Result = pci_write_config_word(pdev, PCI_COMMAND, usPciCommand);
			if (Result != 0) {
				RTW_INFO("pci_write_config_word (Command) Result=%d\n", Result);
				ret = _FAIL;
				break;
			}

			Result = pci_read_config_word(pdev, PCI_COMMAND, &usPciCommand);
			if (Result != 0) {
				RTW_INFO("pci_read_config_word (Command) Result=%d\n", Result);
				ret = _FAIL;
				break;
			}

			if ((usPciCommand & PCI_CMD_DISABLE_INTERRUPT)) {
				RTW_INFO("Failed to set INTDIS to 0! usPciCommand=%x\n", usPciCommand);
				ret = _FAIL;
				break;
			}
		}

		/*  */
		/* Description: Find PCI express capability offset. Porting from 818xB by tynli 2008.12.19 */
		/*  */
		/* ------------------------------------------------------------- */

		/* 3 PCIeCap */
		if (pdev->pcie_cap) {
			pcipriv->pciehdr_offset = pdev->pcie_cap;
			RTW_INFO("PCIe Header Offset =%x\n", pdev->pcie_cap);

			/* 3 Link Control Register */
			/* Read "Link Control Register" Field (80h ~81h) */
			Result = pci_read_config_byte(pdev, pdev->pcie_cap + 0x10, &LinkCtrlReg);
			if (Result != 0) {
				RTW_INFO("pci_read_config_byte (Link Control Register) Result=%d\n", Result);
				break;
			}

			pcipriv->linkctrl_reg = LinkCtrlReg;
			RTW_INFO("Link Control Register =%x\n", LinkCtrlReg);

			/* 3 Get Capability of PCI Clock Request */
			/* The clock request setting is located at 0x81[0] */
			Result = pci_read_config_byte(pdev, pdev->pcie_cap + 0x11, &ClkReqReg);
			if (Result != 0) {
				pcipriv->pci_clk_req = _FALSE;
				RTW_INFO("pci_read_config_byte (Clock Request Register) Result=%d\n", Result);
				break;
			}
			if (ClkReqReg & BIT(0))
				pcipriv->pci_clk_req = _TRUE;
			else
				pcipriv->pci_clk_req = _FALSE;
			RTW_INFO("Clock Request =%x\n", pcipriv->pci_clk_req);
		} else {
			/* We didn't find a PCIe capability. */
			RTW_INFO("Didn't Find PCIe Capability\n");
			break;
		}

		/* 3 Fill Cacheline */
		ret = rtw_set_pci_cache_line_size(pdev, 8);
		if (ret != _SUCCESS) {
			RTW_INFO("rtw_set_pci_cache_line_size fail\n");
			break;
		}

		/* Include 92C suggested by SD1. Added by tynli. 2009.11.25.
		 * Enable the Backdoor
		 */
		{
			u8	tmp;

			Result = pci_read_config_byte(pdev, 0x98, &tmp);

			tmp |= BIT4;

			Result = pci_write_config_byte(pdev, 0x98, tmp);

		}
		ret = _SUCCESS;
	} while (_FALSE);

	return ret;
}

#ifdef CONFIG_64BIT_DMA
static void rtw_pci_enable_dma64(struct pci_dev *pdev)
{
	u8 value;

	pci_read_config_byte(pdev, 0x719, &value);

	/* 0x719 Bit5 is DMA64 bit fetch. */
	value |= (BIT5);

	pci_write_config_byte(pdev, 0x719, value);
}
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 0)) || (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 18))
	#define rtw_pci_interrupt(x, y, z) rtw_pci_interrupt(x, y)
#endif

static irqreturn_t rtw_pci_interrupt(int irq, void *priv, struct pt_regs *regs)
{
	struct dvobj_priv *dvobj = (struct dvobj_priv *)priv;
	_adapter *adapter = dvobj_get_primary_adapter(dvobj);

	if (dvobj->irq_enabled == 0)
		return IRQ_HANDLED;

	if (rtw_hal_interrupt_handler(adapter) == _FAIL)
		return IRQ_HANDLED;
	/* return IRQ_NONE; */

	return IRQ_HANDLED;
}

#if defined(RTK_DMP_PLATFORM) || defined(CONFIG_PLATFORM_RTL8197D)
	#define pci_iounmap(x, y) iounmap(y)
#endif

int pci_alloc_irq(struct dvobj_priv *dvobj)
{
	int err;
	struct pci_dev *pdev = dvobj->ppcidev;
	int ret;

#ifndef CONFIG_RTW_PCI_MSI_DISABLE
	ret = pci_enable_msi(pdev);

	RTW_INFO("pci_enable_msi ret=%d\n", ret);
#endif

#if defined(IRQF_SHARED)
	err = request_irq(pdev->irq, &rtw_pci_interrupt, IRQF_SHARED, DRV_NAME, dvobj);
#else
	err = request_irq(pdev->irq, &rtw_pci_interrupt, SA_SHIRQ, DRV_NAME, dvobj);
#endif
	if (err)
		RTW_INFO("Error allocating IRQ %d", pdev->irq);
	else {
		dvobj->irq_alloc = 1;
		dvobj->irq = pdev->irq;
		RTW_INFO("Request_irq OK, IRQ %d\n", pdev->irq);
	}

	return err ? _FAIL : _SUCCESS;
}

static void rtw_decide_chip_type_by_pci_driver_data(struct dvobj_priv *pdvobj, const struct pci_device_id *pdid)
{
	pdvobj->chip_type = pdid->driver_data;

#ifdef CONFIG_RTL8188E
	if (pdvobj->chip_type == RTL8188E) {
		pdvobj->HardwareType = HARDWARE_TYPE_RTL8188EE;
		RTW_INFO("CHIP TYPE: RTL8188E\n");
	}
#endif

#ifdef CONFIG_RTL8812A
	if (pdvobj->chip_type == RTL8812) {
		pdvobj->HardwareType = HARDWARE_TYPE_RTL8812E;
		RTW_INFO("CHIP TYPE: RTL8812AE\n");
	}
#endif

#ifdef CONFIG_RTL8821A
	if (pdvobj->chip_type == RTL8821) {
		pdvobj->HardwareType = HARDWARE_TYPE_RTL8821E;
		RTW_INFO("CHIP TYPE: RTL8821AE\n");
	}
#endif

#ifdef CONFIG_RTL8723B
	if (pdvobj->chip_type == RTL8723B) {
		pdvobj->HardwareType = HARDWARE_TYPE_RTL8723BE;
		RTW_INFO("CHIP TYPE: RTL8723BE\n");
	}
#endif
#ifdef CONFIG_RTL8723D
	if (pdvobj->chip_type == RTL8723D) {
		pdvobj->HardwareType = HARDWARE_TYPE_RTL8723DE;
		RTW_INFO("CHIP TYPE: RTL8723DE\n");
	}
#endif
#ifdef CONFIG_RTL8192E
	if (pdvobj->chip_type == RTL8192E) {
		pdvobj->HardwareType = HARDWARE_TYPE_RTL8192EE;
		RTW_INFO("CHIP TYPE: RTL8192EE\n");
	}
#endif

#ifdef CONFIG_RTL8192F
	if (pdvobj->chip_type == RTL8192F) {
		pdvobj->HardwareType = HARDWARE_TYPE_RTL8192FE;
		RTW_INFO("CHIP TYPE: RTL8192FE\n");
	}
#endif
#ifdef CONFIG_RTL8814A
	if (pdvobj->chip_type == RTL8814A) {
		pdvobj->HardwareType = HARDWARE_TYPE_RTL8814AE;
		RTW_INFO("CHIP TYPE: RTL8814AE\n");
	}
#endif

#if defined(CONFIG_RTL8822B)
	if (pdvobj->chip_type == RTL8822B) {
		pdvobj->HardwareType = HARDWARE_TYPE_RTL8822BE;
		RTW_INFO("CHIP TYPE: RTL8822BE\n");
	}
#endif

#if defined(CONFIG_RTL8821C)
	if (pdvobj->chip_type == RTL8821C) {
		pdvobj->HardwareType = HARDWARE_TYPE_RTL8821CE;
		RTW_INFO("CHIP TYPE: RTL8821CE\n");
	}
#endif

#if defined(CONFIG_RTL8822C)
	if (pdvobj->chip_type == RTL8822C) {
		pdvobj->HardwareType = HARDWARE_TYPE_RTL8822CE;
		RTW_INFO("CHIP TYPE: RTL8822CE\n");
	}
#endif

#if defined(CONFIG_RTL8814B)
	if (pdvobj->chip_type == RTL8814B) {
		pdvobj->HardwareType = HARDWARE_TYPE_RTL8814BE;
		RTW_INFO("CHIP TYPE: RTL8814BE\n");
	}
#endif
}

static struct dvobj_priv	*pci_dvobj_init(struct pci_dev *pdev, const struct pci_device_id *pdid)
{
	int err;
	u32	status = _FAIL;
	struct dvobj_priv	*dvobj = NULL;
	struct pci_priv	*pcipriv = NULL;
	struct pci_dev	*bridge_pdev = pdev->bus->self;
	/* u32	pci_cfg_space[16]; */
	unsigned long pmem_start, pmem_len, pmem_flags;
	int	i;


	dvobj = devobj_init();
	if (dvobj == NULL)
		goto exit;


	dvobj->ppcidev = pdev;
	pcipriv = &(dvobj->pcipriv);
	pci_set_drvdata(pdev, dvobj);


	err = pci_enable_device(pdev);
	if (err != 0) {
		RTW_ERR("%s : Cannot enable new PCI device\n", pci_name(pdev));
		goto free_dvobj;
	}

#ifdef CONFIG_64BIT_DMA
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 18, 0)
	if (!pci_set_dma_mask(pdev, DMA_BIT_MASK(64))) {
		RTW_INFO("RTL819xCE: Using 64bit DMA\n");
		err = pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(64));
#else
	if (!dma_set_mask(&pdev->dev, DMA_BIT_MASK(64))) {
		RTW_INFO("RTL819xCE: Using 64bit DMA\n");
		err = dma_set_coherent_mask(&pdev->dev, DMA_BIT_MASK(64));
#endif
		if (err != 0) {
			RTW_ERR("Unable to obtain 64bit DMA for consistent allocations\n");
			goto disable_picdev;
		}
		RTW_INFO("PCIE: Using 64bit DMA\n");
		dvobj->bdma64 = _TRUE;
		rtw_pci_enable_dma64(pdev);
	} else
#endif
	{
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 18, 0)
		if (!pci_set_dma_mask(pdev, DMA_BIT_MASK(32))) {
			err = pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(32));
#else
		if (!dma_set_mask(&pdev->dev, DMA_BIT_MASK(32))) {
			err = dma_set_coherent_mask(&pdev->dev, DMA_BIT_MASK(32));
#endif
			if (err != 0) {
				RTW_ERR("Unable to obtain 32bit DMA for consistent allocations\n");
				goto disable_picdev;
			}
		}
		RTW_INFO("PCIE: Using 32bit DMA\n");
		dvobj->bdma64 = _FALSE;
	}

	pci_set_master(pdev);

	err = pci_request_regions(pdev, DRV_NAME);
	if (err != 0) {
		RTW_ERR("Can't obtain PCI resources\n");
		goto disable_picdev;
	}

#ifdef CONFIG_PLATFORM_RTK129X
	if (pdev->bus->number == 0x00) {
		pmem_start = PCIE_SLOT1_MEM_START;
		pmem_len   = PCIE_SLOT1_MEM_LEN;
		pmem_flags = 0;
		RTW_PRINT("RTD129X: PCIE SLOT1\n");
	} else if (pdev->bus->number == 0x01) {
		pmem_start = PCIE_SLOT2_MEM_START;
		pmem_len   = PCIE_SLOT2_MEM_LEN;
		pmem_flags = 0;
		RTW_PRINT("RTD129X: PCIE SLOT2\n");
	} else {
		RTW_ERR(KERN_ERR "RTD129X: Wrong Slot Num\n");
		goto release_regions;
	}
#else
	/* Search for memory map resource (index 0~5) */
	for (i = 0 ; i < 6 ; i++) {
		pmem_start = pci_resource_start(pdev, i);
		pmem_len = pci_resource_len(pdev, i);
		pmem_flags = pci_resource_flags(pdev, i);

		if (pmem_flags & IORESOURCE_MEM)
			break;
	}

	if (i == 6) {
		RTW_ERR("%s: No MMIO resource found, abort!\n", __func__);
		goto release_regions;
	}
#endif /* RTK_DMP_PLATFORM */

#ifdef RTK_DMP_PLATFORM
	dvobj->pci_mem_start = (unsigned long)ioremap_nocache(pmem_start, pmem_len);
#elif defined(CONFIG_PLATFORM_RTK129X)
	if (pdev->bus->number == 0x00)
		dvobj->ctrl_start =
			(unsigned long)ioremap(PCIE_SLOT1_CTRL_START, 0x200);
	else if (pdev->bus->number == 0x01)
		dvobj->ctrl_start =
			(unsigned long)ioremap(PCIE_SLOT2_CTRL_START, 0x200);

	if (dvobj->ctrl_start == 0) {
		RTW_ERR("RTD129X: Can't map CTRL mem\n");
		goto release_regions;
	}

	dvobj->mask_addr = dvobj->ctrl_start + PCIE_MASK_OFFSET;
	dvobj->tran_addr = dvobj->ctrl_start + PCIE_TRANSLATE_OFFSET;

	dvobj->pci_mem_start =
		(unsigned long)ioremap_nocache(pmem_start, pmem_len);
#else
	/* shared mem start */
	dvobj->pci_mem_start = (unsigned long)pci_iomap(pdev, i, pmem_len);
#endif
	if (dvobj->pci_mem_start == 0) {
		RTW_ERR("Can't map PCI mem\n");
		goto release_regions;
	}

	RTW_INFO("Memory mapped space start: 0x%08lx len:%08lx flags:%08lx, after map:0x%08lx\n",
		 pmem_start, pmem_len, pmem_flags, dvobj->pci_mem_start);

#if 0
	/* Read PCI configuration Space Header */
	for (i = 0; i < 16; i++)
		pci_read_config_dword(pdev, (i << 2), &pci_cfg_space[i]);
#endif

	/*step 1-1., decide the chip_type via device info*/
	dvobj->interface_type = RTW_PCIE;
	rtw_decide_chip_type_by_pci_driver_data(dvobj, pdid);


	/* rtw_pci_parse_configuration(pdev, dvobj, (u8 *)&pci_cfg_space); */
	if (rtw_pci_parse_configuration(pdev, dvobj) == _FAIL) {
		RTW_ERR("PCI parse configuration error\n");
		goto iounmap;
	}

	if (bridge_pdev) {
		pci_read_config_byte(bridge_pdev,
				     bridge_pdev->pcie_cap + PCI_EXP_LNKCTL,
				     &pcipriv->pcibridge_linkctrlreg);

		if (bridge_pdev->vendor == AMD_VENDOR_ID)
			pcipriv->amd_l1_patch = rtw_pci_get_amd_l1_patch(dvobj, bridge_pdev);
	}

	status = _SUCCESS;

iounmap:
	if (status != _SUCCESS && dvobj->pci_mem_start != 0) {
#if 1/* def RTK_DMP_PLATFORM */
		pci_iounmap(pdev, (void *)dvobj->pci_mem_start);
#endif
		dvobj->pci_mem_start = 0;
	}

#ifdef CONFIG_PLATFORM_RTK129X
	if (status != _SUCCESS && dvobj->ctrl_start != 0) {
		pci_iounmap(pdev, (void *)dvobj->ctrl_start);
		dvobj->ctrl_start = 0;
	}
#endif

release_regions:
	if (status != _SUCCESS)
		pci_release_regions(pdev);
disable_picdev:
	if (status != _SUCCESS)
		pci_disable_device(pdev);
free_dvobj:
	if (status != _SUCCESS && dvobj) {
		pci_set_drvdata(pdev, NULL);
		devobj_deinit(dvobj);
		dvobj = NULL;
	}
exit:
	return dvobj;
}


static void pci_dvobj_deinit(struct pci_dev *pdev)
{
	struct dvobj_priv *dvobj = pci_get_drvdata(pdev);

	pci_set_drvdata(pdev, NULL);
	if (dvobj) {
		if (dvobj->irq_alloc) {
			free_irq(pdev->irq, dvobj);
#ifndef CONFIG_RTW_PCI_MSI_DISABLE
			pci_disable_msi(pdev);
#endif
			dvobj->irq_alloc = 0;
		}

		if (dvobj->pci_mem_start != 0) {
#if 1/* def RTK_DMP_PLATFORM */
			pci_iounmap(pdev, (void *)dvobj->pci_mem_start);
#endif
			dvobj->pci_mem_start = 0;
		}

#ifdef CONFIG_PLATFORM_RTK129X
		if (dvobj->ctrl_start != 0) {
			pci_iounmap(pdev, (void *)dvobj->ctrl_start);
			dvobj->ctrl_start = 0;
		}
#endif
		devobj_deinit(dvobj);
	}

	pci_release_regions(pdev);
	pci_disable_device(pdev);

}


u8 rtw_set_hal_ops(_adapter *padapter)
{
	/* alloc memory for HAL DATA */
	if (rtw_hal_data_init(padapter) == _FAIL)
		return _FAIL;

#ifdef CONFIG_RTL8188E
	if (rtw_get_chip_type(padapter) == RTL8188E)
		rtl8188ee_set_hal_ops(padapter);
#endif

#if defined(CONFIG_RTL8812A) || defined(CONFIG_RTL8821A)
	if ((rtw_get_chip_type(padapter) == RTL8812) || (rtw_get_chip_type(padapter) == RTL8821))
		rtl8812ae_set_hal_ops(padapter);
#endif

#ifdef CONFIG_RTL8723B
	if (rtw_get_chip_type(padapter) == RTL8723B)
		rtl8723be_set_hal_ops(padapter);
#endif

#ifdef CONFIG_RTL8723D
	if (rtw_get_chip_type(padapter) == RTL8723D)
		rtl8723de_set_hal_ops(padapter);
#endif

#ifdef CONFIG_RTL8192E
	if (rtw_get_chip_type(padapter) == RTL8192E)
		rtl8192ee_set_hal_ops(padapter);
#endif

#ifdef CONFIG_RTL8192F
	if (rtw_get_chip_type(padapter) == RTL8192F)
		rtl8192fe_set_hal_ops(padapter);
#endif

#ifdef CONFIG_RTL8814A
	if (rtw_get_chip_type(padapter) == RTL8814A)
		rtl8814ae_set_hal_ops(padapter);
#endif

#if defined(CONFIG_RTL8822B)
	if (rtw_get_chip_type(padapter) == RTL8822B)
		rtl8822be_set_hal_ops(padapter);
#endif
#if defined(CONFIG_RTL8821C)
	if (rtw_get_chip_type(padapter) == RTL8821C)
		rtl8821ce_set_hal_ops(padapter);
#endif

#if defined(CONFIG_RTL8822C)
	if (rtw_get_chip_type(padapter) == RTL8822C)
		rtl8822ce_set_hal_ops(padapter);
#endif
#if defined(CONFIG_RTL8814B)
	if (rtw_get_chip_type(padapter) == RTL8814B)
		rtl8814be_set_hal_ops(padapter);
#endif

	if (rtw_hal_ops_check(padapter) == _FAIL)
		return _FAIL;

	if (hal_spec_init(padapter) == _FAIL)
		return _FAIL;

	return _SUCCESS;
}

void pci_set_intf_ops(_adapter *padapter, struct _io_ops *pops)
{
#ifdef CONFIG_RTL8188E
	if (rtw_get_chip_type(padapter) == RTL8188E)
		rtl8188ee_set_intf_ops(pops);
#endif

#if defined(CONFIG_RTL8812A) || defined(CONFIG_RTL8821A)
	if ((rtw_get_chip_type(padapter) == RTL8812) || (rtw_get_chip_type(padapter) == RTL8821))
		rtl8812ae_set_intf_ops(pops);
#endif

#ifdef CONFIG_RTL8723B
	if (rtw_get_chip_type(padapter) == RTL8723B)
		rtl8723be_set_intf_ops(pops);
#endif

#ifdef CONFIG_RTL8723D
	if (rtw_get_chip_type(padapter) == RTL8723D)
		rtl8723de_set_intf_ops(pops);
#endif

#ifdef CONFIG_RTL8192E
	if (rtw_get_chip_type(padapter) == RTL8192E)
		rtl8192ee_set_intf_ops(pops);
#endif

#ifdef CONFIG_RTL8192F
	if (rtw_get_chip_type(padapter) == RTL8192F)
		rtl8192fe_set_intf_ops(pops);
#endif

#ifdef CONFIG_RTL8814A
	if (rtw_get_chip_type(padapter) == RTL8814A)
		rtl8814ae_set_intf_ops(pops);
#endif

#if defined(CONFIG_RTL8822B)
	if (rtw_get_chip_type(padapter) == RTL8822B)
		rtl8822be_set_intf_ops(pops);
#endif

#if defined(CONFIG_RTL8821C)
	if (rtw_get_chip_type(padapter) == RTL8821C)
		rtl8821ce_set_intf_ops(pops);
#endif

#if defined(CONFIG_RTL8822C)
	if (rtw_get_chip_type(padapter) == RTL8822C)
		rtl8822ce_set_intf_ops(pops);
#endif

#if defined(CONFIG_RTL8814B)
	if (rtw_get_chip_type(padapter) == RTL8814B)
		rtl8814be_set_intf_ops(pops);
#endif

}

static void pci_intf_start(_adapter *padapter)
{
	u8 en_sw_bcn = _TRUE;

	RTW_INFO("+pci_intf_start\n");

	/* Enable hw interrupt */
	rtw_hal_enable_interrupt(padapter);
	rtw_hal_set_hwreg(padapter, HW_VAR_ENSWBCN, &en_sw_bcn);

#ifdef CONFIG_PCI_TX_POLLING
	rtw_tx_poll_init(padapter);
#endif

	RTW_INFO("-pci_intf_start\n");
}
static void rtw_mi_pci_tasklets_kill(_adapter *padapter)
{
	int i;
	_adapter *iface;
	struct dvobj_priv *dvobj = adapter_to_dvobj(padapter);

	for (i = 0; i < dvobj->iface_nums; i++) {
		iface = dvobj->padapters[i];
		if ((iface) && rtw_is_adapter_up(iface)) {
			tasklet_kill(&(padapter->recvpriv.recv_tasklet));
			tasklet_kill(&(padapter->recvpriv.irq_prepare_beacon_tasklet));
			tasklet_kill(&(padapter->xmitpriv.xmit_tasklet));
		}
	}
}

static void pci_intf_stop(_adapter *padapter)
{


	/* Disable hw interrupt */
	if (!rtw_is_surprise_removed(padapter)) {
		/* device still exists, so driver can do i/o operation */
		rtw_hal_disable_interrupt(padapter);
		rtw_mi_pci_tasklets_kill(padapter);

		rtw_hal_set_hwreg(padapter, HW_VAR_PCIE_STOP_TX_DMA, 0);

		rtw_hal_irp_reset(padapter);

	} else {
		/* Clear irq_enabled to prevent handle interrupt function. */
		adapter_to_dvobj(padapter)->irq_enabled = 0;
	}

#ifdef CONFIG_PCI_TX_POLLING
	rtw_tx_poll_timer_cancel(padapter);
#endif

}

static void disable_ht_for_spec_devid(const struct pci_device_id *pdid)
{
#ifdef CONFIG_80211N_HT
	u16 vid, pid;
	u32 flags;
	int i;
	int num = sizeof(specific_device_id_tbl) / sizeof(struct specific_device_id);

	for (i = 0; i < num; i++) {
		vid = specific_device_id_tbl[i].idVendor;
		pid = specific_device_id_tbl[i].idProduct;
		flags = specific_device_id_tbl[i].flags;

		if ((pdid->vendor == vid) && (pdid->device == pid) && (flags & SPEC_DEV_ID_DISABLE_HT)) {
			rtw_ht_enable = 0;
			rtw_bw_mode = 0;
			rtw_ampdu_enable = 0;
		}

	}
#endif
}

#ifdef CONFIG_PM
static int rtw_pci_suspend(struct pci_dev *pdev, pm_message_t state)
{
	int ret = 0;
	struct dvobj_priv *dvobj = pci_get_drvdata(pdev);
	_adapter *padapter = dvobj_get_primary_adapter(dvobj);

	ret = rtw_suspend_common(padapter);
	ret = pci_save_state(pdev);
	if (ret != 0) {
		RTW_INFO("%s Failed on pci_save_state (%d)\n", __func__, ret);
		goto exit;
	}

#ifdef CONFIG_WOWLAN
	device_set_wakeup_enable(&pdev->dev, true);
#endif
	pci_disable_device(pdev);

#ifdef CONFIG_WOWLAN
	ret = pci_enable_wake(pdev, pci_choose_state(pdev, state), true);
	if (ret != 0)
		RTW_INFO("%s Failed on pci_enable_wake (%d)\n", __func__, ret);
#endif
	ret = pci_set_power_state(pdev, pci_choose_state(pdev, state));
	if (ret != 0)
		RTW_INFO("%s Failed on pci_set_power_state (%d)\n", __func__, ret);

exit:
	return ret;

}

int rtw_resume_process(_adapter *padapter)
{
	return rtw_resume_common(padapter);
}

static int rtw_pci_resume(struct pci_dev *pdev)
{
	struct dvobj_priv *dvobj = pci_get_drvdata(pdev);
	_adapter *padapter = dvobj_get_primary_adapter(dvobj);
	struct net_device *pnetdev = padapter->pnetdev;
	struct pwrctrl_priv *pwrpriv = dvobj_to_pwrctl(dvobj);
	int	err = 0;

	err = pci_set_power_state(pdev, PCI_D0);
	if (err != 0) {
		RTW_INFO("%s Failed on pci_set_power_state (%d)\n", __func__, err);
		goto exit;
	}

	err = pci_enable_device(pdev);
	if (err != 0) {
		RTW_INFO("%s Failed on pci_enable_device (%d)\n", __func__, err);
		goto exit;
	}


#ifdef CONFIG_WOWLAN
	err =  pci_enable_wake(pdev, PCI_D0, 0);
	if (err != 0) {
		RTW_INFO("%s Failed on pci_enable_wake (%d)\n", __func__, err);
		goto exit;
	}
#endif
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 37))
	pci_restore_state(pdev);
#else
	err = pci_restore_state(pdev);
	if (err != 0) {
		RTW_INFO("%s Failed on pci_restore_state (%d)\n", __func__, err);
		goto exit;
	}
#endif

#ifdef CONFIG_WOWLAN
	device_set_wakeup_enable(&pdev->dev, false);
#endif

	if (pwrpriv->wowlan_mode || pwrpriv->wowlan_ap_mode) {
		rtw_resume_lock_suspend();
		err = rtw_resume_process(padapter);
		rtw_resume_unlock_suspend();
	} else {
#ifdef CONFIG_RESUME_IN_WORKQUEUE
		rtw_resume_in_workqueue(pwrpriv);
#else
		if (rtw_is_earlysuspend_registered(pwrpriv)) {
			/* jeff: bypass resume here, do in late_resume */
			rtw_set_do_late_resume(pwrpriv, _TRUE);
		} else {
			rtw_resume_lock_suspend();
			err = rtw_resume_process(padapter);
			rtw_resume_unlock_suspend();
		}
#endif
	}

exit:

	return err;
}
#endif/* CONFIG_PM */

_adapter *rtw_pci_primary_adapter_init(struct dvobj_priv *dvobj, struct pci_dev *pdev)
{
	_adapter *padapter = NULL;
	int status = _FAIL;

	padapter = (_adapter *)rtw_zvmalloc(sizeof(*padapter));
	if (padapter == NULL)
		goto exit;

	if (loadparam(padapter) != _SUCCESS)
		goto free_adapter;

	padapter->dvobj = dvobj;

	rtw_set_drv_stopped(padapter);/*init*/

	dvobj->padapters[dvobj->iface_nums++] = padapter;
	padapter->iface_id = IFACE_ID0;

	/* set adapter_type/iface type for primary padapter */
	padapter->isprimary = _TRUE;
	padapter->adapter_type = PRIMARY_ADAPTER;
#ifdef CONFIG_MI_WITH_MBSSID_CAM
	padapter->hw_port = HW_PORT0;
#else
	padapter->hw_port = HW_PORT0;
#endif

	if (rtw_init_io_priv(padapter, pci_set_intf_ops) == _FAIL)
		goto free_adapter;

	/* step 2.	hook HalFunc, allocate HalData */
	/* hal_set_hal_ops(padapter); */
	if (rtw_set_hal_ops(padapter) == _FAIL)
		goto free_hal_data;

	/* step 3. */
	padapter->intf_start = &pci_intf_start;
	padapter->intf_stop = &pci_intf_stop;

	/* .3 */
	rtw_hal_read_chip_version(padapter);

	/* .4 */
	rtw_hal_chip_configure(padapter);

#ifdef CONFIG_BT_COEXIST
	rtw_btcoex_Initialize(padapter);
#endif
	rtw_btcoex_wifionly_initialize(padapter);

	/* step 4. read efuse/eeprom data and get mac_addr */
	if (rtw_hal_read_chip_info(padapter) == _FAIL)
		goto free_hal_data;

	/* step 5. */
	if (rtw_init_drv_sw(padapter) == _FAIL)
		goto free_hal_data;

	if (rtw_hal_inirp_init(padapter) == _FAIL)
		goto free_hal_data;

	rtw_macaddr_cfg(adapter_mac_addr(padapter),  get_hal_mac_addr(padapter));

#ifdef CONFIG_MI_WITH_MBSSID_CAM
	rtw_mbid_camid_alloc(padapter, adapter_mac_addr(padapter));
#endif
#ifdef CONFIG_P2P
	rtw_init_wifidirect_addrs(padapter, adapter_mac_addr(padapter), adapter_mac_addr(padapter));
#endif /* CONFIG_P2P */

	rtw_hal_disable_interrupt(padapter);

	RTW_INFO("bDriverStopped:%s, bSurpriseRemoved:%s, bup:%d, hw_init_completed:%s\n"
		 , rtw_is_drv_stopped(padapter) ? "True" : "False"
		 , rtw_is_surprise_removed(padapter) ? "True" : "False"
		 , padapter->bup
		 , rtw_is_hw_init_completed(padapter) ? "True" : "False"
		);

	status = _SUCCESS;

free_hal_data:
	if (status != _SUCCESS && padapter->HalData)
		rtw_hal_free_data(padapter);

free_adapter:
	if (status != _SUCCESS && padapter) {
		#ifdef RTW_HALMAC
		rtw_halmac_deinit_adapter(dvobj);
		#endif
		rtw_vmfree((u8 *)padapter, sizeof(*padapter));
		padapter = NULL;
	}
exit:
	return padapter;
}

static void rtw_pci_primary_adapter_deinit(_adapter *padapter)
{
	struct mlme_priv *pmlmepriv = &padapter->mlmepriv;

	/*	padapter->intf_stop(padapter); */

	if (check_fwstate(pmlmepriv, WIFI_ASOC_STATE))
		rtw_disassoc_cmd(padapter, 0, RTW_CMDF_DIRECTLY);

#ifdef CONFIG_AP_MODE
	if (MLME_IS_AP(padapter) || MLME_IS_MESH(padapter)) {
		free_mlme_ap_info(padapter);
#ifdef CONFIG_HOSTAPD_MLME
		hostapd_mode_unload(padapter);
#endif
	}
#endif

	/*rtw_cancel_all_timer(padapte);*/
#ifdef CONFIG_WOWLAN
	adapter_to_pwrctl(padapter)->wowlan_mode = _FALSE;
#endif /* CONFIG_WOWLAN */
	rtw_dev_unload(padapter);

	RTW_INFO("%s, hw_init_completed=%s\n", __func__, rtw_is_hw_init_completed(padapter) ? "_TRUE" : "_FALSE");

	rtw_hal_inirp_deinit(padapter);
	rtw_free_drv_sw(padapter);

	/* TODO: use rtw_os_ndevs_deinit instead at the first stage of driver's dev deinit function */
	rtw_os_ndev_free(padapter);

#ifdef RTW_HALMAC
	rtw_halmac_deinit_adapter(adapter_to_dvobj(padapter));
#endif /* RTW_HALMAC */

	rtw_vmfree((u8 *)padapter, sizeof(_adapter));

#ifdef CONFIG_PLATFORM_RTD2880B
	RTW_INFO("wlan link down\n");
	rtd2885_wlan_netlink_sendMsg("linkdown", "8712");
#endif
}

/*
 * drv_init() - a device potentially for us
 *
 * notes: drv_init() is called when the bus driver has located a card for us to support.
 *        We accept the new device by returning 0.
*/
static int rtw_drv_init(struct pci_dev *pdev, const struct pci_device_id *pdid)
{
	int i, err = -ENODEV;

	int status = _FAIL;
	_adapter *padapter = NULL;
	struct dvobj_priv *dvobj;

	/* RTW_INFO("+rtw_drv_init\n"); */

	/* step 0. */
	disable_ht_for_spec_devid(pdid);

	/* Initialize dvobj_priv */
	dvobj = pci_dvobj_init(pdev, pdid);
	if (dvobj == NULL)
		goto exit;

	/* Initialize primary adapter */
	padapter = rtw_pci_primary_adapter_init(dvobj, pdev);
	if (padapter == NULL) {
		RTW_INFO("rtw_pci_primary_adapter_init Failed!\n");
		goto free_dvobj;
	}

	/* Initialize virtual interface */
#ifdef CONFIG_CONCURRENT_MODE
	if (padapter->registrypriv.virtual_iface_num > (CONFIG_IFACE_NUMBER - 1))
		padapter->registrypriv.virtual_iface_num = (CONFIG_IFACE_NUMBER - 1);

	for (i = 0; i < padapter->registrypriv.virtual_iface_num; i++) {
		if (rtw_drv_add_vir_if(padapter, pci_set_intf_ops) == NULL) {
			RTW_INFO("rtw_drv_add_iface failed! (%d)\n", i);
			goto free_if_vir;
		}
	}
#endif

#ifdef CONFIG_GLOBAL_UI_PID
	if (ui_pid[1] != 0) {
		RTW_INFO("ui_pid[1]:%d\n", ui_pid[1]);
		rtw_signal_process(ui_pid[1], SIGUSR2);
	}
#endif

	/* dev_alloc_name && register_netdev */
	if (rtw_os_ndevs_init(dvobj) != _SUCCESS)
		goto free_if_vir;

#ifdef CONFIG_HOSTAPD_MLME
	hostapd_mode_init(padapter);
#endif

#ifdef CONFIG_PLATFORM_RTD2880B
	RTW_INFO("wlan link up\n");
	rtd2885_wlan_netlink_sendMsg("linkup", "8712");
#endif

	/* alloc irq */
	if (pci_alloc_irq(dvobj) != _SUCCESS)
		goto os_ndevs_deinit;

	/* RTW_INFO("-871x_drv - drv_init, success!\n"); */

	status = _SUCCESS;

os_ndevs_deinit:
	if (status != _SUCCESS)
		rtw_os_ndevs_deinit(dvobj);
free_if_vir:
	if (status != _SUCCESS) {
#ifdef CONFIG_CONCURRENT_MODE
		rtw_drv_stop_vir_ifaces(dvobj);
		rtw_drv_free_vir_ifaces(dvobj);
#endif
	}

	if (status != _SUCCESS && padapter)
		rtw_pci_primary_adapter_deinit(padapter);

free_dvobj:
	if (status != _SUCCESS)
		pci_dvobj_deinit(pdev);
exit:
	return status == _SUCCESS ? 0 : -ENODEV;
}

/*
 * dev_remove() - our device is being removed
*/
/* rmmod module & unplug(SurpriseRemoved) will call r871xu_dev_remove() => how to recognize both */
static void rtw_dev_remove(struct pci_dev *pdev)
{
	struct dvobj_priv *pdvobjpriv = pci_get_drvdata(pdev);
	_adapter *padapter = dvobj_get_primary_adapter(pdvobjpriv);
	struct net_device *pnetdev = padapter->pnetdev;

	if (pdvobjpriv->processing_dev_remove == _TRUE) {
		RTW_WARN("%s-line%d: Warning! device has been removed!\n", __func__, __LINE__);
		return;
	}

	RTW_INFO("+rtw_dev_remove\n");

	pdvobjpriv->processing_dev_remove = _TRUE;

	if (unlikely(!padapter))
		return;

	/* TODO: use rtw_os_ndevs_deinit instead at the first stage of driver's dev deinit function */
	rtw_os_ndevs_unregister(pdvobjpriv);

#if 0
#ifdef RTK_DMP_PLATFORM
	rtw_clr_surprise_removed(padapter);	/* always trate as device exists*/
	/* this will let the driver to disable it's interrupt */
#else
	if (pci_drvpriv.drv_registered == _TRUE) {
		/* RTW_INFO("r871xu_dev_remove():padapter->bSurpriseRemoved == _TRUE\n"); */
		rtw_set_surprise_removed(padapter);
	}
	/*else
	{

		rtw_set_hw_init_completed(padapter, _FALSE);

	}*/
#endif
#endif

#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_ANDROID_POWER)
	rtw_unregister_early_suspend(dvobj_to_pwrctl(pdvobjpriv));
#endif

	if (GET_HAL_DATA(padapter)->bFWReady == _TRUE) {
		rtw_pm_set_ips(padapter, IPS_NONE);
		rtw_pm_set_lps(padapter, PS_MODE_ACTIVE);

		LeaveAllPowerSaveMode(padapter);
	}

	rtw_set_drv_stopped(padapter);	/*for stop thread*/
	rtw_stop_cmd_thread(padapter);
#ifdef CONFIG_CONCURRENT_MODE
	rtw_drv_stop_vir_ifaces(pdvobjpriv);
#endif

#ifdef CONFIG_BT_COEXIST
#ifdef CONFIG_BT_COEXIST_SOCKET_TRX
	if (GET_HAL_DATA(padapter)->EEPROMBluetoothCoexist)
		rtw_btcoex_close_socket(padapter);
#endif
	rtw_btcoex_HaltNotify(padapter);
#endif

	rtw_pci_dynamic_aspm_set_mode(padapter, ASPM_MODE_DEF);

	rtw_pci_primary_adapter_deinit(padapter);

#ifdef CONFIG_CONCURRENT_MODE
	rtw_drv_free_vir_ifaces(pdvobjpriv);
#endif

	pci_dvobj_deinit(pdev);

	RTW_INFO("-r871xu_dev_remove, done\n");

	return;
}

static void rtw_dev_shutdown(struct pci_dev *pdev)
{
	struct dvobj_priv *pdvobjpriv = pci_get_drvdata(pdev);
	_adapter *padapter = dvobj_get_primary_adapter(pdvobjpriv);
	struct net_device *pnetdev = padapter->pnetdev;

#ifdef CONFIG_RTL8723D
	if (IS_HARDWARE_TYPE_8723DE(padapter)) {
		u8 u1Tmp;

		u1Tmp = PlatformEFIORead1Byte(padapter, 0x75 /*REG_HCI_OPT_CTRL_8723D+1*/);
		PlatformEFIOWrite1Byte(padapter, 0x75 /*REG_HCI_OPT_CTRL_8723D+1*/, (u1Tmp|BIT0));/*Disable USB Suspend Signal*/
	}
#endif

	rtw_dev_remove(pdev);
}

static int __init rtw_drv_entry(void)
{
	int ret = 0;

	RTW_PRINT("module init start\n");
	dump_drv_version(RTW_DBGDUMP);
#ifdef BTCOEXVERSION
	RTW_PRINT(DRV_NAME" BT-Coex version = %s\n", BTCOEXVERSION);
#endif /* BTCOEXVERSION */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 24))
	/* console_suspend_enabled=0; */
#endif

	pci_drvpriv.drv_registered = _TRUE;
	rtw_suspend_lock_init();
	rtw_drv_proc_init();
	rtw_nlrtw_init();
#ifdef CONFIG_PLATFORM_CMAP_INTFS
	cmap_intfs_init();
#endif
	rtw_ndev_notifier_register();
	rtw_inetaddr_notifier_register();

	ret = pci_register_driver(&pci_drvpriv.rtw_pci_drv);

	if (ret != 0) {
		pci_drvpriv.drv_registered = _FALSE;
		rtw_suspend_lock_uninit();
		rtw_drv_proc_deinit();
		rtw_nlrtw_deinit();
#ifdef CONFIG_PLATFORM_CMAP_INTFS
		cmap_intfs_deinit();
#endif
		rtw_ndev_notifier_unregister();
		rtw_inetaddr_notifier_unregister();
		goto exit;
	}

exit:
	RTW_PRINT("module init ret=%d\n", ret);
	return ret;
}

static void __exit rtw_drv_halt(void)
{
	RTW_PRINT("module exit start\n");

	pci_drvpriv.drv_registered = _FALSE;

	pci_unregister_driver(&pci_drvpriv.rtw_pci_drv);

	rtw_suspend_lock_uninit();
	rtw_drv_proc_deinit();
	rtw_nlrtw_deinit();
#ifdef CONFIG_PLATFORM_CMAP_INTFS
	cmap_intfs_deinit();
#endif
	rtw_ndev_notifier_unregister();
	rtw_inetaddr_notifier_unregister();

	RTW_PRINT("module exit success\n");

	rtw_mstat_dump(RTW_DBGDUMP);
}


module_init(rtw_drv_entry);
module_exit(rtw_drv_halt);
