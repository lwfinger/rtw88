// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* Copyright(c) 2026  Realtek Corporation
 */

#include <linux/pci.h>
#include "main.h"
#include "debug.h"
#include "fw.h"
#include "pci.h"
#include "pci_old.h"
#include "reg.h"
#include "rx.h"
#include "tx.h"

extern bool rtw_pci_disable_aspm;

static int rtw_pci_old_init_tx_ring(struct rtw_dev *rtwdev,
				    struct rtw_pci_tx_ring *tx_ring,
				    u8 desc_size, u32 len)
{
	struct pci_dev *pdev = to_pci_dev(rtwdev->dev);
	struct rtw_tx_desc_pci_old *tx_desc;
	int ring_sz = desc_size * len;
	dma_addr_t dma, next;
	u8 *head;
	int i;

	head = dma_alloc_coherent(&pdev->dev, ring_sz, &dma, GFP_KERNEL);
	if (!head) {
		rtw_err(rtwdev, "failed to allocate tx ring\n");
		return -ENOMEM;
	}

	skb_queue_head_init(&tx_ring->queue);
	tx_ring->r.head = head;
	tx_ring->r.dma = dma;
	tx_ring->r.len = len;
	tx_ring->r.desc_size = desc_size;
	tx_ring->r.wp = 0;
	tx_ring->r.rp = 0;

	tx_desc = (struct rtw_tx_desc_pci_old *)head;

	for (i = 0; i < len; i++) {
		next = dma + ((i + 1) % len) * desc_size;

		le32p_replace_bits(&tx_desc[i].w12, next,
				   RTW_TX_DESC_W12_NEXT_DESC_ADDRESS);
	}

	return 0;
}

static int rtw_pci_old_reset_rx_desc(struct rtw_dev *rtwdev,
				     struct sk_buff *skb,
				     struct rtw_pci_rx_ring *rx_ring,
				     u32 idx, u32 desc_sz)
{
	struct pci_dev *pdev = to_pci_dev(rtwdev->dev);
	struct rtw_rx_desc_pci_old *rx_desc;
	dma_addr_t dma;

	if (!skb)
		return -EINVAL;

	dma = dma_map_single(&pdev->dev, skb->data, RTK_PCI_RX_BUF_SIZE,
			     DMA_FROM_DEVICE);
	if (dma_mapping_error(&pdev->dev, dma))
		return -EBUSY;

	*((dma_addr_t *)skb->cb) = dma;

	rx_desc = (struct rtw_rx_desc_pci_old *)rx_ring->r.head;

	memset(&rx_desc[idx], 0, sizeof(*rx_desc));

	le32p_replace_bits(&rx_desc[idx].w6, dma, RTW_RX_DESC_W6_BUF_ADDR);
	le32p_replace_bits(&rx_desc[idx].w0_5.w0, RTK_PCI_RX_BUF_SIZE,
			   RTW_RX_DESC_W0_PKT_LEN);
	le32p_replace_bits(&rx_desc[idx].w0_5.w0, 1, RTW_RX_DESC_W0_OWN);

	if (idx == rx_ring->r.len - 1)
		le32p_replace_bits(&rx_desc[idx].w0_5.w0, 1,
				   RTW_RX_DESC_W0_EOR);

	return 0;
}

