// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2022, Linaro Limited */

#include <sys/mman.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "debugcc.h"

static struct gcc_mux gcc = {
	.mux = {
		.phys = 0x100000,
		.size = 0x1f0000,

		.measure = measure_gcc,

		.enable_reg = 0x3500c,
		.enable_mask = BIT(0),

		.mux_reg = 0x35f08,
		.mux_mask = 0x3ff,

		.div_reg = 0x35008,
		.div_mask = 0x7,
		.div_val = 4,
	},

	.xo_div4_reg = 0x2c008,
	.debug_ctl_reg = 0x35f24,
	.debug_status_reg = 0x35f28,
};

static struct debug_mux cam_cc = {
	.phys = 0xad00000,
	.size = 0x16000,
	.block_name = "cam",

	.measure = measure_leaf,
	.parent = &gcc.mux,
	.parent_mux_val = 0x3f,

	.enable_reg = 0x15008,
	.enable_mask = BIT(0),

	.mux_reg = 0x15100,
	.mux_mask = 0xff,

	.div_reg = 0x15004,
	.div_mask = 0x3,
	.div_val = 2,
};

static struct debug_mux cpu_cc = {
	.phys = 0x182a0000,
	.size = 0x1000,
	.block_name = "cpu",

	.measure = measure_leaf,
	.parent = &gcc.mux,
	.parent_mux_val = 0xbe,

	.enable_reg = 0x18,
	.enable_mask = BIT(0),

	.mux_reg = 0x18,
	.mux_mask = 0x7f << 4,
	.mux_shift = 4,

	.div_reg = 0x18,
	.div_mask = 0xf << 11,
	.div_shift = 11,
	.div_val = 8,
};

static struct debug_mux disp_cc = {
	.phys = 0xaf00000,
	.size = 0x20000,
	.block_name = "disp",

	.measure = measure_leaf,
	.parent = &gcc.mux,
	.parent_mux_val = 0x40,

	.enable_reg = 0x3004,
	.enable_mask = BIT(0),

	.mux_reg = 0x7000,
	.mux_mask = 0xff,

	.div_reg = 0x3000,
	.div_mask = 0x3,
	.div_val = 4,
};

static struct debug_mux gpu_cc = {
	.phys = 0x3d90000,
	.size = 0x9000,
	.block_name = "gpu",

	.measure = measure_leaf,
	.parent = &gcc.mux,
	.parent_mux_val = 0x107,

	.enable_reg = 0x1100,
	.enable_mask = BIT(0),

	.mux_reg = 0x1568,
	.mux_mask = 0xff,

	.div_reg = 0x10fc,
	.div_mask = 0x3,
	.div_val = 2,
};

static struct debug_mux mc_cc = {
	.phys = 0x90b0000,
	.size = 0x1000,
	.block_name = "mc",

	.measure = measure_mccc,
	.parent = &gcc.mux,
	.parent_mux_val = 0xab,
};

static struct debug_mux npu_cc = {
	.phys = 0x9980000,
	.size = 0x10000,
	.block_name = "npu",

	.measure = measure_leaf,
	.parent = &gcc.mux,
	.parent_mux_val = 0x11a,

	.enable_reg = 0x3008,
	.enable_mask = BIT(0),

	.mux_reg = 0x3000,
	.mux_mask = 0xff,

	.div_reg = 0x3004,
	.div_mask = 0x3,
	.div_val = 2,
};

static struct debug_mux video_cc = {
	.phys = 0xaaf0000,
	.size = 0x10000,
	.block_name = "video",

	.measure = measure_leaf,
	.parent = &gcc.mux,
	.parent_mux_val = 0x41,

	.enable_reg = 0x6004,
	.enable_mask = BIT(0),

	.mux_reg = 0x9000,
	.mux_mask = 0x3f,

	.div_reg = 0x6000,
	.div_mask = 0x7,
	.div_val = 2,
};

static struct measure_clk sm6350_clocks[] = {
	{ "l3_clk", &cpu_cc, 0x41 },
	{ "pwrcl_clk", &cpu_cc, 0x21 },
	{ "perfcl_clk", &cpu_cc, 0x25 },

