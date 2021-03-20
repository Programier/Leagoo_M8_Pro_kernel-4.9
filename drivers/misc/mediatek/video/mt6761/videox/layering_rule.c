/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/module.h>
#include <linux/wait.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>

#if defined(CONFIG_MTK_DRAMC)
#include "mtk_dramc.h"
#endif
#include "layering_rule.h"
#include "disp_drv_log.h"
#include "ddp_rsz.h"
#include "primary_display.h"
#include "disp_lowpower.h"
#include "mtk_disp_mgr.h"

static struct layering_rule_ops l_rule_ops;
static struct layering_rule_info_t l_rule_info;

int emi_bound_table[HRT_BOUND_NUM][HRT_LEVEL_NUM] = {
	/* HRT_BOUND_TYPE_LP4 */
	{500, 600, 700, 700},
	/* HRT_BOUND_TYPE_LP4_PLUS */
	{400, 500, 600, 600},
	/* HRT_BOUND_TYPE_LP3 */
	{350, 350, 350, 350},
	/* HRT_BOUND_TYPE_LP3_PLUS */
	{250, 250, 250, 250},
	/* HRT_BOUND_TYPE_LP4_1CH */
	{350, 350, 350, 350},
	/* HRT_BOUND_TYPE_LP4_HYBRID */
	{400, 400, 400, 600},
	/* HRT_BOUND_TYPE_LP3_HD */
	{750, 750, 750, 750},
	/* HRT_BOUND_TYPE_LP4_HD */
	{1100, 1350, 1550, 1550},
	/* HRT_BOUND_TYPE_LP3_HD_PLUS */
	{550, 550, 550, 550},
	/* HRT_BOUND_TYPE_LP4_HD_PLUS */
	{900, 1100, 1350, 1350},
};

int larb_bound_table[HRT_BOUND_NUM][HRT_LEVEL_NUM] = {
	/* HRT_BOUND_TYPE_LP4 */
	{1200, 1200, 1200, 1200},
	/* HRT_BOUND_TYPE_LP4_PLUS */
	{1200, 1200, 1200, 1200},
	/* HRT_BOUND_TYPE_LP3 */
	{1200, 1200, 1200, 1200},
	/* HRT_BOUND_TYPE_LP3_PLUS */
	{1200, 1200, 1200, 1200},
	/* HRT_BOUND_TYPE_LP4_1CH */
	{1200, 1200, 1200, 1200},
	/* HRT_BOUND_TYPE_LP4_HYBRID */
	{1200, 1200, 1200, 1200},
	/* HRT_BOUND_TYPE_LP3_HD */
	{1200, 1200, 1200, 1200},
	/* HRT_BOUND_TYPE_LP4_HD */
	{1200, 1200, 1200, 1200},
	/* HRT_BOUND_TYPE_LP3_HD_PLUS */
	{1200, 1200, 1200, 1200},
	/* HRT_BOUND_TYPE_LP4_HD_PLUS */
	{1200, 1200, 1200, 1200},
};

int mm_freq_table[HRT_DRAMC_TYPE_NUM][HRT_OPP_LEVEL_NUM] = {
	/* HRT_DRAMC_TYPE_LP4_3733 */
	{457, 312, 228},
	/* HRT_DRAMC_TYPE_LP4_3200 */
	{457, 312, 228},
	/* HRT_DRAMC_TYPE_LP3 */
	{457, 312, 228},
};

/**
 * The layer mapping table define ovl layer dispatch rule for both
 * primary and secondary display.Each table has 16 elements which
 * represent the layer mapping rule by the number of input layers.
 */
#ifndef CONFIG_MTK_ROUND_CORNER_SUPPORT
static int layer_mapping_table[HRT_TB_NUM][TOTAL_OVL_LAYER_NUM] = {
	/* HRT_TB_TYPE_GENERAL */
	{0x00010001, 0x00030003, 0x00030007, 0x0003000F, 0x0003001F, 0x0003003F,
	0x0003003F, 0x0003003F, 0x0003003F, 0x0003003F, 0x0003003F, 0x0003003F},
	/* HRT_TB_TYPE_RPO */
	{0x00010001, 0x00030005, 0x0003000D, 0x0003001D, 0x0003003D, 0x0003003D,
	0x0003003D, 0x0003003D, 0x0003003D, 0x0003003D, 0x0003003D, 0x0003003D},
	/* HRT_TB_TYPE_RPO_DIM_L0 */
	{0x00010001, 0x00030003, 0x00030007, 0x0003000F, 0x0003001F, 0x0003003F,
	0x0003003F, 0x0003003F, 0x0003003F, 0x0003003F, 0x0003003F, 0x0003003F},
};

