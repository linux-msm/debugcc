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

/* Enabling APSS can cause Bus error issues, so do not enable them by default */
#define ENABLE_SM8450_APSS_CLOCKS 0

static struct gcc_mux gcc = {
	.mux = {
		.phys =	0x100000,
		.size = 0x1f0000,

		.measure = measure_gcc,

		.enable_reg = 0x72008,
		.enable_mask = BIT(0),

		.mux_reg = 0x72000,
		.mux_mask = 0x3ff,

		.div_reg = 0x72004,
		.div_mask = 0xf,
		.div_val = 2,
	},

	.xo_div4_reg = 0x7200c,
	.debug_ctl_reg = 0x72038,
	.debug_status_reg = 0x7203c,
};

#if ENABLE_SM8450_APSS_CLOCKS
static struct debug_mux apss_cc = {
	.phys = 0x17a80000,
	.size = 0x21000,

	.measure = measure_leaf,
	.parent = &gcc.mux,
	.parent_mux_val = 0x117,

	.enable_reg = 0x20108,
	.enable_mask = BIT(0),

	.mux_reg = 0x20100,
	.mux_mask = 0xff,

	.div_reg = 0x20104,
	.div_mask = 0xf,
	.div_val = 4,
};
#endif

/* All leaf clocks are disabled for now untill we enable corresponding GDSCs */
static struct debug_mux cam_cc = {
	.phys = 0xade0000,
	.size = 0x20000,
	.block_name = "cam",

	.measure = measure_leaf,
	.parent = &gcc.mux,
	.parent_mux_val = 0x6b,

	.enable_reg = 0x14008,
	.enable_mask = BIT(0),

	.mux_reg = 0x16000,
	.mux_mask = 0xff,

	.div_reg = 0x14004,
	.div_mask = 0x3,
	.div_val = 4,
};

static struct debug_mux disp_cc = {
	.phys = 0xaf00000,
	.size = 0x20000,
	.block_name = "disp",

	.measure = measure_leaf,
	.parent = &gcc.mux,
	.parent_mux_val = 0x71,

	.enable_reg = 0xd00c,
	.enable_mask = BIT(0),

	.mux_reg = 0x11000,
	.mux_mask = 0xff,

	.div_reg = 0xd008,
	.div_mask = 0x3,
	.div_val = 4,
};

static struct debug_mux gpu_cc = {
	.phys = 0x3d90000,
	.size = 0xa000,
	.block_name = "gpu",

	.measure = measure_leaf,
	.parent = &gcc.mux,
	.parent_mux_val = 0x16e,

	.enable_reg = 0x9274,
	.enable_mask = BIT(0),

	.mux_reg = 0x9564,
	.mux_mask = 0xff,

	.div_reg = 0x9270,
	.div_mask = 0xf,
	.div_val = 2,
};

static struct debug_mux video_cc = {
	.phys = 0xaaf0000,
	.size = 0x10000,
	.block_name = "video",

	.measure = measure_leaf,
	.parent = &gcc.mux,
	.parent_mux_val = 0x7a,

	.enable_reg = 0x80ec,
	.enable_mask = BIT(0),

	.mux_reg = 0x9a4c,
	.mux_mask = 0x3f,

	.div_reg = 0x80eb,
	.div_mask = 0x7,
	.div_val = 3,
};

