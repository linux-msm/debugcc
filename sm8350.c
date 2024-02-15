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

static struct gcc_mux gcc = {
	.mux = {
		.phys =	0x162000,
		.size = 0x1f0000,

		.measure = measure_gcc,

		.enable_reg = 0x8,
		.enable_mask = BIT(0),

		.mux_reg = 0x0,
		.mux_mask = 0x3ff,

		.div_reg = 0x4,
		.div_mask = 0xf,
		.div_val = 2,
	},

	.xo_div4_reg = 0xc,
	.debug_ctl_reg = 0x38,
	.debug_status_reg = 0x3c,
};

static struct debug_mux cam_cc = {
	.phys = 0xad00000,
	.size = 0x10000,
	.block_name = "cam",

	.measure = measure_leaf,
	.parent = &gcc.mux,
	.parent_mux_val = 0x4d,

	.enable_reg = 0xd008,
	.enable_mask = BIT(0),

	.mux_reg = 0xd000,
	.mux_mask = 0xff,

	.div_reg = 0xd004,
	.div_mask = 0x3,
	.div_val = 4,
};

static struct debug_mux disp_cc = {
	.phys = 0xaf00000,
	.size = 0x10000,
	.block_name = "disp",

	.measure = measure_leaf,
	.parent = &gcc.mux,
	.parent_mux_val = 0x53,

	.enable_reg = 0x500c,
	.enable_mask = BIT(0),

	.mux_reg = 0x7000,
	.mux_mask = 0xff,

	.div_reg = 0x5008,
	.div_mask = 0x3,
	.div_val = 4,
};

static struct debug_mux gpu_cc = {
	.phys = 0x3d90000,
	.size = 0x9000,
	.block_name = "gpu",

	.measure = measure_leaf,
	.parent = &gcc.mux,
	.parent_mux_val = 0x153,

	.enable_reg = 0x1100,
	.enable_mask = BIT(0),

	.mux_reg = 0x1568,
	.mux_mask = 0xff,

	.div_reg = 0x10fc,
	.div_mask = 0xf,
	.div_val = 2,
};

static struct debug_mux video_cc = {
	.phys = 0xaaf0000,
	.size = 0x10000,
	.block_name = "video",

	.measure = measure_leaf,
	.parent = &gcc.mux,
	.parent_mux_val = 0x5a,

	.enable_reg = 0xebc,
	.enable_mask = BIT(0),

	.mux_reg = 0xa4c,
	.mux_mask = 0x3f,

	.div_reg = 0xe9c,
	.div_mask = 0x7,
	.div_val = 3,
};


/*
struct measure_clk {
	char *name;
	struct debug_mux *primary;
	int mux;
	int post_div;

	struct debug_mux *leaf;
	int leaf_mux;
	int leaf_div;

	unsigned int fixed_div;
};
*/