#else
static int layer_mapping_table[HRT_TB_NUM][TOTAL_OVL_LAYER_NUM] = {
	/* HRT_TB_TYPE_GENERAL */
	{0x00010001, 0x00030003, 0x00030007, 0x0003000F, 0x0003001F, 0x0003001F,
	0x0003001F, 0x0003001F, 0x0003001F, 0x0003001F},
	/* HRT_TB_TYPE_RPO */
	{0x00010001, 0x00030005, 0x0003000D, 0x0003001D, 0x0003001D, 0x0003001D,
	0x0003001D, 0x0003001D, 0x0003001D, 0x0003001D},
	/* HRT_TB_TYPE_RPO_DIM_L0 */
	{0x00010001, 0x00030003, 0x00030007, 0x0003000F, 0x0003001F, 0x0003001F,
	0x0003001F, 0x0003001F, 0x0003001F, 0x0003001F},
};
#endif
/**
 * The larb mapping table represent the relation between LARB and OVL.
 */
static int larb_mapping_table[HRT_TB_NUM] = {
	0x00010010, 0x00010010, 0x00010010,
};

/**
 * The OVL mapping table is used to get the OVL index of correcponding layer.
 * The bit value 1 means the position of the last layer in OVL engine.
 */
#ifndef CONFIG_MTK_ROUND_CORNER_SUPPORT
static int ovl_mapping_table[HRT_TB_NUM] = {
	0x00020022, 0x00020022, 0x00020022,
};
#else
static int ovl_mapping_table[HRT_TB_NUM] = {
	0x00020012, 0x00020012, 0x00020012,
};
#endif
#define GET_SYS_STATE(sys_state) \
	((l_rule_info.hrt_sys_state >> sys_state) & 0x1)

static bool has_rsz_layer(struct disp_layer_info *disp_info, int disp_idx)
{
	int i = 0;
	struct layer_config *c = NULL;

	for (i = 0; i < disp_info->layer_num[disp_idx]; i++) {
		c = &disp_info->input_config[disp_idx][i];

		if (is_gles_layer(disp_info, disp_idx, i))
			continue;

		if ((c->src_height != c->dst_height) ||
		    (c->src_width != c->dst_width))
			return true;
	}

	return false;
}

static bool is_RPO(struct disp_layer_info *disp_info, int disp_idx,
		   int *rsz_idx)
{
	int i = 0;
	struct layer_config *c = NULL;
	int gpu_rsz_idx = 0;

	if (disp_info->layer_num[disp_idx] <= 0)
		return false;

	for (*rsz_idx = 0; *rsz_idx < 2; (*rsz_idx)++) {
		c = &disp_info->input_config[disp_idx][*rsz_idx];

		if (*rsz_idx == 0 && c->src_fmt == DISP_FORMAT_DIM)
			continue;

		if (disp_info->gles_head[disp_idx] >= 0 &&
		    disp_info->gles_head[disp_idx] <= *rsz_idx)
			return false;
		if (c->src_width == c->dst_width &&
		    c->src_height == c->dst_height)
			return false;
		if (c->src_width > c->dst_width ||
		    c->src_height > c->dst_height)
			return false;
		/*
		 * HWC adjusts MDP layer alignment after query_valid_layer.
		 * This makes the decision of layering rule unreliable. Thus we
		 * add constraint to avoid frame_cfg becoming scale-down.
		 *
		 * TODO: If HWC adjusts MDP layer alignment before
		 * query_valid_layer, we could remove this if statement.
		 */
		if ((has_layer_cap(c, MDP_RSZ_LAYER) ||
		     has_layer_cap(c, MDP_ROT_LAYER)) &&
		    (c->dst_width - c->src_width <= MDP_ALIGNMENT_MARGIN ||
		     c->dst_height - c->src_height <= MDP_ALIGNMENT_MARGIN))
			return false;
		if (c->src_width > RSZ_TILE_LENGTH - RSZ_ALIGNMENT_MARGIN ||
		    c->src_height > RSZ_IN_MAX_HEIGHT)
			return false;

		c->layer_caps |= DISP_RSZ_LAYER;
		break;
	}
	if (*rsz_idx == 2) {
		DISPERR("%s:error: rsz layer idx >= 2\n", __func__);
		return false;
	}

	for (i = *rsz_idx + 1; i < disp_info->layer_num[disp_idx]; i++) {
		c = &disp_info->input_config[disp_idx][i];
		if (c->src_width != c->dst_width ||
		    c->src_height != c->dst_height) {
			if (has_layer_cap(c, MDP_RSZ_LAYER))
				continue;

			gpu_rsz_idx = i;
			break;
		}
	}

	if (gpu_rsz_idx)
		rollback_resize_layer_to_GPU_range(disp_info, disp_idx,
			gpu_rsz_idx, disp_info->layer_num[disp_idx] - 1);

	return true;
}