static struct measure_clk sm8450_clocks[] = {
#if ENABLE_SM8450_APSS_CLOCKS
	{ "measure_only_apcs_gold_post_acd_clk", &apss_cc, 0x4, 8 },
	{ "measure_only_apcs_goldplus_post_acd_clk", &apss_cc, 0x8, 8 },
	{ "measure_only_apcs_l3_post_acd_clk", &apss_cc, 0x6, 4 },
	{ "measure_only_apcs_silver_post_acd_clk", &apss_cc, 0x2, 4 },
#endif
	{ .name = "cam_cc_bps_ahb_clk", .clk_mux = &cam_cc, .mux = 0x17 },
	{ .name = "cam_cc_bps_clk", .clk_mux = &cam_cc, .mux = 0x18 },
	{ .name = "cam_cc_bps_fast_ahb_clk", .clk_mux = &cam_cc, .mux = 0x16 },
	{ .name = "cam_cc_camnoc_axi_clk", .clk_mux = &cam_cc, .mux = 0x49 },
	{ .name = "cam_cc_camnoc_dcd_xo_clk", .clk_mux = &cam_cc, .mux = 0x4A },
	{ .name = "cam_cc_cci_0_clk", .clk_mux = &cam_cc, .mux = 0x44 },
	{ .name = "cam_cc_cci_1_clk", .clk_mux = &cam_cc, .mux = 0x45 },
	{ .name = "cam_cc_core_ahb_clk", .clk_mux = &cam_cc, .mux = 0x4D },
	{ .name = "cam_cc_cpas_ahb_clk", .clk_mux = &cam_cc, .mux = 0x46 },
	{ .name = "cam_cc_cpas_bps_clk", .clk_mux = &cam_cc, .mux = 0x19 },
	{ .name = "cam_cc_cpas_fast_ahb_clk", .clk_mux = &cam_cc, .mux = 0x47 },
	{ .name = "cam_cc_cpas_ife_0_clk", .clk_mux = &cam_cc, .mux = 0x25 },
	{ .name = "cam_cc_cpas_ife_1_clk", .clk_mux = &cam_cc, .mux = 0x2A },
	{ .name = "cam_cc_cpas_ife_2_clk", .clk_mux = &cam_cc, .mux = 0x2F },
	{ .name = "cam_cc_cpas_ife_lite_clk", .clk_mux = &cam_cc, .mux = 0x34 },
	{ .name = "cam_cc_cpas_ipe_nps_clk", .clk_mux = &cam_cc, .mux = 0x1B },
	{ .name = "cam_cc_cpas_sbi_clk", .clk_mux = &cam_cc, .mux = 0x22 },
	{ .name = "cam_cc_cpas_sfe_0_clk", .clk_mux = &cam_cc, .mux = 0x39 },
	{ .name = "cam_cc_cpas_sfe_1_clk", .clk_mux = &cam_cc, .mux = 0x3D },
	{ .name = "cam_cc_csi0phytimer_clk", .clk_mux = &cam_cc, .mux = 0x9 },
	{ .name = "cam_cc_csi1phytimer_clk", .clk_mux = &cam_cc, .mux = 0xC },
	{ .name = "cam_cc_csi2phytimer_clk", .clk_mux = &cam_cc, .mux = 0xE },
	{ .name = "cam_cc_csi3phytimer_clk", .clk_mux = &cam_cc, .mux = 0x10 },
	{ .name = "cam_cc_csi4phytimer_clk", .clk_mux = &cam_cc, .mux = 0x12 },
	{ .name = "cam_cc_csi5phytimer_clk", .clk_mux = &cam_cc, .mux = 0x14 },
	{ .name = "cam_cc_csid_clk", .clk_mux = &cam_cc, .mux = 0x48 },
	{ .name = "cam_cc_csid_csiphy_rx_clk", .clk_mux = &cam_cc, .mux = 0xB },
	{ .name = "cam_cc_csiphy0_clk", .clk_mux = &cam_cc, .mux = 0xA },
	{ .name = "cam_cc_csiphy1_clk", .clk_mux = &cam_cc, .mux = 0xD },
	{ .name = "cam_cc_csiphy2_clk", .clk_mux = &cam_cc, .mux = 0xF },
	{ .name = "cam_cc_csiphy3_clk", .clk_mux = &cam_cc, .mux = 0x11 },
	{ .name = "cam_cc_csiphy4_clk", .clk_mux = &cam_cc, .mux = 0x13 },
	{ .name = "cam_cc_csiphy5_clk", .clk_mux = &cam_cc, .mux = 0x15 },
	{ .name = "cam_cc_gdsc_clk", .clk_mux = &cam_cc, .mux = 0x4E },
	{ .name = "cam_cc_icp_ahb_clk", .clk_mux = &cam_cc, .mux = 0x43 },
	{ .name = "cam_cc_icp_clk", .clk_mux = &cam_cc, .mux = 0x42 },
	{ .name = "cam_cc_ife_0_clk", .clk_mux = &cam_cc, .mux = 0x24 },
	{ .name = "cam_cc_ife_0_dsp_clk", .clk_mux = &cam_cc, .mux = 0x26 },
	{ .name = "cam_cc_ife_0_fast_ahb_clk", .clk_mux = &cam_cc, .mux = 0x28 },
	{ .name = "cam_cc_ife_1_clk", .clk_mux = &cam_cc, .mux = 0x29 },
	{ .name = "cam_cc_ife_1_dsp_clk", .clk_mux = &cam_cc, .mux = 0x2B },
	{ .name = "cam_cc_ife_1_fast_ahb_clk", .clk_mux = &cam_cc, .mux = 0x2D },
	{ .name = "cam_cc_ife_2_clk", .clk_mux = &cam_cc, .mux = 0x2E },
	{ .name = "cam_cc_ife_2_dsp_clk", .clk_mux = &cam_cc, .mux = 0x30 },
	{ .name = "cam_cc_ife_2_fast_ahb_clk", .clk_mux = &cam_cc, .mux = 0x32 },
	{ .name = "cam_cc_ife_lite_ahb_clk", .clk_mux = &cam_cc, .mux = 0x37 },
	{ .name = "cam_cc_ife_lite_clk", .clk_mux = &cam_cc, .mux = 0x33 },
	{ .name = "cam_cc_ife_lite_cphy_rx_clk", .clk_mux = &cam_cc, .mux = 0x36 },
	{ .name = "cam_cc_ife_lite_csid_clk", .clk_mux = &cam_cc, .mux = 0x35 },
	{ .name = "cam_cc_ipe_nps_ahb_clk", .clk_mux = &cam_cc, .mux = 0x1E },
	{ .name = "cam_cc_ipe_nps_clk", .clk_mux = &cam_cc, .mux = 0x1A },
	{ .name = "cam_cc_ipe_nps_fast_ahb_clk", .clk_mux = &cam_cc, .mux = 0x1F },
	{ .name = "cam_cc_ipe_pps_clk", .clk_mux = &cam_cc, .mux = 0x1C },
	{ .name = "cam_cc_ipe_pps_fast_ahb_clk", .clk_mux = &cam_cc, .mux = 0x20 },
	{ .name = "cam_cc_jpeg_clk", .clk_mux = &cam_cc, .mux = 0x40 },
	{ .name = "cam_cc_mclk0_clk", .clk_mux = &cam_cc, .mux = 0x1 },
	{ .name = "cam_cc_mclk1_clk", .clk_mux = &cam_cc, .mux = 0x2 },
	{ .name = "cam_cc_mclk2_clk", .clk_mux = &cam_cc, .mux = 0x3 },
	{ .name = "cam_cc_mclk3_clk", .clk_mux = &cam_cc, .mux = 0x4 },
	{ .name = "cam_cc_mclk4_clk", .clk_mux = &cam_cc, .mux = 0x5 },
	{ .name = "cam_cc_mclk5_clk", .clk_mux = &cam_cc, .mux = 0x6 },
	{ .name = "cam_cc_mclk6_clk", .clk_mux = &cam_cc, .mux = 0x7 },
	{ .name = "cam_cc_mclk7_clk", .clk_mux = &cam_cc, .mux = 0x8 },
	{ .name = "cam_cc_qdss_debug_clk", .clk_mux = &cam_cc, .mux = 0x4B },
	{ .name = "cam_cc_qdss_debug_xo_clk", .clk_mux = &cam_cc, .mux = 0x4C },
	{ .name = "cam_cc_sbi_ahb_clk", .clk_mux = &cam_cc, .mux = 0x23 },
	{ .name = "cam_cc_sbi_clk", .clk_mux = &cam_cc, .mux = 0x21 },
	{ .name = "cam_cc_sfe_0_clk", .clk_mux = &cam_cc, .mux = 0x38 },
	{ .name = "cam_cc_sfe_0_fast_ahb_clk", .clk_mux = &cam_cc, .mux = 0x3B },
	{ .name = "cam_cc_sfe_1_clk", .clk_mux = &cam_cc, .mux = 0x3C },
	{ .name = "cam_cc_sfe_1_fast_ahb_clk", .clk_mux = &cam_cc, .mux = 0x3F },
	{ .name = "cam_cc_sleep_clk", .clk_mux = &cam_cc, .mux = 0x4F },

