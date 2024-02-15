// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2023, Linaro Limited */

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

		.measure = measure_gcc,

		.enable_reg = 0x30004,
		.enable_mask = BIT(0),

		.mux_reg = 0x62000,
		.mux_mask = 0x3ff,

		.div_reg = 0x30000,
		.div_mask = 0xf,
		.div_val = 4,
	},

	.xo_div4_reg = 0x28008,
	.debug_ctl_reg = 0x62038,
	.debug_status_reg = 0x6203c,
};

static struct debug_mux disp_cc = {
	.phys = 0x5f00000,
	.size = 0x20000,
	.block_name = "disp",

	.measure = measure_leaf,
	.parent = &gcc.mux,
	.parent_mux_val = 0x41,

	.enable_reg = 0x500c,
	.enable_mask = BIT(0),

	.mux_reg = 0x7000,
	.mux_mask = 0xff,

	.div_reg = 0x5008,
	.div_mask = 0x3,
	.div_val = 4,
};

static struct debug_mux gpu_cc = {
	.phys = 0x5990000,
	.size = 0x9000,
	.block_name = "gpu",

	.measure = measure_leaf,
	.parent = &gcc.mux,
	.parent_mux_val = 0xe3,

	.enable_reg = 0x1100,
	.enable_mask = BIT(0),

	.mux_reg = 0x1568,
	.mux_mask = 0xff,

	.div_reg = 0x10fc,
	.div_mask = 0x3,
	.div_val = 2,
};

static struct debug_mux mc_cc = {
	.phys = 0x447d000, /* 0x447d220 */
	.size = 0x1000, /* 0x100 */
	.block_name = "mc",

	.measure = measure_mccc,
};

static struct debug_mux cpu_cc = {
	.phys = 0xf111000,
	.size = 0x1000,
	.block_name = "cpu",

	.measure = measure_leaf,
	.parent = &gcc.mux,
	.parent_mux_val = 0xab,

	.enable_reg = 0x1c,
	.enable_mask = BIT(0),

	.mux_reg = 0x1c,
	.mux_mask = 0x3ff << 8,
	.mux_shift = 8,

	.div_reg = 0x1c,
	.div_mask = 0xf << 28,
	.div_shift = 28,
};

static struct measure_clk qcm2290_clocks[] = {
	{ "pwrcl_clk", &cpu_cc, 0x0 },

