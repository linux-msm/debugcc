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

static struct measure_clk sm6350_clocks[] = {
	//{ "cam_cc_debug_mux", &gcc.mux, 0x3f },
	//{ "npu_cc_debug_mux", &gcc.mux, 0x11a },
	//{ "video_cc_debug_mux", &gcc.mux, 0x41 },

	{ .name = "l3_clk", .clk_mux = &cpu_cc, .mux = 0x41 },
	{ .name = "pwrcl_clk", .clk_mux = &cpu_cc, .mux = 0x21 },
	{ .name = "perfcl_clk", .clk_mux = &cpu_cc, .mux = 0x25 },

	{ .name = "gcc_aggre_ufs_phy_axi_clk", .clk_mux = &gcc.mux, .mux = 0xe2 },
	{ .name = "gcc_aggre_usb3_prim_axi_clk", .clk_mux = &gcc.mux, .mux = 0xe1 },
	{ .name = "gcc_boot_rom_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x80 },
	{ .name = "gcc_camera_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x32 },
	{ .name = "gcc_camera_axi_clk", .clk_mux = &gcc.mux, .mux = 0x36 },
	{ .name = "gcc_camera_throttle_nrt_axi_clk", .clk_mux = &gcc.mux, .mux = 0x4a },
	{ .name = "gcc_camera_throttle_rt_axi_clk", .clk_mux = &gcc.mux, .mux = 0x39 },
	{ .name = "gcc_camera_xo_clk", .clk_mux = &gcc.mux, .mux = 0x3c },
	{ .name = "gcc_ce1_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x92 },
	{ .name = "gcc_ce1_axi_clk", .clk_mux = &gcc.mux, .mux = 0x91 },
	{ .name = "gcc_ce1_clk", .clk_mux = &gcc.mux, .mux = 0x90 },
	{ .name = "gcc_cfg_noc_usb3_prim_axi_clk", .clk_mux = &gcc.mux, .mux = 0x18 },
	{ .name = "gcc_cpuss_ahb_clk", .clk_mux = &gcc.mux, .mux = 0xb7 },
	{ .name = "gcc_cpuss_gnoc_clk", .clk_mux = &gcc.mux, .mux = 0xb8 },
	{ .name = "gcc_cpuss_rbcpr_clk", .clk_mux = &gcc.mux, .mux = 0xb9 },
	{ .name = "gcc_ddrss_gpu_axi_clk", .clk_mux = &gcc.mux, .mux = 0xa5 },
	{ .name = "gcc_disp_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x33 },
	{ .name = "gcc_disp_axi_clk", .clk_mux = &gcc.mux, .mux = 0x37 },
	{ .name = "gcc_disp_cc_sleep_clk", .clk_mux = &gcc.mux, .mux = 0x49 },
	{ .name = "gcc_disp_cc_xo_clk", .clk_mux = &gcc.mux, .mux = 0x48 },
	{ .name = "gcc_disp_gpll0_clk", .clk_mux = &gcc.mux, .mux = 0x44 },
	{ .name = "gcc_disp_throttle_axi_clk", .clk_mux = &gcc.mux, .mux = 0x3a },
	{ .name = "gcc_disp_xo_clk", .clk_mux = &gcc.mux, .mux = 0x3d },
	{ .name = "gcc_gp1_clk", .clk_mux = &gcc.mux, .mux = 0xc5 },
	{ .name = "gcc_gp2_clk", .clk_mux = &gcc.mux, .mux = 0xc6 },
	{ .name = "gcc_gp3_clk", .clk_mux = &gcc.mux, .mux = 0xc7 },
	{ .name = "gcc_gpu_cfg_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x105 },
	{ .name = "gcc_gpu_gpll0_clk", .clk_mux = &gcc.mux, .mux = 0x10b },
	{ .name = "gcc_gpu_gpll0_div_clk", .clk_mux = &gcc.mux, .mux = 0x10c },
	{ .name = "gcc_gpu_memnoc_gfx_clk", .clk_mux = &gcc.mux, .mux = 0x108 },
	{ .name = "gcc_gpu_snoc_dvm_gfx_clk", .clk_mux = &gcc.mux, .mux = 0x109 },
	{ .name = "gcc_npu_axi_clk", .clk_mux = &gcc.mux, .mux = 0x116 },
	{ .name = "gcc_npu_bwmon_axi_clk", .clk_mux = &gcc.mux, .mux = 0x11c },
	{ .name = "gcc_npu_bwmon_dma_cfg_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x11d },
	{ .name = "gcc_npu_bwmon_dsp_cfg_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x11e },
	{ .name = "gcc_npu_cfg_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x115 },
	{ .name = "gcc_npu_dma_clk", .clk_mux = &gcc.mux, .mux = 0x11b },
	{ .name = "gcc_npu_gpll0_clk", .clk_mux = &gcc.mux, .mux = 0x118 },
	{ .name = "gcc_npu_gpll0_div_clk", .clk_mux = &gcc.mux, .mux = 0x119 },
	{ .name = "gcc_pdm2_clk", .clk_mux = &gcc.mux, .mux = 0x7d },
	{ .name = "gcc_pdm_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x7b },
	{ .name = "gcc_pdm_xo4_clk", .clk_mux = &gcc.mux, .mux = 0x7c },
	{ .name = "gcc_prng_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x7e },
	{ .name = "gcc_qupv3_wrap0_core_2x_clk", .clk_mux = &gcc.mux, .mux = 0x6a },
	{ .name = "gcc_qupv3_wrap0_core_clk", .clk_mux = &gcc.mux, .mux = 0x69 },
	{ .name = "gcc_qupv3_wrap0_s0_clk", .clk_mux = &gcc.mux, .mux = 0x6b },
	{ .name = "gcc_qupv3_wrap0_s1_clk", .clk_mux = &gcc.mux, .mux = 0x6c },
	{ .name = "gcc_qupv3_wrap0_s2_clk", .clk_mux = &gcc.mux, .mux = 0x6d },
	{ .name = "gcc_qupv3_wrap0_s3_clk", .clk_mux = &gcc.mux, .mux = 0x6e },
	{ .name = "gcc_qupv3_wrap0_s4_clk", .clk_mux = &gcc.mux, .mux = 0x6f },
	{ .name = "gcc_qupv3_wrap0_s5_clk", .clk_mux = &gcc.mux, .mux = 0x70 },
	{ .name = "gcc_qupv3_wrap1_core_2x_clk", .clk_mux = &gcc.mux, .mux = 0x71 },
	{ .name = "gcc_qupv3_wrap1_core_clk", .clk_mux = &gcc.mux, .mux = 0x72 },
	{ .name = "gcc_qupv3_wrap1_s0_clk", .clk_mux = &gcc.mux, .mux = 0x75 },
	{ .name = "gcc_qupv3_wrap1_s1_clk", .clk_mux = &gcc.mux, .mux = 0x76 },
	{ .name = "gcc_qupv3_wrap1_s2_clk", .clk_mux = &gcc.mux, .mux = 0x77 },
	{ .name = "gcc_qupv3_wrap1_s3_clk", .clk_mux = &gcc.mux, .mux = 0x78 },
	{ .name = "gcc_qupv3_wrap1_s4_clk", .clk_mux = &gcc.mux, .mux = 0x79 },
	{ .name = "gcc_qupv3_wrap1_s5_clk", .clk_mux = &gcc.mux, .mux = 0x7a },
	{ .name = "gcc_qupv3_wrap_0_m_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x67 },
	{ .name = "gcc_qupv3_wrap_0_s_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x68 },
	{ .name = "gcc_qupv3_wrap_1_m_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x73 },
	{ .name = "gcc_qupv3_wrap_1_s_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x74 },
	{ .name = "gcc_sdcc1_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x112 },
	{ .name = "gcc_sdcc1_apps_clk", .clk_mux = &gcc.mux, .mux = 0x113 },
	{ .name = "gcc_sdcc1_ice_core_clk", .clk_mux = &gcc.mux, .mux = 0x114 },
	{ .name = "gcc_sdcc2_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x66 },
	{ .name = "gcc_sdcc2_apps_clk", .clk_mux = &gcc.mux, .mux = 0x65 },
	{ .name = "gcc_sys_noc_cpuss_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x9 },
	{ .name = "gcc_ufs_phy_ahb_clk", .clk_mux = &gcc.mux, .mux = 0xc8 },
	{ .name = "gcc_ufs_phy_axi_clk", .clk_mux = &gcc.mux, .mux = 0xcc },
	{ .name = "gcc_ufs_phy_ice_core_clk", .clk_mux = &gcc.mux, .mux = 0xd1 },
	{ .name = "gcc_ufs_phy_phy_aux_clk", .clk_mux = &gcc.mux, .mux = 0xd2 },
	{ .name = "gcc_ufs_phy_rx_symbol_0_clk", .clk_mux = &gcc.mux, .mux = 0xca },
	{ .name = "gcc_ufs_phy_rx_symbol_1_clk", .clk_mux = &gcc.mux, .mux = 0xcb },
	{ .name = "gcc_ufs_phy_tx_symbol_0_clk", .clk_mux = &gcc.mux, .mux = 0xc9 },
	{ .name = "gcc_ufs_phy_unipro_core_clk", .clk_mux = &gcc.mux, .mux = 0xd0 },
	{ .name = "gcc_usb30_prim_master_clk", .clk_mux = &gcc.mux, .mux = 0x5b },
	{ .name = "gcc_usb30_prim_mock_utmi_clk", .clk_mux = &gcc.mux, .mux = 0x5d },
	{ .name = "gcc_usb30_prim_sleep_clk", .clk_mux = &gcc.mux, .mux = 0x5c },
	{ .name = "gcc_usb3_prim_phy_aux_clk", .clk_mux = &gcc.mux, .mux = 0x5e },
	{ .name = "gcc_usb3_prim_phy_com_aux_clk", .clk_mux = &gcc.mux, .mux = 0x5f },
	{ .name = "gcc_usb3_prim_phy_pipe_clk", .clk_mux = &gcc.mux, .mux = 0x60 },
	{ .name = "gcc_video_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x31 },
	{ .name = "gcc_video_axi_clk", .clk_mux = &gcc.mux, .mux = 0x35 },
	{ .name = "gcc_video_throttle_axi_clk", .clk_mux = &gcc.mux, .mux = 0x38 },
	{ .name = "gcc_video_xo_clk", .clk_mux = &gcc.mux, .mux = 0x3b },
	{ .name = "measure_only_cnoc_clk", .clk_mux = &gcc.mux, .mux = 0x14 },
	{ .name = "measure_only_ipa_2x_clk", .clk_mux = &gcc.mux, .mux = 0xec },
	{ .name = "measure_only_snoc_clk", .clk_mux = &gcc.mux, .mux = 0x07 },

	{ .name = "disp_cc_mdss_ahb_clk", .clk_mux = &disp_cc, .mux = 0x14 },
	{ .name = "disp_cc_mdss_byte0_clk", .clk_mux = &disp_cc, .mux = 0xc },
	{ .name = "disp_cc_mdss_byte0_intf_clk", .clk_mux = &disp_cc, .mux = 0xd },
	{ .name = "disp_cc_mdss_dp_aux_clk", .clk_mux = &disp_cc, .mux = 0x13 },
	{ .name = "disp_cc_mdss_dp_crypto_clk", .clk_mux = &disp_cc, .mux = 0x11 },
	{ .name = "disp_cc_mdss_dp_link_clk", .clk_mux = &disp_cc, .mux = 0xf },
	{ .name = "disp_cc_mdss_dp_link_intf_clk", .clk_mux = &disp_cc, .mux = 0x10 },
	{ .name = "disp_cc_mdss_dp_pixel_clk", .clk_mux = &disp_cc, .mux = 0x12 },
	{ .name = "disp_cc_mdss_esc0_clk", .clk_mux = &disp_cc, .mux = 0xe },
	{ .name = "disp_cc_mdss_mdp_clk", .clk_mux = &disp_cc, .mux = 0x8 },
	{ .name = "disp_cc_mdss_mdp_lut_clk", .clk_mux = &disp_cc, .mux = 0xa },
	{ .name = "disp_cc_mdss_non_gdsc_ahb_clk", .clk_mux = &disp_cc, .mux = 0x15 },
	{ .name = "disp_cc_mdss_pclk0_clk", .clk_mux = &disp_cc, .mux = 0x7 },
	{ .name = "disp_cc_mdss_rot_clk", .clk_mux = &disp_cc, .mux = 0x9 },
	{ .name = "disp_cc_mdss_rscc_ahb_clk", .clk_mux = &disp_cc, .mux = 0x17 },
	{ .name = "disp_cc_mdss_rscc_vsync_clk", .clk_mux = &disp_cc, .mux = 0x16 },
	{ .name = "disp_cc_mdss_vsync_clk", .clk_mux = &disp_cc, .mux = 0xb },
	{ .name = "disp_cc_sleep_clk", .clk_mux = &disp_cc, .mux = 0x1d },
	{ .name = "disp_cc_xo_clk", .clk_mux = &disp_cc, .mux = 0x1e },

	{ .name = "gpu_cc_acd_ahb_clk", .clk_mux = &gpu_cc, .mux = 0x20 },
	{ .name = "gpu_cc_acd_cxo_clk", .clk_mux = &gpu_cc, .mux = 0x1f },
	{ .name = "gpu_cc_ahb_clk", .clk_mux = &gpu_cc, .mux = 0x11 },
	{ .name = "gpu_cc_crc_ahb_clk", .clk_mux = &gpu_cc, .mux = 0x12 },
	{ .name = "gpu_cc_cx_gfx3d_clk", .clk_mux = &gpu_cc, .mux = 0x1a },
	{ .name = "gpu_cc_cx_gfx3d_slv_clk", .clk_mux = &gpu_cc, .mux = 0x1b },
	{ .name = "gpu_cc_cx_gmu_clk", .clk_mux = &gpu_cc, .mux = 0x19 },
	{ .name = "gpu_cc_cx_snoc_dvm_clk", .clk_mux = &gpu_cc, .mux = 0x16 },
	{ .name = "gpu_cc_cxo_aon_clk", .clk_mux = &gpu_cc, .mux = 0xb },
	{ .name = "gpu_cc_cxo_clk", .clk_mux = &gpu_cc, .mux = 0xa },
	{ .name = "gpu_cc_gx_cxo_clk", .clk_mux = &gpu_cc, .mux = 0xf },
	{ .name = "gpu_cc_gx_gfx3d_clk", .clk_mux = &gpu_cc, .mux = 0xc },
	{ .name = "gpu_cc_gx_gmu_clk", .clk_mux = &gpu_cc, .mux = 0x10 },
	{ .name = "gpu_cc_gx_vsense_clk", .clk_mux = &gpu_cc, .mux = 0xd },

	{ .name = "mccc_clk", .clk_mux = &mc_cc, .mux = 0x50 },
	{}
};

struct debugcc_platform sm6350_debugcc = {
	.name = "sm6350",
	.clocks = sm6350_clocks,
};