	{ "gcc_aggre_ufs_phy_axi_clk", &gcc.mux, 0xe2 },
	{ "gcc_aggre_usb3_prim_axi_clk", &gcc.mux, 0xe1 },
	{ "gcc_boot_rom_ahb_clk", &gcc.mux, 0x80 },
	{ "gcc_camera_ahb_clk", &gcc.mux, 0x32 },
	{ "gcc_camera_axi_clk", &gcc.mux, 0x36 },
	{ "gcc_camera_throttle_nrt_axi_clk", &gcc.mux, 0x4a },
	{ "gcc_camera_throttle_rt_axi_clk", &gcc.mux, 0x39 },
	{ "gcc_camera_xo_clk", &gcc.mux, 0x3c },
	{ "gcc_ce1_ahb_clk", &gcc.mux, 0x92 },
	{ "gcc_ce1_axi_clk", &gcc.mux, 0x91 },
	{ "gcc_ce1_clk", &gcc.mux, 0x90 },
	{ "gcc_cfg_noc_usb3_prim_axi_clk", &gcc.mux, 0x18 },
	{ "gcc_cpuss_ahb_clk", &gcc.mux, 0xb7 },
	{ "gcc_cpuss_gnoc_clk", &gcc.mux, 0xb8 },
	{ "gcc_cpuss_rbcpr_clk", &gcc.mux, 0xb9 },
	{ "gcc_ddrss_gpu_axi_clk", &gcc.mux, 0xa5 },
	{ "gcc_disp_ahb_clk", &gcc.mux, 0x33 },
	{ "gcc_disp_axi_clk", &gcc.mux, 0x37 },
	{ "gcc_disp_cc_sleep_clk", &gcc.mux, 0x49 },
	{ "gcc_disp_cc_xo_clk", &gcc.mux, 0x48 },
	{ "gcc_disp_gpll0_clk", &gcc.mux, 0x44 },
	{ "gcc_disp_throttle_axi_clk", &gcc.mux, 0x3a },
	{ "gcc_disp_xo_clk", &gcc.mux, 0x3d },
	{ "gcc_gp1_clk", &gcc.mux, 0xc5 },
	{ "gcc_gp2_clk", &gcc.mux, 0xc6 },
	{ "gcc_gp3_clk", &gcc.mux, 0xc7 },
	{ "gcc_gpu_cfg_ahb_clk", &gcc.mux, 0x105 },
	{ "gcc_gpu_gpll0_clk", &gcc.mux, 0x10b },
	{ "gcc_gpu_gpll0_div_clk", &gcc.mux, 0x10c },
	{ "gcc_gpu_memnoc_gfx_clk", &gcc.mux, 0x108 },
	{ "gcc_gpu_snoc_dvm_gfx_clk", &gcc.mux, 0x109 },
	{ "gcc_npu_axi_clk", &gcc.mux, 0x116 },
	{ "gcc_npu_bwmon_axi_clk", &gcc.mux, 0x11c },
	{ "gcc_npu_bwmon_dma_cfg_ahb_clk", &gcc.mux, 0x11d },
	{ "gcc_npu_bwmon_dsp_cfg_ahb_clk", &gcc.mux, 0x11e },
	{ "gcc_npu_cfg_ahb_clk", &gcc.mux, 0x115 },
	{ "gcc_npu_dma_clk", &gcc.mux, 0x11b },
	{ "gcc_npu_gpll0_clk", &gcc.mux, 0x118 },
	{ "gcc_npu_gpll0_div_clk", &gcc.mux, 0x119 },
	{ "gcc_pdm2_clk", &gcc.mux, 0x7d },
	{ "gcc_pdm_ahb_clk", &gcc.mux, 0x7b },
	{ "gcc_pdm_xo4_clk", &gcc.mux, 0x7c },
	{ "gcc_prng_ahb_clk", &gcc.mux, 0x7e },
	{ "gcc_qupv3_wrap0_core_2x_clk", &gcc.mux, 0x6a },
	{ "gcc_qupv3_wrap0_core_clk", &gcc.mux, 0x69 },
	{ "gcc_qupv3_wrap0_s0_clk", &gcc.mux, 0x6b },
	{ "gcc_qupv3_wrap0_s1_clk", &gcc.mux, 0x6c },
	{ "gcc_qupv3_wrap0_s2_clk", &gcc.mux, 0x6d },
	{ "gcc_qupv3_wrap0_s3_clk", &gcc.mux, 0x6e },
	{ "gcc_qupv3_wrap0_s4_clk", &gcc.mux, 0x6f },
	{ "gcc_qupv3_wrap0_s5_clk", &gcc.mux, 0x70 },
	{ "gcc_qupv3_wrap1_core_2x_clk", &gcc.mux, 0x71 },
	{ "gcc_qupv3_wrap1_core_clk", &gcc.mux, 0x72 },
	{ "gcc_qupv3_wrap1_s0_clk", &gcc.mux, 0x75 },
	{ "gcc_qupv3_wrap1_s1_clk", &gcc.mux, 0x76 },
	{ "gcc_qupv3_wrap1_s2_clk", &gcc.mux, 0x77 },
	{ "gcc_qupv3_wrap1_s3_clk", &gcc.mux, 0x78 },
	{ "gcc_qupv3_wrap1_s4_clk", &gcc.mux, 0x79 },
	{ "gcc_qupv3_wrap1_s5_clk", &gcc.mux, 0x7a },
	{ "gcc_qupv3_wrap_0_m_ahb_clk", &gcc.mux, 0x67 },
	{ "gcc_qupv3_wrap_0_s_ahb_clk", &gcc.mux, 0x68 },
	{ "gcc_qupv3_wrap_1_m_ahb_clk", &gcc.mux, 0x73 },
	{ "gcc_qupv3_wrap_1_s_ahb_clk", &gcc.mux, 0x74 },
	{ "gcc_sdcc1_ahb_clk", &gcc.mux, 0x112 },
	{ "gcc_sdcc1_apps_clk", &gcc.mux, 0x113 },
	{ "gcc_sdcc1_ice_core_clk", &gcc.mux, 0x114 },
	{ "gcc_sdcc2_ahb_clk", &gcc.mux, 0x66 },
	{ "gcc_sdcc2_apps_clk", &gcc.mux, 0x65 },
	{ "gcc_sys_noc_cpuss_ahb_clk", &gcc.mux, 0x9 },
	{ "gcc_ufs_phy_ahb_clk", &gcc.mux, 0xc8 },
	{ "gcc_ufs_phy_axi_clk", &gcc.mux, 0xcc },
	{ "gcc_ufs_phy_ice_core_clk", &gcc.mux, 0xd1 },
	{ "gcc_ufs_phy_phy_aux_clk", &gcc.mux, 0xd2 },
	{ "gcc_ufs_phy_rx_symbol_0_clk", &gcc.mux, 0xca },
	{ "gcc_ufs_phy_rx_symbol_1_clk", &gcc.mux, 0xcb },
	{ "gcc_ufs_phy_tx_symbol_0_clk", &gcc.mux, 0xc9 },
	{ "gcc_ufs_phy_unipro_core_clk", &gcc.mux, 0xd0 },
	{ "gcc_usb30_prim_master_clk", &gcc.mux, 0x5b },
	{ "gcc_usb30_prim_mock_utmi_clk", &gcc.mux, 0x5d },
	{ "gcc_usb30_prim_sleep_clk", &gcc.mux, 0x5c },
	{ "gcc_usb3_prim_phy_aux_clk", &gcc.mux, 0x5e },
	{ "gcc_usb3_prim_phy_com_aux_clk", &gcc.mux, 0x5f },
	{ "gcc_usb3_prim_phy_pipe_clk", &gcc.mux, 0x60 },
	{ "gcc_video_ahb_clk", &gcc.mux, 0x31 },
	{ "gcc_video_axi_clk", &gcc.mux, 0x35 },
	{ "gcc_video_throttle_axi_clk", &gcc.mux, 0x38 },
	{ "gcc_video_xo_clk", &gcc.mux, 0x3b },
	{ "measure_only_cnoc_clk", &gcc.mux, 0x14 },
	{ "measure_only_ipa_2x_clk", &gcc.mux, 0xec },
	{ "measure_only_snoc_clk", &gcc.mux, 0x07 },