static void rtw_pci_old_reset_buf_desc(struct rtw_dev *rtwdev)
{
	static const u32 addr[] = {
		[RTW_TX_QUEUE_BK]	= REG_BKQ_DESA,
		[RTW_TX_QUEUE_BE]	= REG_BEQ_DESA,
		[RTW_TX_QUEUE_VI]	= REG_VIQ_DESA,
		[RTW_TX_QUEUE_VO]	= REG_VOQ_DESA,
		[RTW_TX_QUEUE_BCN]	= REG_BCNQ_DESA,
		[RTW_TX_QUEUE_MGMT]	= REG_MGQ_DESA,
		[RTW_TX_QUEUE_HI0]	= REG_HQ_DESA,
	};
	struct rtw_pci *rtwpci = (struct rtw_pci *)rtwdev->priv;
	struct rtw_pci_tx_ring *ring;
	u8 queue;

	for (queue = 0; queue <= RTW_TX_QUEUE_HI0; queue++) {
		ring = &rtwpci->tx_rings[queue];

		ring->r.rp = 0;
		ring->r.wp = 0;
		rtw_write32(rtwdev, addr[queue], ring->r.dma);
	}

	rtwpci->rx_rings[RTW_RX_QUEUE_MPDU].r.rp = 0;
	rtwpci->rx_rings[RTW_RX_QUEUE_MPDU].r.wp = 0;
	rtw_write32(rtwdev, REG_RX_DESA,
		    rtwpci->rx_rings[RTW_RX_QUEUE_MPDU].r.dma);

	/* TODO: Maybe the rest should go in the chip's mac init function. */

	rtw_write8(rtwdev, RTK_PCI_CTRL + 3, 0x77);

	rtw_write32(rtwdev, REG_INT_MIG, 0);

	rtw_write32(rtwdev, REG_MCUTST_1, 0);

	rtw_write8(rtwdev, REG_MISC_CTRL, BIT_DIS_SECOND_CCA);
}

static int rtw_pci_old_reset(struct rtw_dev *rtwdev)
{
	u8 trx_hang;

	rtw_pci_old_reset_buf_desc(rtwdev);

	/// _rtl8821ae_check_pcie_dma_hang and _rtl8821ae_reset_pcie_interface_dma might go here
	if (!rtw_read8_mask(rtwdev, REG_DBI_CTRL + 3, BIT(2))) {
		rtw_write8_set(rtwdev, REG_DBI_CTRL + 3, BIT(2));
		msleep(100);
	}

	trx_hang = rtw_read8_mask(rtwdev, REG_DBI_CTRL + 3, GENMASK(1, 0));
	if (trx_hang)
		rtw_warn(rtwdev, "%s: trx hang: %#x\n", __func__, trx_hang);

	return 0;
}

static void rtw_pci_old_flush_queue(struct rtw_dev *rtwdev, u8 pci_q, bool drop)
{
	struct rtw_pci *rtwpci = (struct rtw_pci *)rtwdev->priv;
	struct rtw_pci_tx_ring *ring = &rtwpci->tx_rings[pci_q];
	u8 i;

	/* Because the time taked by the I/O in __pci_get_hw_tx_ring_rp is a
	 * bit dynamic, it's hard to define a reasonable fixed total timeout to
	 * use read_poll_timeout* helper. Instead, we can ensure a reasonable
	 * polling times, so we just use for loop with udelay here.
	 */
	for (i = 0; i < 50; i++) {
		if (skb_queue_len(&ring->queue) == 0)
			return;

		udelay(1);
	}

	if (!drop)
		rtw_dbg(rtwdev, RTW_DBG_UNEXP,
			"timed out to flush pci tx ring[%d]\n", pci_q);
}

static void rtw_pci_old_tx_kick_off_queue(struct rtw_dev *rtwdev,
					  enum rtw_tx_queue_type queue)
{
	struct rtw_pci *rtwpci = (struct rtw_pci *)rtwdev->priv;
	u16 queue_bit;

	spin_lock_bh(&rtwpci->irq_lock);

	if (queue >= RTW_TX_QUEUE_MGMT)
		queue_bit = BIT(queue + 1);
	else
		queue_bit = BIT(queue);

	rtw_write16(rtwdev, RTK_PCI_CTRL, queue_bit);

	spin_unlock_bh(&rtwpci->irq_lock);
}

static int rtw_pci_old_tx_write_data(struct rtw_dev *rtwdev,
				     struct rtw_tx_pkt_info *pkt_info,
				     struct sk_buff *skb,
				     enum rtw_tx_queue_type queue)
{
	struct rtw_pci *rtwpci = (struct rtw_pci *)rtwdev->priv;
	struct rtw_tx_desc_pci_old *tx_desc;
	struct rtw_tx_desc tmp_tx_desc = {};
	struct rtw_pci_tx_data *tx_data;
	struct rtw_pci_tx_ring *ring;
	dma_addr_t dma;
	u32 wp;