	{ .name = "disp_cc_mdss_ahb1_clk", .clk_mux = &disp_cc, .mux = 0x39 },
	{ .name = "disp_cc_mdss_ahb_clk", .clk_mux = &disp_cc, .mux = 0x34 },
	{ .name = "disp_cc_mdss_byte0_clk", .clk_mux = &disp_cc, .mux = 0x15 },
	{ .name = "disp_cc_mdss_byte0_intf_clk", .clk_mux = &disp_cc, .mux = 0x16 },
	{ .name = "disp_cc_mdss_byte1_clk", .clk_mux = &disp_cc, .mux = 0x17 },
	{ .name = "disp_cc_mdss_byte1_intf_clk", .clk_mux = &disp_cc, .mux = 0x18 },
	{ .name = "disp_cc_mdss_dptx0_aux_clk", .clk_mux = &disp_cc, .mux = 0x21 },
	{ .name = "disp_cc_mdss_dptx0_crypto_clk", .clk_mux = &disp_cc, .mux = 0x1E },
	{ .name = "disp_cc_mdss_dptx0_link_clk", .clk_mux = &disp_cc, .mux = 0x1B },
	{ .name = "disp_cc_mdss_dptx0_link_intf_clk", .clk_mux = &disp_cc, .mux = 0x1D },
	{ .name = "disp_cc_mdss_dptx0_pixel0_clk", .clk_mux = &disp_cc, .mux = 0x1F },
	{ .name = "disp_cc_mdss_dptx0_pixel1_clk", .clk_mux = &disp_cc, .mux = 0x20 },
	{ .name = "disp_cc_mdss_dptx0_usb_router_link_intf_clk", .clk_mux = &disp_cc, .mux = 0x1C },
	{ .name = "disp_cc_mdss_dptx1_aux_clk", .clk_mux = &disp_cc, .mux = 0x28 },
	{ .name = "disp_cc_mdss_dptx1_crypto_clk", .clk_mux = &disp_cc, .mux = 0x27 },
	{ .name = "disp_cc_mdss_dptx1_link_clk", .clk_mux = &disp_cc, .mux = 0x24 },
	{ .name = "disp_cc_mdss_dptx1_link_intf_clk", .clk_mux = &disp_cc, .mux = 0x26 },
	{ .name = "disp_cc_mdss_dptx1_pixel0_clk", .clk_mux = &disp_cc, .mux = 0x22 },
	{ .name = "disp_cc_mdss_dptx1_pixel1_clk", .clk_mux = &disp_cc, .mux = 0x23 },
	{ .name = "disp_cc_mdss_dptx1_usb_router_link_intf_clk", .clk_mux = &disp_cc, .mux = 0x25 },
	{ .name = "disp_cc_mdss_dptx2_aux_clk", .clk_mux = &disp_cc, .mux = 0x2E },
	{ .name = "disp_cc_mdss_dptx2_crypto_clk", .clk_mux = &disp_cc, .mux = 0x2D },
	{ .name = "disp_cc_mdss_dptx2_link_clk", .clk_mux = &disp_cc, .mux = 0x2B },
	{ .name = "disp_cc_mdss_dptx2_link_intf_clk", .clk_mux = &disp_cc, .mux = 0x2C },
	{ .name = "disp_cc_mdss_dptx2_pixel0_clk", .clk_mux = &disp_cc, .mux = 0x29 },
	{ .name = "disp_cc_mdss_dptx2_pixel1_clk", .clk_mux = &disp_cc, .mux = 0x2A },
	{ .name = "disp_cc_mdss_dptx3_aux_clk", .clk_mux = &disp_cc, .mux = 0x32 },
	{ .name = "disp_cc_mdss_dptx3_crypto_clk", .clk_mux = &disp_cc, .mux = 0x33 },
	{ .name = "disp_cc_mdss_dptx3_link_clk", .clk_mux = &disp_cc, .mux = 0x30 },
	{ .name = "disp_cc_mdss_dptx3_link_intf_clk", .clk_mux = &disp_cc, .mux = 0x31 },
	{ .name = "disp_cc_mdss_dptx3_pixel0_clk", .clk_mux = &disp_cc, .mux = 0x2F },
	{ .name = "disp_cc_mdss_esc0_clk", .clk_mux = &disp_cc, .mux = 0x19 },
	{ .name = "disp_cc_mdss_esc1_clk", .clk_mux = &disp_cc, .mux = 0x1A },
	{ .name = "disp_cc_mdss_mdp1_clk", .clk_mux = &disp_cc, .mux = 0x35 },
	{ .name = "disp_cc_mdss_mdp_clk", .clk_mux = &disp_cc, .mux = 0x11 },
	{ .name = "disp_cc_mdss_mdp_lut1_clk", .clk_mux = &disp_cc, .mux = 0x37 },
	{ .name = "disp_cc_mdss_mdp_lut_clk", .clk_mux = &disp_cc, .mux = 0x13 },
	{ .name = "disp_cc_mdss_non_gdsc_ahb_clk", .clk_mux = &disp_cc, .mux = 0x3A },
	{ .name = "disp_cc_mdss_pclk0_clk", .clk_mux = &disp_cc, .mux = 0xF },
	{ .name = "disp_cc_mdss_pclk1_clk", .clk_mux = &disp_cc, .mux = 0x10 },
	{ .name = "disp_cc_mdss_rot1_clk", .clk_mux = &disp_cc, .mux = 0x36 },
	{ .name = "disp_cc_mdss_rot_clk", .clk_mux = &disp_cc, .mux = 0x12 },
	{ .name = "disp_cc_mdss_rscc_ahb_clk", .clk_mux = &disp_cc, .mux = 0x3C },
	{ .name = "disp_cc_mdss_rscc_vsync_clk", .clk_mux = &disp_cc, .mux = 0x3B },
	{ .name = "disp_cc_mdss_vsync1_clk", .clk_mux = &disp_cc, .mux = 0x38 },
	{ .name = "disp_cc_mdss_vsync_clk", .clk_mux = &disp_cc, .mux = 0x14 },
	{ .name = "disp_cc_sleep_clk", .clk_mux = &disp_cc, .mux = 0x45 },
	{ .name = "disp_cc_xo_clk", .clk_mux = &disp_cc, .mux = 0x44 },

