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
		.phys = 0x1400000,
		.size = 0x1f0000,

		.enable_reg = 0x30004,
		.enable_mask = BIT(0),

		.mux_reg = 0x62000,
		.mux_mask = 0x3ff,

		.div_reg = 0x30000,
		.div_mask = 0xf,
		.div_val = 1,
	},

	.xo_div4_reg = 0x28008,
	.debug_ctl_reg = 0x62038,
	.debug_status_reg = 0x6203c,
};

static struct debug_mux cpu_cc = {
	.phys = 0xfaa0000,
	.size = 0x1000,
	.block_name = "cpu",

	.measure = measure_leaf,
	.parent = &gcc.mux,
	.parent_mux_val = 0xbf,

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
	.phys = 0x5f00000,
	.size = 0x20000,
	.block_name = "disp",

	.measure = measure_leaf,
	.parent = &gcc.mux,
	.parent_mux_val = 0x43,

	.enable_reg = 0x3004,
	.enable_mask = BIT(0),

	.mux_reg = 0x7000,
	.mux_mask = 0xff,

	.div_reg = 0x3000,
	.div_mask = 0xf,
	.div_val = 4,
};

static struct debug_mux gpu_cc = {
	.phys = 0x5990000,
	.size = 0x9000,
	.block_name = "gpu",

	.measure = measure_leaf,
	.parent = &gcc.mux,
	.parent_mux_val = 0xfb,

	.enable_reg = 0x1100,
	.enable_mask = BIT(0),

	.mux_reg = 0x1568,
	.mux_mask = 0xff,

	.div_reg = 0x10fc,
	.div_mask = 0xf,
	.div_val = 2,
};

static struct debug_mux mc_cc = {
	/* It's really <0x447d200 0x100>, but we need to reserve a whole 4096-long page.. */
	.phys = 0x447d000,
	.size = 0x1000,
	.block_name = "mc",

	.measure = measure_mccc,
	.parent = &gcc.mux,
	.parent_mux_val = 0xae,
};

static struct measure_clk sm6375_clocks[] = {
	{ .name = "l3_clk", .clk_mux = &cpu_cc, .mux = 0x41 },
	{ .name = "perfcl_clk", .clk_mux = &cpu_cc, .mux = 0x25 },
	{ .name = "pwrcl_clk", .clk_mux = &cpu_cc, .mux = 0x21 },