	ring = &rtwpci->tx_rings[queue];

	wp = ring->r.wp;
	tx_desc = (struct rtw_tx_desc_pci_old *)ring->r.head;

	if (queue == RTW_TX_QUEUE_BCN)
		rtw_pci_release_rsvd_page(rtwpci, ring);
	else if (le32_get_bits(tx_desc[wp].w0_9.w0, RTW_TX_DESC_W0_OWN))
		return -ENOSPC;

	dma = dma_map_single(&rtwpci->pdev->dev, skb->data, skb->len,
			     DMA_TO_DEVICE);
	if (dma_mapping_error(&rtwpci->pdev->dev, dma))
		return -EBUSY;

	pkt_info->qsel = rtw_pci_get_tx_qsel(skb, queue);

	rtw_tx_fill_tx_desc(rtwdev, pkt_info, &tmp_tx_desc);

	/*
	 * OWN (same bit as DISQSELSEQ) can only be set when everything
	 * is ready for DMA, or bad things will happen.
	 */
	le32p_replace_bits(&tmp_tx_desc.w0, 0, RTW_TX_DESC_W0_OWN);

	memcpy(&tx_desc[wp].w0_9, &tmp_tx_desc, sizeof(tmp_tx_desc));

	le32p_replace_bits(&tx_desc[wp].w0_9.w0, 1, RTW_TX_DESC_W0_FS);
	le32p_replace_bits(&tx_desc[wp].w0_9.w7, skb->len,
			   RTW_TX_DESC_W7_TX_BUFFER_SIZE);
	le32p_replace_bits(&tx_desc[wp].w10, dma,
			   RTW_TX_DESC_W10_TX_BUFFER_ADDRESS);

	tx_data = rtw_pci_get_tx_data(skb);
	tx_data->dma = dma;
	tx_data->sn = pkt_info->sn;

	spin_lock_bh(&rtwpci->irq_lock);

	skb_queue_tail(&ring->queue, skb);

	/* The hardware must see this bit after all of the above. */
	wmb();
	le32p_replace_bits(&tx_desc[wp].w0_9.w0, 1, RTW_TX_DESC_W0_OWN);

	if (queue == RTW_TX_QUEUE_BCN)
		goto out_unlock;

	/* update write-index, and kick it off later */
	set_bit(queue, rtwpci->tx_queued);

	ring->r.wp = (ring->r.wp + 1) % ring->r.len;

out_unlock:
	spin_unlock_bh(&rtwpci->irq_lock);

	return 0;
}

static void rtw_pci_old_kick_beacon_queue(struct rtw_dev *rtwdev)
{
	rtw_write16(rtwdev, RTK_PCI_CTRL, BIT(RTW_TX_QUEUE_BCN));
}

static void rtw_pci_old_tx_isr(struct rtw_dev *rtwdev,
			       struct rtw_pci *rtwpci, u8 hw_queue)
{
	struct ieee80211_hw *hw = rtwdev->hw;
	struct rtw_tx_desc_pci_old *tx_desc;
	struct rtw_pci_tx_data *tx_data;
	struct ieee80211_tx_info *info;
	struct rtw_pci_tx_ring *ring;
	struct sk_buff *skb;
	dma_addr_t dma;

	ring = &rtwpci->tx_rings[hw_queue];

	tx_desc = (struct rtw_tx_desc_pci_old *)ring->r.head;

