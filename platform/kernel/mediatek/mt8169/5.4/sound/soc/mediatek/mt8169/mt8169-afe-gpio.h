/* SPDX-License-Identifier: GPL-2.0 */
/*
 * mt6833-afe-gpio.h  --  Mediatek 6833 afe gpio ctrl definition
 *
 * Copyright (c) 2021 MediaTek Inc.
 * Author: Jiaxin Yu <jiaxin.yu@mediatek.com>
 */

#ifndef _MT8169_AFE_GPIO_H_
#define _MT8169_AFE_GPIO_H_

struct mtk_base_afe;

int mt8169_afe_gpio_init(struct device *dev);

int mt8169_afe_gpio_request(struct device *dev, bool enable,
			    int dai, int uplink);

#endif
