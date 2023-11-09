/*
 * Copyright (c) 2019, Linaro Ltd.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/mman.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "debugcc.h"

#define GCC_BASE	0x100000
#define GCC_SIZE	0x1f0000

#define GCC_DEBUG_POST_DIV		0x62000
#define GCC_DEBUG_CBCR			0x62004
#define GCC_DEBUG_OFFSET		0x62008
#define GCC_DEBUG_CTL			0x62024
#define GCC_DEBUG_STATUS		0x62028
#define GCC_XO_DIV4_CBCR		0x43008

static struct gcc_mux gcc = {
	.mux = {
		.phys =	GCC_BASE,
		.size = GCC_SIZE,

		.measure = measure_gcc,

		.enable_reg = GCC_DEBUG_CBCR,
		.enable_mask = BIT(0),

		.mux_reg = GCC_DEBUG_OFFSET,
		.mux_mask = 0x3ff,

		.div_reg = GCC_DEBUG_POST_DIV,
		.div_mask = 0xf,
		.div_val = 4,
	},

	.xo_div4_reg = GCC_XO_DIV4_CBCR,
	.debug_ctl_reg = GCC_DEBUG_CTL,
	.debug_status_reg = GCC_DEBUG_STATUS,
};

static struct debug_mux cam_cc = {
	.phys = 0xad00000,
	.size = 0x10000,
	.block_name = "cam",

	.measure = measure_leaf,
	.parent = &gcc.mux,
	.parent_mux_val = 70,

	.enable_reg = 0xc008,
	.enable_mask = BIT(0),

	.mux_reg = 0xc000,
	.mux_mask = 0xff,

	.div_reg = 0xc004,
	.div_mask = 0x3,
};

static struct debug_mux cpu = {
	.phys = 0x17970000,
	.size = 4096,
	.block_name = "cpu",

	.measure = measure_leaf,
	.parent = &gcc.mux,
	.parent_mux_val = 214,

	.enable_reg = 0x18,
	.enable_mask = BIT(0),

	.mux_reg = 0x18,
	.mux_mask = 0x7f << 4,
	.mux_shift = 4,

	.div_reg = 0x18,
	.div_mask = 0xf << 11,
	.div_shift = 11,
};

static struct debug_mux disp_cc = {
	.phys = 0xaf00000,
	.size = 0x10000,
	.block_name = "disp",

	.measure = measure_leaf,
	.parent = &gcc.mux,
	.parent_mux_val = 71,

	.enable_reg = 0x600c,
	.enable_mask = BIT(0),

	.mux_reg = 0x6000,
	.mux_mask = 0xff,

	.div_reg = 0x6008,
	.div_mask = 0x3,
};

static struct debug_mux gpu_cc = {
	.phys = 0x5090000,
	.size = 0x9000,
	.block_name = "gpu",

	.measure = measure_leaf,
	.parent = &gcc.mux,
	.parent_mux_val = 324,

	.enable_reg = 0x1100,
	.enable_mask = BIT(0),

	.mux_reg = 0x1568,
	.mux_mask = 0xff,

	.div_reg = 0x10fc,
	.div_mask = 0x3,
};

static struct debug_mux video_cc = {
	.phys = 0xab00000,
	.size = 0x10000,
	.block_name = "video",

	.measure = measure_leaf,
	.parent = &gcc.mux,
	.parent_mux_val = 72,

	.enable_reg = 0xa58,
	.enable_mask = BIT(0),

	.mux_reg = 0xa4c,
	.mux_mask = 0x3f,

	.div_reg = 0xa50,
	.div_mask = 0x7,
};

static struct measure_clk sdm845_clocks[] = {
	{ .name = "measure_only_snoc_clk", .clk_mux = &gcc.mux, .mux = 7 },
	{ .name = "gcc_sys_noc_cpuss_ahb_clk", .clk_mux = &gcc.mux, .mux = 12 },
	{ .name = "measure_only_cnoc_clk", .clk_mux = &gcc.mux, .mux = 21 },
	{ .name = "gcc_cfg_noc_usb3_prim_axi_clk", .clk_mux = &gcc.mux, .mux = 29 },
	{ .name = "gcc_cfg_noc_usb3_sec_axi_clk", .clk_mux = &gcc.mux, .mux = 30 },
	{ .name = "gcc_aggre_noc_pcie_tbu_clk", .clk_mux = &gcc.mux, .mux = 45 },
	{ .name = "gcc_video_ahb_clk", .clk_mux = &gcc.mux, .mux = 57 },
	{ .name = "gcc_camera_ahb_clk", .clk_mux = &gcc.mux, .mux = 58 },
	{ .name = "gcc_disp_ahb_clk", .clk_mux = &gcc.mux, .mux = 59 },
	{ .name = "gcc_qmip_video_ahb_clk", .clk_mux = &gcc.mux, .mux = 60 },
	{ .name = "gcc_qmip_camera_ahb_clk", .clk_mux = &gcc.mux, .mux = 61 },
	{ .name = "gcc_qmip_disp_ahb_clk", .clk_mux = &gcc.mux, .mux = 62 },
	{ .name = "gcc_video_axi_clk", .clk_mux = &gcc.mux, .mux = 63 },
	{ .name = "gcc_camera_axi_clk", .clk_mux = &gcc.mux, .mux = 64 },
	{ .name = "gcc_disp_axi_clk", .clk_mux = &gcc.mux, .mux = 65 },
	{ .name = "gcc_video_xo_clk", .clk_mux = &gcc.mux, .mux = 66 },
	{ .name = "gcc_camera_xo_clk", .clk_mux = &gcc.mux, .mux = 67 },
	{ .name = "gcc_disp_xo_clk", .clk_mux = &gcc.mux, .mux = 68 },
	{ .name = "cam_cc_mclk0_clk", .clk_mux = &cam_cc, .mux = 1 },
	{ .name = "cam_cc_mclk1_clk", .clk_mux = &cam_cc, .mux = 2 },
	{ .name = "cam_cc_mclk2_clk", .clk_mux = &cam_cc, .mux = 3 },
	{ .name = "cam_cc_mclk3_clk", .clk_mux = &cam_cc, .mux = 4 },
	{ .name = "cam_cc_csi0phytimer_clk", .clk_mux = &cam_cc, .mux = 5 },
	{ .name = "cam_cc_csiphy0_clk", .clk_mux = &cam_cc, .mux = 6 },
	{ .name = "cam_cc_csi1phytimer_clk", .clk_mux = &cam_cc, .mux = 7 },
	{ .name = "cam_cc_csiphy1_clk", .clk_mux = &cam_cc, .mux = 8 },
	{ .name = "cam_cc_csi2phytimer_clk", .clk_mux = &cam_cc, .mux = 9 },
	{ .name = "cam_cc_csiphy2_clk", .clk_mux = &cam_cc, .mux = 10 },
	{ .name = "cam_cc_bps_clk", .clk_mux = &cam_cc, .mux = 11 },
	{ .name = "cam_cc_bps_axi_clk", .clk_mux = &cam_cc, .mux = 12 },
	{ .name = "cam_cc_bps_areg_clk", .clk_mux = &cam_cc, .mux = 13 },
	{ .name = "cam_cc_bps_ahb_clk", .clk_mux = &cam_cc, .mux = 14 },
	{ .name = "cam_cc_ipe_0_clk", .clk_mux = &cam_cc, .mux = 15 },
	{ .name = "cam_cc_ipe_0_axi_clk", .clk_mux = &cam_cc, .mux = 16 },
	{ .name = "cam_cc_ipe_0_areg_clk", .clk_mux = &cam_cc, .mux = 17 },
	{ .name = "cam_cc_ipe_0_ahb_clk", .clk_mux = &cam_cc, .mux = 18 },
	{ .name = "cam_cc_ipe_1_clk", .clk_mux = &cam_cc, .mux = 19 },
	{ .name = "cam_cc_ipe_1_axi_clk", .clk_mux = &cam_cc, .mux = 20 },
	{ .name = "cam_cc_ipe_1_areg_clk", .clk_mux = &cam_cc, .mux = 21 },
	{ .name = "cam_cc_ipe_1_ahb_clk", .clk_mux = &cam_cc, .mux = 22 },
	{ .name = "cam_cc_ife_0_clk", .clk_mux = &cam_cc, .mux = 23 },
	{ .name = "cam_cc_ife_0_dsp_clk", .clk_mux = &cam_cc, .mux = 24 },
	{ .name = "cam_cc_ife_0_csid_clk", .clk_mux = &cam_cc, .mux = 25 },
	{ .name = "cam_cc_ife_0_cphy_rx_clk", .clk_mux = &cam_cc, .mux = 26 },
	{ .name = "cam_cc_ife_0_axi_clk", .clk_mux = &cam_cc, .mux = 27 },
	{ .name = "cam_cc_ife_1_clk", .clk_mux = &cam_cc, .mux = 29 },
	{ .name = "cam_cc_ife_1_dsp_clk", .clk_mux = &cam_cc, .mux = 30 },
	{ .name = "cam_cc_ife_1_csid_clk", .clk_mux = &cam_cc, .mux = 31 },
	{ .name = "cam_cc_ife_1_cphy_rx_clk", .clk_mux = &cam_cc, .mux = 32 },
	{ .name = "cam_cc_ife_1_axi_clk", .clk_mux = &cam_cc, .mux = 33 },
	{ .name = "cam_cc_ife_lite_clk", .clk_mux = &cam_cc, .mux = 34 },
	{ .name = "cam_cc_ife_lite_csid_clk", .clk_mux = &cam_cc, .mux = 35 },
	{ .name = "cam_cc_ife_lite_cphy_rx_clk", .clk_mux = &cam_cc, .mux = 36 },
	{ .name = "cam_cc_jpeg_clk", .clk_mux = &cam_cc, .mux = 37 },
	{ .name = "cam_cc_icp_clk", .clk_mux = &cam_cc, .mux = 38 },
	{ .name = "cam_cc_fd_core_clk", .clk_mux = &cam_cc, .mux = 40 },
	{ .name = "cam_cc_fd_core_uar_clk", .clk_mux = &cam_cc, .mux = 41 },
	{ .name = "cam_cc_cci_clk", .clk_mux = &cam_cc, .mux = 42 },
	{ .name = "cam_cc_lrme_clk", .clk_mux = &cam_cc, .mux = 43 },
	{ .name = "cam_cc_cpas_ahb_clk", .clk_mux = &cam_cc, .mux = 44 },
	{ .name = "cam_cc_camnoc_axi_clk", .clk_mux = &cam_cc, .mux = 45 },
	{ .name = "cam_cc_soc_ahb_clk", .clk_mux = &cam_cc, .mux = 46 },
	{ .name = "cam_cc_icp_atb_clk", .clk_mux = &cam_cc, .mux = 47 },
	{ .name = "cam_cc_icp_cti_clk", .clk_mux = &cam_cc, .mux = 48 },
	{ .name = "cam_cc_icp_ts_clk", .clk_mux = &cam_cc, .mux = 49 },
	{ .name = "cam_cc_icp_apb_clk", .clk_mux = &cam_cc, .mux = 50 },
	{ .name = "cam_cc_sys_tmr_clk", .clk_mux = &cam_cc, .mux = 51 },
	{ .name = "cam_cc_camnoc_atb_clk", .clk_mux = &cam_cc, .mux = 52 },
	{ .name = "cam_cc_csiphy3_clk", .clk_mux = &cam_cc, .mux = 54 },
	{ .name = "disp_cc_mdss_pclk0_clk", .clk_mux = &disp_cc, .mux = 1 },
	{ .name = "disp_cc_mdss_pclk1_clk", .clk_mux = &disp_cc, .mux = 2 },
	{ .name = "disp_cc_mdss_mdp_clk", .clk_mux = &disp_cc, .mux = 3 },
	{ .name = "disp_cc_mdss_rot_clk", .clk_mux = &disp_cc, .mux = 4 },
	{ .name = "disp_cc_mdss_mdp_lut_clk", .clk_mux = &disp_cc, .mux = 5 },
	{ .name = "disp_cc_mdss_vsync_clk", .clk_mux = &disp_cc, .mux = 6 },
	{ .name = "disp_cc_mdss_byte0_clk", .clk_mux = &disp_cc, .mux = 7 },
	{ .name = "disp_cc_mdss_byte0_intf_clk", .clk_mux = &disp_cc, .mux = 8 },
	{ .name = "disp_cc_mdss_byte1_clk", .clk_mux = &disp_cc, .mux = 9 },
	{ .name = "disp_cc_mdss_byte1_intf_clk", .clk_mux = &disp_cc, .mux = 10 },
	{ .name = "disp_cc_mdss_esc0_clk", .clk_mux = &disp_cc, .mux = 11 },
	{ .name = "disp_cc_mdss_esc1_clk", .clk_mux = &disp_cc, .mux = 12 },
	{ .name = "disp_cc_mdss_dp_link_clk", .clk_mux = &disp_cc, .mux = 13 },
	{ .name = "disp_cc_mdss_dp_link_intf_clk", .clk_mux = &disp_cc, .mux = 14 },
	{ .name = "disp_cc_mdss_dp_crypto_clk", .clk_mux = &disp_cc, .mux = 15 },
	{ .name = "disp_cc_mdss_dp_pixel_clk", .clk_mux = &disp_cc, .mux = 16 },
	{ .name = "disp_cc_mdss_dp_pixel1_clk", .clk_mux = &disp_cc, .mux = 17 },
	{ .name = "disp_cc_mdss_dp_aux_clk", .clk_mux = &disp_cc, .mux = 18 },
	{ .name = "disp_cc_mdss_ahb_clk", .clk_mux = &disp_cc, .mux = 19 },
	{ .name = "disp_cc_mdss_axi_clk", .clk_mux = &disp_cc, .mux = 20 },
	{ .name = "disp_cc_mdss_qdss_at_clk", .clk_mux = &disp_cc, .mux = 21 },
	{ .name = "disp_cc_mdss_qdss_tsctr_div8_clk", .clk_mux = &disp_cc, .mux = 22 },
	{ .name = "disp_cc_mdss_rscc_ahb_clk", .clk_mux = &disp_cc, .mux = 23 },
	{ .name = "disp_cc_mdss_rscc_vsync_clk", .clk_mux = &disp_cc, .mux = 24 },
	{ .name = "video_cc_venus_ctl_core_clk", .clk_mux = &video_cc, .mux = 1 },
	{ .name = "video_cc_vcodec0_core_clk", .clk_mux = &video_cc, .mux = 2 },
	{ .name = "video_cc_vcodec1_core_clk", .clk_mux = &video_cc, .mux = 3 },
	{ .name = "video_cc_venus_ctl_axi_clk", .clk_mux = &video_cc, .mux = 4 },
	{ .name = "video_cc_vcodec0_axi_clk", .clk_mux = &video_cc, .mux = 5 },
	{ .name = "video_cc_vcodec1_axi_clk", .clk_mux = &video_cc, .mux = 6 },
	{ .name = "video_cc_qdss_trig_clk", .clk_mux = &video_cc, .mux = 7 },
	{ .name = "video_cc_apb_clk", .clk_mux = &video_cc, .mux = 8 },
	{ .name = "video_cc_venus_ahb_clk", .clk_mux = &video_cc, .mux = 9 },
	{ .name = "video_cc_qdss_tsctr_div8_clk", .clk_mux = &video_cc, .mux = 10 },
	{ .name = "video_cc_at_clk", .clk_mux = &video_cc, .mux = 11 },
	{ .name = "gcc_disp_gpll0_clk_src", .clk_mux = &gcc.mux, .mux = 76 },
	{ .name = "gcc_disp_gpll0_div_clk_src", .clk_mux = &gcc.mux, .mux = 77 },
	{ .name = "gcc_usb30_prim_master_clk", .clk_mux = &gcc.mux, .mux = 95 },
	{ .name = "gcc_usb30_prim_sleep_clk", .clk_mux = &gcc.mux, .mux = 96 },
	{ .name = "gcc_usb30_prim_mock_utmi_clk", .clk_mux = &gcc.mux, .mux = 97 },
	{ .name = "gcc_usb3_prim_phy_aux_clk", .clk_mux = &gcc.mux, .mux = 98 },
	{ .name = "gcc_usb3_prim_phy_com_aux_clk", .clk_mux = &gcc.mux, .mux = 99 },
	{ .name = "gcc_usb3_prim_phy_pipe_clk", .clk_mux = &gcc.mux, .mux = 100 },
	{ .name = "gcc_usb30_sec_master_clk", .clk_mux = &gcc.mux, .mux = 101 },
	{ .name = "gcc_usb30_sec_sleep_clk", .clk_mux = &gcc.mux, .mux = 102 },
	{ .name = "gcc_usb30_sec_mock_utmi_clk", .clk_mux = &gcc.mux, .mux = 103 },
	{ .name = "gcc_usb3_sec_phy_aux_clk", .clk_mux = &gcc.mux, .mux = 104 },
	{ .name = "gcc_usb3_sec_phy_com_aux_clk", .clk_mux = &gcc.mux, .mux = 105 },
	{ .name = "gcc_usb3_sec_phy_pipe_clk", .clk_mux = &gcc.mux, .mux = 106 },
	{ .name = "gcc_usb_phy_cfg_ahb2phy_clk", .clk_mux = &gcc.mux, .mux = 111 },
	{ .name = "gcc_sdcc2_apps_clk", .clk_mux = &gcc.mux, .mux = 112 },
	{ .name = "gcc_sdcc2_ahb_clk", .clk_mux = &gcc.mux, .mux = 113 },
	{ .name = "gcc_sdcc4_apps_clk", .clk_mux = &gcc.mux, .mux = 114 },
	{ .name = "gcc_sdcc4_ahb_clk", .clk_mux = &gcc.mux, .mux = 115 },
	{ .name = "gcc_qupv3_wrap_0_m_ahb_clk", .clk_mux = &gcc.mux, .mux = 116 },
	{ .name = "gcc_qupv3_wrap_0_s_ahb_clk", .clk_mux = &gcc.mux, .mux = 117 },
	{ .name = "gcc_qupv3_wrap0_core_clk", .clk_mux = &gcc.mux, .mux = 118 },
	{ .name = "gcc_qupv3_wrap0_core_2x_clk", .clk_mux = &gcc.mux, .mux = 119 },
	{ .name = "gcc_qupv3_wrap0_s0_clk", .clk_mux = &gcc.mux, .mux = 120 },
	{ .name = "gcc_qupv3_wrap0_s1_clk", .clk_mux = &gcc.mux, .mux = 121 },
	{ .name = "gcc_qupv3_wrap0_s2_clk", .clk_mux = &gcc.mux, .mux = 122 },
	{ .name = "gcc_qupv3_wrap0_s3_clk", .clk_mux = &gcc.mux, .mux = 123 },
	{ .name = "gcc_qupv3_wrap0_s4_clk", .clk_mux = &gcc.mux, .mux = 124 },
	{ .name = "gcc_qupv3_wrap0_s5_clk", .clk_mux = &gcc.mux, .mux = 125 },
	{ .name = "gcc_qupv3_wrap0_s6_clk", .clk_mux = &gcc.mux, .mux = 126 },
	{ .name = "gcc_qupv3_wrap0_s7_clk", .clk_mux = &gcc.mux, .mux = 127 },
	{ .name = "gcc_qupv3_wrap1_core_2x_clk", .clk_mux = &gcc.mux, .mux = 128 },
	{ .name = "gcc_qupv3_wrap1_core_clk", .clk_mux = &gcc.mux, .mux = 129 },
	{ .name = "gcc_qupv3_wrap_1_m_ahb_clk", .clk_mux = &gcc.mux, .mux = 130 },
	{ .name = "gcc_qupv3_wrap_1_s_ahb_clk", .clk_mux = &gcc.mux, .mux = 131 },
	{ .name = "gcc_qupv3_wrap1_s0_clk", .clk_mux = &gcc.mux, .mux = 132 },
	{ .name = "gcc_qupv3_wrap1_s1_clk", .clk_mux = &gcc.mux, .mux = 133 },
	{ .name = "gcc_qupv3_wrap1_s2_clk", .clk_mux = &gcc.mux, .mux = 134 },
	{ .name = "gcc_qupv3_wrap1_s3_clk", .clk_mux = &gcc.mux, .mux = 135 },
	{ .name = "gcc_qupv3_wrap1_s4_clk", .clk_mux = &gcc.mux, .mux = 136 },
	{ .name = "gcc_qupv3_wrap1_s5_clk", .clk_mux = &gcc.mux, .mux = 137 },
	{ .name = "gcc_qupv3_wrap1_s6_clk", .clk_mux = &gcc.mux, .mux = 138 },
	{ .name = "gcc_qupv3_wrap1_s7_clk", .clk_mux = &gcc.mux, .mux = 139 },
	{ .name = "gcc_pdm_ahb_clk", .clk_mux = &gcc.mux, .mux = 140 },
	{ .name = "gcc_pdm_xo4_clk", .clk_mux = &gcc.mux, .mux = 141 },
	{ .name = "gcc_pdm2_clk", .clk_mux = &gcc.mux, .mux = 142 },
	{ .name = "gcc_prng_ahb_clk", .clk_mux = &gcc.mux, .mux = 143 },
	{ .name = "gcc_tsif_ahb_clk", .clk_mux = &gcc.mux, .mux = 144 },
	{ .name = "gcc_tsif_ref_clk", .clk_mux = &gcc.mux, .mux = 145 },
	{ .name = "gcc_tsif_inactivity_timers_clk", .clk_mux = &gcc.mux, .mux = 146 },
	{ .name = "gcc_boot_rom_ahb_clk", .clk_mux = &gcc.mux, .mux = 148 },
	{ .name = "gcc_ce1_clk", .clk_mux = &gcc.mux, .mux = 167 },
	{ .name = "gcc_ce1_axi_clk", .clk_mux = &gcc.mux, .mux = 168 },
	{ .name = "gcc_ce1_ahb_clk", .clk_mux = &gcc.mux, .mux = 169 },
	{ .name = "gcc_ddrss_gpu_axi_clk", .clk_mux = &gcc.mux, .mux = 187 },
	{ .name = "measure_only_bimc_clk", .clk_mux = &gcc.mux, .mux = 194 },
	{ .name = "gcc_cpuss_ahb_clk", .clk_mux = &gcc.mux, .mux = 206 },
	{ .name = "gcc_cpuss_gnoc_clk", .clk_mux = &gcc.mux, .mux = 207 },
	{ .name = "gcc_cpuss_rbcpr_clk", .clk_mux = &gcc.mux, .mux = 208 },
	{ .name = "gcc_cpuss_dvm_bus_clk", .clk_mux = &gcc.mux, .mux = 211 },
	{ .name = "pwrcl_clk", .clk_mux = &cpu, .mux = 68, .fixed_div = 16 },
	{ .name = "perfcl_clk", .clk_mux = &cpu, .mux = 69, .fixed_div = 16 },
	{ .name = "l3_clk", .clk_mux = &cpu, .mux = 70, .fixed_div = 16 },
	{ .name = "gcc_gp1_clk", .clk_mux = &gcc.mux, .mux = 222 },
	{ .name = "gcc_gp2_clk", .clk_mux = &gcc.mux, .mux = 223 },
	{ .name = "gcc_gp3_clk", .clk_mux = &gcc.mux, .mux = 224 },
	{ .name = "gcc_pcie_0_slv_q2a_axi_clk", .clk_mux = &gcc.mux, .mux = 225 },
	{ .name = "gcc_pcie_0_slv_axi_clk", .clk_mux = &gcc.mux, .mux = 226 },
	{ .name = "gcc_pcie_0_mstr_axi_clk", .clk_mux = &gcc.mux, .mux = 227 },
	{ .name = "gcc_pcie_0_cfg_ahb_clk", .clk_mux = &gcc.mux, .mux = 228 },
	{ .name = "gcc_pcie_0_aux_clk", .clk_mux = &gcc.mux, .mux = 229 },
	{ .name = "gcc_pcie_0_pipe_clk", .clk_mux = &gcc.mux, .mux = 230 },
	{ .name = "gcc_pcie_1_slv_q2a_axi_clk", .clk_mux = &gcc.mux, .mux = 232 },
	{ .name = "gcc_pcie_1_slv_axi_clk", .clk_mux = &gcc.mux, .mux = 233 },
	{ .name = "gcc_pcie_1_mstr_axi_clk", .clk_mux = &gcc.mux, .mux = 234 },
	{ .name = "gcc_pcie_1_cfg_ahb_clk", .clk_mux = &gcc.mux, .mux = 235 },
	{ .name = "gcc_pcie_1_aux_clk", .clk_mux = &gcc.mux, .mux = 236 },
	{ .name = "gcc_pcie_1_pipe_clk", .clk_mux = &gcc.mux, .mux = 237 },
	{ .name = "gcc_pcie_phy_aux_clk", .clk_mux = &gcc.mux, .mux = 239 },
	{ .name = "gcc_ufs_card_axi_clk", .clk_mux = &gcc.mux, .mux = 240 },
	{ .name = "gcc_ufs_card_ahb_clk", .clk_mux = &gcc.mux, .mux = 241 },
	{ .name = "gcc_ufs_card_tx_symbol_0_clk", .clk_mux = &gcc.mux, .mux = 242 },
	{ .name = "gcc_ufs_card_rx_symbol_0_clk", .clk_mux = &gcc.mux, .mux = 243 },
	{ .name = "gcc_ufs_card_unipro_core_clk", .clk_mux = &gcc.mux, .mux = 246 },
	{ .name = "gcc_ufs_card_ice_core_clk", .clk_mux = &gcc.mux, .mux = 247 },
	{ .name = "gcc_ufs_card_phy_aux_clk", .clk_mux = &gcc.mux, .mux = 248 },
	{ .name = "gcc_ufs_card_rx_symbol_1_clk", .clk_mux = &gcc.mux, .mux = 249 },
	{ .name = "gcc_ufs_phy_axi_clk", .clk_mux = &gcc.mux, .mux = 251 },
	{ .name = "gcc_ufs_phy_ahb_clk", .clk_mux = &gcc.mux, .mux = 252 },
	{ .name = "gcc_ufs_phy_tx_symbol_0_clk", .clk_mux = &gcc.mux, .mux = 253 },
	{ .name = "gcc_ufs_phy_rx_symbol_0_clk", .clk_mux = &gcc.mux, .mux = 254 },
	{ .name = "gcc_ufs_phy_unipro_core_clk", .clk_mux = &gcc.mux, .mux = 257 },
	{ .name = "gcc_ufs_phy_ice_core_clk", .clk_mux = &gcc.mux, .mux = 258 },
	{ .name = "gcc_ufs_phy_phy_aux_clk", .clk_mux = &gcc.mux, .mux = 259 },
	{ .name = "gcc_ufs_phy_rx_symbol_1_clk", .clk_mux = &gcc.mux, .mux = 260 },
	{ .name = "gcc_vddcx_vs_clk", .clk_mux = &gcc.mux, .mux = 268 },
	{ .name = "gcc_vddmx_vs_clk", .clk_mux = &gcc.mux, .mux = 269 },
	{ .name = "gcc_vdda_vs_clk", .clk_mux = &gcc.mux, .mux = 270 },
	{ .name = "gcc_vs_ctrl_clk", .clk_mux = &gcc.mux, .mux = 271 },
	{ .name = "gcc_vs_ctrl_ahb_clk", .clk_mux = &gcc.mux, .mux = 272 },
	{ .name = "gcc_mss_vs_clk", .clk_mux = &gcc.mux, .mux = 273 },
	{ .name = "gcc_gpu_vs_clk", .clk_mux = &gcc.mux, .mux = 274 },
	{ .name = "gcc_apc_vs_clk", .clk_mux = &gcc.mux, .mux = 275 },
	{ .name = "gcc_aggre_usb3_prim_axi_clk", .clk_mux = &gcc.mux, .mux = 283 },
	{ .name = "gcc_aggre_usb3_sec_axi_clk", .clk_mux = &gcc.mux, .mux = 284 },
	{ .name = "gcc_aggre_ufs_phy_axi_clk", .clk_mux = &gcc.mux, .mux = 285 },
	{ .name = "gcc_aggre_ufs_card_axi_clk", .clk_mux = &gcc.mux, .mux = 286 },
	{ .name = "measure_only_ipa_2x_clk", .clk_mux = &gcc.mux, .mux = 296 },
	{ .name = "gcc_mss_cfg_ahb_clk", .clk_mux = &gcc.mux, .mux = 301 },
	{ .name = "gcc_mss_mfab_axis_clk", .clk_mux = &gcc.mux, .mux = 302 },
	{ .name = "gcc_mss_axis2_clk", .clk_mux = &gcc.mux, .mux = 303 },
	{ .name = "gcc_mss_gpll0_div_clk_src", .clk_mux = &gcc.mux, .mux = 307 },
	{ .name = "gcc_mss_snoc_axi_clk", .clk_mux = &gcc.mux, .mux = 308 },
	{ .name = "gcc_mss_q6_memnoc_axi_clk", .clk_mux = &gcc.mux, .mux = 309 },
	{ .name = "gcc_gpu_cfg_ahb_clk", .clk_mux = &gcc.mux, .mux = 322 },
	{ .name = "gpu_cc_cxo_clk", .clk_mux = &gpu_cc, .mux = 10 },
	{ .name = "gpu_cc_cxo_aon_clk", .clk_mux = &gpu_cc, .mux = 11 },
	{ .name = "gpu_cc_gx_gfx3d_clk", .clk_mux = &gpu_cc, .mux = 12 },
	{ .name = "gpu_cc_gx_vsense_clk", .clk_mux = &gpu_cc, .mux = 13 },
	{ .name = "gpu_cc_gx_qdss_tsctr_clk", .clk_mux = &gpu_cc, .mux = 14 },
	{ .name = "gpu_cc_gx_gmu_clk", .clk_mux = &gpu_cc, .mux = 16 },
	{ .name = "gpu_cc_crc_ahb_clk", .clk_mux = &gpu_cc, .mux = 18 },
	{ .name = "gpu_cc_cx_qdss_at_clk", .clk_mux = &gpu_cc, .mux = 19 },
	{ .name = "gpu_cc_cx_qdss_tsctr_clk", .clk_mux = &gpu_cc, .mux = 20 },
	{ .name = "gpu_cc_cx_apb_clk", .clk_mux = &gpu_cc, .mux = 21 },
	{ .name = "gpu_cc_cx_snoc_dvm_clk", .clk_mux = &gpu_cc, .mux = 22 },
	{ .name = "gpu_cc_sleep_clk", .clk_mux = &gpu_cc, .mux = 23 },
	{ .name = "gpu_cc_cx_qdss_trig_clk", .clk_mux = &gpu_cc, .mux = 24 },
	{ .name = "gpu_cc_cx_gmu_clk", .clk_mux = &gpu_cc, .mux = 25 },
	{ .name = "gpu_cc_cx_gfx3d_clk", .clk_mux = &gpu_cc, .mux = 26 },
	{ .name = "gpu_cc_cx_gfx3d_slv_clk", .clk_mux = &gpu_cc, .mux = 27 },
	{ .name = "gpu_cc_rbcpr_clk", .clk_mux = &gpu_cc, .mux = 28 },
	{ .name = "gpu_cc_rbcpr_ahb_clk", .clk_mux = &gpu_cc, .mux = 29 },
	{ .name = "gpu_cc_acd_cxo_clk", .clk_mux = &gpu_cc, .mux = 31 },
	{ .name = "gcc_gpu_memnoc_gfx_clk", .clk_mux = &gcc.mux, .mux = 325 },
	{ .name = "gcc_gpu_snoc_dvm_gfx_clk", .clk_mux = &gcc.mux, .mux = 327 },
	{ .name = "gcc_gpu_gpll0_clk_src", .clk_mux = &gcc.mux, .mux = 328 },
	{ .name = "gcc_gpu_gpll0_div_clk_src", .clk_mux = &gcc.mux, .mux = 329 },
	{ .name = "gcc_sdcc1_apps_clk", .clk_mux = &gcc.mux, .mux = 347 },
	{ .name = "gcc_sdcc1_ahb_clk", .clk_mux = &gcc.mux, .mux = 348 },
	{ .name = "gcc_sdcc1_ice_core_clk", .clk_mux = &gcc.mux, .mux = 349 },
	{ .name = "gcc_pcie_phy_refgen_clk", .clk_mux = &gcc.mux, .mux = 352 },
	{}
};

struct debugcc_platform sdm845_debugcc = {
	.name = "sdm845",
	.clocks = sdm845_clocks,
};
