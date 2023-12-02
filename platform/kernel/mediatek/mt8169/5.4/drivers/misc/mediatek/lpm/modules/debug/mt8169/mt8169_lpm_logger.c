// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/io.h>
#include <linux/rtc.h>
#include <linux/wakeup_reason.h>
#include <linux/syscore_ops.h>

#include <lpm.h>
#include <lpm_module.h>
#include <lpm_spm_comm.h>
#include <mt8169_spm_reg.h>
#include <mt8169_pwr_ctrl.h>
#include <mt-plat/mtk_ccci_common.h>
#include <lpm_timer.h>
#include <mtk_lpm_sysfs.h>
#include "mt8169_lpm_compat.h"

#define MT8169_LOG_DEFAULT_MS		5000

#define PCM_32K_TICKS_PER_SEC		(32768)
#define PCM_TICK_TO_SEC(TICK)	(TICK / PCM_32K_TICKS_PER_SEC)

#define aee_sram_printk pr_info

enum PLAT_SPM_COND {
	PLAT_SPM_COND_MTCMOS_0 = 0,
	PLAT_SPM_COND_CG_INFRA_0,
	PLAT_SPM_COND_CG_INFRA_1,
	PLAT_SPM_COND_CG_INFRA_2,
	PLAT_SPM_COND_CG_INFRA_3,
	PLAT_SPM_COND_CG_INFRA_4,
	PLAT_SPM_COND_CG_INFRA_5,
	PLAT_SPM_COND_CG_MMSYS_0,
	PLAT_SPM_COND_CG_MMSYS_1,
	PLAT_SPM_COND_CG_MMSYS_2,
	PLAT_SPM_COND_CG_MMSYS_3,
	PLAT_SPM_COND_MAX,
};

static char *mt6781_spm_cond_cg_str[PLAT_SPM_COND_MAX] = {
	[PLAT_SPM_COND_MTCMOS_0]	= "MTCMOS_0",
	[PLAT_SPM_COND_CG_INFRA_0]	= "INFRA_0",
	[PLAT_SPM_COND_CG_INFRA_1]	= "INFRA_1",
	[PLAT_SPM_COND_CG_INFRA_2]	= "INFRA_2",
	[PLAT_SPM_COND_CG_INFRA_3]	= "INFRA_3",
	[PLAT_SPM_COND_CG_INFRA_4]	= "INFRA_4",
	[PLAT_SPM_COND_CG_INFRA_5]	= "INFRA_5",
	[PLAT_SPM_COND_CG_MMSYS_0]	= "MMSYS_0",
	[PLAT_SPM_COND_CG_MMSYS_1]	= "MMSYS_1",
	[PLAT_SPM_COND_CG_MMSYS_2]	= "MMSYS_2",
	[PLAT_SPM_COND_CG_MMSYS_3]	= "MMSYS_3",
};

const char *mt8169_wakesrc_str[32] = {
	[0] = " R12_PCM_TIMER",
	[1] = " R12_TWAM_IRQ_B",
	[2] = " R12_KP_IRQ_B",
	[3] = " R12_APWDT_EVENT_B",
	[4] = " R12_APXGPT1_EVENT_B",
	[5] = " R12_CONN2AP_SPM_WAKEUP_B",
	[6] = " R12_EINT_EVENT_B",
	[7] = " R12_CONN_WDT_IRQ_B",
	[8] = " R12_CCIF0_EVENT_B",
	[9] = " R12_LOWBATTERY_IRQ_B",
	[10] = " R12_SSPM2SPM_WAKEUP_B",
	[11] = " R12_SCP2SPM_WAKEUP_B",
	[12] = " R12_SC_ADSP2SPM_WAKEUP_B",
	[13] = " R12_PCM_WDT_WAKEUP_B",
	[14] = " R12_USB_CDSC_B",
	[15] = " R12_USB_POWERDWN_B",
	[16] = " R12_SYS_TIMER_EVENT_B",
	[17] = " R12_EINT_EVENT_SECURE_B",
	[18] = " R12_CCIF1_EVENT_B",
	[19] = " R12_UART0_IRQ_B",
	[20] = " R12_AFE_IRQ_MCU_B",
	[21] = " R12_THERM_CTRL_EVENT_B",
	[22] = " R12_SYS_CIRQ_IRQ_B",
	[23] = " R12_MD2AP_PEER_EVENT_B",
	[24] = " R12_CSYSPWREQ_B",
	[25] = " R12_NNA_WAKEUP",
	[26] = " R12_CLDMA_EVENT_B",
	[27] = " R12_SEJ_EVENT_B",
	[28] = " R12_REG_CPU_WAKEUP",
	[29] = " R12_CPU_IRQOUT",
	[30] = " R12_CPU_WFI",
	[31] = " R12_MCUSYS_IDLE",
};