	{ "cam_cc_bps_ahb_clk", &cam_cc, 0x12 },
	{ "cam_cc_bps_areg_clk", &cam_cc, 0x11 },
	{ "cam_cc_bps_axi_clk", &cam_cc, 0x10 },
	{ "cam_cc_bps_clk", &cam_cc, 0xe },
	{ "cam_cc_camnoc_axi_clk", &cam_cc, 0x38 },
	{ "cam_cc_cci_0_clk", &cam_cc, 0x34 },
	{ "cam_cc_cci_1_clk", &cam_cc, 0x35 },
	/*
	 * From debugcc-lagoon.c:
	 * 0x3b measures gcc_camera_ahb_clk which is incorrect,
	 * thus use the other CBC Mux sel to measure cam_cc_core_ahb_clk.
	 */
	{ "cam_cc_core_ahb_clk", &cam_cc, 0x37 },
	{ "cam_cc_cpas_ahb_clk", &cam_cc, 0x37 },
	{ "cam_cc_csi0phytimer_clk", &cam_cc, 0x6 },
	{ "cam_cc_csi1phytimer_clk", &cam_cc, 0x8 },
	{ "cam_cc_csi2phytimer_clk", &cam_cc, 0xa },
	{ "cam_cc_csi3phytimer_clk", &cam_cc, 0xc },
	{ "cam_cc_csiphy0_clk", &cam_cc, 0x7 },
	{ "cam_cc_csiphy1_clk", &cam_cc, 0x9 },
	{ "cam_cc_csiphy2_clk", &cam_cc, 0xb },
	{ "cam_cc_csiphy3_clk", &cam_cc, 0xd },
	{ "cam_cc_icp_clk", &cam_cc, 0x32 },
	{ "cam_cc_icp_ts_clk", &cam_cc, 0x30 },
	{ "cam_cc_ife_0_axi_clk", &cam_cc, 0x1e },
	{ "cam_cc_ife_0_clk", &cam_cc, 0x18 },
	{ "cam_cc_ife_0_cphy_rx_clk", &cam_cc, 0x1d },
	{ "cam_cc_ife_0_csid_clk", &cam_cc, 0x1b },
	{ "cam_cc_ife_0_dsp_clk", &cam_cc, 0x1a },
	{ "cam_cc_ife_1_axi_clk", &cam_cc, 0x23 },
	{ "cam_cc_ife_1_clk", &cam_cc, 0x1f },
	{ "cam_cc_ife_1_cphy_rx_clk", &cam_cc, 0x22 },
	{ "cam_cc_ife_1_csid_clk", &cam_cc, 0x21 },
	{ "cam_cc_ife_1_dsp_clk", &cam_cc, 0x20 },
	{ "cam_cc_ife_2_axi_clk", &cam_cc, 0x28 },
	{ "cam_cc_ife_2_clk", &cam_cc, 0x24 },
	{ "cam_cc_ife_2_cphy_rx_clk", &cam_cc, 0x27 },
	{ "cam_cc_ife_2_csid_clk", &cam_cc, 0x26 },
	{ "cam_cc_ife_2_dsp_clk", &cam_cc, 0x25 },
	{ "cam_cc_ife_lite_clk", &cam_cc, 0x29 },
	{ "cam_cc_ife_lite_cphy_rx_clk", &cam_cc, 0x2b },
	{ "cam_cc_ife_lite_csid_clk", &cam_cc, 0x2a },
	{ "cam_cc_ipe_0_ahb_clk", &cam_cc, 0x17 },
	{ "cam_cc_ipe_0_areg_clk", &cam_cc, 0x16 },
	{ "cam_cc_ipe_0_axi_clk", &cam_cc, 0x15 },
	{ "cam_cc_ipe_0_clk", &cam_cc, 0x13 },
	{ "cam_cc_jpeg_clk", &cam_cc, 0x2c },
	{ "cam_cc_lrme_clk", &cam_cc, 0x36 },
	{ "cam_cc_mclk0_clk", &cam_cc, 0x1 },
	{ "cam_cc_mclk1_clk", &cam_cc, 0x2 },
	{ "cam_cc_mclk2_clk", &cam_cc, 0x3 },
	{ "cam_cc_mclk3_clk", &cam_cc, 0x4 },
	{ "cam_cc_mclk4_clk", &cam_cc, 0x5 },
	{ "cam_cc_soc_ahb_clk", &cam_cc, 0x3a },
	{ "cam_cc_sys_tmr_clk", &cam_cc, 0x33 },