	{ .name = "gcc_aggre_noc_pcie_0_axi_clk", .clk_mux = &gcc.mux, .mux = 0x3D },
	{ .name = "gcc_aggre_noc_pcie_1_axi_clk", .clk_mux = &gcc.mux, .mux = 0x3E },
	{ .name = "gcc_aggre_ufs_phy_axi_clk", .clk_mux = &gcc.mux, .mux = 0x40 },
	{ .name = "gcc_aggre_usb3_prim_axi_clk", .clk_mux = &gcc.mux, .mux = 0x3F },
	{ .name = "gcc_boot_rom_ahb_clk", .clk_mux = &gcc.mux, .mux = 0xD9 },
	{ .name = "gcc_camera_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x63 },
	{ .name = "gcc_camera_hf_axi_clk", .clk_mux = &gcc.mux, .mux = 0x66 },
	{ .name = "gcc_camera_sf_axi_clk", .clk_mux = &gcc.mux, .mux = 0x68 },
	{ .name = "gcc_camera_xo_clk", .clk_mux = &gcc.mux, .mux = 0x6A },
	{ .name = "gcc_cfg_noc_pcie_anoc_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x2D },
	{ .name = "gcc_cfg_noc_usb3_prim_axi_clk", .clk_mux = &gcc.mux, .mux = 0x20 },
	{ .name = "gcc_ddrss_gpu_axi_clk", .clk_mux = &gcc.mux, .mux = 0xF5 },
	{ .name = "gcc_ddrss_pcie_sf_tbu_clk", .clk_mux = &gcc.mux, .mux = 0xF6 },
	{ .name = "gcc_disp_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x6C },
	{ .name = "gcc_disp_hf_axi_clk", .clk_mux = &gcc.mux, .mux = 0x6E },
	{ .name = "gcc_disp_sf_axi_clk", .clk_mux = &gcc.mux, .mux = 0x6F },
	{ .name = "gcc_disp_xo_clk", .clk_mux = &gcc.mux, .mux = 0x70 },
	{ .name = "gcc_gp1_clk", .clk_mux = &gcc.mux, .mux = 0x122 },
	{ .name = "gcc_gp2_clk", .clk_mux = &gcc.mux, .mux = 0x123 },
	{ .name = "gcc_gp3_clk", .clk_mux = &gcc.mux, .mux = 0x124 },
	{ .name = "gcc_gpu_cfg_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x16B },
	{ .name = "gcc_gpu_gpll0_clk_src", .clk_mux = &gcc.mux, .mux = 0x172 },
	{ .name = "gcc_gpu_gpll0_div_clk_src", .clk_mux = &gcc.mux, .mux = 0x173 },
	{ .name = "gcc_gpu_memnoc_gfx_clk", .clk_mux = &gcc.mux, .mux = 0x16F },
	{ .name = "gcc_gpu_snoc_dvm_gfx_clk", .clk_mux = &gcc.mux, .mux = 0x171 },
	{ .name = "gcc_pcie_0_aux_clk", .clk_mux = &gcc.mux, .mux = 0x12A },
	{ .name = "gcc_pcie_0_cfg_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x129 },
	{ .name = "gcc_pcie_0_mstr_axi_clk", .clk_mux = &gcc.mux, .mux = 0x128 },
	{ .name = "gcc_pcie_0_phy_rchng_clk", .clk_mux = &gcc.mux, .mux = 0x12C },
	{ .name = "gcc_pcie_0_pipe_clk", .clk_mux = &gcc.mux, .mux = 0x12B },
	{ .name = "gcc_pcie_0_slv_axi_clk", .clk_mux = &gcc.mux, .mux = 0x127 },
	{ .name = "gcc_pcie_0_slv_q2a_axi_clk", .clk_mux = &gcc.mux, .mux = 0x126 },
	{ .name = "gcc_pcie_1_aux_clk", .clk_mux = &gcc.mux, .mux = 0x133 },
	{ .name = "gcc_pcie_1_cfg_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x132 },
	{ .name = "gcc_pcie_1_mstr_axi_clk", .clk_mux = &gcc.mux, .mux = 0x131 },
	{ .name = "gcc_pcie_1_phy_aux_clk", .clk_mux = &gcc.mux, .mux = 0x134 },
	{ .name = "gcc_pcie_1_phy_rchng_clk", .clk_mux = &gcc.mux, .mux = 0x136 },
	{ .name = "gcc_pcie_1_pipe_clk", .clk_mux = &gcc.mux, .mux = 0x135 },
	{ .name = "gcc_pcie_1_slv_axi_clk", .clk_mux = &gcc.mux, .mux = 0x130 },
	{ .name = "gcc_pcie_1_slv_q2a_axi_clk", .clk_mux = &gcc.mux, .mux = 0x12F },
	{ .name = "gcc_pdm2_clk", .clk_mux = &gcc.mux, .mux = 0xCB },
	{ .name = "gcc_pdm_ahb_clk", .clk_mux = &gcc.mux, .mux = 0xC9 },
	{ .name = "gcc_pdm_xo4_clk", .clk_mux = &gcc.mux, .mux = 0xCA },
	{ .name = "gcc_qmip_camera_nrt_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x64 },
	{ .name = "gcc_qmip_camera_rt_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x65 },
	{ .name = "gcc_qmip_disp_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x6D },
	{ .name = "gcc_qmip_gpu_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x16C },
	{ .name = "gcc_qmip_pcie_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x125 },
	{ .name = "gcc_qmip_video_cv_cpu_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x76 },
	{ .name = "gcc_qmip_video_cvp_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x73 },
	{ .name = "gcc_qmip_video_v_cpu_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x75 },
	{ .name = "gcc_qmip_video_vcodec_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x74 },
	{ .name = "gcc_qupv3_wrap0_core_2x_clk", .clk_mux = &gcc.mux, .mux = 0xAA },
	{ .name = "gcc_qupv3_wrap0_core_clk", .clk_mux = &gcc.mux, .mux = 0xA9 },
	{ .name = "gcc_qupv3_wrap0_s0_clk", .clk_mux = &gcc.mux, .mux = 0xAB },
	{ .name = "gcc_qupv3_wrap0_s1_clk", .clk_mux = &gcc.mux, .mux = 0xAC },
	{ .name = "gcc_qupv3_wrap0_s2_clk", .clk_mux = &gcc.mux, .mux = 0xAD },
	{ .name = "gcc_qupv3_wrap0_s3_clk", .clk_mux = &gcc.mux, .mux = 0xAE },
	{ .name = "gcc_qupv3_wrap0_s4_clk", .clk_mux = &gcc.mux, .mux = 0xAF },
	{ .name = "gcc_qupv3_wrap0_s5_clk", .clk_mux = &gcc.mux, .mux = 0xB0 },
	{ .name = "gcc_qupv3_wrap0_s6_clk", .clk_mux = &gcc.mux, .mux = 0xB1 },
	{ .name = "gcc_qupv3_wrap0_s7_clk", .clk_mux = &gcc.mux, .mux = 0xB2 },
	{ .name = "gcc_qupv3_wrap1_core_2x_clk", .clk_mux = &gcc.mux, .mux = 0xB6 },
	{ .name = "gcc_qupv3_wrap1_core_clk", .clk_mux = &gcc.mux, .mux = 0xB5 },
	{ .name = "gcc_qupv3_wrap1_s0_clk", .clk_mux = &gcc.mux, .mux = 0xB7 },
	{ .name = "gcc_qupv3_wrap1_s1_clk", .clk_mux = &gcc.mux, .mux = 0xB8 },
	{ .name = "gcc_qupv3_wrap1_s2_clk", .clk_mux = &gcc.mux, .mux = 0xB9 },
	{ .name = "gcc_qupv3_wrap1_s3_clk", .clk_mux = &gcc.mux, .mux = 0xBA },
	{ .name = "gcc_qupv3_wrap1_s4_clk", .clk_mux = &gcc.mux, .mux = 0xBB },
	{ .name = "gcc_qupv3_wrap1_s5_clk", .clk_mux = &gcc.mux, .mux = 0xBC },
	{ .name = "gcc_qupv3_wrap1_s6_clk", .clk_mux = &gcc.mux, .mux = 0xBD },
	{ .name = "gcc_qupv3_wrap2_core_2x_clk", .clk_mux = &gcc.mux, .mux = 0xC1 },
	{ .name = "gcc_qupv3_wrap2_core_clk", .clk_mux = &gcc.mux, .mux = 0xC0 },
	{ .name = "gcc_qupv3_wrap2_s0_clk", .clk_mux = &gcc.mux, .mux = 0xC2 },
	{ .name = "gcc_qupv3_wrap2_s1_clk", .clk_mux = &gcc.mux, .mux = 0xC3 },
	{ .name = "gcc_qupv3_wrap2_s2_clk", .clk_mux = &gcc.mux, .mux = 0xC4 },
	{ .name = "gcc_qupv3_wrap2_s3_clk", .clk_mux = &gcc.mux, .mux = 0xC5 },
	{ .name = "gcc_qupv3_wrap2_s4_clk", .clk_mux = &gcc.mux, .mux = 0xC6 },
	{ .name = "gcc_qupv3_wrap2_s5_clk", .clk_mux = &gcc.mux, .mux = 0xC7 },
	{ .name = "gcc_qupv3_wrap2_s6_clk", .clk_mux = &gcc.mux, .mux = 0xC8 },
	{ .name = "gcc_qupv3_wrap_0_m_ahb_clk", .clk_mux = &gcc.mux, .mux = 0xA7 },
	{ .name = "gcc_qupv3_wrap_0_s_ahb_clk", .clk_mux = &gcc.mux, .mux = 0xA8 },
	{ .name = "gcc_qupv3_wrap_1_m_ahb_clk", .clk_mux = &gcc.mux, .mux = 0xB3 },
	{ .name = "gcc_qupv3_wrap_1_s_ahb_clk", .clk_mux = &gcc.mux, .mux = 0xB4 },
	{ .name = "gcc_qupv3_wrap_2_m_ahb_clk", .clk_mux = &gcc.mux, .mux = 0xBE },
	{ .name = "gcc_qupv3_wrap_2_s_ahb_clk", .clk_mux = &gcc.mux, .mux = 0xBF },
	{ .name = "gcc_sdcc2_ahb_clk", .clk_mux = &gcc.mux, .mux = 0xA2 },
	{ .name = "gcc_sdcc2_apps_clk", .clk_mux = &gcc.mux, .mux = 0xA1 },
	{ .name = "gcc_sdcc2_at_clk", .clk_mux = &gcc.mux, .mux = 0xA3 },
	{ .name = "gcc_sdcc4_ahb_clk", .clk_mux = &gcc.mux, .mux = 0xA5 },
	{ .name = "gcc_sdcc4_apps_clk", .clk_mux = &gcc.mux, .mux = 0xA4 },
	{ .name = "gcc_sdcc4_at_clk", .clk_mux = &gcc.mux, .mux = 0xA6 },
	{ .name = "gcc_ufs_phy_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x13B },
	{ .name = "gcc_ufs_phy_axi_clk", .clk_mux = &gcc.mux, .mux = 0x13A },
	{ .name = "gcc_ufs_phy_ice_core_clk", .clk_mux = &gcc.mux, .mux = 0x141 },
	{ .name = "gcc_ufs_phy_phy_aux_clk", .clk_mux = &gcc.mux, .mux = 0x142 },
	{ .name = "gcc_ufs_phy_rx_symbol_0_clk", .clk_mux = &gcc.mux, .mux = 0x13D },
	{ .name = "gcc_ufs_phy_rx_symbol_1_clk", .clk_mux = &gcc.mux, .mux = 0x143 },
	{ .name = "gcc_ufs_phy_tx_symbol_0_clk", .clk_mux = &gcc.mux, .mux = 0x13C },
	{ .name = "gcc_ufs_phy_unipro_core_clk", .clk_mux = &gcc.mux, .mux = 0x140 },
	{ .name = "gcc_usb30_prim_master_clk", .clk_mux = &gcc.mux, .mux = 0x94 },
	{ .name = "gcc_usb30_prim_mock_utmi_clk", .clk_mux = &gcc.mux, .mux = 0x96 },
	{ .name = "gcc_usb30_prim_sleep_clk", .clk_mux = &gcc.mux, .mux = 0x95 },
	{ .name = "gcc_usb3_prim_phy_aux_clk", .clk_mux = &gcc.mux, .mux = 0x97 },
	{ .name = "gcc_usb3_prim_phy_com_aux_clk", .clk_mux = &gcc.mux, .mux = 0x98 },
	{ .name = "gcc_usb3_prim_phy_pipe_clk", .clk_mux = &gcc.mux, .mux = 0x99 },
	{ .name = "gcc_video_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x72 },
	{ .name = "gcc_video_axi0_clk", .clk_mux = &gcc.mux, .mux = 0x77 },
	{ .name = "gcc_video_axi1_clk", .clk_mux = &gcc.mux, .mux = 0x78 },
	{ .name = "gcc_video_xo_clk", .clk_mux = &gcc.mux, .mux = 0x79 },
	{ .name = "measure_only_cnoc_clk", .clk_mux = &gcc.mux, .mux = 0x19 },
	{ .name = "measure_only_ipa_2x_clk", .clk_mux = &gcc.mux, .mux = 0x158 },
	{ .name = "measure_only_memnoc_clk", .clk_mux = &gcc.mux, .mux = 0xFB },
	{ .name = "measure_only_snoc_clk", .clk_mux = &gcc.mux, .mux = 0xC },
	{ .name = "pcie_0_pipe_clk", .clk_mux = &gcc.mux, .mux = 0x12D },
	{ .name = "pcie_1_phy_aux_clk", .clk_mux = &gcc.mux, .mux = 0x138 },
	{ .name = "pcie_1_pipe_clk", .clk_mux = &gcc.mux, .mux = 0x137 },
	{ .name = "ufs_phy_rx_symbol_0_clk", .clk_mux = &gcc.mux, .mux = 0x13F },
	{ .name = "ufs_phy_rx_symbol_1_clk", .clk_mux = &gcc.mux, .mux = 0x144 },
	{ .name = "ufs_phy_tx_symbol_0_clk", .clk_mux = &gcc.mux, .mux = 0x13E },
	{ .name = "usb3_phy_wrapper_gcc_usb30_pipe_clk", .clk_mux = &gcc.mux, .mux = 0x9D },
	{ .name = "mc_cc_debug_mux", .clk_mux = &gcc.mux, .mux = 0x100 },

	{ .name = "gpu_cc_ahb_clk", .clk_mux = &gpu_cc, .mux = 0x16 },
	{ .name = "gpu_cc_crc_ahb_clk", .clk_mux = &gpu_cc, .mux = 0x17 },
	{ .name = "gpu_cc_cx_apb_clk", .clk_mux = &gpu_cc, .mux = 0x1A },
	{ .name = "gpu_cc_cx_ff_clk", .clk_mux = &gpu_cc, .mux = 0x21 },
	{ .name = "gpu_cc_cx_gmu_clk", .clk_mux = &gpu_cc, .mux = 0x1E },
	{ .name = "gpu_cc_cx_snoc_dvm_clk", .clk_mux = &gpu_cc, .mux = 0x1B },
	{ .name = "gpu_cc_cxo_aon_clk", .clk_mux = &gpu_cc, .mux = 0xB },
	{ .name = "gpu_cc_cxo_clk", .clk_mux = &gpu_cc, .mux = 0x1F },
	{ .name = "gpu_cc_demet_clk", .clk_mux = &gpu_cc, .mux = 0xD },
	{ .name = "gpu_cc_freq_measure_clk", .clk_mux = &gpu_cc, .mux = 0xC },
	{ .name = "gpu_cc_gx_ff_clk", .clk_mux = &gpu_cc, .mux = 0x13 },
	{ .name = "gpu_cc_gx_gfx3d_rdvm_clk", .clk_mux = &gpu_cc, .mux = 0x15 },
	{ .name = "gpu_cc_gx_gmu_clk", .clk_mux = &gpu_cc, .mux = 0x12 },
	{ .name = "gpu_cc_gx_vsense_clk", .clk_mux = &gpu_cc, .mux = 0xF },
	{ .name = "gpu_cc_hub_aon_clk", .clk_mux = &gpu_cc, .mux = 0x2F },
	{ .name = "gpu_cc_hub_cx_int_clk", .clk_mux = &gpu_cc, .mux = 0x20 },
	{ .name = "gpu_cc_memnoc_gfx_clk", .clk_mux = &gpu_cc, .mux = 0x22 },
	{ .name = "gpu_cc_mnd1x_0_gfx3d_clk", .clk_mux = &gpu_cc, .mux = 0x29 },
	{ .name = "gpu_cc_mnd1x_1_gfx3d_clk", .clk_mux = &gpu_cc, .mux = 0x2A },
	{ .name = "gpu_cc_sleep_clk", .clk_mux = &gpu_cc, .mux = 0x1C },
	{ .name = "measure_only_gpu_cc_cx_gfx3d_clk", .clk_mux = &gpu_cc, .mux = 0x25 },
	{ .name = "measure_only_gpu_cc_cx_gfx3d_slv_clk", .clk_mux = &gpu_cc, .mux = 0x26 },
	{ .name = "measure_only_gpu_cc_gx_gfx3d_clk", .clk_mux = &gpu_cc, .mux = 0xE },

	{ .name = "video_cc_ahb_clk", .clk_mux = &video_cc, .mux = 0x7 },
	{ .name = "video_cc_mvs0_clk", .clk_mux = &video_cc, .mux = 0x3 },
	{ .name = "video_cc_mvs0c_clk", .clk_mux = &video_cc, .mux = 0x1 },
	{ .name = "video_cc_mvs1_clk", .clk_mux = &video_cc, .mux = 0x5 },
	{ .name = "video_cc_mvs1c_clk", .clk_mux = &video_cc, .mux = 0x9 },
	{ .name = "video_cc_sleep_clk", .clk_mux = &video_cc, .mux = 0xC },
	{ .name = "video_cc_xo_clk", .clk_mux = &video_cc, .mux = 0xB },
	{}
};

struct debugcc_platform sm8450_debugcc = {
	.name = "sm8450",
	.clocks = sm8450_clocks,
};