#define plat_mmio_read(offset)	__raw_readl(lpm_spm_base + offset)

u64 ap_pd_count;
u64 ap_slp_duration;
u64 spm_26M_off_count;
u64 spm_26M_off_duration;
u32 before_ap_slp_duration;

#define	STATE_NUM	10
#define	STATE_NAME_SIZE	15

static struct lpm_spm_wake_status mt8169_wakesrc;

static struct lpm_log_helper mt8169_log_help = {
	.wakesrc = &mt8169_wakesrc,
	.cur = 0,
	.prev = 0,
};

static void lpm_get_spm_wakesrc_irq(void)
{
}

static int lpm_get_wakeup_status(void)
{
	struct lpm_log_helper *help = &mt8169_log_help;

	if (!help->wakesrc || !lpm_spm_base)
		return -EINVAL;

	help->wakesrc->r12 = plat_mmio_read(SPM_BK_WAKE_EVENT);
	help->wakesrc->r12_ext = plat_mmio_read(SPM_WAKEUP_STA);
	help->wakesrc->raw_sta = plat_mmio_read(SPM_WAKEUP_STA);
	help->wakesrc->raw_ext_sta = plat_mmio_read(SPM_WAKEUP_EXT_STA);
	help->wakesrc->md32pcm_wakeup_sta = plat_mmio_read(MD32PCM_WAKEUP_STA);
	help->wakesrc->md32pcm_event_sta = plat_mmio_read(MD32PCM_EVENT_STA);

	help->wakesrc->src_req = plat_mmio_read(SPM_SRC_REQ);

	/* backup of SPM_WAKEUP_MISC */
	help->wakesrc->wake_misc = plat_mmio_read(SPM_BK_WAKE_MISC);

	/* get sleep time */
	/* backup of PCM_TIMER_OUT */
	help->wakesrc->timer_out = plat_mmio_read(SPM_BK_PCM_TIMER);

	/* get other SYS and co-clock status */
	help->wakesrc->r13 = plat_mmio_read(PCM_REG13_DATA);
	help->wakesrc->idle_sta = plat_mmio_read(SUBSYS_IDLE_STA);
	help->wakesrc->req_sta0 = plat_mmio_read(SRC_REQ_STA_0);
	help->wakesrc->req_sta1 = plat_mmio_read(SRC_REQ_STA_1);
	help->wakesrc->req_sta2 = plat_mmio_read(SRC_REQ_STA_2);
	help->wakesrc->req_sta3 = plat_mmio_read(SRC_REQ_STA_3);
	help->wakesrc->req_sta4 = plat_mmio_read(SRC_REQ_STA_4);

	/* get HW CG check status */
	help->wakesrc->cg_check_sta = plat_mmio_read(SPM_CG_CHECK_STA);

	/* get debug flag for PCM execution check */
	help->wakesrc->debug_flag = plat_mmio_read(PCM_WDT_LATCH_SPARE_0);
	help->wakesrc->debug_flag1 = plat_mmio_read(PCM_WDT_LATCH_SPARE_1);

	/* get backup SW flag status */
	help->wakesrc->b_sw_flag0 = plat_mmio_read(SPM_SW_RSV_7);
	help->wakesrc->b_sw_flag1 = plat_mmio_read(SPM_SW_RSV_8);

	help->wakesrc->rt_req_sta0 = plat_mmio_read(SPM_SW_RSV_2);
	help->wakesrc->rt_req_sta1 = plat_mmio_read(SPM_SW_RSV_3);
	help->wakesrc->rt_req_sta2 = plat_mmio_read(SPM_SW_RSV_4);
	help->wakesrc->rt_req_sta3 = plat_mmio_read(SPM_SW_RSV_5);
	help->wakesrc->rt_req_sta4 = plat_mmio_read(SPM_SW_RSV_6);
	/* get ISR status */
	help->wakesrc->isr = plat_mmio_read(SPM_IRQ_STA);

	/* get SW flag status */
	help->wakesrc->sw_flag0 = plat_mmio_read(SPM_SW_FLAG_0);
	help->wakesrc->sw_flag1 = plat_mmio_read(SPM_SW_FLAG_1);

	/* get CLK SETTLE */
	help->wakesrc->clk_settle = plat_mmio_read(SPM_CLK_SETTLE);
	/* check abort */
	help->wakesrc->is_abort =
		help->wakesrc->debug_flag & DEBUG_ABORT_MASK;
	help->wakesrc->is_abort |=
		help->wakesrc->debug_flag1 & DEBUG_ABORT_MASK_1;

	help->cur += 1;

	return 0;
}