	{ .name = "gcc_ahb2phy_csi_clk", .clk_mux = &gcc.mux, .mux = 0x67 },
	{ .name = "gcc_ahb2phy_usb_clk", .clk_mux = &gcc.mux, .mux = 0x68 },
	{ .name = "gcc_bimc_gpu_axi_clk", .clk_mux = &gcc.mux, .mux = 0x9d },
	{ .name = "gcc_boot_rom_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x84 },
	{ .name = "gcc_cam_throttle_nrt_clk", .clk_mux = &gcc.mux, .mux = 0x4d },
	{ .name = "gcc_cam_throttle_rt_clk", .clk_mux = &gcc.mux, .mux = 0x4c },
	{ .name = "gcc_camss_axi_clk", .clk_mux = &gcc.mux, .mux = 0x154 },
	{ .name = "gcc_camss_cci_0_clk", .clk_mux = &gcc.mux, .mux = 0x151 },
	{ .name = "gcc_camss_cci_1_clk", .clk_mux = &gcc.mux, .mux = 0x152 },
	{ .name = "gcc_camss_cphy_0_clk", .clk_mux = &gcc.mux, .mux = 0x140 },
	{ .name = "gcc_camss_cphy_1_clk", .clk_mux = &gcc.mux, .mux = 0x141 },
	{ .name = "gcc_camss_cphy_2_clk", .clk_mux = &gcc.mux, .mux = 0x142 },
	{ .name = "gcc_camss_cphy_3_clk", .clk_mux = &gcc.mux, .mux = 0x143 },
	{ .name = "gcc_camss_csi0phytimer_clk", .clk_mux = &gcc.mux, .mux = 0x130 },
	{ .name = "gcc_camss_csi1phytimer_clk", .clk_mux = &gcc.mux, .mux = 0x131 },
	{ .name = "gcc_camss_csi2phytimer_clk", .clk_mux = &gcc.mux, .mux = 0x132 },
	{ .name = "gcc_camss_csi3phytimer_clk", .clk_mux = &gcc.mux, .mux = 0x133 },
	{ .name = "gcc_camss_mclk0_clk", .clk_mux = &gcc.mux, .mux = 0x134 },
	{ .name = "gcc_camss_mclk1_clk", .clk_mux = &gcc.mux, .mux = 0x135 },
	{ .name = "gcc_camss_mclk2_clk", .clk_mux = &gcc.mux, .mux = 0x136 },
	{ .name = "gcc_camss_mclk3_clk", .clk_mux = &gcc.mux, .mux = 0x137 },
	{ .name = "gcc_camss_mclk4_clk", .clk_mux = &gcc.mux, .mux = 0x138 },
	{ .name = "gcc_camss_nrt_axi_clk", .clk_mux = &gcc.mux, .mux = 0x158 },
	{ .name = "gcc_camss_ope_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x150 },
	{ .name = "gcc_camss_ope_clk", .clk_mux = &gcc.mux, .mux = 0x14e },
	{ .name = "gcc_camss_rt_axi_clk", .clk_mux = &gcc.mux, .mux = 0x15a },
	{ .name = "gcc_camss_tfe_0_clk", .clk_mux = &gcc.mux, .mux = 0x139 },
	{ .name = "gcc_camss_tfe_0_cphy_rx_clk", .clk_mux = &gcc.mux, .mux = 0x13d },
	{ .name = "gcc_camss_tfe_0_csid_clk", .clk_mux = &gcc.mux, .mux = 0x144 },
	{ .name = "gcc_camss_tfe_1_clk", .clk_mux = &gcc.mux, .mux = 0x13a },
	{ .name = "gcc_camss_tfe_1_cphy_rx_clk", .clk_mux = &gcc.mux, .mux = 0x13e },
	{ .name = "gcc_camss_tfe_1_csid_clk", .clk_mux = &gcc.mux, .mux = 0x146 },
	{ .name = "gcc_camss_tfe_2_clk", .clk_mux = &gcc.mux, .mux = 0x13b },
	{ .name = "gcc_camss_tfe_2_cphy_rx_clk", .clk_mux = &gcc.mux, .mux = 0x13f },
	{ .name = "gcc_camss_tfe_2_csid_clk", .clk_mux = &gcc.mux, .mux = 0x148 },
	{ .name = "gcc_camss_top_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x153 },
	{ .name = "gcc_cfg_noc_usb3_prim_axi_clk", .clk_mux = &gcc.mux, .mux = 0x1f },
	{ .name = "gcc_disp_gpll0_div_clk_src", .clk_mux = &gcc.mux, .mux = 0x48 },
	{ .name = "gcc_disp_hf_axi_clk", .clk_mux = &gcc.mux, .mux = 0x3e },
	{ .name = "gcc_disp_sleep_clk", .clk_mux = &gcc.mux, .mux = 0x4e },
	{ .name = "gcc_disp_throttle_core_clk", .clk_mux = &gcc.mux, .mux = 0x4a },
	{ .name = "gcc_gp1_clk", .clk_mux = &gcc.mux, .mux = 0xca },
	{ .name = "gcc_gp2_clk", .clk_mux = &gcc.mux, .mux = 0xcb },
	{ .name = "gcc_gp3_clk", .clk_mux = &gcc.mux, .mux = 0xcc },
	{ .name = "gcc_gpu_gpll0_clk_src", .clk_mux = &gcc.mux, .mux = 0xff },
	{ .name = "gcc_gpu_gpll0_div_clk_src", .clk_mux = &gcc.mux, .mux = 0x100 },
	{ .name = "gcc_gpu_memnoc_gfx_clk", .clk_mux = &gcc.mux, .mux = 0xfc },
	{ .name = "gcc_gpu_snoc_dvm_gfx_clk", .clk_mux = &gcc.mux, .mux = 0xfe },
	{ .name = "gcc_gpu_throttle_core_clk", .clk_mux = &gcc.mux, .mux = 0x103 },
	{ .name = "gcc_pdm2_clk", .clk_mux = &gcc.mux, .mux = 0x81 },
	{ .name = "gcc_pdm_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x7f },
	{ .name = "gcc_pdm_xo4_clk", .clk_mux = &gcc.mux, .mux = 0x80 },
	{ .name = "gcc_prng_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x82 },
	{ .name = "gcc_qmip_camera_nrt_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x3b },
	{ .name = "gcc_qmip_camera_rt_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x49 },
	{ .name = "gcc_qmip_disp_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x3c },
	{ .name = "gcc_qmip_gpu_cfg_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x101 },
	{ .name = "gcc_qmip_video_vcodec_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x3a },
	{ .name = "gcc_qupv3_wrap0_core_2x_clk", .clk_mux = &gcc.mux, .mux = 0x6e },
	{ .name = "gcc_qupv3_wrap0_core_clk", .clk_mux = &gcc.mux, .mux = 0x6d },
	{ .name = "gcc_qupv3_wrap0_s0_clk", .clk_mux = &gcc.mux, .mux = 0x6f },
	{ .name = "gcc_qupv3_wrap0_s1_clk", .clk_mux = &gcc.mux, .mux = 0x70 },
	{ .name = "gcc_qupv3_wrap0_s2_clk", .clk_mux = &gcc.mux, .mux = 0x71 },
	{ .name = "gcc_qupv3_wrap0_s3_clk", .clk_mux = &gcc.mux, .mux = 0x72 },
	{ .name = "gcc_qupv3_wrap0_s4_clk", .clk_mux = &gcc.mux, .mux = 0x73 },
	{ .name = "gcc_qupv3_wrap0_s5_clk", .clk_mux = &gcc.mux, .mux = 0x74 },
	{ .name = "gcc_qupv3_wrap1_core_2x_clk", .clk_mux = &gcc.mux, .mux = 0x78 },
	{ .name = "gcc_qupv3_wrap1_core_clk", .clk_mux = &gcc.mux, .mux = 0x77 },
	{ .name = "gcc_qupv3_wrap1_s0_clk", .clk_mux = &gcc.mux, .mux = 0x79 },
	{ .name = "gcc_qupv3_wrap1_s1_clk", .clk_mux = &gcc.mux, .mux = 0x7a },
	{ .name = "gcc_qupv3_wrap1_s2_clk", .clk_mux = &gcc.mux, .mux = 0x7b },
	{ .name = "gcc_qupv3_wrap1_s3_clk", .clk_mux = &gcc.mux, .mux = 0x7c },
	{ .name = "gcc_qupv3_wrap1_s5_clk", .clk_mux = &gcc.mux, .mux = 0x7e },
	{ .name = "gcc_qupv3_wrap_0_m_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x6b },
	{ .name = "gcc_qupv3_wrap_0_s_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x6c },
	{ .name = "gcc_sdcc1_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x108 },
	{ .name = "gcc_sdcc1_apps_clk", .clk_mux = &gcc.mux, .mux = 0x107 },
	{ .name = "gcc_sdcc1_ice_core_clk", .clk_mux = &gcc.mux, .mux = 0x109 },
	{ .name = "gcc_sdcc2_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x6a },
	{ .name = "gcc_sdcc2_apps_clk", .clk_mux = &gcc.mux, .mux = 0x69 },
	{ .name = "gcc_sys_noc_cpuss_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x9 },
	{ .name = "gcc_sys_noc_ufs_phy_axi_clk", .clk_mux = &gcc.mux, .mux = 0x1b },
	{ .name = "gcc_sys_noc_usb3_prim_axi_clk", .clk_mux = &gcc.mux, .mux = 0x1a },
	{ .name = "gcc_ufs_phy_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x127 },
	{ .name = "gcc_ufs_phy_axi_clk", .clk_mux = &gcc.mux, .mux = 0x126 },
	{ .name = "gcc_ufs_phy_ice_core_clk", .clk_mux = &gcc.mux, .mux = 0x12d },
	{ .name = "gcc_ufs_phy_phy_aux_clk", .clk_mux = &gcc.mux, .mux = 0x12e },
	{ .name = "gcc_ufs_phy_rx_symbol_0_clk", .clk_mux = &gcc.mux, .mux = 0x129 },
	{ .name = "gcc_ufs_phy_tx_symbol_0_clk", .clk_mux = &gcc.mux, .mux = 0x128 },
	{ .name = "gcc_ufs_phy_unipro_core_clk", .clk_mux = &gcc.mux, .mux = 0x12c },
	{ .name = "gcc_usb30_prim_master_clk", .clk_mux = &gcc.mux, .mux = 0x5e },
	{ .name = "gcc_usb30_prim_mock_utmi_clk", .clk_mux = &gcc.mux, .mux = 0x60 },
	{ .name = "gcc_usb30_prim_sleep_clk", .clk_mux = &gcc.mux, .mux = 0x5f },
	{ .name = "gcc_usb3_prim_phy_com_aux_clk", .clk_mux = &gcc.mux, .mux = 0x61 },
	{ .name = "gcc_usb3_prim_phy_pipe_clk", .clk_mux = &gcc.mux, .mux = 0x62 },
	{ .name = "gcc_vcodec0_axi_clk", .clk_mux = &gcc.mux, .mux = 0x160 },
	{ .name = "gcc_venus_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x161 },
	{ .name = "gcc_venus_ctl_axi_clk", .clk_mux = &gcc.mux, .mux = 0x15f },
	{ .name = "gcc_video_axi0_clk", .clk_mux = &gcc.mux, .mux = 0x3d },
	{ .name = "gcc_video_throttle_core_clk", .clk_mux = &gcc.mux, .mux = 0x4b },
	{ .name = "gcc_video_vcodec0_sys_clk", .clk_mux = &gcc.mux, .mux = 0x15d },
	{ .name = "gcc_video_venus_ctl_clk", .clk_mux = &gcc.mux, .mux = 0x15b },
	{ .name = "gcc_video_xo_clk", .clk_mux = &gcc.mux, .mux = 0x3f },
	{ .name = "measure_only_cnoc_clk", .clk_mux = &gcc.mux, .mux = 0x1d },
	{ .name = "measure_only_gcc_camera_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x38 },
	{ .name = "measure_only_gcc_camera_xo_clk", .clk_mux = &gcc.mux, .mux = 0x40 },
	{ .name = "measure_only_gcc_cpuss_gnoc_clk", .clk_mux = &gcc.mux, .mux = 0xba },
	{ .name = "measure_only_gcc_disp_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x39 },
	{ .name = "measure_only_gcc_disp_xo_clk", .clk_mux = &gcc.mux, .mux = 0x41 },
	{ .name = "measure_only_gcc_gpu_cfg_ahb_clk", .clk_mux = &gcc.mux, .mux = 0xf9 },
	{ .name = "measure_only_gcc_qupv3_wrap1_s4_clk", .clk_mux = &gcc.mux, .mux = 0x7d },
	{ .name = "measure_only_gcc_qupv3_wrap_1_m_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x75 },
	{ .name = "measure_only_gcc_qupv3_wrap_1_s_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x76 },
	{ .name = "measure_only_gcc_video_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x37 },
	{ .name = "measure_only_hwkm_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x166 },
	{ .name = "measure_only_hwkm_km_core_clk", .clk_mux = &gcc.mux, .mux = 0x167 },
	{ .name = "measure_only_ipa_2x_clk", .clk_mux = &gcc.mux, .mux = 0xd7 },
	{ .name = "measure_only_pka_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x162 },
	{ .name = "measure_only_pka_core_clk", .clk_mux = &gcc.mux, .mux = 0x163 },
	{ .name = "measure_only_snoc_clk", .clk_mux = &gcc.mux, .mux = 0x7 },

	{ .name = "disp_cc_mdss_ahb_clk", .clk_mux = &disp_cc, .mux = 0x14 },
	{ .name = "disp_cc_mdss_byte0_clk", .clk_mux = &disp_cc, .mux = 0xc },
	{ .name = "disp_cc_mdss_byte0_intf_clk", .clk_mux = &disp_cc, .mux = 0xd },
	{ .name = "disp_cc_mdss_esc0_clk", .clk_mux = &disp_cc, .mux = 0xe },
	{ .name = "disp_cc_mdss_mdp_clk", .clk_mux = &disp_cc, .mux = 0x8 },
	{ .name = "disp_cc_mdss_mdp_lut_clk", .clk_mux = &disp_cc, .mux = 0xa },
	{ .name = "disp_cc_mdss_non_gdsc_ahb_clk", .clk_mux = &disp_cc, .mux = 0x15 },
	{ .name = "disp_cc_mdss_pclk0_clk", .clk_mux = &disp_cc, .mux = 0x7 },
	{ .name = "disp_cc_mdss_rot_clk", .clk_mux = &disp_cc, .mux = 0x9 },
	{ .name = "disp_cc_mdss_rscc_ahb_clk", .clk_mux = &disp_cc, .mux = 0x17 },
	{ .name = "disp_cc_mdss_rscc_vsync_clk", .clk_mux = &disp_cc, .mux = 0x16 },
	{ .name = "disp_cc_mdss_vsync_clk", .clk_mux = &disp_cc, .mux = 0xb },
	{ .name = "measure_only_disp_cc_sleep_clk", .clk_mux = &disp_cc, .mux = 0x1d },
	{ .name = "measure_only_disp_cc_xo_clk", .clk_mux = &disp_cc, .mux = 0x1e },

	{ .name = "gpu_cc_ahb_clk", .clk_mux = &gpu_cc, .mux = 0x11 },
	{ .name = "gpu_cc_cx_gfx3d_clk", .clk_mux = &gpu_cc, .mux = 0x1a },
	{ .name = "gpu_cc_cx_gfx3d_slv_clk", .clk_mux = &gpu_cc, .mux = 0x1b },
	{ .name = "gpu_cc_cx_gmu_clk", .clk_mux = &gpu_cc, .mux = 0x19 },
	{ .name = "gpu_cc_cx_snoc_dvm_clk", .clk_mux = &gpu_cc, .mux = 0x16 },
	{ .name = "gpu_cc_cxo_aon_clk", .clk_mux = &gpu_cc, .mux = 0xb },
	{ .name = "gpu_cc_cxo_clk", .clk_mux = &gpu_cc, .mux = 0xa },
	{ .name = "gpu_cc_gx_cxo_clk", .clk_mux = &gpu_cc, .mux = 0xf },
	{ .name = "gpu_cc_gx_gfx3d_clk", .clk_mux = &gpu_cc, .mux = 0xc },
	{ .name = "gpu_cc_gx_gmu_clk", .clk_mux = &gpu_cc, .mux = 0x10 },
	{ .name = "gpu_cc_sleep_clk", .clk_mux = &gpu_cc, .mux = 0x17 },

	{ .name = "mccc_clk", .clk_mux = &mc_cc, .mux = 0x220 },
	{}
};

struct debugcc_platform sm6375_debugcc = {
	.name = "sm6375",
	.clocks = sm6375_clocks,
};
