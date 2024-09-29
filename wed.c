// SPDX-License-Identifier: ISC
/*
 * Copyright (C) 2023 Lorenzo Bianconi <lorenzo@kernel.org>
 */

#include "mt76.h"
#include "dma.h"

void mt76_wed_release_rx_buf(struct mtk_wed_device *wed)
{
	struct mt76_dev *dev = container_of(wed, struct mt76_dev, mmio.wed);
	int i;

	for (i = 0; i < dev->rx_token_size; i++) {
		struct mt76_txwi_cache *t;

		t = mt76_rx_token_release(dev, i);
		if (!t || !t->ptr)
			continue;

		skb_free_frag(t->ptr);
		t->ptr = NULL;

		mt76_put_rxwi(dev, t);
	}

	mt76_free_pending_rxwi(dev);
}
EXPORT_SYMBOL_GPL(mt76_wed_release_rx_buf);

void mt76_wed_offload_disable(struct mtk_wed_device *wed)
{
	struct mt76_dev *dev = container_of(wed, struct mt76_dev, mmio.wed);

	spin_lock_bh(&dev->token_lock);
	dev->token_size = dev->drv->token_size;
	spin_unlock_bh(&dev->token_lock);
}
EXPORT_SYMBOL_GPL(mt76_wed_offload_disable);

void mt76_wed_reset_complete(struct mtk_wed_device *wed)
{
	struct mt76_dev *dev = container_of(wed, struct mt76_dev, mmio.wed);

	complete(&dev->mmio.wed_reset_complete);
}
EXPORT_SYMBOL_GPL(mt76_wed_reset_complete);

int mt76_wed_net_setup_tc(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
			  struct net_device *netdev, enum tc_setup_type type,
			  void *type_data)
{
	struct mt76_phy *phy = hw->priv;
	struct mtk_wed_device *wed = &phy->dev->mmio.wed;

	if (!mtk_wed_device_active(wed))
		return -EOPNOTSUPP;

	return mtk_wed_device_setup_tc(wed, netdev, type, type_data);
}
EXPORT_SYMBOL_GPL(mt76_wed_net_setup_tc);

void mt76_wed_dma_reset(struct mt76_dev *dev)
{
	struct mt76_mmio *mmio = &dev->mmio;

	if (!test_bit(MT76_STATE_WED_RESET, &dev->phy.state))
		return;

	complete(&mmio->wed_reset);

	if (!wait_for_completion_timeout(&mmio->wed_reset_complete, 3 * HZ))
		dev_err(dev->dev, "wed reset complete timeout\n");
}
EXPORT_SYMBOL_GPL(mt76_wed_dma_reset);