static void lpm_save_sleep_info(void)
{
#define AVOID_OVERFLOW (0xFFFFFFFF00000000)
	u32 off_26M_duration;
	u32 slp_duration;

	slp_duration = plat_mmio_read(SPM_BK_PCM_TIMER);
	if (slp_duration == before_ap_slp_duration)
		return;

	/* Save ap off counter and duration */
	if (ap_pd_count >= AVOID_OVERFLOW)
		ap_pd_count = 0;
	else
		ap_pd_count++;

	if (ap_slp_duration >= AVOID_OVERFLOW)
		ap_slp_duration = 0;
	else {
		ap_slp_duration = ap_slp_duration + slp_duration;
		before_ap_slp_duration = slp_duration;
	}

	/* Save 26M's off counter and duration */
	if (spm_26M_off_duration >= AVOID_OVERFLOW)
		spm_26M_off_duration = 0;
	else {
		off_26M_duration = plat_mmio_read(SPM_BK_VTCXO_DUR);
		if (off_26M_duration == 0)
			return;

		spm_26M_off_duration = spm_26M_off_duration +
			off_26M_duration;
	}

	if (spm_26M_off_count >= AVOID_OVERFLOW)
		spm_26M_off_count = 0;
	else
		spm_26M_off_count =
			(plat_mmio_read(SPM_VTCXO_EVENT_COUNT_STA) & 0xffff)
			+ spm_26M_off_count;
}

static void mt8169_suspend_show_detailed_wakeup_reason(
	struct lpm_spm_wake_status *wakesta)
{
}

static void dump_lp_cond(void)
{
	int i;
	u32 blkcg;

	for (i = 1 ; i < PLAT_SPM_COND_MAX ; i++) {
		blkcg = lpm_smc_spm_dbg(MT_SPM_DBG_SMC_UID_BLOCK_DETAIL, MT_LPM_SMC_ACT_GET, 0, i);
		if (blkcg != 0)
			pr_info("suspend warning: CG: %6s = 0x%08lx\n"
				, mt6781_spm_cond_cg_str[i], blkcg);
	}
}

static void mt8169_suspend_spm_rsc_req_check(
	struct lpm_spm_wake_status *wakesta)
{
#define LOG_BUF_SIZE		        256
#define IS_BLOCKED_OVER_TIMES		10
#undef AVOID_OVERFLOW
#define AVOID_OVERFLOW (0xF0000000)
	static u32 is_blocked_cnt;
	char log_buf[LOG_BUF_SIZE] = { 0 };
	int log_size = 0;
	u32 is_no_blocked = 0;
	u32 req_sta_0, req_sta_1, req_sta_4;
	u32 src_req;

	if (is_blocked_cnt >= AVOID_OVERFLOW)
		is_blocked_cnt = 0;

	/* Check if ever enter deepest System LPM */
	is_no_blocked = wakesta->debug_flag & 0x2;

	/* Check if System LPM ever is blocked over 10 times */
	if (!is_no_blocked)
		is_blocked_cnt++;
	else
		is_blocked_cnt = 0;

	if (is_blocked_cnt < IS_BLOCKED_OVER_TIMES)
		return;

	/* Show who is blocking system LPM */
	log_size += scnprintf(log_buf + log_size,
		LOG_BUF_SIZE - log_size,
		"suspend warning:(OneShot) System LPM is blocked by ");

	req_sta_0 = plat_mmio_read(SRC_REQ_STA_0);
	if (req_sta_0 & 0xFFF)
		log_size += scnprintf(log_buf + log_size,
			LOG_BUF_SIZE - log_size, "md ");