/* lr_rsz_layout - layering rule resize layer layout */
static bool lr_rsz_layout(struct disp_layer_info *disp_info)
{
	int disp_idx;

	if (is_ext_path(disp_info))
		rollback_all_resize_layer_to_GPU(disp_info, HRT_SECONDARY);

	for (disp_idx = 0; disp_idx < 2; disp_idx++) {
		int rsz_idx = 0;

		if (disp_info->layer_num[disp_idx] <= 0)
			continue;

		/* only support resize layer on Primary Display */
		if (disp_idx == HRT_SECONDARY)
			continue;

		if (!has_rsz_layer(disp_info, disp_idx)) {
			l_rule_info.scale_rate = HRT_SCALE_NONE;
			l_rule_info.disp_path = HRT_PATH_UNKNOWN;
		} else if (is_RPO(disp_info, disp_idx, &rsz_idx)) {
			if (rsz_idx == 0)
				l_rule_info.disp_path = HRT_PATH_RPO;
			else if (rsz_idx == 1)
				l_rule_info.disp_path = HRT_PATH_RPO_DIM_L0;
			else
				DISPERR("%s:RPO but rsz_idx(%d) error\n",
					__func__, rsz_idx);
		} else {
			rollback_all_resize_layer_to_GPU(disp_info,
								HRT_PRIMARY);
			l_rule_info.scale_rate = HRT_SCALE_NONE;
			l_rule_info.disp_path = HRT_PATH_UNKNOWN;
		}
	}

	return 0;
}

static bool
lr_unset_disp_rsz_attr(struct disp_layer_info *disp_info, int disp_idx)
{
	struct layer_config *lc = &disp_info->input_config[disp_idx][0];

	if (l_rule_info.disp_path == HRT_PATH_RPO &&
	    has_layer_cap(lc, MDP_RSZ_LAYER) &&
	    has_layer_cap(lc, DISP_RSZ_LAYER)) {
		lc->layer_caps &= ~DISP_RSZ_LAYER;
		l_rule_info.disp_path = HRT_PATH_GENERAL;
		l_rule_info.layer_tb_idx = HRT_TB_TYPE_GENERAL;
		return true;
	}
	return false;
}

static void lr_gpu_change_rsz_info(void)
{
}