	{ "disp_cc_mdss_ahb_clk", &disp_cc, 0x14 },
	{ "disp_cc_mdss_byte0_clk", &disp_cc, 0xc },
	{ "disp_cc_mdss_byte0_intf_clk", &disp_cc, 0xd },
	{ "disp_cc_mdss_dp_aux_clk", &disp_cc, 0x13 },
	{ "disp_cc_mdss_dp_crypto_clk", &disp_cc, 0x11 },
	{ "disp_cc_mdss_dp_link_clk", &disp_cc, 0xf },
	{ "disp_cc_mdss_dp_link_intf_clk", &disp_cc, 0x10 },
	{ "disp_cc_mdss_dp_pixel_clk", &disp_cc, 0x12 },
	{ "disp_cc_mdss_esc0_clk", &disp_cc, 0xe },
	{ "disp_cc_mdss_mdp_clk", &disp_cc, 0x8 },
	{ "disp_cc_mdss_mdp_lut_clk", &disp_cc, 0xa },
	{ "disp_cc_mdss_non_gdsc_ahb_clk", &disp_cc, 0x15 },
	{ "disp_cc_mdss_pclk0_clk", &disp_cc, 0x7 },
	{ "disp_cc_mdss_rot_clk", &disp_cc, 0x9 },
	{ "disp_cc_mdss_rscc_ahb_clk", &disp_cc, 0x17 },
	{ "disp_cc_mdss_rscc_vsync_clk", &disp_cc, 0x16 },
	{ "disp_cc_mdss_vsync_clk", &disp_cc, 0xb },
	{ "disp_cc_sleep_clk", &disp_cc, 0x1d },
	{ "disp_cc_xo_clk", &disp_cc, 0x1e },