	if (req_sta_0 & (0x3F << 12))
		log_size += scnprintf(log_buf + log_size,
			LOG_BUF_SIZE - log_size, "conn ");

	if (req_sta_0 & (0x7 << 18))
		log_size += scnprintf(log_buf + log_size,
			LOG_BUF_SIZE - log_size, "nfc ");

	if (req_sta_0 & (0xF << 26))
		log_size += scnprintf(log_buf + log_size,
			LOG_BUF_SIZE - log_size, "disp ");

	req_sta_1 = plat_mmio_read(SRC_REQ_STA_1);
	if (req_sta_1 & 0x1F)
		log_size += scnprintf(log_buf + log_size,
			LOG_BUF_SIZE - log_size, "scp ");

	if (req_sta_1 & (0x1F << 5))
		log_size += scnprintf(log_buf + log_size,
			LOG_BUF_SIZE - log_size, "adsp ");

	if (req_sta_1 & (0x1F << 10))
		log_size += scnprintf(log_buf + log_size,
			LOG_BUF_SIZE - log_size, "ufs ");

	if (req_sta_1 & (0xF << 15))
		log_size += scnprintf(log_buf + log_size,
			LOG_BUF_SIZE - log_size, "gce ");

	if (req_sta_1 & (0x3FF << 21))
		log_size += scnprintf(log_buf + log_size,
			LOG_BUF_SIZE - log_size, "msdc ");

	req_sta_4 = plat_mmio_read(SRC_REQ_STA_4);
	if (req_sta_4 & 0x1F)
		log_size += scnprintf(log_buf + log_size,
			LOG_BUF_SIZE - log_size, "apu ");

	src_req = plat_mmio_read(SPM_SRC_REQ);
	if (src_req & 0x9B) {
		dump_lp_cond();
		log_size += scnprintf(log_buf + log_size,
			LOG_BUF_SIZE - log_size, "spm ");
	}

	WARN_ON(strlen(log_buf) >= LOG_BUF_SIZE);

	pr_info("[name:spm&][SPM] %s", log_buf);
}

