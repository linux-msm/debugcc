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
	{ "cam_cc_bps_ahb_clk", &cam_cc, 0x17 },
	{ "cam_cc_bps_clk", &cam_cc, 0x18 },
	{ "cam_cc_bps_fast_ahb_clk", &cam_cc, 0x16 },
	{ "cam_cc_camnoc_axi_clk", &cam_cc, 0x49 },
	{ "cam_cc_camnoc_dcd_xo_clk", &cam_cc, 0x4A },
	{ "cam_cc_cci_0_clk", &cam_cc, 0x44 },
	{ "cam_cc_cci_1_clk", &cam_cc, 0x45 },
	{ "cam_cc_core_ahb_clk", &cam_cc, 0x4D },
	{ "cam_cc_cpas_ahb_clk", &cam_cc, 0x46 },
	{ "cam_cc_cpas_bps_clk", &cam_cc, 0x19 },
	{ "cam_cc_cpas_fast_ahb_clk", &cam_cc, 0x47 },
	{ "cam_cc_cpas_ife_0_clk", &cam_cc, 0x25 },
	{ "cam_cc_cpas_ife_1_clk", &cam_cc, 0x2A },
	{ "cam_cc_cpas_ife_2_clk", &cam_cc, 0x2F },
	{ "cam_cc_cpas_ife_lite_clk", &cam_cc, 0x34 },
	{ "cam_cc_cpas_ipe_nps_clk", &cam_cc, 0x1B },
	{ "cam_cc_cpas_sbi_clk", &cam_cc, 0x22 },
	{ "cam_cc_cpas_sfe_0_clk", &cam_cc, 0x39 },
	{ "cam_cc_cpas_sfe_1_clk", &cam_cc, 0x3D },
	{ "cam_cc_csi0phytimer_clk", &cam_cc, 0x9 },
	{ "cam_cc_csi1phytimer_clk", &cam_cc, 0xC },
	{ "cam_cc_csi2phytimer_clk", &cam_cc, 0xE },
	{ "cam_cc_csi3phytimer_clk", &cam_cc, 0x10 },
	{ "cam_cc_csi4phytimer_clk", &cam_cc, 0x12 },
	{ "cam_cc_csi5phytimer_clk", &cam_cc, 0x14 },
	{ "cam_cc_csid_clk", &cam_cc, 0x48 },
	{ "cam_cc_csid_csiphy_rx_clk", &cam_cc, 0xB },
	{ "cam_cc_csiphy0_clk", &cam_cc, 0xA },
	{ "cam_cc_csiphy1_clk", &cam_cc, 0xD },
	{ "cam_cc_csiphy2_clk", &cam_cc, 0xF },
	{ "cam_cc_csiphy3_clk", &cam_cc, 0x11 },
	{ "cam_cc_csiphy4_clk", &cam_cc, 0x13 },
	{ "cam_cc_csiphy5_clk", &cam_cc, 0x15 },
	{ "cam_cc_gdsc_clk", &cam_cc, 0x4E },
	{ "cam_cc_icp_ahb_clk", &cam_cc, 0x43 },
	{ "cam_cc_icp_clk", &cam_cc, 0x42 },
	{ "cam_cc_ife_0_clk", &cam_cc, 0x24 },
	{ "cam_cc_ife_0_dsp_clk", &cam_cc, 0x26 },
	{ "cam_cc_ife_0_fast_ahb_clk", &cam_cc, 0x28 },
	{ "cam_cc_ife_1_clk", &cam_cc, 0x29 },
	{ "cam_cc_ife_1_dsp_clk", &cam_cc, 0x2B },
	{ "cam_cc_ife_1_fast_ahb_clk", &cam_cc, 0x2D },
	{ "cam_cc_ife_2_clk", &cam_cc, 0x2E },
	{ "cam_cc_ife_2_dsp_clk", &cam_cc, 0x30 },
	{ "cam_cc_ife_2_fast_ahb_clk", &cam_cc, 0x32 },
	{ "cam_cc_ife_lite_ahb_clk", &cam_cc, 0x37 },
	{ "cam_cc_ife_lite_clk", &cam_cc, 0x33 },
	{ "cam_cc_ife_lite_cphy_rx_clk", &cam_cc, 0x36 },
	{ "cam_cc_ife_lite_csid_clk", &cam_cc, 0x35 },
	{ "cam_cc_ipe_nps_ahb_clk", &cam_cc, 0x1E },
	{ "cam_cc_ipe_nps_clk", &cam_cc, 0x1A },
	{ "cam_cc_ipe_nps_fast_ahb_clk", &cam_cc, 0x1F },
	{ "cam_cc_ipe_pps_clk", &cam_cc, 0x1C },
	{ "cam_cc_ipe_pps_fast_ahb_clk", &cam_cc, 0x20 },
	{ "cam_cc_jpeg_clk", &cam_cc, 0x40 },
	{ "cam_cc_mclk0_clk", &cam_cc, 0x1 },
	{ "cam_cc_mclk1_clk", &cam_cc, 0x2 },
	{ "cam_cc_mclk2_clk", &cam_cc, 0x3 },
	{ "cam_cc_mclk3_clk", &cam_cc, 0x4 },
	{ "cam_cc_mclk4_clk", &cam_cc, 0x5 },
	{ "cam_cc_mclk5_clk", &cam_cc, 0x6 },
	{ "cam_cc_mclk6_clk", &cam_cc, 0x7 },
	{ "cam_cc_mclk7_clk", &cam_cc, 0x8 },
	{ "cam_cc_qdss_debug_clk", &cam_cc, 0x4B },
	{ "cam_cc_qdss_debug_xo_clk", &cam_cc, 0x4C },
	{ "cam_cc_sbi_ahb_clk", &cam_cc, 0x23 },
	{ "cam_cc_sbi_clk", &cam_cc, 0x21 },
	{ "cam_cc_sfe_0_clk", &cam_cc, 0x38 },
	{ "cam_cc_sfe_0_fast_ahb_clk", &cam_cc, 0x3B },
	{ "cam_cc_sfe_1_clk", &cam_cc, 0x3C },
	{ "cam_cc_sfe_1_fast_ahb_clk", &cam_cc, 0x3F },
	{ "cam_cc_sleep_clk", &cam_cc, 0x4F },

	{ "disp_cc_mdss_ahb1_clk", &disp_cc, 0x39 },
	{ "disp_cc_mdss_ahb_clk", &disp_cc, 0x34 },
	{ "disp_cc_mdss_byte0_clk", &disp_cc, 0x15 },
	{ "disp_cc_mdss_byte0_intf_clk", &disp_cc, 0x16 },
	{ "disp_cc_mdss_byte1_clk", &disp_cc, 0x17 },
	{ "disp_cc_mdss_byte1_intf_clk", &disp_cc, 0x18 },
	{ "disp_cc_mdss_dptx0_aux_clk", &disp_cc, 0x21 },
	{ "disp_cc_mdss_dptx0_crypto_clk", &disp_cc, 0x1E },
	{ "disp_cc_mdss_dptx0_link_clk", &disp_cc, 0x1B },
	{ "disp_cc_mdss_dptx0_link_intf_clk", &disp_cc, 0x1D },
	{ "disp_cc_mdss_dptx0_pixel0_clk", &disp_cc, 0x1F },
	{ "disp_cc_mdss_dptx0_pixel1_clk", &disp_cc, 0x20 },
	{ "disp_cc_mdss_dptx0_usb_router_link_intf_clk", &disp_cc, 0x1C },
	{ "disp_cc_mdss_dptx1_aux_clk", &disp_cc, 0x28 },
	{ "disp_cc_mdss_dptx1_crypto_clk", &disp_cc, 0x27 },
	{ "disp_cc_mdss_dptx1_link_clk", &disp_cc, 0x24 },
	{ "disp_cc_mdss_dptx1_link_intf_clk", &disp_cc, 0x26 },
	{ "disp_cc_mdss_dptx1_pixel0_clk", &disp_cc, 0x22 },
	{ "disp_cc_mdss_dptx1_pixel1_clk", &disp_cc, 0x23 },
	{ "disp_cc_mdss_dptx1_usb_router_link_intf_clk", &disp_cc, 0x25 },
	{ "disp_cc_mdss_dptx2_aux_clk", &disp_cc, 0x2E },
	{ "disp_cc_mdss_dptx2_crypto_clk", &disp_cc, 0x2D },
	{ "disp_cc_mdss_dptx2_link_clk", &disp_cc, 0x2B },
	{ "disp_cc_mdss_dptx2_link_intf_clk", &disp_cc, 0x2C },
	{ "disp_cc_mdss_dptx2_pixel0_clk", &disp_cc, 0x29 },
	{ "disp_cc_mdss_dptx2_pixel1_clk", &disp_cc, 0x2A },
	{ "disp_cc_mdss_dptx3_aux_clk", &disp_cc, 0x32 },
	{ "disp_cc_mdss_dptx3_crypto_clk", &disp_cc, 0x33 },
	{ "disp_cc_mdss_dptx3_link_clk", &disp_cc, 0x30 },
	{ "disp_cc_mdss_dptx3_link_intf_clk", &disp_cc, 0x31 },
	{ "disp_cc_mdss_dptx3_pixel0_clk", &disp_cc, 0x2F },
	{ "disp_cc_mdss_esc0_clk", &disp_cc, 0x19 },
	{ "disp_cc_mdss_esc1_clk", &disp_cc, 0x1A },
	{ "disp_cc_mdss_mdp1_clk", &disp_cc, 0x35 },
	{ "disp_cc_mdss_mdp_clk", &disp_cc, 0x11 },
	{ "disp_cc_mdss_mdp_lut1_clk", &disp_cc, 0x37 },
	{ "disp_cc_mdss_mdp_lut_clk", &disp_cc, 0x13 },
	{ "disp_cc_mdss_non_gdsc_ahb_clk", &disp_cc, 0x3A },
	{ "disp_cc_mdss_pclk0_clk", &disp_cc, 0xF },
	{ "disp_cc_mdss_pclk1_clk", &disp_cc, 0x10 },
	{ "disp_cc_mdss_rot1_clk", &disp_cc, 0x36 },
	{ "disp_cc_mdss_rot_clk", &disp_cc, 0x12 },
	{ "disp_cc_mdss_rscc_ahb_clk", &disp_cc, 0x3C },
	{ "disp_cc_mdss_rscc_vsync_clk", &disp_cc, 0x3B },
	{ "disp_cc_mdss_vsync1_clk", &disp_cc, 0x38 },
	{ "disp_cc_mdss_vsync_clk", &disp_cc, 0x14 },
	{ "disp_cc_sleep_clk", &disp_cc, 0x45 },
	{ "disp_cc_xo_clk", &disp_cc, 0x44 },

	{ "gcc_aggre_noc_pcie_0_axi_clk", &gcc.mux, 0x3D },
	{ "gcc_aggre_noc_pcie_1_axi_clk", &gcc.mux, 0x3E },
	{ "gcc_aggre_ufs_phy_axi_clk", &gcc.mux, 0x40 },
	{ "gcc_aggre_usb3_prim_axi_clk", &gcc.mux, 0x3F },
	{ "gcc_boot_rom_ahb_clk", &gcc.mux, 0xD9 },
	{ "gcc_camera_ahb_clk", &gcc.mux, 0x63 },
	{ "gcc_camera_hf_axi_clk", &gcc.mux, 0x66 },
	{ "gcc_camera_sf_axi_clk", &gcc.mux, 0x68 },
	{ "gcc_camera_xo_clk", &gcc.mux, 0x6A },
	{ "gcc_cfg_noc_pcie_anoc_ahb_clk", &gcc.mux, 0x2D },
	{ "gcc_cfg_noc_usb3_prim_axi_clk", &gcc.mux, 0x20 },
	{ "gcc_ddrss_gpu_axi_clk", &gcc.mux, 0xF5 },
	{ "gcc_ddrss_pcie_sf_tbu_clk", &gcc.mux, 0xF6 },
	{ "gcc_disp_ahb_clk", &gcc.mux, 0x6C },
	{ "gcc_disp_hf_axi_clk", &gcc.mux, 0x6E },
	{ "gcc_disp_sf_axi_clk", &gcc.mux, 0x6F },
	{ "gcc_disp_xo_clk", &gcc.mux, 0x70 },
	{ "gcc_gp1_clk", &gcc.mux, 0x122 },
	{ "gcc_gp2_clk", &gcc.mux, 0x123 },
	{ "gcc_gp3_clk", &gcc.mux, 0x124 },
	{ "gcc_gpu_cfg_ahb_clk", &gcc.mux, 0x16B },
	{ "gcc_gpu_gpll0_clk_src", &gcc.mux, 0x172 },
	{ "gcc_gpu_gpll0_div_clk_src", &gcc.mux, 0x173 },
	{ "gcc_gpu_memnoc_gfx_clk", &gcc.mux, 0x16F },
	{ "gcc_gpu_snoc_dvm_gfx_clk", &gcc.mux, 0x171 },
	{ "gcc_pcie_0_aux_clk", &gcc.mux, 0x12A },
	{ "gcc_pcie_0_cfg_ahb_clk", &gcc.mux, 0x129 },
	{ "gcc_pcie_0_mstr_axi_clk", &gcc.mux, 0x128 },
	{ "gcc_pcie_0_phy_rchng_clk", &gcc.mux, 0x12C },
	{ "gcc_pcie_0_pipe_clk", &gcc.mux, 0x12B },
	{ "gcc_pcie_0_slv_axi_clk", &gcc.mux, 0x127 },
	{ "gcc_pcie_0_slv_q2a_axi_clk", &gcc.mux, 0x126 },
	{ "gcc_pcie_1_aux_clk", &gcc.mux, 0x133 },
	{ "gcc_pcie_1_cfg_ahb_clk", &gcc.mux, 0x132 },
	{ "gcc_pcie_1_mstr_axi_clk", &gcc.mux, 0x131 },
	{ "gcc_pcie_1_phy_aux_clk", &gcc.mux, 0x134 },
	{ "gcc_pcie_1_phy_rchng_clk", &gcc.mux, 0x136 },
	{ "gcc_pcie_1_pipe_clk", &gcc.mux, 0x135 },
	{ "gcc_pcie_1_slv_axi_clk", &gcc.mux, 0x130 },
	{ "gcc_pcie_1_slv_q2a_axi_clk", &gcc.mux, 0x12F },
	{ "gcc_pdm2_clk", &gcc.mux, 0xCB },
	{ "gcc_pdm_ahb_clk", &gcc.mux, 0xC9 },
	{ "gcc_pdm_xo4_clk", &gcc.mux, 0xCA },
	{ "gcc_qmip_camera_nrt_ahb_clk", &gcc.mux, 0x64 },
	{ "gcc_qmip_camera_rt_ahb_clk", &gcc.mux, 0x65 },
	{ "gcc_qmip_disp_ahb_clk", &gcc.mux, 0x6D },
	{ "gcc_qmip_gpu_ahb_clk", &gcc.mux, 0x16C },
	{ "gcc_qmip_pcie_ahb_clk", &gcc.mux, 0x125 },
	{ "gcc_qmip_video_cv_cpu_ahb_clk", &gcc.mux, 0x76 },
	{ "gcc_qmip_video_cvp_ahb_clk", &gcc.mux, 0x73 },
	{ "gcc_qmip_video_v_cpu_ahb_clk", &gcc.mux, 0x75 },
	{ "gcc_qmip_video_vcodec_ahb_clk", &gcc.mux, 0x74 },
	{ "gcc_qupv3_wrap0_core_2x_clk", &gcc.mux, 0xAA },
	{ "gcc_qupv3_wrap0_core_clk", &gcc.mux, 0xA9 },
	{ "gcc_qupv3_wrap0_s0_clk", &gcc.mux, 0xAB },
	{ "gcc_qupv3_wrap0_s1_clk", &gcc.mux, 0xAC },
	{ "gcc_qupv3_wrap0_s2_clk", &gcc.mux, 0xAD },
	{ "gcc_qupv3_wrap0_s3_clk", &gcc.mux, 0xAE },
	{ "gcc_qupv3_wrap0_s4_clk", &gcc.mux, 0xAF },
	{ "gcc_qupv3_wrap0_s5_clk", &gcc.mux, 0xB0 },
	{ "gcc_qupv3_wrap0_s6_clk", &gcc.mux, 0xB1 },
	{ "gcc_qupv3_wrap0_s7_clk", &gcc.mux, 0xB2 },
	{ "gcc_qupv3_wrap1_core_2x_clk", &gcc.mux, 0xB6 },
	{ "gcc_qupv3_wrap1_core_clk", &gcc.mux, 0xB5 },
	{ "gcc_qupv3_wrap1_s0_clk", &gcc.mux, 0xB7 },
	{ "gcc_qupv3_wrap1_s1_clk", &gcc.mux, 0xB8 },
	{ "gcc_qupv3_wrap1_s2_clk", &gcc.mux, 0xB9 },
	{ "gcc_qupv3_wrap1_s3_clk", &gcc.mux, 0xBA },
	{ "gcc_qupv3_wrap1_s4_clk", &gcc.mux, 0xBB },
	{ "gcc_qupv3_wrap1_s5_clk", &gcc.mux, 0xBC },
	{ "gcc_qupv3_wrap1_s6_clk", &gcc.mux, 0xBD },
	{ "gcc_qupv3_wrap2_core_2x_clk", &gcc.mux, 0xC1 },
	{ "gcc_qupv3_wrap2_core_clk", &gcc.mux, 0xC0 },
	{ "gcc_qupv3_wrap2_s0_clk", &gcc.mux, 0xC2 },
	{ "gcc_qupv3_wrap2_s1_clk", &gcc.mux, 0xC3 },
	{ "gcc_qupv3_wrap2_s2_clk", &gcc.mux, 0xC4 },
	{ "gcc_qupv3_wrap2_s3_clk", &gcc.mux, 0xC5 },
	{ "gcc_qupv3_wrap2_s4_clk", &gcc.mux, 0xC6 },
	{ "gcc_qupv3_wrap2_s5_clk", &gcc.mux, 0xC7 },
	{ "gcc_qupv3_wrap2_s6_clk", &gcc.mux, 0xC8 },
	{ "gcc_qupv3_wrap_0_m_ahb_clk", &gcc.mux, 0xA7 },
	{ "gcc_qupv3_wrap_0_s_ahb_clk", &gcc.mux, 0xA8 },
	{ "gcc_qupv3_wrap_1_m_ahb_clk", &gcc.mux, 0xB3 },
	{ "gcc_qupv3_wrap_1_s_ahb_clk", &gcc.mux, 0xB4 },
	{ "gcc_qupv3_wrap_2_m_ahb_clk", &gcc.mux, 0xBE },
	{ "gcc_qupv3_wrap_2_s_ahb_clk", &gcc.mux, 0xBF },
	{ "gcc_sdcc2_ahb_clk", &gcc.mux, 0xA2 },
	{ "gcc_sdcc2_apps_clk", &gcc.mux, 0xA1 },
	{ "gcc_sdcc2_at_clk", &gcc.mux, 0xA3 },
	{ "gcc_sdcc4_ahb_clk", &gcc.mux, 0xA5 },
	{ "gcc_sdcc4_apps_clk", &gcc.mux, 0xA4 },
	{ "gcc_sdcc4_at_clk", &gcc.mux, 0xA6 },
	{ "gcc_ufs_phy_ahb_clk", &gcc.mux, 0x13B },
	{ "gcc_ufs_phy_axi_clk", &gcc.mux, 0x13A },
	{ "gcc_ufs_phy_ice_core_clk", &gcc.mux, 0x141 },
	{ "gcc_ufs_phy_phy_aux_clk", &gcc.mux, 0x142 },
	{ "gcc_ufs_phy_rx_symbol_0_clk", &gcc.mux, 0x13D },
	{ "gcc_ufs_phy_rx_symbol_1_clk", &gcc.mux, 0x143 },
	{ "gcc_ufs_phy_tx_symbol_0_clk", &gcc.mux, 0x13C },
	{ "gcc_ufs_phy_unipro_core_clk", &gcc.mux, 0x140 },
	{ "gcc_usb30_prim_master_clk", &gcc.mux, 0x94 },
	{ "gcc_usb30_prim_mock_utmi_clk", &gcc.mux, 0x96 },
	{ "gcc_usb30_prim_sleep_clk", &gcc.mux, 0x95 },
	{ "gcc_usb3_prim_phy_aux_clk", &gcc.mux, 0x97 },
	{ "gcc_usb3_prim_phy_com_aux_clk", &gcc.mux, 0x98 },
	{ "gcc_usb3_prim_phy_pipe_clk", &gcc.mux, 0x99 },
	{ "gcc_video_ahb_clk", &gcc.mux, 0x72 },
	{ "gcc_video_axi0_clk", &gcc.mux, 0x77 },
	{ "gcc_video_axi1_clk", &gcc.mux, 0x78 },
	{ "gcc_video_xo_clk", &gcc.mux, 0x79 },
	{ "measure_only_cnoc_clk", &gcc.mux, 0x19 },
	{ "measure_only_ipa_2x_clk", &gcc.mux, 0x158 },
	{ "measure_only_memnoc_clk", &gcc.mux, 0xFB },
	{ "measure_only_snoc_clk", &gcc.mux, 0xC },
	{ "pcie_0_pipe_clk", &gcc.mux, 0x12D },
	{ "pcie_1_phy_aux_clk", &gcc.mux, 0x138 },
	{ "pcie_1_pipe_clk", &gcc.mux, 0x137 },
	{ "ufs_phy_rx_symbol_0_clk", &gcc.mux, 0x13F },
	{ "ufs_phy_rx_symbol_1_clk", &gcc.mux, 0x144 },
	{ "ufs_phy_tx_symbol_0_clk", &gcc.mux, 0x13E },
	{ "usb3_phy_wrapper_gcc_usb30_pipe_clk", &gcc.mux, 0x9D },
	{ "mc_cc_debug_mux", &gcc.mux, 0x100 },

	{ "gpu_cc_ahb_clk", &gpu_cc, 0x16 },
	{ "gpu_cc_crc_ahb_clk", &gpu_cc, 0x17 },
	{ "gpu_cc_cx_apb_clk", &gpu_cc, 0x1A },
	{ "gpu_cc_cx_ff_clk", &gpu_cc, 0x21 },
	{ "gpu_cc_cx_gmu_clk", &gpu_cc, 0x1E },
	{ "gpu_cc_cx_snoc_dvm_clk", &gpu_cc, 0x1B },
	{ "gpu_cc_cxo_aon_clk", &gpu_cc, 0xB },
	{ "gpu_cc_cxo_clk", &gpu_cc, 0x1F },
	{ "gpu_cc_demet_clk", &gpu_cc, 0xD },
	{ "gpu_cc_freq_measure_clk", &gpu_cc, 0xC },
	{ "gpu_cc_gx_ff_clk", &gpu_cc, 0x13 },
	{ "gpu_cc_gx_gfx3d_rdvm_clk", &gpu_cc, 0x15 },
	{ "gpu_cc_gx_gmu_clk", &gpu_cc, 0x12 },
	{ "gpu_cc_gx_vsense_clk", &gpu_cc, 0xF },
	{ "gpu_cc_hub_aon_clk", &gpu_cc, 0x2F },
	{ "gpu_cc_hub_cx_int_clk", &gpu_cc, 0x20 },
	{ "gpu_cc_memnoc_gfx_clk", &gpu_cc, 0x22 },
	{ "gpu_cc_mnd1x_0_gfx3d_clk", &gpu_cc, 0x29 },
	{ "gpu_cc_mnd1x_1_gfx3d_clk", &gpu_cc, 0x2A },
	{ "gpu_cc_sleep_clk", &gpu_cc, 0x1C },
	{ "measure_only_gpu_cc_cx_gfx3d_clk", &gpu_cc, 0x25 },
	{ "measure_only_gpu_cc_cx_gfx3d_slv_clk", &gpu_cc, 0x26 },
	{ "measure_only_gpu_cc_gx_gfx3d_clk", &gpu_cc, 0xE },

	{ "video_cc_ahb_clk", &video_cc, 0x7 },
	{ "video_cc_mvs0_clk", &video_cc, 0x3 },
	{ "video_cc_mvs0c_clk", &video_cc, 0x1 },
	{ "video_cc_mvs1_clk", &video_cc, 0x5 },
	{ "video_cc_mvs1c_clk", &video_cc, 0x9 },
	{ "video_cc_sleep_clk", &video_cc, 0xC },
	{ "video_cc_xo_clk", &video_cc, 0xB },
	{}
};

struct debugcc_platform sm8450_debugcc = {
	"sm8450",
	sm8450_clocks,
};
