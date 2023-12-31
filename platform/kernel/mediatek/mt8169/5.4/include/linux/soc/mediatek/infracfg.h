/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2021 MediaTek Inc.
 * Author: chun-jie.chen <chun-jie.chen@mediatek.com>
 */


#define MT8169_TOP_AXI_PROT_EN_SET			(0x2A0)
#define MT8169_TOP_AXI_PROT_EN_CLR			(0x2A4)
#define MT8169_TOP_AXI_PROT_EN_STA			(0x228)

#define MT8169_TOP_AXI_PROT_EN_1_SET			(0x2A8)
#define MT8169_TOP_AXI_PROT_EN_1_CLR			(0x2AC)
#define MT8169_TOP_AXI_PROT_EN_1_STA			(0x258)

#define MT8169_TOP_AXI_PROT_EN_2_SET			(0x2B0)
#define MT8169_TOP_AXI_PROT_EN_2_CLR			(0x2B4)
#define MT8169_TOP_AXI_PROT_EN_2_STA			(0x26C)

#define MT8169_TOP_AXI_PROT_EN_3_SET			(0x2B8)
#define MT8169_TOP_AXI_PROT_EN_3_CLR			(0x2BC)
#define MT8169_TOP_AXI_PROT_EN_3_STA			(0x2C8)

#define MT8169_NNA_PROT_SET				(0x314)
#define MT8169_NNA_PROT_CLR				(0x318)
#define MT8169_NNA_PROT_STA				(0x320)

#define MT8169_NNA_WAYEN_SET				(0xC48)
#define MT8169_NNA_WAYEN_CLR				(0xC48)
#define MT8169_NNA_WAYEN_STA				(0xC48)

#define MT8169_NNA_PROT_EXT_SET				(0xC20)
#define MT8169_NNA_PROT_EXT_CLR				(0xC24)
#define MT8169_NNA_PROT_EXT_STA				(0xC2C)



/* MFG1 */
#define MT8169_TOP_AXI_PROT_EN_1_MFG1_STEP1			(GENMASK(28, 27))
#define MT8169_TOP_AXI_PROT_EN_MFG1_STEP2			(GENMASK(22, 21))
#define MT8169_TOP_AXI_PROT_EN_MFG1_STEP3			(BIT(25))
#define MT8169_TOP_AXI_PROT_EN_1_MFG1_STEP4			(BIT(29))
/* DIS */
#define MT8169_TOP_AXI_PROT_EN_1_DIS_STEP1			(GENMASK(12, 11))
#define MT8169_TOP_AXI_PROT_EN_DIS_STEP2			(GENMASK(2, 1) | GENMASK(11, 10))
/* IMG */
#define MT8169_TOP_AXI_PROT_EN_1_IMG_STEP1			(BIT(23))
#define MT8169_TOP_AXI_PROT_EN_1_IMG_STEP2			(BIT(15))
/* IPE */
#define MT8169_TOP_AXI_PROT_EN_1_IPE_STEP1			(BIT(24))
#define MT8169_TOP_AXI_PROT_EN_1_IPE_STEP2			(BIT(16))
/* CAM */
#define MT8169_TOP_AXI_PROT_EN_1_CAM_STEP1			(GENMASK(22, 21))
#define MT8169_TOP_AXI_PROT_EN_1_CAM_STEP2			(GENMASK(14, 13))
/* VENC */
#define MT8169_TOP_AXI_PROT_EN_1_VENC_STEP1			(BIT(31))
#define MT8169_TOP_AXI_PROT_EN_1_VENC_STEP2			(BIT(19))
/* VDEC */
#define MT8169_TOP_AXI_PROT_EN_1_VDEC_STEP1			(BIT(30))
#define MT8169_TOP_AXI_PROT_EN_1_VDEC_STEP2			(BIT(17))
/* WPE */
#define MT8169_TOP_AXI_PROT_EN_2_WPE_STEP1			(BIT(17))
#define MT8169_TOP_AXI_PROT_EN_2_WPE_STEP2			(BIT(16))
/* CONN_ON */
#define MT8169_TOP_AXI_PROT_EN_1_CONN_ON_STEP1			(BIT(18))
#define MT8169_TOP_AXI_PROT_EN_CONN_ON_STEP2			(BIT(14))
#define MT8169_TOP_AXI_PROT_EN_CONN_ON_STEP3			(BIT(13))
#define MT8169_TOP_AXI_PROT_EN_CONN_ON_STEP4			(BIT(16))
/* NNA */
#define MT8169_TOP_AXI_PROT_EN_2_NNA_STEP1			(GENMASK(6, 3))
/* NNA0 */
#define MT8169_NNA_PROT_NNA0_STEP1				(GENMASK(1, 0))
#define MT8169_NNA_WAYEN_NNA0_STEP2				(BIT(5))
#define MT8169_NNA_PROT_EXT_NNA0_STEP3				(BIT(1))
/* NNA1 */
#define MT8169_NNA_PROT_NNA1_STEP1				(GENMASK(3, 2))
#define MT8169_NNA_WAYEN_NNA1_STEP2				(BIT(6))
#define MT8169_NNA_PROT_EXT_NNA1_STEP3				(BIT(2))
/* NNA2 */
#define MT8169_NNA_PROT_NNA2_STEP1				(GENMASK(5, 4))
#define MT8169_NNA_WAYEN_NNA2_STEP2				(BIT(7))
#define MT8169_NNA_PROT_EXT_NNA2_STEP3				(BIT(3))
/* ADSP_TOP */
#define MT8169_TOP_AXI_PROT_EN_3_ADSP_TOP_STEP1			(GENMASK(12, 11))
#define MT8169_TOP_AXI_PROT_EN_3_ADSP_TOP_STEP2			(GENMASK(1, 0))

#define MT8169_SMI_COMMON_CLAMP_EN_STA				(0x3C0)
#define MT8169_SMI_COMMON_CLAMP_EN_SET				(0x3C4)
#define MT8169_SMI_COMMON_CLAMP_EN_CLR				(0x3C8)

#define MT8169_SMI_COMMON_SMI_CLAMP_IMG2			(BIT(1))
#define MT8169_SMI_COMMON_SMI_CLAMP_CAM_RAWB			(BIT(3))
#define MT8169_SMI_COMMON_SMI_CLAMP_CAM_RAWA			(BIT(2))

#define MT8169_SMI_LARB11_RESET_ADDR_STEP1			(0xC)
#define MT8169_SMI_LARB11_RESET_ADDR_STEP2			(0xC)
#define MT8169_SMI_LARB16_RESET_ADDR				(0xC)
#define MT8169_SMI_LARB17_RESET_ADDR				(0xC)

#define MT8169_SMI_LARB11_RESET_STEP1				(BIT(2))
#define MT8169_SMI_LARB11_RESET_STEP2				(BIT(0))
#define MT8169_SMI_LARB16_RESET					(BIT(0))
#define MT8169_SMI_LARB17_RESET					(BIT(0))