	while (skb_queue_len(&ring->queue)) {
		if (le32_get_bits(tx_desc[ring->r.rp].w0_9.w0,
				  RTW_TX_DESC_W0_OWN))
			return;

		skb = skb_dequeue(&ring->queue);

		tx_data = rtw_pci_get_tx_data(skb);

		dma = le32_get_bits(tx_desc[ring->r.rp].w10,
				    RTW_TX_DESC_W10_TX_BUFFER_ADDRESS);

		if (dma != tx_data->dma)
			rtw_err(rtwdev,
				"dma from tx_data %#llx != dma from tx_desc %#llx\n",
				tx_data->dma, dma);

		dma_unmap_single(&rtwpci->pdev->dev, dma, skb->len,
				 DMA_TO_DEVICE);

		ring->r.rp = (ring->r.rp + 1) % ring->r.len;

		if (ring->queue_stopped &&
		    avail_desc(ring->r.wp, ring->r.rp, ring->r.len) <= 4) {
			ieee80211_wake_queue(hw, skb_get_queue_mapping(skb));
			ring->queue_stopped = false;
		}

		info = IEEE80211_SKB_CB(skb);

		/* enqueue to wait for tx report */
		if (info->flags & IEEE80211_TX_CTL_REQ_TX_STATUS) {
			rtw_tx_report_enqueue(rtwdev, skb, tx_data->sn);
			continue;
		}

		/* always ACK for others, then they won't be marked as drop */
		if (info->flags & IEEE80211_TX_CTL_NO_ACK)
			info->flags |= IEEE80211_TX_STAT_NOACK_TRANSMITTED;
		else
			info->flags |= IEEE80211_TX_STAT_ACK;

		ieee80211_tx_info_clear_status(info);
		ieee80211_tx_status_irqsafe(hw, skb);
	}
}

static int rtw_pci_old_get_hw_rx_ring_nr(struct rtw_dev *rtwdev,
					 struct rtw_pci *rtwpci)
{
	/* TODO: is it correct? it seems to work, but should we count
	 * the number of occupied buffers for rtw_pci_napi_poll?
	 */
	return 0;
}

static u32 rtw_pci_old_rx_napi(struct rtw_dev *rtwdev,
			       struct rtw_pci *rtwpci,
			       u8 hw_queue, u32 limit)
{
	struct rtw_pci_rx_ring *ring = &rtwpci->rx_rings[RTW_RX_QUEUE_MPDU];
	struct ieee80211_rx_status rx_status;
	struct rtw_rx_desc_pci_old *rx_desc;
	struct device *dev = rtwdev->dev;
	struct rtw_rx_pkt_stat pkt_stat;
	struct sk_buff *skb, *new;
	u32 cur_rp = ring->r.rp;
	u32 count, rx_done = 0;
	dma_addr_t dma;
	u32 pkt_offset;
	u32 new_len;
	u8 *rx_buf;

	rx_desc = (struct rtw_rx_desc_pci_old *)ring->r.head;

	count = min(ring->r.len, limit);

	while (count--) {
		/* Wait for data to be filled by hardware */
		if (le32_get_bits(rx_desc[cur_rp].w0_5.w0, RTW_RX_DESC_W0_OWN))
			break;

		skb = ring->buf[cur_rp];
		dma = *((dma_addr_t *)skb->cb);
		dma_sync_single_for_cpu(dev, dma, RTK_PCI_RX_BUF_SIZE,
					DMA_FROM_DEVICE);

		rx_buf = skb->data;
		rtw_rx_query_rx_desc(rtwdev, &rx_desc[cur_rp].w0_5,
				     rx_buf, &pkt_stat, &rx_status);

		pkt_offset = pkt_stat.shift + pkt_stat.drv_info_sz;

		new_len = pkt_stat.pkt_len + pkt_offset;
		new = dev_alloc_skb(new_len);
		if (WARN_ONCE(!new, "rx routine starvation\n"))
			goto next_rp;

		skb_put_data(new, skb->data, new_len);

		if (pkt_stat.is_c2h) {
			rtw_fw_c2h_cmd_rx_irqsafe(rtwdev, pkt_offset, new);
		} else {
			/* remove phy_status */
			skb_pull(new, pkt_offset);

			rtw_rx_stats(rtwdev, pkt_stat.vif, new);
			memcpy(new->cb, &rx_status, sizeof(rx_status));
			ieee80211_rx_napi(rtwdev->hw, NULL, new, &rtwpci->napi);
			rx_done++;
		}

next_rp:
		/* new skb delivered to mac80211, re-enable original skb DMA */
		dma_sync_single_for_device(dev, dma, RTK_PCI_RX_BUF_SIZE,
					   DMA_FROM_DEVICE);

		/* Reset the descriptor */
		le32p_replace_bits(&rx_desc[cur_rp].w6, dma, RTW_RX_DESC_W6_BUF_ADDR);
		le32p_replace_bits(&rx_desc[cur_rp].w0_5.w0,
				   RTK_PCI_RX_BUF_SIZE, RTW_RX_DESC_W0_PKT_LEN);
		/* The hardware must see this bit after all of the above. */
		wmb();
		le32p_replace_bits(&rx_desc[cur_rp].w0_5.w0, 1,
				   RTW_RX_DESC_W0_OWN);

		if (cur_rp == ring->r.len - 1)
			le32p_replace_bits(&rx_desc[cur_rp].w0_5.w0, 1,
					   RTW_RX_DESC_W0_EOR);

		cur_rp = (cur_rp + 1) % ring->r.len;
	}

	ring->r.rp = cur_rp;

	return rx_done;
}