	{ "gpu_cc_acd_ahb_clk", &gpu_cc, 0x20 },
	{ "gpu_cc_acd_cxo_clk", &gpu_cc, 0x1f },
	{ "gpu_cc_ahb_clk", &gpu_cc, 0x11 },
	{ "gpu_cc_crc_ahb_clk", &gpu_cc, 0x12 },
	{ "gpu_cc_cx_gfx3d_clk", &gpu_cc, 0x1a },
	{ "gpu_cc_cx_gfx3d_slv_clk", &gpu_cc, 0x1b },
	{ "gpu_cc_cx_gmu_clk", &gpu_cc, 0x19 },
	{ "gpu_cc_cx_snoc_dvm_clk", &gpu_cc, 0x16 },
	{ "gpu_cc_cxo_aon_clk", &gpu_cc, 0xb },
	{ "gpu_cc_cxo_clk", &gpu_cc, 0xa },
	{ "gpu_cc_gx_cxo_clk", &gpu_cc, 0xf },
	{ "gpu_cc_gx_gfx3d_clk", &gpu_cc, 0xc },
	{ "gpu_cc_gx_gmu_clk", &gpu_cc, 0x10 },
	{ "gpu_cc_gx_vsense_clk", &gpu_cc, 0xd },

	{ "npu_cc_bto_core_clk", &npu_cc, 0x19 },
	{ "npu_cc_bwmon_clk", &npu_cc, 0x18 },
	{ "npu_cc_cal_hm0_cdc_clk", &npu_cc, 0xb },
	{ "npu_cc_cal_hm0_clk", &npu_cc, 0x2 },
	{ "npu_cc_cal_hm0_perf_cnt_clk", &npu_cc, 0xd },
	{ "npu_cc_core_clk", &npu_cc, 0x4 },
	{ "npu_cc_dsp_ahbm_clk", &npu_cc, 0x1c },
	{ "npu_cc_dsp_ahbs_clk", &npu_cc, 0x1b },
	{ "npu_cc_dsp_axi_clk", &npu_cc, 0x1e },
	{ "npu_cc_noc_ahb_clk", &npu_cc, 0x13 },
	{ "npu_cc_noc_axi_clk", &npu_cc, 0x12 },
	{ "npu_cc_noc_dma_clk", &npu_cc, 0x11 },
	{ "npu_cc_rsc_xo_clk", &npu_cc, 0x1a },
	{ "npu_cc_s2p_clk", &npu_cc, 0x16 },
	{ "npu_cc_xo_clk", &npu_cc, 0x1 },

	{ "video_cc_iris_ahb_clk", &video_cc, 0x5 },
	{ "video_cc_mvs0_axi_clk", &video_cc, 0x9 },
	{ "video_cc_mvs0_core_clk", &video_cc, 0x3 },
	{ "video_cc_mvsc_core_clk", &video_cc, 0x1 },
	{ "video_cc_mvsc_ctl_axi_clk", &video_cc, 0x8 },
	{ "video_cc_sleep_clk", &video_cc, 0x7 },
	{ "video_cc_venus_ahb_clk", &video_cc, 0xc },
	{ "video_cc_xo_clk", &video_cc, 0x6 },

	{ "mccc_clk", &mc_cc, 0x50 },
	{}
};

struct debugcc_platform sm6350_debugcc = {
	"sm6350",
	sm6350_clocks,
};