static struct measure_clk sm8350_clocks[] = {
/* cam_cc_debug_mux is 0x4D */
	{ "cam_cc_bps_ahb_clk", &cam_cc, 0x18 },
	{ "cam_cc_bps_areg_clk", &cam_cc, 0x17 },
	{ "cam_cc_bps_axi_clk", &cam_cc, 0x16 },
	{ "cam_cc_bps_clk", &cam_cc, 0x14 },
	{ "cam_cc_camnoc_axi_clk", &cam_cc, 0x3C },
	{ "cam_cc_camnoc_dcd_xo_clk", &cam_cc, 0x3D },
	{ "cam_cc_cci_0_clk", &cam_cc, 0x39 },
	{ "cam_cc_cci_1_clk", &cam_cc, 0x3A },
	{ "cam_cc_core_ahb_clk", &cam_cc, 0x40 },
	{ "cam_cc_cpas_ahb_clk", &cam_cc, 0x3B },
	{ "cam_cc_csi0phytimer_clk", &cam_cc, 0x8 },
	{ "cam_cc_csi1phytimer_clk", &cam_cc, 0xA },
	{ "cam_cc_csi2phytimer_clk", &cam_cc, 0xC },
	{ "cam_cc_csi3phytimer_clk", &cam_cc, 0xE },
	{ "cam_cc_csi4phytimer_clk", &cam_cc, 0x10 },
	{ "cam_cc_csi5phytimer_clk", &cam_cc, 0x12 },
	{ "cam_cc_csiphy0_clk", &cam_cc, 0x9 },
	{ "cam_cc_csiphy1_clk", &cam_cc, 0xB },
	{ "cam_cc_csiphy2_clk", &cam_cc, 0xD },
	{ "cam_cc_csiphy3_clk", &cam_cc, 0xF },
	{ "cam_cc_csiphy4_clk", &cam_cc, 0x11 },
	{ "cam_cc_csiphy5_clk", &cam_cc, 0x13 },
	{ "cam_cc_icp_ahb_clk", &cam_cc, 0x36 },
	{ "cam_cc_icp_clk", &cam_cc, 0x35 },
	{ "cam_cc_ife_0_ahb_clk", &cam_cc, 0x26 },
	{ "cam_cc_ife_0_areg_clk", &cam_cc, 0x1F },
	{ "cam_cc_ife_0_axi_clk", &cam_cc, 0x25 },
	{ "cam_cc_ife_0_clk", &cam_cc, 0x1E },
	{ "cam_cc_ife_0_cphy_rx_clk", &cam_cc, 0x24 },
	{ "cam_cc_ife_0_csid_clk", &cam_cc, 0x22 },
	{ "cam_cc_ife_0_dsp_clk", &cam_cc, 0x21 },
	{ "cam_cc_ife_1_ahb_clk", &cam_cc, 0x2E },
	{ "cam_cc_ife_1_areg_clk", &cam_cc, 0x29 },
	{ "cam_cc_ife_1_axi_clk", &cam_cc, 0x2D },
	{ "cam_cc_ife_1_clk", &cam_cc, 0x27 },
	{ "cam_cc_ife_1_cphy_rx_clk", &cam_cc, 0x2C },
	{ "cam_cc_ife_1_csid_clk", &cam_cc, 0x2B },
	{ "cam_cc_ife_1_dsp_clk", &cam_cc, 0x2A },
	{ "cam_cc_ife_2_ahb_clk", &cam_cc, 0x54 },
	{ "cam_cc_ife_2_areg_clk", &cam_cc, 0x37 },
	{ "cam_cc_ife_2_axi_clk", &cam_cc, 0x53 },
	{ "cam_cc_ife_2_clk", &cam_cc, 0x7 },
	{ "cam_cc_ife_2_cphy_rx_clk", &cam_cc, 0x52 },
	{ "cam_cc_ife_2_csid_clk", &cam_cc, 0x51 },
	{ "cam_cc_ife_lite_ahb_clk", &cam_cc, 0x32 },
	{ "cam_cc_ife_lite_axi_clk", &cam_cc, 0x49 },
	{ "cam_cc_ife_lite_clk", &cam_cc, 0x2F },
	{ "cam_cc_ife_lite_cphy_rx_clk", &cam_cc, 0x31 },
	{ "cam_cc_ife_lite_csid_clk", &cam_cc, 0x30 },
	{ "cam_cc_ipe_0_ahb_clk", &cam_cc, 0x1D },
	{ "cam_cc_ipe_0_areg_clk", &cam_cc, 0x1C },
	{ "cam_cc_ipe_0_axi_clk", &cam_cc, 0x1B },
	{ "cam_cc_ipe_0_clk", &cam_cc, 0x19 },
	{ "cam_cc_jpeg_clk", &cam_cc, 0x33 },
	{ "cam_cc_mclk0_clk", &cam_cc, 0x1 },
	{ "cam_cc_mclk1_clk", &cam_cc, 0x2 },
	{ "cam_cc_mclk2_clk", &cam_cc, 0x3 },
	{ "cam_cc_mclk3_clk", &cam_cc, 0x4 },
	{ "cam_cc_mclk4_clk", &cam_cc, 0x5 },
	{ "cam_cc_mclk5_clk", &cam_cc, 0x6 },
	{ "cam_cc_sbi_ahb_clk", &cam_cc, 0x4E },
	{ "cam_cc_sbi_axi_clk", &cam_cc, 0x4D },
	{ "cam_cc_sbi_clk", &cam_cc, 0x4A },
	{ "cam_cc_sbi_cphy_rx_0_clk", &cam_cc, 0x4C },
	{ "cam_cc_sbi_cphy_rx_1_clk", &cam_cc, 0x56 },
	{ "cam_cc_sbi_csid_0_clk", &cam_cc, 0x4B },
	{ "cam_cc_sbi_csid_1_clk", &cam_cc, 0x57 },
	{ "cam_cc_sbi_ife_0_clk", &cam_cc, 0x4F },
	{ "cam_cc_sbi_ife_1_clk", &cam_cc, 0x50 },
	{ "cam_cc_sbi_ife_2_clk", &cam_cc, 0x55 },
	{ "cam_cc_sleep_clk", &cam_cc, 0x42 },
/* disp_cc_debug_mux is 0x53 */
	{ "disp_cc_mdss_ahb_clk", &disp_cc, 0x2A },
	{ "disp_cc_mdss_byte0_clk", &disp_cc, 0x15 },
	{ "disp_cc_mdss_byte0_intf_clk", &disp_cc, 0x16 },
	{ "disp_cc_mdss_byte1_clk", &disp_cc, 0x17 },
	{ "disp_cc_mdss_byte1_intf_clk", &disp_cc, 0x18 },
	{ "disp_cc_mdss_dp_aux1_clk", &disp_cc, 0x25 },
	{ "disp_cc_mdss_dp_aux_clk", &disp_cc, 0x20 },
	{ "disp_cc_mdss_dp_link1_clk", &disp_cc, 0x22 },
	{ "disp_cc_mdss_dp_link1_intf_clk", &disp_cc, 0x23 },
	{ "disp_cc_mdss_dp_link_clk", &disp_cc, 0x1B },
	{ "disp_cc_mdss_dp_link_intf_clk", &disp_cc, 0x1C },
	{ "disp_cc_mdss_dp_pixel1_clk", &disp_cc, 0x1F },
	{ "disp_cc_mdss_dp_pixel2_clk", &disp_cc, 0x21 },
	{ "disp_cc_mdss_dp_pixel_clk", &disp_cc, 0x1E },
	{ "disp_cc_mdss_edp_aux_clk", &disp_cc, 0x29 },
	{ "disp_cc_mdss_edp_link_clk", &disp_cc, 0x27 },
	{ "disp_cc_mdss_edp_link_intf_clk", &disp_cc, 0x28 },
	{ "disp_cc_mdss_edp_pixel_clk", &disp_cc, 0x26 },
	{ "disp_cc_mdss_esc0_clk", &disp_cc, 0x19 },
	{ "disp_cc_mdss_esc1_clk", &disp_cc, 0x1A },
	{ "disp_cc_mdss_mdp_clk", &disp_cc, 0x11 },
	{ "disp_cc_mdss_mdp_lut_clk", &disp_cc, 0x13 },
	{ "disp_cc_mdss_non_gdsc_ahb_clk", &disp_cc, 0x2B },
	{ "disp_cc_mdss_pclk0_clk", &disp_cc, 0xF },
	{ "disp_cc_mdss_pclk1_clk", &disp_cc, 0x10 },
	{ "disp_cc_mdss_rot_clk", &disp_cc, 0x12 },
	{ "disp_cc_mdss_rscc_ahb_clk", &disp_cc, 0x2D },
	{ "disp_cc_mdss_rscc_vsync_clk", &disp_cc, 0x2C },
	{ "disp_cc_mdss_vsync_clk", &disp_cc, 0x14 },
	{ "disp_cc_sleep_clk", &disp_cc, 0x36 },