static void layering_rule_senario_decision(struct disp_layer_info *disp_info)
{
	mmprofile_log_ex(ddp_mmp_get_events()->hrt, MMPROFILE_FLAG_START,
		l_rule_info.disp_path,
		l_rule_info.layer_tb_idx | (l_rule_info.bound_tb_idx << 16));

	if (GET_SYS_STATE(DISP_HRT_MULTI_TUI_ON)) {
		l_rule_info.disp_path = HRT_PATH_GENERAL;
		/* layer_tb_idx = HRT_TB_TYPE_MULTI_WINDOW_TUI;*/
		l_rule_info.layer_tb_idx = HRT_TB_TYPE_GENERAL;
	} else {
		if (l_rule_info.disp_path == HRT_PATH_RPO) {
			l_rule_info.layer_tb_idx = HRT_TB_TYPE_RPO;
		} else if (l_rule_info.disp_path == HRT_PATH_RPO_DIM_L0) {
			l_rule_info.layer_tb_idx = HRT_TB_TYPE_RPO_DIM_L0;
		} else {
			l_rule_info.layer_tb_idx = HRT_TB_TYPE_GENERAL;
			l_rule_info.disp_path = HRT_PATH_GENERAL;
		}
	}

	l_rule_info.primary_fps = 60;
#if defined(CONFIG_MTK_DRAMC)
	if (get_ddr_type() == TYPE_LPDDR3) {
		if (primary_display_get_width() < 800) {
			if (primary_display_get_height() < 1500)
				/* LP3 HD & HD+(<=18:9) */
				l_rule_info.bound_tb_idx =
					HRT_BOUND_TYPE_LP3_HD;
			else
				/* LP3 HD+(>18:9) */
				l_rule_info.bound_tb_idx =
					HRT_BOUND_TYPE_LP3_HD_PLUS;
		} else {
			if (primary_display_get_height() < 2200)
				/* LP3 FHD & FHD+(<=18:9) */
				l_rule_info.bound_tb_idx =
					HRT_BOUND_TYPE_LP3;
			else
				/* LP3 FHD+(>18:9) */
				l_rule_info.bound_tb_idx =
					HRT_BOUND_TYPE_LP3_PLUS;
		}

	} else {
		/* LPDDR4, LPDDR4X */
		if (primary_display_get_width() < 800) {
			if (primary_display_get_height() < 1500) {
				/* LP4 HD & HD+(<=18:9) */
				if (get_emi_ch_num() == 2)
					l_rule_info.bound_tb_idx =
						HRT_BOUND_TYPE_LP4_HD;
				else
					l_rule_info.bound_tb_idx =
						HRT_BOUND_TYPE_LP3_HD;
			} else {
				/* LP4 HD+(>18:9) */
				if (get_emi_ch_num() == 2)
					l_rule_info.bound_tb_idx =
						HRT_BOUND_TYPE_LP4_HD_PLUS;
				else
					l_rule_info.bound_tb_idx =
						HRT_BOUND_TYPE_LP3_HD_PLUS;
			}
		} else {
			if (primary_display_get_height() < 2200) {
				/* LP4 FHD & FHD+(<=18:9) */
				if (get_emi_ch_num() == 2)
					l_rule_info.bound_tb_idx =
						HRT_BOUND_TYPE_LP4;
				else
					l_rule_info.bound_tb_idx =
						HRT_BOUND_TYPE_LP4_1CH;
			} else {
				/* LP4 FHD+(>18:9) */
				if (get_emi_ch_num() == 2)
					l_rule_info.bound_tb_idx =
						HRT_BOUND_TYPE_LP4_PLUS;
				else
					l_rule_info.bound_tb_idx =
						HRT_BOUND_TYPE_LP3_PLUS;
			}
		}
	}
#endif
	mmprofile_log_ex(ddp_mmp_get_events()->hrt, MMPROFILE_FLAG_END,
		l_rule_info.disp_path,
		l_rule_info.layer_tb_idx | (l_rule_info.bound_tb_idx << 16));
}

static bool filter_by_hw_limitation(struct disp_layer_info *disp_info)
{
	bool flag = false;
	unsigned int i = 0;
	struct layer_config *info;
	unsigned int disp_idx = 0;
#if 0
	/* ovl only support 1 yuv layer */
	for (disp_idx = 0 ; disp_idx < 2 ; disp_idx++) {
		for (i = 0; i < disp_info->layer_num[disp_idx]; i++) {
			info = &(disp_info->input_config[disp_idx][i]);
			if (is_gles_layer(disp_info, disp_idx, i))
				continue;
			if (is_yuv(info->src_fmt)) {
				if (flag) {
					/* push to GPU */
					if (disp_info->gles_head[disp_idx] ==
						-1 ||
						i <
						disp_info->gles_head[disp_idx])
						disp_info->gles_head[disp_idx] =
						i;
					if (disp_info->gles_tail[disp_idx] ==
						-1 ||
						i >
						disp_info->gles_tail[disp_idx])
						disp_info->gles_tail[disp_idx] =
						i;
				} else {
					flag = true;
				}
			}
		}
	}
#else
	unsigned int layer_cnt = 0;

	disp_idx = 1;
	for (i = 0; i < disp_info->layer_num[disp_idx]; i++) {
		info = &(disp_info->input_config[disp_idx][i]);
		if (is_gles_layer(disp_info, disp_idx, i))
			continue;

		layer_cnt++;
		if (layer_cnt > SECONDARY_OVL_LAYER_NUM) {
			/* push to GPU */
			if (disp_info->gles_head[disp_idx] == -1 ||
				i < disp_info->gles_head[disp_idx])
				disp_info->gles_head[disp_idx] = i;
			if (disp_info->gles_tail[disp_idx] == -1 ||
				i > disp_info->gles_tail[disp_idx])
				disp_info->gles_tail[disp_idx] = i;

			flag = false;
		}
	}
#endif
	return flag;
}


