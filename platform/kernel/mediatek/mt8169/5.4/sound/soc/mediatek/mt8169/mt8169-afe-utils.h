/* SPDX-License-Identifier: GPL-2.0 */
/*
 * mt8169-afe-utils.h  --  Mediatek 8169 afe utils definition
 *
 * Copyright (c) 2021 MediaTek Inc.
 * Author: Chunxu Li <chunxu.li@mediatek.com>
 */

#ifndef _MT8169_AFE_UTILS_H_
#define _MT8169_AFE_UTILS_H_

#include <sound/pcm.h>

struct afe_hostless_pcm_config {
	unsigned int channels;
	unsigned int rate;
	unsigned int format;
	unsigned int period_size;
	unsigned int period_count;
};

int afe_hostless_pcm_open(struct snd_pcm *pcm,
			  struct snd_pcm_substream **substream, int dir);
void afe_hostless_pcm_close(struct snd_pcm_substream *substream);
int afe_hostless_pcm_params(struct snd_pcm_substream *substream,
			    unsigned int rate,
			    unsigned int channels,
			    unsigned int format,
			    unsigned int period_size,
			    unsigned int period_count);
int afe_hostless_pcm_start(struct snd_pcm_substream *substream);
int afe_hostless_pcm_stop(struct snd_pcm_substream *substream);
int afe_hostless_enable(struct snd_pcm *pcm,
			struct snd_pcm_substream **rsubstream,
			struct afe_hostless_pcm_config *config,
			int dir);
void afe_hostless_disable(struct snd_pcm_substream **rsubstream);
#endif