	// { "apcs_debug_mux", &gcc.mux, 0xab },
	// { "disp_cc_debug_mux", &gcc.mux, 0x41 },
	{ "gcc_ahb2phy_csi_clk", &gcc.mux, 0x62 },
	{ "gcc_ahb2phy_usb_clk", &gcc.mux, 0x63 },
	{ "gcc_bimc_gpu_axi_clk", &gcc.mux, 0x8d },
	{ "gcc_boot_rom_ahb_clk", &gcc.mux, 0x75 },
	{ "gcc_cam_throttle_nrt_clk", &gcc.mux, 0x4b },
	{ "gcc_cam_throttle_rt_clk", &gcc.mux, 0x4a },
	{ "gcc_camera_ahb_clk", &gcc.mux, 0x36 },
	{ "gcc_camera_xo_clk", &gcc.mux, 0x3e },
	{ "gcc_camss_axi_clk", &gcc.mux, 0x120 },
	{ "gcc_camss_camnoc_atb_clk", &gcc.mux, 0x122 },
	{ "gcc_camss_camnoc_nts_xo_clk", &gcc.mux, 0x123 },
	{ "gcc_camss_cci_0_clk", &gcc.mux, 0x11e },
	{ "gcc_camss_cphy_0_clk", &gcc.mux, 0x115 },
	{ "gcc_camss_cphy_1_clk", &gcc.mux, 0x116 },
	{ "gcc_camss_csi0phytimer_clk", &gcc.mux, 0x10b },
	{ "gcc_camss_csi1phytimer_clk", &gcc.mux, 0x10c },
	{ "gcc_camss_mclk0_clk", &gcc.mux, 0x10d },
	{ "gcc_camss_mclk1_clk", &gcc.mux, 0x10e },
	{ "gcc_camss_mclk2_clk", &gcc.mux, 0x10f },
	{ "gcc_camss_mclk3_clk", &gcc.mux, 0x110 },
	{ "gcc_camss_nrt_axi_clk", &gcc.mux, 0x124 },
	{ "gcc_camss_ope_ahb_clk", &gcc.mux, 0x11d },
	{ "gcc_camss_ope_clk", &gcc.mux, 0x11b },
	{ "gcc_camss_rt_axi_clk", &gcc.mux, 0x126 },
	{ "gcc_camss_tfe_0_clk", &gcc.mux, 0x111 },
	{ "gcc_camss_tfe_0_cphy_rx_clk", &gcc.mux, 0x113 },
	{ "gcc_camss_tfe_0_csid_clk", &gcc.mux, 0x117 },
	{ "gcc_camss_tfe_1_clk", &gcc.mux, 0x112 },
	{ "gcc_camss_tfe_1_cphy_rx_clk", &gcc.mux, 0x114 },
	{ "gcc_camss_tfe_1_csid_clk", &gcc.mux, 0x119 },
	{ "gcc_camss_top_ahb_clk", &gcc.mux, 0x11f },
	{ "gcc_cfg_noc_usb3_prim_axi_clk", &gcc.mux, 0x1c },
	{ "gcc_disp_ahb_clk", &gcc.mux, 0x37 },
	{ "gcc_disp_gpll0_div_clk_src", &gcc.mux, 0x46 },
	{ "gcc_disp_hf_axi_clk", &gcc.mux, 0x3c },
	{ "gcc_disp_throttle_core_clk", &gcc.mux, 0x48 },
	{ "gcc_disp_xo_clk", &gcc.mux, 0x3f },
	{ "gcc_gp1_clk", &gcc.mux, 0xb6 },
	{ "gcc_gp2_clk", &gcc.mux, 0xb7 },
	{ "gcc_gp3_clk", &gcc.mux, 0xb8 },
	{ "gcc_gpu_cfg_ahb_clk", &gcc.mux, 0xe1 },
	{ "gcc_gpu_gpll0_clk_src", &gcc.mux, 0xe7 },
	{ "gcc_gpu_gpll0_div_clk_src", &gcc.mux, 0xe8 },
	{ "gcc_gpu_memnoc_gfx_clk", &gcc.mux, 0xe4 },
	{ "gcc_gpu_snoc_dvm_gfx_clk", &gcc.mux, 0xe6 },
	{ "gcc_gpu_throttle_core_clk", &gcc.mux, 0xeb },
	{ "gcc_pdm2_clk", &gcc.mux, 0x72 },
	{ "gcc_pdm_ahb_clk", &gcc.mux, 0x70 },
	{ "gcc_pdm_xo4_clk", &gcc.mux, 0x71 },
	{ "gcc_pwm0_xo512_clk", &gcc.mux, 0x73 },
	{ "gcc_qmip_camera_nrt_ahb_clk", &gcc.mux, 0x39 },
	{ "gcc_qmip_camera_rt_ahb_clk", &gcc.mux, 0x47 },
	{ "gcc_qmip_disp_ahb_clk", &gcc.mux, 0x3a },
	{ "gcc_qmip_gpu_cfg_ahb_clk", &gcc.mux, 0xe9 },
	{ "gcc_qmip_video_vcodec_ahb_clk", &gcc.mux, 0x38 },
	{ "gcc_qupv3_wrap0_core_2x_clk", &gcc.mux, 0x69 },
	{ "gcc_qupv3_wrap0_core_clk", &gcc.mux, 0x68 },
	{ "gcc_qupv3_wrap0_s0_clk", &gcc.mux, 0x6a },
	{ "gcc_qupv3_wrap0_s1_clk", &gcc.mux, 0x6b },
	{ "gcc_qupv3_wrap0_s2_clk", &gcc.mux, 0x6c },
	{ "gcc_qupv3_wrap0_s3_clk", &gcc.mux, 0x6d },
	{ "gcc_qupv3_wrap0_s4_clk", &gcc.mux, 0x6e },
	{ "gcc_qupv3_wrap0_s5_clk", &gcc.mux, 0x6f },
	{ "gcc_qupv3_wrap_0_m_ahb_clk", &gcc.mux, 0x66 },
	{ "gcc_qupv3_wrap_0_s_ahb_clk", &gcc.mux, 0x67 },
	{ "gcc_sdcc1_ahb_clk", &gcc.mux, 0xef },
	{ "gcc_sdcc1_apps_clk", &gcc.mux, 0xee },
	{ "gcc_sdcc1_ice_core_clk", &gcc.mux, 0xf0 },
	{ "gcc_sdcc2_ahb_clk", &gcc.mux, 0x65 },
	{ "gcc_sdcc2_apps_clk", &gcc.mux, 0x64 },
	{ "gcc_sys_noc_cpuss_ahb_clk", &gcc.mux, 0x9 },
	{ "gcc_sys_noc_usb3_prim_axi_clk", &gcc.mux, 0x18 },
	{ "gcc_usb30_prim_master_clk", &gcc.mux, 0x5b },
	{ "gcc_usb30_prim_mock_utmi_clk", &gcc.mux, 0x5d },
	{ "gcc_usb30_prim_sleep_clk", &gcc.mux, 0x5c },
	{ "gcc_usb3_prim_phy_com_aux_clk", &gcc.mux, 0x5e },
	{ "gcc_usb3_prim_phy_pipe_clk", &gcc.mux, 0x5f },
	{ "gcc_vcodec0_axi_clk", &gcc.mux, 0x12c },
	{ "gcc_venus_ahb_clk", &gcc.mux, 0x12d },
	{ "gcc_venus_ctl_axi_clk", &gcc.mux, 0x12b },
	{ "gcc_video_ahb_clk", &gcc.mux, 0x35 },
	{ "gcc_video_axi0_clk", &gcc.mux, 0x3b },
	{ "gcc_video_throttle_core_clk", &gcc.mux, 0x49 },
	{ "gcc_video_vcodec0_sys_clk", &gcc.mux, 0x129 },
	{ "gcc_video_venus_ctl_clk", &gcc.mux, 0x127 },
	{ "gcc_video_xo_clk", &gcc.mux, 0x3d },
	// { "gpu_cc_debug_mux", &gcc.mux, 0xe3 },
	// { "mc_cc_debug_mux", &gcc.mux, 0x9b },
	{ "measure_only_cnoc_clk", &gcc.mux, 0x19 },
	{ "measure_only_ipa_2x_clk", &gcc.mux, 0xc2 },
	{ "measure_only_snoc_clk", &gcc.mux, 0x7 },
	{ "measure_only_qpic_clk", &gcc.mux, 0x9c },
	{ "measure_only_qpic_ahb_clk", &gcc.mux, 0x9e },
	{ "measure_only_hwkm_km_core_clk", &gcc.mux, 0xa0 },
	{ "measure_only_hwkm_ahb_clk", &gcc.mux, 0xa2 },
	{ "measure_only_pka_core_clk", &gcc.mux, 0xa3 },
	{ "measure_only_pka_ahb_clk", &gcc.mux, 0xa4 },
	{ "measure_only_cpuss_gnoc_clk", &gcc.mux, 0xa6 },