static int get_hrt_bound(int is_larb, int hrt_level)
{
	if (is_larb)
		return larb_bound_table[l_rule_info.bound_tb_idx][hrt_level];
	else
		return emi_bound_table[l_rule_info.bound_tb_idx][hrt_level];
}

static int *get_bound_table(enum DISP_HW_MAPPING_TB_TYPE tb_type)
{
	switch (tb_type) {
	case DISP_HW_EMI_BOUND_TB:
		return emi_bound_table[l_rule_info.bound_tb_idx];
	case DISP_HW_LARB_BOUND_TB:
		return larb_bound_table[l_rule_info.bound_tb_idx];
	default:
		return NULL;
	}
}

static int get_mapping_table(enum DISP_HW_MAPPING_TB_TYPE tb_type, int param)
{
	int layer_tb_idx = l_rule_info.layer_tb_idx;

	switch (tb_type) {
	case DISP_HW_OVL_TB:
		return ovl_mapping_table[layer_tb_idx];
	case DISP_HW_LARB_TB:
		return larb_mapping_table[layer_tb_idx];
	case DISP_HW_LAYER_TB:
		if (param < MAX_PHY_OVL_CNT && param >= 0)
			return layer_mapping_table[layer_tb_idx][param];
		else
			return -1;
	default:
		return -1;
	}
}

void layering_rule_init(void)
{
	l_rule_info.primary_fps = 60;
	register_layering_rule_ops(&l_rule_ops, &l_rule_info);

	set_layering_opt(LYE_OPT_RPO,
		disp_helper_get_option(DISP_OPT_RPO));
	set_layering_opt(LYE_OPT_EXT_LAYER,
		disp_helper_get_option(DISP_OPT_OVL_EXT_LAYER));
}

int layering_rule_get_mm_freq_table(enum HRT_OPP_LEVEL opp_level)
{
	enum HRT_DRAMC_TYPE dramc_type = HRT_DRAMC_TYPE_LP4_3733;

	if (opp_level == HRT_OPP_LEVEL_DEFAULT) {
		DISPINFO("skip opp level=%d\n", opp_level);
		return 0;
	} else if (opp_level > HRT_OPP_LEVEL_DEFAULT) {
		DISPERR("unsupport opp level=%d\n", opp_level);
		return 0;
	}

#if defined(CONFIG_MTK_DRAMC)
	if (get_ddr_type() == TYPE_LPDDR3)
		dramc_type = HRT_DRAMC_TYPE_LP3;
	else {
		/* LPDDR4-3733, LPDDR4-3200 */
		if (dram_steps_freq(0) == 3600)
			dramc_type = HRT_DRAMC_TYPE_LP4_3733;
		else
			dramc_type = HRT_DRAMC_TYPE_LP4_3200;
	}
#endif
	mmprofile_log_ex(ddp_mmp_get_events()->dvfs,
		MMPROFILE_FLAG_PULSE, dramc_type, opp_level);

	return mm_freq_table[dramc_type][opp_level];
}

static struct layering_rule_ops l_rule_ops = {
	.resizing_rule = lr_rsz_layout,
	.rsz_by_gpu_info_change = lr_gpu_change_rsz_info,
	.scenario_decision = layering_rule_senario_decision,
	.get_bound_table = get_bound_table,
	.get_hrt_bound = get_hrt_bound,
	.get_mapping_table = get_mapping_table,
	.rollback_to_gpu_by_hw_limitation = filter_by_hw_limitation,
	.unset_disp_rsz_attr = lr_unset_disp_rsz_attr,
};