static int lpm_show_message(int type, const char *prefix, void *data)
{
#undef LOG_BUF_SIZE
	#define LOG_BUF_SIZE		256
	#define LOG_BUF_OUT_SZ		768
	#define IS_WAKE_MISC(x)	(wakesrc->wake_misc & x)
	#define IS_LOGBUF(ptr, newstr) \
		((strlen(ptr) + strlen(newstr)) < LOG_BUF_SIZE)

	struct lpm_spm_wake_status *wakesrc = mt8169_log_help.wakesrc;
	int i;
	unsigned int spm_26M_off_pct = 0;
	char buf[LOG_BUF_SIZE] = { 0 };
	char log_buf[LOG_BUF_OUT_SZ] = { 0 };
	char *local_ptr = NULL;
	int log_size = 0;
	unsigned int wr = WR_UNKNOWN;
	const char *scenario = prefix ?: "UNKNOWN";

	if (wakesrc->is_abort != 0) {
		/* add size check for vcoredvfs */
		aee_sram_printk("SPM ABORT (%s), r13 = 0x%x, ",
			scenario, wakesrc->r13);
		log_size += scnprintf(log_buf + log_size,
			LOG_BUF_OUT_SZ - log_size,
			"[SPM] ABORT (%s), r13 = 0x%x, ",
			scenario, wakesrc->r13);

		aee_sram_printk(" debug_flag = 0x%x 0x%x",
			wakesrc->debug_flag, wakesrc->debug_flag1);
		log_size += scnprintf(log_buf + log_size,
			LOG_BUF_OUT_SZ - log_size,
			" debug_flag = 0x%x 0x%x",
			wakesrc->debug_flag, wakesrc->debug_flag1);

		aee_sram_printk(" sw_flag = 0x%x 0x%x",
			wakesrc->sw_flag0, wakesrc->sw_flag1);
		log_size += scnprintf(log_buf + log_size,
			LOG_BUF_OUT_SZ - log_size,
			" sw_flag = 0x%x 0x%x",
			wakesrc->sw_flag0, wakesrc->sw_flag1);

		aee_sram_printk(" b_sw_flag = 0x%x 0x%x",
			wakesrc->b_sw_flag0, wakesrc->b_sw_flag1);
		log_size += scnprintf(log_buf + log_size,
			LOG_BUF_OUT_SZ - log_size,
			" b_sw_flag = 0x%x 0x%x",
			wakesrc->b_sw_flag0, wakesrc->b_sw_flag1);

		wr =  WR_ABORT;
	} else {
		if (wakesrc->r12 & R12_PCM_TIMER) {
			if (wakesrc->wake_misc & WAKE_MISC_PCM_TIMER_EVENT) {
				local_ptr = " PCM_TIMER";
				if (IS_LOGBUF(buf, local_ptr))
					strncat(buf, local_ptr,
						strlen(local_ptr));
				wr = WR_PCM_TIMER;
			}
		}

		if (wakesrc->r12 & R12_TWAM_IRQ_B) {
			if (IS_WAKE_MISC(WAKE_MISC_DVFSRC_IRQ)) {
				local_ptr = " DVFSRC";
				if (IS_LOGBUF(buf, local_ptr))
					strncat(buf, local_ptr,
						strlen(local_ptr));
				wr = WR_DVFSRC;
			}
			if (IS_WAKE_MISC(WAKE_MISC_TWAM_IRQ_B)) {
				local_ptr = " TWAM";
				if (IS_LOGBUF(buf, local_ptr))
					strncat(buf, local_ptr,
						strlen(local_ptr));
				wr = WR_TWAM;
			}
			if (IS_WAKE_MISC(WAKE_MISC_PMSR_IRQ_B_SET0)) {
				local_ptr = " PMSR";
				if (IS_LOGBUF(buf, local_ptr))
					strncat(buf, local_ptr,
						strlen(local_ptr));
				wr = WR_PMSR;
			}
			if (IS_WAKE_MISC(WAKE_MISC_PMSR_IRQ_B_SET1)) {
				local_ptr = " PMSR";
				if (IS_LOGBUF(buf, local_ptr))
					strncat(buf, local_ptr,
						strlen(local_ptr));
				wr = WR_PMSR;
			}
			if (IS_WAKE_MISC(WAKE_MISC_SPM_ACK_CHK_WAKEUP_0)) {
				local_ptr = " SPM_ACK_CHK";
				if (IS_LOGBUF(buf, local_ptr))
					strncat(buf, local_ptr,
						strlen(local_ptr));
				wr = WR_SPM_ACK_CHK;
			}
			if (IS_WAKE_MISC(WAKE_MISC_SPM_ACK_CHK_WAKEUP_1)) {
				local_ptr = " SPM_ACK_CHK";
				if (IS_LOGBUF(buf, local_ptr))
					strncat(buf, local_ptr,
						strlen(local_ptr));
				wr = WR_SPM_ACK_CHK;
			}
			if (IS_WAKE_MISC(WAKE_MISC_SPM_ACK_CHK_WAKEUP_2)) {
				local_ptr = " SPM_ACK_CHK";
				if (IS_LOGBUF(buf, local_ptr))
					strncat(buf, local_ptr,
						strlen(local_ptr));
				wr = WR_SPM_ACK_CHK;
			}
			if (IS_WAKE_MISC(WAKE_MISC_SPM_ACK_CHK_WAKEUP_3)) {
				local_ptr = " SPM_ACK_CHK";
				if (IS_LOGBUF(buf, local_ptr))
					strncat(buf, local_ptr,
						strlen(local_ptr));
				wr = WR_SPM_ACK_CHK;
			}
			if (IS_WAKE_MISC(WAKE_MISC_SPM_ACK_CHK_WAKEUP_ALL)) {
				local_ptr = " SPM_ACK_CHK";
				if (IS_LOGBUF(buf, local_ptr))
					strncat(buf, local_ptr,
						strlen(local_ptr));
				wr = WR_SPM_ACK_CHK;
			}
		}
		for (i = 1; i < 32; i++) {
			if (wakesrc->r12 & (1U << i)) {
				if (IS_LOGBUF(buf, mt8169_wakesrc_str[i]))
					strncat(buf, mt8169_wakesrc_str[i],
						strlen(mt8169_wakesrc_str[i]));

				wr = WR_WAKE_SRC;
			}
		}
		WARN_ON(strlen(buf) >= LOG_BUF_SIZE);

		log_size += scnprintf(log_buf + log_size,
			LOG_BUF_OUT_SZ - log_size,
			"%s wake up by %s, timer_out = %u, r13 = 0x%x, debug_flag = 0x%x 0x%x, ",
			scenario, buf, wakesrc->timer_out, wakesrc->r13,
			wakesrc->debug_flag, wakesrc->debug_flag1);

		log_size += scnprintf(log_buf + log_size,
			LOG_BUF_OUT_SZ - log_size,
			"r12 = 0x%x, r12_ext = 0x%x, raw_sta = 0x%x 0x%x 0x%x, idle_sta = 0x%x, ",
			wakesrc->r12, wakesrc->r12_ext,
			wakesrc->raw_sta,
			wakesrc->md32pcm_wakeup_sta,
			wakesrc->md32pcm_event_sta,
			wakesrc->idle_sta);

		log_size += scnprintf(log_buf + log_size,
			  LOG_BUF_OUT_SZ - log_size,
			  "req_sta =  0x%x 0x%x 0x%x 0x%x 0x%x, cg_check_sta =0x%x, isr = 0x%x, rt_req_sta0 = 0x%x rt_req_sta1 = 0x%x rt_req_sta2 = 0x%x rt_req_sta3 = 0x%x dram_sw_con_3 = 0x%x, ",
			  wakesrc->req_sta0, wakesrc->req_sta1,
			  wakesrc->req_sta2, wakesrc->req_sta3,
			  wakesrc->req_sta4, wakesrc->cg_check_sta,
			  wakesrc->isr, wakesrc->rt_req_sta0,
			  wakesrc->rt_req_sta1, wakesrc->rt_req_sta2,
			  wakesrc->rt_req_sta3, wakesrc->rt_req_sta4);

		log_size += scnprintf(log_buf + log_size,
				LOG_BUF_OUT_SZ - log_size,
				"raw_ext_sta = 0x%x, wake_misc = 0x%x, pcm_flag = 0x%x 0x%x 0x%x 0x%x, req = 0x%x, ",
				wakesrc->raw_ext_sta,
				wakesrc->wake_misc,
				wakesrc->sw_flag0,
				wakesrc->sw_flag1, wakesrc->b_sw_flag0,
				wakesrc->b_sw_flag0,
				wakesrc->src_req);

		log_size += scnprintf(log_buf + log_size,
				LOG_BUF_OUT_SZ - log_size,
				" clk_settle = 0x%x, ", wakesrc->clk_settle);

		if (type == LPM_ISSUER_SUSPEND) {
			/* calculate 26M off percentage in suspend period */
			if (wakesrc->timer_out != 0) {
				spm_26M_off_pct =
					(100 * plat_mmio_read(SPM_BK_VTCXO_DUR))
							/ wakesrc->timer_out;
			}
			log_size += scnprintf(log_buf + log_size,
				LOG_BUF_OUT_SZ - log_size,
				"wlk_cntcv_l = 0x%x, wlk_cntcv_h = 0x%x, 26M_off_pct = %d, ",
				plat_mmio_read(SYS_TIMER_VALUE_L),
				plat_mmio_read(SYS_TIMER_VALUE_H),
				spm_26M_off_pct);
		}
	}
	WARN_ON(log_size >= LOG_BUF_OUT_SZ);

	if (type == LPM_ISSUER_SUSPEND) {
		pr_info("[name:spm&][SPM] %s", log_buf);
		mt8169_suspend_show_detailed_wakeup_reason(wakesrc);
		mt8169_suspend_spm_rsc_req_check(wakesrc);

		pr_info("[name:spm&][SPM] Suspended for %d.%03d seconds",
			PCM_TICK_TO_SEC(wakesrc->timer_out),
			PCM_TICK_TO_SEC((wakesrc->timer_out %
				PCM_32K_TICKS_PER_SEC) * 1000));
	} else
		pr_info("[name:spm&][SPM] %s", log_buf);

	return wr;
}

static struct lpm_dbg_plat_ops mt8169_dbg_ops = {
	.lpm_show_message = lpm_show_message,
	.lpm_save_sleep_info = lpm_save_sleep_info,
	.lpm_get_spm_wakesrc_irq = lpm_get_spm_wakesrc_irq,
	.lpm_get_wakeup_status = lpm_get_wakeup_status,
};

int mt8169_dbg_ops_register(void)
{
	int ret;

	ret = lpm_dbg_plat_ops_register(&mt8169_dbg_ops);

	return ret;
}