	{ "disp_cc_mdss_ahb_clk", &disp_cc, 0x1a },
	{ "disp_cc_mdss_byte0_clk", &disp_cc, 0x11 },
	{ "disp_cc_mdss_byte0_intf_clk", &disp_cc, 0x12 },
	{ "disp_cc_mdss_esc0_clk", &disp_cc, 0x13 },
	{ "disp_cc_mdss_mdp_clk", &disp_cc, 0xe },
	{ "disp_cc_mdss_mdp_lut_clk", &disp_cc, 0xf },
	{ "disp_cc_mdss_non_gdsc_ahb_clk", &disp_cc, 0x1b },
	{ "disp_cc_mdss_pclk0_clk", &disp_cc, 0xd },
	{ "disp_cc_mdss_vsync_clk", &disp_cc, 0x10 },
	{ "disp_cc_sleep_clk", &disp_cc, 0x24 },
	{ "disp_cc_xo_clk", &disp_cc, 0x23 },

	{ "gpu_cc_ahb_clk", &gpu_cc, 0x10 },
	{ "gpu_cc_crc_ahb_clk", &gpu_cc, 0x11 },
	{ "gpu_cc_cx_gfx3d_clk", &gpu_cc, 0x1a },
	{ "gpu_cc_cx_gmu_clk", &gpu_cc, 0x18 },
	{ "gpu_cc_cx_snoc_dvm_clk", &gpu_cc, 0x15 },
	{ "gpu_cc_cxo_aon_clk", &gpu_cc, 0xa },
	{ "gpu_cc_cxo_clk", &gpu_cc, 0x19 },
	{ "gpu_cc_gx_cxo_clk", &gpu_cc, 0xe },
	{ "gpu_cc_gx_gfx3d_clk", &gpu_cc, 0xb },
	{ "gpu_cc_sleep_clk", &gpu_cc, 0x16 },

	{ "mccc_clk", &mc_cc, 0x220 },

	{}
};

struct debugcc_platform qcm2290_debugcc = {
	"qcm2290",
	qcm2290_clocks,
};