static void rtw_pci_old_clkreq_set(struct rtw_dev *rtwdev, bool enable)
{
}

static void rtw_pci_old_aspm_set(struct rtw_dev *rtwdev, bool enable)
{
	struct rtw_pci *rtwpci = (struct rtw_pci *)rtwdev->priv;
	struct pci_dev *pdev = rtwpci->pdev;

	if (rtw_pci_disable_aspm)
		return;

	if (enable) {
		pcie_capability_set_word(pdev, PCI_EXP_LNKCTL,
			PCI_EXP_LNKCTL_CCC | PCI_EXP_LNKCTL_ASPMC);

		pcie_capability_set_word(pdev, PCI_EXP_LNKCTL,
					 PCI_EXP_LNKCTL_CLKREQ_EN);
	} else {
		pcie_capability_clear_word(pdev, PCI_EXP_LNKCTL,
					   PCI_EXP_LNKCTL_CLKREQ_EN);

		pcie_capability_clear_and_set_word(pdev, PCI_EXP_LNKCTL,
			PCI_EXP_LNKCTL_CCC | PCI_EXP_LNKCTL_ASPMC,
			PCI_EXP_LNKCTL_CCC);
	}
}

const struct rtw_pci_gen rtw_pci_gen_old = {
	.init_tx_ring = rtw_pci_old_init_tx_ring,
	.reset_rx_desc = rtw_pci_old_reset_rx_desc,
	.reset = rtw_pci_old_reset,
	.flush_queue = rtw_pci_old_flush_queue,
	.tx_kick_off_queue = rtw_pci_old_tx_kick_off_queue,
	.tx_write_data = rtw_pci_old_tx_write_data,
	.kick_beacon_queue = rtw_pci_old_kick_beacon_queue,
	.tx_isr = rtw_pci_old_tx_isr,
	.get_hw_rx_ring_nr = rtw_pci_old_get_hw_rx_ring_nr,
	.rx_napi = rtw_pci_old_rx_napi,
	.clkreq_set = rtw_pci_old_clkreq_set,
	.aspm_set = rtw_pci_old_aspm_set,

	.irq_mask = { IMR_HIGHDOK | IMR_MGNTDOK | IMR_BKDOK |
		      IMR_BEDOK | IMR_VIDOK | IMR_VODOK | IMR_ROK |
		      IMR_BCNDMAINT_E | IMR_C2HCMD | IMR_RDU,
		      IMR_TXFOVW | IMR_RXFOVW,
		      0,
		      0, }
};
EXPORT_SYMBOL(rtw_pci_gen_old);
