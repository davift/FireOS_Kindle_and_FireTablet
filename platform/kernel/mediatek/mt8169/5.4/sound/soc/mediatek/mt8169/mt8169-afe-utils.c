// SPDX-License-Identifier: GPL-2.0
/*
 *  Mediatek ALSA SoC afe utility driver for 8169
 *
 *  Copyright (c) 2021 MediaTek Inc.
 *  Author: Chunxu Li <chunxu.li@mediatek.com>
 */

#include "mt8169-afe-utils.h"

static void hostless_pcm_param_set_mask(struct snd_pcm_hw_params *p,
					int n, unsigned int bit)
{
	if (bit >= SNDRV_MASK_MAX)
		return;
	if (hw_is_mask(n)) {
		struct snd_mask *m = hw_param_mask(p, n);

		m->bits[0] = 0;
		m->bits[1] = 0;
		m->bits[bit >> 5] |= (1 << (bit & 31));
	}
}

static void hostless_pcm_param_set_min(struct snd_pcm_hw_params *p,
				       int n, unsigned int val)
{
	if (hw_is_interval(n)) {
		struct snd_interval *i = hw_param_interval(p, n);

		i->min = val;
	}
}

static void hostless_pcm_param_set_int(struct snd_pcm_hw_params *p,
				       int n, unsigned int val)
{
	if (hw_is_interval(n)) {
		struct snd_interval *i = hw_param_interval(p, n);

		i->min = val;
		i->max = val;
		i->integer = 1;
	}
}

int afe_hostless_pcm_open(struct snd_pcm *pcm,
			  struct snd_pcm_substream **substream, int dir)
{
	struct file dummy_file;
	int err;

	dummy_file.f_flags = O_RDWR;

	err = snd_pcm_open_substream(pcm, dir, &dummy_file, substream);
	if (err < 0) {
		pr_info("%s hostless pcm open fail err:%d\n",
			__func__, err);
		return err;
	}
	return err;
}
EXPORT_SYMBOL_GPL(afe_hostless_pcm_open);

void afe_hostless_pcm_close(struct snd_pcm_substream *substream)
{
	snd_pcm_release_substream(substream);
}
EXPORT_SYMBOL_GPL(afe_hostless_pcm_close);

int afe_hostless_pcm_params(struct snd_pcm_substream *substream,
			    unsigned int rate,
			    unsigned int channels,
			    unsigned int format,
			    unsigned int period_size,
			    unsigned int period_count)
{
	struct snd_pcm_hw_params hw_params;
	struct snd_pcm_sw_params sw_params;
	int err;

	_snd_pcm_hw_params_any(&hw_params);

	if (format == 32)
		hostless_pcm_param_set_mask(&hw_params,
					    SNDRV_PCM_HW_PARAM_FORMAT,
					    SNDRV_PCM_FORMAT_S32_LE);
	else
		hostless_pcm_param_set_mask(&hw_params,
					    SNDRV_PCM_HW_PARAM_FORMAT,
					    SNDRV_PCM_FORMAT_S16_LE);

	hostless_pcm_param_set_mask(&hw_params,
				    SNDRV_PCM_HW_PARAM_SUBFORMAT,
				    SNDRV_PCM_SUBFORMAT_STD);
	hostless_pcm_param_set_min(&hw_params,
				   SNDRV_PCM_HW_PARAM_PERIOD_SIZE,
				   period_size);
	hostless_pcm_param_set_int(&hw_params,
				   SNDRV_PCM_HW_PARAM_SAMPLE_BITS,
				   format);
	hostless_pcm_param_set_int(&hw_params,
				   SNDRV_PCM_HW_PARAM_FRAME_BITS,
				   format * channels);
	hostless_pcm_param_set_int(&hw_params,
				   SNDRV_PCM_HW_PARAM_CHANNELS,
				   channels);
	hostless_pcm_param_set_int(&hw_params,
				   SNDRV_PCM_HW_PARAM_PERIODS,
				   period_count);
	hostless_pcm_param_set_int(&hw_params,
				   SNDRV_PCM_HW_PARAM_RATE,
				   rate);
	hostless_pcm_param_set_mask(&hw_params,
				    SNDRV_PCM_HW_PARAM_ACCESS,
				    SNDRV_PCM_ACCESS_RW_INTERLEAVED);

	err = snd_pcm_kernel_ioctl(substream,
				   SNDRV_PCM_IOCTL_HW_PARAMS,
				   (void *)&hw_params);
	if (err < 0) {
		pr_info("%s hostless pcm hw params fail err:%d\n",
			__func__, err);
		return err;
	}

	memset(&sw_params, 0, sizeof(sw_params));
	sw_params.start_threshold = 1;
	sw_params.stop_threshold = period_size * period_count * 10;
	sw_params.avail_min = 1;
	sw_params.xfer_align = period_size / 2;
	sw_params.silence_threshold = 0;
	sw_params.silence_size = 0;
	sw_params.boundary = period_size * period_count;
	err = snd_pcm_kernel_ioctl(substream,
				   SNDRV_PCM_IOCTL_SW_PARAMS,
				   (void *)&sw_params);
	if (err < 0) {
		pr_info("%s hostless pcm sw params fail err:%d\n",
			__func__, err);
		return err;
	}

	return err;
}
EXPORT_SYMBOL_GPL(afe_hostless_pcm_params);

int afe_hostless_pcm_start(struct snd_pcm_substream *substream)
{
	int err;

	err = snd_pcm_kernel_ioctl(substream, SNDRV_PCM_IOCTL_PREPARE, NULL);
	if (err < 0) {
		pr_info("%s hostless pcm prepare fail err:%d\n",
			__func__, err);
		return err;
	}

	err = snd_pcm_kernel_ioctl(substream, SNDRV_PCM_IOCTL_START, NULL);
	if (err < 0) {
		pr_info("%s hostless pcm start fail err:%d\n",
			__func__, err);
		return err;
	}

	return err;
}
EXPORT_SYMBOL_GPL(afe_hostless_pcm_start);

int afe_hostless_pcm_stop(struct snd_pcm_substream *substream)
{
	int err;

	err = snd_pcm_kernel_ioctl(substream, SNDRV_PCM_IOCTL_DROP, NULL);
	if (err < 0) {
		pr_info("%s hostless pcm stop fail err:%d\n",
			__func__, err);
		return err;
	}

	return err;
}
EXPORT_SYMBOL_GPL(afe_hostless_pcm_stop);

int afe_hostless_enable(struct snd_pcm *pcm,
			struct snd_pcm_substream **rsubstream,
			struct afe_hostless_pcm_config *config,
			int dir)
{
	int err;
	struct snd_pcm_substream *substream;

	err = afe_hostless_pcm_open(pcm, &substream, dir);
	if (err < 0)
		return err;

	err = afe_hostless_pcm_params(substream,
				      config->rate,
				      config->channels,
				      config->format,
				      config->period_size,
				      config->period_count);
	if (err < 0) {
		afe_hostless_pcm_close(substream);
		return err;
	}
	err = afe_hostless_pcm_start(substream);
	if (err < 0) {
		afe_hostless_pcm_close(substream);
		return err;
	}
	*rsubstream = substream;

	return err;
}
EXPORT_SYMBOL_GPL(afe_hostless_enable);

void afe_hostless_disable(struct snd_pcm_substream **rsubstream)
{
	struct snd_pcm_substream *substream = *rsubstream;

	if (substream != NULL) {
		afe_hostless_pcm_stop(substream);
		afe_hostless_pcm_close(substream);
		substream = NULL;
	}
}
EXPORT_SYMBOL_GPL(afe_hostless_disable);