	{ "core_bi_pll_test_se", &gcc.mux, 0x5 },
	{ "gcc_aggre_noc_pcie_0_axi_clk", &gcc.mux, 0x138 },
	{ "gcc_aggre_noc_pcie_1_axi_clk", &gcc.mux, 0x139 },
	{ "gcc_aggre_noc_pcie_tbu_clk", &gcc.mux, 0x34 },
	{ "gcc_aggre_ufs_card_axi_clk", &gcc.mux, 0x13D },
	{ "gcc_aggre_ufs_phy_axi_clk", &gcc.mux, 0x13C },
	{ "gcc_aggre_usb3_prim_axi_clk", &gcc.mux, 0x13A },
	{ "gcc_aggre_usb3_sec_axi_clk", &gcc.mux, 0x13B },
	{ "gcc_boot_rom_ahb_clk", &gcc.mux, 0xA8 },
	{ "gcc_camera_ahb_clk", &gcc.mux, 0x47 },
	{ "gcc_camera_hf_axi_clk", &gcc.mux, 0x4A },
	{ "gcc_camera_sf_axi_clk", &gcc.mux, 0x4B },
	{ "gcc_camera_xo_clk", &gcc.mux, 0x4C },
	{ "gcc_cfg_noc_usb3_prim_axi_clk", &gcc.mux, 0x1F },
	{ "gcc_cfg_noc_usb3_sec_axi_clk", &gcc.mux, 0x20 },
	{ "gcc_ddrss_gpu_axi_clk", &gcc.mux, 0xC9 },
	{ "gcc_ddrss_pcie_sf_tbu_clk", &gcc.mux, 0xCA },
	{ "gcc_disp_ahb_clk", &gcc.mux, 0x4E },
	{ "gcc_disp_hf_axi_clk", &gcc.mux, 0x50 },
	{ "gcc_disp_sf_axi_clk", &gcc.mux, 0x51 },
	{ "gcc_disp_xo_clk", &gcc.mux, 0x52 },
	{ "gcc_gp1_clk", &gcc.mux, 0xF1 },
	{ "gcc_gp2_clk", &gcc.mux, 0xF2 },
	{ "gcc_gp3_clk", &gcc.mux, 0xF3 },
	{ "gcc_gpu_cfg_ahb_clk", &gcc.mux, 0x151 },
	{ "gcc_gpu_gpll0_clk_src", &gcc.mux, 0x158 },
	{ "gcc_gpu_gpll0_div_clk_src", &gcc.mux, 0x159 },
	{ "gcc_gpu_memnoc_gfx_clk", &gcc.mux, 0x154 },
	{ "gcc_gpu_snoc_dvm_gfx_clk", &gcc.mux, 0x157 },
	{ "gcc_pcie0_phy_rchng_clk", &gcc.mux, 0xFA },
	{ "gcc_pcie1_phy_rchng_clk", &gcc.mux, 0x103 },
	{ "gcc_pcie_0_aux_clk", &gcc.mux, 0xF8 },
	{ "gcc_pcie_0_cfg_ahb_clk", &gcc.mux, 0xF7 },
	{ "gcc_pcie_0_mstr_axi_clk", &gcc.mux, 0xF6 },
	{ "gcc_pcie_0_pipe_clk", &gcc.mux, 0xF9 },
	{ "gcc_pcie_0_slv_axi_clk", &gcc.mux, 0xF5 },
	{ "gcc_pcie_0_slv_q2a_axi_clk", &gcc.mux, 0xF4 },
	{ "gcc_pcie_1_aux_clk", &gcc.mux, 0x101 },
	{ "gcc_pcie_1_cfg_ahb_clk", &gcc.mux, 0x100 },
	{ "gcc_pcie_1_mstr_axi_clk", &gcc.mux, 0xFF },
	{ "gcc_pcie_1_pipe_clk", &gcc.mux, 0x102 },
	{ "gcc_pcie_1_slv_axi_clk", &gcc.mux, 0xFE },
	{ "gcc_pcie_1_slv_q2a_axi_clk", &gcc.mux, 0xFD },
	{ "gcc_pdm2_clk", &gcc.mux, 0x9E },
	{ "gcc_pdm_ahb_clk", &gcc.mux, 0x9C },
	{ "gcc_pdm_xo4_clk", &gcc.mux, 0x9D },
	{ "gcc_qmip_camera_nrt_ahb_clk", &gcc.mux, 0x48 },
	{ "gcc_qmip_camera_rt_ahb_clk", &gcc.mux, 0x49 },
	{ "gcc_qmip_disp_ahb_clk", &gcc.mux, 0x4F },
	{ "gcc_qmip_video_cvp_ahb_clk", &gcc.mux, 0x55 },
	{ "gcc_qmip_video_vcodec_ahb_clk", &gcc.mux, 0x56 },
	{ "gcc_qupv3_wrap0_core_2x_clk", &gcc.mux, 0x89 },
	{ "gcc_qupv3_wrap0_core_clk", &gcc.mux, 0x88 },
	{ "gcc_qupv3_wrap0_s0_clk", &gcc.mux, 0x8A },
	{ "gcc_qupv3_wrap0_s1_clk", &gcc.mux, 0x8B },
	{ "gcc_qupv3_wrap0_s2_clk", &gcc.mux, 0x8C },
	{ "gcc_qupv3_wrap0_s3_clk", &gcc.mux, 0x8D },
	{ "gcc_qupv3_wrap0_s4_clk", &gcc.mux, 0x8E },
	{ "gcc_qupv3_wrap0_s5_clk", &gcc.mux, 0x8F },
	{ "gcc_qupv3_wrap0_s6_clk", &gcc.mux, 0x90 },
	{ "gcc_qupv3_wrap0_s7_clk", &gcc.mux, 0x91 },
	{ "gcc_qupv3_wrap1_core_2x_clk", &gcc.mux, 0x95 },
	{ "gcc_qupv3_wrap1_core_clk", &gcc.mux, 0x94 },
	{ "gcc_qupv3_wrap1_s0_clk", &gcc.mux, 0x96 },
	{ "gcc_qupv3_wrap1_s1_clk", &gcc.mux, 0x97 },
	{ "gcc_qupv3_wrap1_s2_clk", &gcc.mux, 0x98 },
	{ "gcc_qupv3_wrap1_s3_clk", &gcc.mux, 0x99 },
	{ "gcc_qupv3_wrap1_s4_clk", &gcc.mux, 0x9A },
	{ "gcc_qupv3_wrap2_core_2x_clk", &gcc.mux, 0x16E },
	{ "gcc_qupv3_wrap2_core_clk", &gcc.mux, 0x16D },
	{ "gcc_qupv3_wrap2_s0_clk", &gcc.mux, 0x16F },
	{ "gcc_qupv3_wrap2_s1_clk", &gcc.mux, 0x170 },
	{ "gcc_qupv3_wrap2_s2_clk", &gcc.mux, 0x171 },
	{ "gcc_qupv3_wrap2_s3_clk", &gcc.mux, 0x172 },
	{ "gcc_qupv3_wrap2_s4_clk", &gcc.mux, 0x173 },
	{ "gcc_qupv3_wrap2_s5_clk", &gcc.mux, 0x174 },
	{ "gcc_qupv3_wrap_0_m_ahb_clk", &gcc.mux, 0x86 },
	{ "gcc_qupv3_wrap_0_s_ahb_clk", &gcc.mux, 0x87 },
	{ "gcc_qupv3_wrap_2_m_ahb_clk", &gcc.mux, 0x16B },
	{ "gcc_qupv3_wrap_2_s_ahb_clk", &gcc.mux, 0x16C },
	{ "gcc_sdcc2_ahb_clk", &gcc.mux, 0x83 },
	{ "gcc_sdcc2_apps_clk", &gcc.mux, 0x82 },
	{ "gcc_sdcc4_ahb_clk", &gcc.mux, 0x85 },
	{ "gcc_sdcc4_apps_clk", &gcc.mux, 0x84 },
	{ "gcc_throttle_pcie_ahb_clk", &gcc.mux, 0x40 },
	{ "gcc_ufs_card_ahb_clk", &gcc.mux, 0x107 },
	{ "gcc_ufs_card_axi_clk", &gcc.mux, 0x106 },
	{ "gcc_ufs_card_ice_core_clk", &gcc.mux, 0x10D },
	{ "gcc_ufs_card_phy_aux_clk", &gcc.mux, 0x10E },
	{ "gcc_ufs_card_rx_symbol_0_clk", &gcc.mux, 0x109 },
	{ "gcc_ufs_card_rx_symbol_1_clk", &gcc.mux, 0x10F },
	{ "gcc_ufs_card_tx_symbol_0_clk", &gcc.mux, 0x108 },
	{ "gcc_ufs_card_unipro_core_clk", &gcc.mux, 0x10C },
	{ "gcc_ufs_phy_ahb_clk", &gcc.mux, 0x113 },
	{ "gcc_ufs_phy_axi_clk", &gcc.mux, 0x112 },
	{ "gcc_ufs_phy_ice_core_clk", &gcc.mux, 0x119 },
	{ "gcc_ufs_phy_phy_aux_clk", &gcc.mux, 0x11A },
	{ "gcc_ufs_phy_rx_symbol_0_clk", &gcc.mux, 0x115 },
	{ "gcc_ufs_phy_rx_symbol_1_clk", &gcc.mux, 0x11B },
	{ "gcc_ufs_phy_tx_symbol_0_clk", &gcc.mux, 0x114 },
	{ "gcc_ufs_phy_unipro_core_clk", &gcc.mux, 0x118 },
	{ "gcc_usb30_prim_master_clk", &gcc.mux, 0x6D },
	{ "gcc_usb30_prim_mock_utmi_clk", &gcc.mux, 0x6F },
	{ "gcc_usb30_prim_sleep_clk", &gcc.mux, 0x6E },
	{ "gcc_usb30_sec_master_clk", &gcc.mux, 0x76 },
	{ "gcc_usb30_sec_mock_utmi_clk", &gcc.mux, 0x78 },
	{ "gcc_usb30_sec_sleep_clk", &gcc.mux, 0x77 },
	{ "gcc_usb3_prim_phy_aux_clk", &gcc.mux, 0x70 },
	{ "gcc_usb3_prim_phy_com_aux_clk", &gcc.mux, 0x71 },
	{ "gcc_usb3_prim_phy_pipe_clk", &gcc.mux, 0x72 },
	{ "gcc_usb3_sec_phy_aux_clk", &gcc.mux, 0x79 },
	{ "gcc_usb3_sec_phy_com_aux_clk", &gcc.mux, 0x7A },
	{ "gcc_usb3_sec_phy_pipe_clk", &gcc.mux, 0x7B },
	{ "gcc_video_ahb_clk", &gcc.mux, 0x54 },
	{ "gcc_video_axi0_clk", &gcc.mux, 0x57 },
	{ "gcc_video_axi1_clk", &gcc.mux, 0x58 },
	{ "gcc_video_xo_clk", &gcc.mux, 0x59 },
	{ "gpu_cc_debug_mux", &gcc.mux, 0x153 },
	{ "measure_only_cnoc_clk", &gcc.mux, 0x18 },
	{ "measure_only_ipa_2x_clk", &gcc.mux, 0x140 },
	{ "measure_only_memnoc_clk", &gcc.mux, 0xCF },
	{ "measure_only_snoc_clk", &gcc.mux, 0x9 },
	{ "pcie_0_pipe_clk", &gcc.mux, 0xFB },
	{ "pcie_1_pipe_clk", &gcc.mux, 0x104 },
	{ "ufs_card_rx_symbol_0_clk", &gcc.mux, 0x10B },
	{ "ufs_card_rx_symbol_1_clk", &gcc.mux, 0x110 },
	{ "ufs_card_tx_symbol_0_clk", &gcc.mux, 0x10A },
	{ "ufs_phy_rx_symbol_0_clk", &gcc.mux, 0x117 },
	{ "ufs_phy_rx_symbol_1_clk", &gcc.mux, 0x11C },
	{ "ufs_phy_tx_symbol_0_clk", &gcc.mux, 0x116 },
	{ "usb3_phy_wrapper_gcc_usb30_pipe_clk", &gcc.mux, 0x7C },
	{ "usb3_uni_phy_sec_gcc_usb30_pipe_clk", &gcc.mux, 0x7D },
	{ "mc_cc_debug_mux", &gcc.mux, 0xD3 },
/* gpu_cc_debug_mux is 0x153 */
	{ "gpu_cc_ahb_clk", &gpu_cc, 0x12 },
	{ "gpu_cc_cb_clk", &gpu_cc, 0x26 },
	{ "gpu_cc_crc_ahb_clk", &gpu_cc, 0x13 },
	{ "gpu_cc_cx_apb_clk", &gpu_cc, 0x16 },
	{ "gpu_cc_cx_gmu_clk", &gpu_cc, 0x1A },
	{ "gpu_cc_cx_qdss_at_clk", &gpu_cc, 0x14 },
	{ "gpu_cc_cx_qdss_trig_clk", &gpu_cc, 0x19 },
	{ "gpu_cc_cx_qdss_tsctr_clk", &gpu_cc, 0x15 },
	{ "gpu_cc_cx_snoc_dvm_clk", &gpu_cc, 0x17 },
	{ "gpu_cc_cxo_aon_clk", &gpu_cc, 0xB },
	{ "gpu_cc_cxo_clk", &gpu_cc, 0x1B },
	{ "gpu_cc_freq_measure_clk", &gpu_cc, 0xC },
	{ "gpu_cc_gx_gmu_clk", &gpu_cc, 0x11 },
	{ "gpu_cc_gx_qdss_tsctr_clk", &gpu_cc, 0xF },
	{ "gpu_cc_gx_vsense_clk", &gpu_cc, 0xE },
	{ "gpu_cc_hub_aon_clk", &gpu_cc, 0x27 },
	{ "gpu_cc_hub_cx_int_clk", &gpu_cc, 0x1C },
	{ "gpu_cc_mnd1x_0_gfx3d_clk", &gpu_cc, 0x21 },
	{ "gpu_cc_mnd1x_1_gfx3d_clk", &gpu_cc, 0x22 },
	{ "gpu_cc_sleep_clk", &gpu_cc, 0x18 },
	{ "measure_only_gpu_cc_cx_gfx3d_clk", &gpu_cc, 0x1D },
	{ "measure_only_gpu_cc_cx_gfx3d_slv_clk", &gpu_cc, 0x1E },
	{ "measure_only_gpu_cc_gx_gfx3d_clk", &gpu_cc, 0xD },
/* video_cc_debug_mux is 0x5A */
	{ "video_cc_mvs0_clk", &video_cc, 0x3 },
	{ "video_cc_mvs0c_clk", &video_cc, 0x1 },
	{ "video_cc_mvs1_clk", &video_cc, 0x5 },
	{ "video_cc_mvs1_div2_clk", &video_cc, 0x8 },
	{ "video_cc_mvs1c_clk", &video_cc, 0x9 },
	{ "video_cc_sleep_clk", &video_cc, 0xC },
	{}
};

struct debugcc_platform sm8350_debugcc = {
	"sm8350",
	sm8350_clocks,
};
