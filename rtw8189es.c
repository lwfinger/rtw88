// SPDX-License-Identifier: GPL-2.0-only
/* Copyright(c) 2026 Igor Listopad <dev@listopad.email> */

#include <linux/mmc/sdio_func.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/module.h>
#include "main.h"
#include "rtw8188e.h"
#include "sdio.h"

static const struct sdio_device_id rtw_8189es_id_table[] = {
	{
		SDIO_DEVICE(SDIO_VENDOR_ID_REALTEK, 0x8179),
		.driver_data = (kernel_ulong_t)&rtw8188e_hw_spec,
	},
	{}
};
MODULE_DEVICE_TABLE(sdio, rtw_8189es_id_table);

static struct sdio_driver rtw_8189es_driver = {
	.name = KBUILD_MODNAME,
	.id_table = rtw_8189es_id_table,
	.probe = rtw_sdio_probe,
	.remove = rtw_sdio_remove,
	.drv = {
		.pm = &rtw_sdio_pm_ops,
		.shutdown = rtw_sdio_shutdown,
	}
};
module_sdio_driver(rtw_8189es_driver);

MODULE_AUTHOR("Igor Listopad <dev@listopad.email>");
MODULE_DESCRIPTION("Realtek 802.11n wireless 8189es driver");
MODULE_LICENSE("GPL");
