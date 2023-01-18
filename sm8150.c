/*
 * Copyright (c) 2023, Marijn Suijten.
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

static struct debug_mux gcc = {
	.phys =	0x100000,
	.size = 0x1f0000,

	.enable_reg = 0x62008,
	.enable_mask = BIT(0),

	.mux_reg = 0x62000,
	.mux_mask = 0x3ff,

	.div_reg = 0x62004,
	.div_mask = 0xf,
	.div_val = 1,

	.xo_div4_reg = 0x43008,
	.debug_ctl_reg = 0x62038,
	.debug_status_reg = 0x6203c,
};

static struct debug_mux cam_cc = {
	.phys = 0xad00000,
	.size = 0x10000,
	.block_name = "cam",

	.enable_reg = 0xd008,
	.enable_mask = BIT(0),

	.mux_reg = 0xd000,
	.mux_mask = 0xff,

	.div_reg = 0xd004,
	.div_mask = 0xf,
	.div_val = 4,
};

static struct debug_mux disp_cc = {
	.phys = 0xaf00000,
	.size = 0x10000,
	.block_name = "disp",

	.enable_reg = 0x500c,
	.enable_mask = BIT(0),

	.mux_reg = 0x7000,
	.mux_mask = 0xff,

	.div_reg = 0x5008,
	.div_mask = 0x3,
	.div_val = 4,
};

static struct debug_mux gpu_cc = {
	.phys = 0x2c90000,
	.size = 0x9000,
	.block_name = "gpu",

	.enable_reg = 0x1100,
	.enable_mask = BIT(0),

	.mux_reg = 0x1568,
	.mux_mask = 0xff,

	.div_reg = 0x10fc,
	.div_mask = 0x3,
	.div_val = 2,
};

static struct debug_mux video_cc = {
	.phys = 0xab00000,
	.size = 0x10000,
	.block_name = "video",

	.enable_reg = 0x940,
	.enable_mask = BIT(0),

	.mux_reg = 0xa4c,
	.mux_mask = 0x3f,

	.div_reg = 0x938,
	.div_mask = 0x7,
	.div_val = 5,
};

static struct debug_mux npu_cc = {
	.phys = 0x9910000,
	.size = 0x10000,
	.block_name = "npu",

	.enable_reg = 0x3008,
	.enable_mask = BIT(0),

	.mux_reg = 0x4000,
	.mux_mask = 0xff,

	.div_reg = 0x3004,
	.div_mask = 0x3,
	.div_val = 2,
};

static struct debug_mux mc_cc = {
	.phys =	0x90b0000,
	.size = /* 0x54 */ 0x1000,
	.block_name = "mc",

	.measure = measure_mccc,
};

static struct debug_mux cpu_cc = {
	.phys = 0x182a0000,
	.size = 0x1000,
	.block_name = "cpu",

	.enable_reg = 0x18,
	.enable_mask = BIT(0),

	.mux_reg = 0x18,
	.mux_mask = 0x7f << 4,
	.mux_shift = 4,

	.div_reg = 0x18,
	.div_mask = 0xf << 11,
	.div_shift = 11,
	.div_val = 1,
};

static struct measure_clk sm8150_clocks[] = {
	{ "cam_cc_bps_ahb_clk", &gcc, 0x55, &cam_cc, 0xe },
	{ "cam_cc_bps_areg_clk", &gcc, 0x55, &cam_cc, 0xd },
	{ "cam_cc_bps_axi_clk", &gcc, 0x55, &cam_cc, 0xc },
	{ "cam_cc_bps_clk", &gcc, 0x55, &cam_cc, 0xb },
	{ "cam_cc_camnoc_axi_clk", &gcc, 0x55, &cam_cc, 0x27 },
	{ "cam_cc_camnoc_dcd_xo_clk", &gcc, 0x55, &cam_cc, 0x33 },
	{ "cam_cc_cci_0_clk", &gcc, 0x55, &cam_cc, 0x2a },
	{ "cam_cc_cci_1_clk", &gcc, 0x55, &cam_cc, 0x3b },
	{ "cam_cc_core_ahb_clk", &gcc, 0x55, &cam_cc, 0x2e },
	{ "cam_cc_cpas_ahb_clk", &gcc, 0x55, &cam_cc, 0x2c },
	{ "cam_cc_csi0phytimer_clk", &gcc, 0x55, &cam_cc, 0x5 },
	{ "cam_cc_csi1phytimer_clk", &gcc, 0x55, &cam_cc, 0x7 },
	{ "cam_cc_csi2phytimer_clk", &gcc, 0x55, &cam_cc, 0x9 },
	{ "cam_cc_csi3phytimer_clk", &gcc, 0x55, &cam_cc, 0x35 },
	{ "cam_cc_csiphy0_clk", &gcc, 0x55, &cam_cc, 0x6 },
	{ "cam_cc_csiphy1_clk", &gcc, 0x55, &cam_cc, 0x8 },
	{ "cam_cc_csiphy2_clk", &gcc, 0x55, &cam_cc, 0xa },
	{ "cam_cc_csiphy3_clk", &gcc, 0x55, &cam_cc, 0x36 },
	{ "cam_cc_fd_core_clk", &gcc, 0x55, &cam_cc, 0x28 },
	{ "cam_cc_fd_core_uar_clk", &gcc, 0x55, &cam_cc, 0x29 },
	{ "cam_cc_icp_ahb_clk", &gcc, 0x55, &cam_cc, 0x37 },
	{ "cam_cc_icp_clk", &gcc, 0x55, &cam_cc, 0x26 },
	{ "cam_cc_ife_0_axi_clk", &gcc, 0x55, &cam_cc, 0x1b },
	{ "cam_cc_ife_0_clk", &gcc, 0x55, &cam_cc, 0x17 },
	{ "cam_cc_ife_0_cphy_rx_clk", &gcc, 0x55, &cam_cc, 0x1a },
	{ "cam_cc_ife_0_csid_clk", &gcc, 0x55, &cam_cc, 0x19 },
	{ "cam_cc_ife_0_dsp_clk", &gcc, 0x55, &cam_cc, 0x18 },
	{ "cam_cc_ife_1_axi_clk", &gcc, 0x55, &cam_cc, 0x21 },
	{ "cam_cc_ife_1_clk", &gcc, 0x55, &cam_cc, 0x1d },
	{ "cam_cc_ife_1_cphy_rx_clk", &gcc, 0x55, &cam_cc, 0x20 },
	{ "cam_cc_ife_1_csid_clk", &gcc, 0x55, &cam_cc, 0x1f },
	{ "cam_cc_ife_1_dsp_clk", &gcc, 0x55, &cam_cc, 0x1e },
	{ "cam_cc_ife_lite_0_clk", &gcc, 0x55, &cam_cc, 0x22 },
	{ "cam_cc_ife_lite_0_cphy_rx_clk", &gcc, 0x55, &cam_cc, 0x24 },
	{ "cam_cc_ife_lite_0_csid_clk", &gcc, 0x55, &cam_cc, 0x23 },
	{ "cam_cc_ife_lite_1_clk", &gcc, 0x55, &cam_cc, 0x38 },
	{ "cam_cc_ife_lite_1_cphy_rx_clk", &gcc, 0x55, &cam_cc, 0x3a },
	{ "cam_cc_ife_lite_1_csid_clk", &gcc, 0x55, &cam_cc, 0x39 },
	{ "cam_cc_ipe_0_ahb_clk", &gcc, 0x55, &cam_cc, 0x12 },
	{ "cam_cc_ipe_0_areg_clk", &gcc, 0x55, &cam_cc, 0x11 },
	{ "cam_cc_ipe_0_axi_clk", &gcc, 0x55, &cam_cc, 0x10 },
	{ "cam_cc_ipe_0_clk", &gcc, 0x55, &cam_cc, 0xf },
	{ "cam_cc_ipe_1_ahb_clk", &gcc, 0x55, &cam_cc, 0x16 },
	{ "cam_cc_ipe_1_areg_clk", &gcc, 0x55, &cam_cc, 0x15 },
	{ "cam_cc_ipe_1_axi_clk", &gcc, 0x55, &cam_cc, 0x14 },
	{ "cam_cc_ipe_1_clk", &gcc, 0x55, &cam_cc, 0x13 },
	{ "cam_cc_jpeg_clk", &gcc, 0x55, &cam_cc, 0x25 },
	{ "cam_cc_lrme_clk", &gcc, 0x55, &cam_cc, 0x2b },
	{ "cam_cc_mclk0_clk", &gcc, 0x55, &cam_cc, 0x1 },
	{ "cam_cc_mclk1_clk", &gcc, 0x55, &cam_cc, 0x2 },
	{ "cam_cc_mclk2_clk", &gcc, 0x55, &cam_cc, 0x3 },
	{ "cam_cc_mclk3_clk", &gcc, 0x55, &cam_cc, 0x4 },
	{ "disp_cc_mdss_ahb_clk", &gcc, 0x56, &disp_cc, 0x2b },
	{ "disp_cc_mdss_byte0_clk", &gcc, 0x56, &disp_cc, 0x15 },
	{ "disp_cc_mdss_byte0_intf_clk", &gcc, 0x56, &disp_cc, 0x16 },
	{ "disp_cc_mdss_byte1_clk", &gcc, 0x56, &disp_cc, 0x17 },
	{ "disp_cc_mdss_byte1_intf_clk", &gcc, 0x56, &disp_cc, 0x18 },
	{ "disp_cc_mdss_dp_aux1_clk", &gcc, 0x56, &disp_cc, 0x25 },
	{ "disp_cc_mdss_dp_aux_clk", &gcc, 0x56, &disp_cc, 0x20 },
	{ "disp_cc_mdss_dp_crypto1_clk", &gcc, 0x56, &disp_cc, 0x24 },
	{ "disp_cc_mdss_dp_crypto_clk", &gcc, 0x56, &disp_cc, 0x1d },
	{ "disp_cc_mdss_dp_link1_clk", &gcc, 0x56, &disp_cc, 0x22 },
	{ "disp_cc_mdss_dp_link1_intf_clk", &gcc, 0x56, &disp_cc, 0x23 },
	{ "disp_cc_mdss_dp_link_clk", &gcc, 0x56, &disp_cc, 0x1b },
	{ "disp_cc_mdss_dp_link_intf_clk", &gcc, 0x56, &disp_cc, 0x1c },
	{ "disp_cc_mdss_dp_pixel1_clk", &gcc, 0x56, &disp_cc, 0x1f },
	{ "disp_cc_mdss_dp_pixel2_clk", &gcc, 0x56, &disp_cc, 0x21 },
	{ "disp_cc_mdss_dp_pixel_clk", &gcc, 0x56, &disp_cc, 0x1e },
	{ "disp_cc_mdss_edp_aux_clk", &gcc, 0x56, &disp_cc, 0x29 },
	{ "disp_cc_mdss_edp_gtc_clk", &gcc, 0x56, &disp_cc, 0x2a },
	{ "disp_cc_mdss_edp_link_clk", &gcc, 0x56, &disp_cc, 0x27 },
	{ "disp_cc_mdss_edp_link_intf_clk", &gcc, 0x56, &disp_cc, 0x28 },
	{ "disp_cc_mdss_edp_pixel_clk", &gcc, 0x56, &disp_cc, 0x26 },
	{ "disp_cc_mdss_esc0_clk", &gcc, 0x56, &disp_cc, 0x19 },
	{ "disp_cc_mdss_esc1_clk", &gcc, 0x56, &disp_cc, 0x1a },
	{ "disp_cc_mdss_mdp_clk", &gcc, 0x56, &disp_cc, 0x11 },
	{ "disp_cc_mdss_mdp_lut_clk", &gcc, 0x56, &disp_cc, 0x13 },
	{ "disp_cc_mdss_non_gdsc_ahb_clk", &gcc, 0x56, &disp_cc, 0x2c },
	{ "disp_cc_mdss_pclk0_clk", &gcc, 0x56, &disp_cc, 0xf },
	{ "disp_cc_mdss_pclk1_clk", &gcc, 0x56, &disp_cc, 0x10 },
	{ "disp_cc_mdss_rot_clk", &gcc, 0x56, &disp_cc, 0x12 },
	{ "disp_cc_mdss_rscc_ahb_clk", &gcc, 0x56, &disp_cc, 0x2e },
	{ "disp_cc_mdss_rscc_vsync_clk", &gcc, 0x56, &disp_cc, 0x2d },
	{ "disp_cc_mdss_vsync_clk", &gcc, 0x56, &disp_cc, 0x14 },
	{ "disp_cc_xo_clk", &gcc, 0x56, &disp_cc, 0x36 },
	/* TODO: post_div_val and prim_mux_div_val are 2 */
	// { "measure_only_cdsp_clk", &gcc, 0xdb, 2, GCC, 0xdb, 0x3ff, 0, 0xf, 0, 2, 0x62000, 0x62004, 0x62008 },
	{ "measure_only_cdsp_clk", &gcc, 0xdb, 0, 0, 2 },
	{ "measure_only_snoc_clk", &gcc, 0x7 },
	{ "measure_only_cnoc_clk", &gcc, 0x19 },
	{ "measure_only_mccc_clk", &gcc, 0xd0, &mc_cc, 0x50 },
	{ "measure_only_ipa_2x_clk", &gcc, 0x147 },
	{ "gcc_aggre_noc_pcie_tbu_clk", &gcc, 0x36 },
	{ "gcc_aggre_ufs_card_axi_clk", &gcc, 0x141 },
	{ "gcc_aggre_ufs_phy_axi_clk", &gcc, 0x140 },
	{ "gcc_aggre_usb3_prim_axi_clk", &gcc, 0x13e },
	{ "gcc_aggre_usb3_sec_axi_clk", &gcc, 0x13f },
	{ "gcc_camera_ahb_clk", &gcc, 0x43 },
	{ "gcc_camera_hf_axi_clk", &gcc, 0x4d },
	{ "gcc_camera_sf_axi_clk", &gcc, 0x4e },
	{ "gcc_camera_xo_clk", &gcc, 0x52 },
	{ "gcc_ce1_ahb_clk", &gcc, 0xb6 },
	{ "gcc_ce1_axi_clk", &gcc, 0xb5 },
	{ "gcc_ce1_clk", &gcc, 0xb4 },
	{ "gcc_cfg_noc_usb3_prim_axi_clk", &gcc, 0x22 },
	{ "gcc_cfg_noc_usb3_sec_axi_clk", &gcc, 0x23 },
	{ "gcc_cpuss_ahb_clk", &gcc, 0xe0 },
	{ "gcc_cpuss_rbcpr_clk", &gcc, 0xe2 },
	{ "gcc_ddrss_gpu_axi_clk", &gcc, 0xc0 },
	{ "gcc_disp_ahb_clk", &gcc, 0x44 },
	{ "gcc_disp_hf_axi_clk", &gcc, 0x4f },
	{ "gcc_disp_sf_axi_clk", &gcc, 0x50 },
	{ "gcc_disp_xo_clk", &gcc, 0x53 },
	{ "gcc_emac_axi_clk", &gcc, 0x18d },
	{ "gcc_emac_ptp_clk", &gcc, 0x190 },
	{ "gcc_emac_rgmii_clk", &gcc, 0x18f },
	{ "gcc_emac_slv_ahb_clk", &gcc, 0x18e },
	{ "gcc_gp1_clk", &gcc, 0xf0 },
	{ "gcc_gp2_clk", &gcc, 0xf1 },
	{ "gcc_gp3_clk", &gcc, 0xf2 },
	{ "gcc_gpu_cfg_ahb_clk", &gcc, 0x160 },
	{ "gcc_gpu_gpll0_clk_src", &gcc, 0x166 },
	{ "gcc_gpu_gpll0_div_clk_src", &gcc, 0x167 },
	{ "gcc_gpu_memnoc_gfx_clk", &gcc, 0x163 },
	{ "gcc_gpu_snoc_dvm_gfx_clk", &gcc, 0x165 },
	{ "gcc_npu_at_clk", &gcc, 0x17d },
	{ "gcc_npu_axi_clk", &gcc, 0x17b },
	{ "gcc_npu_cfg_ahb_clk", &gcc, 0x17a },
	{ "gcc_npu_gpll0_clk_src", &gcc, 0x17e },
	{ "gcc_npu_gpll0_div_clk_src", &gcc, 0x17f },
	{ "gcc_npu_trig_clk", &gcc, 0x17c },
	{ "gcc_pcie0_phy_refgen_clk", &gcc, 0x104 },
	{ "gcc_pcie1_phy_refgen_clk", &gcc, 0x105 },
	{ "gcc_pcie_0_aux_clk", &gcc, 0xf7 },
	{ "gcc_pcie_0_cfg_ahb_clk", &gcc, 0xf6 },
	{ "gcc_pcie_0_mstr_axi_clk", &gcc, 0xf5 },
	{ "gcc_pcie_0_pipe_clk", &gcc, 0xf8 },
	{ "gcc_pcie_0_slv_axi_clk", &gcc, 0xf4 },
	{ "gcc_pcie_0_slv_q2a_axi_clk", &gcc, 0xf3 },
	{ "gcc_pcie_1_aux_clk", &gcc, 0xff },
	{ "gcc_pcie_1_cfg_ahb_clk", &gcc, 0xfe },
	{ "gcc_pcie_1_mstr_axi_clk", &gcc, 0xfd },
	{ "gcc_pcie_1_pipe_clk", &gcc, 0x100 },
	{ "gcc_pcie_1_slv_axi_clk", &gcc, 0xfc },
	{ "gcc_pcie_1_slv_q2a_axi_clk", &gcc, 0xfb },
	{ "gcc_pcie_phy_aux_clk", &gcc, 0x103 },
	{ "gcc_pdm2_clk", &gcc, 0x9a },
	{ "gcc_pdm_ahb_clk", &gcc, 0x98 },
	{ "gcc_pdm_xo4_clk", &gcc, 0x99 },
	{ "gcc_prng_ahb_clk", &gcc, 0x9b },
	{ "gcc_qspi_cnoc_periph_ahb_clk", &gcc, 0x178 },
	{ "gcc_qspi_core_clk", &gcc, 0x179 },
	{ "gcc_qupv3_wrap0_core_2x_clk", &gcc, 0x85 },
	{ "gcc_qupv3_wrap0_core_clk", &gcc, 0x84 },
	{ "gcc_qupv3_wrap0_s0_clk", &gcc, 0x86 },
	{ "gcc_qupv3_wrap0_s1_clk", &gcc, 0x87 },
	{ "gcc_qupv3_wrap0_s2_clk", &gcc, 0x88 },
	{ "gcc_qupv3_wrap0_s3_clk", &gcc, 0x89 },
	{ "gcc_qupv3_wrap0_s4_clk", &gcc, 0x8a },
	{ "gcc_qupv3_wrap0_s5_clk", &gcc, 0x8b },
	{ "gcc_qupv3_wrap0_s6_clk", &gcc, 0x8c },
	{ "gcc_qupv3_wrap0_s7_clk", &gcc, 0x8d },
	{ "gcc_qupv3_wrap1_core_2x_clk", &gcc, 0x91 },
	{ "gcc_qupv3_wrap1_core_clk", &gcc, 0x90 },
	{ "gcc_qupv3_wrap1_s0_clk", &gcc, 0x92 },
	{ "gcc_qupv3_wrap1_s1_clk", &gcc, 0x93 },
	{ "gcc_qupv3_wrap1_s2_clk", &gcc, 0x94 },
	{ "gcc_qupv3_wrap1_s3_clk", &gcc, 0x95 },
	{ "gcc_qupv3_wrap1_s4_clk", &gcc, 0x96 },
	{ "gcc_qupv3_wrap1_s5_clk", &gcc, 0x97 },
	{ "gcc_qupv3_wrap2_core_2x_clk", &gcc, 0x184 },
	{ "gcc_qupv3_wrap2_core_clk", &gcc, 0x183 },
	{ "gcc_qupv3_wrap2_s0_clk", &gcc, 0x185 },
	{ "gcc_qupv3_wrap2_s1_clk", &gcc, 0x186 },
	{ "gcc_qupv3_wrap2_s2_clk", &gcc, 0x187 },
	{ "gcc_qupv3_wrap2_s3_clk", &gcc, 0x188 },
	{ "gcc_qupv3_wrap2_s4_clk", &gcc, 0x189 },
	{ "gcc_qupv3_wrap2_s5_clk", &gcc, 0x18a },
	{ "gcc_sdcc2_ahb_clk", &gcc, 0x7f },
	{ "gcc_sdcc2_apps_clk", &gcc, 0x7e },
	{ "gcc_sdcc4_ahb_clk", &gcc, 0x81 },
	{ "gcc_sdcc4_apps_clk", &gcc, 0x80 },
	{ "gcc_sys_noc_cpuss_ahb_clk", &gcc, 0xc },
	{ "gcc_tsif_ahb_clk", &gcc, 0x9c },
	{ "gcc_tsif_ref_clk", &gcc, 0x9d },
	{ "gcc_ufs_card_ahb_clk", &gcc, 0x107 },
	{ "gcc_ufs_card_axi_clk", &gcc, 0x106 },
	{ "gcc_ufs_card_ice_core_clk", &gcc, 0x10d },
	{ "gcc_ufs_card_phy_aux_clk", &gcc, 0x10e },
	{ "gcc_ufs_card_rx_symbol_0_clk", &gcc, 0x109 },
	{ "gcc_ufs_card_rx_symbol_1_clk", &gcc, 0x10f },
	{ "gcc_ufs_card_tx_symbol_0_clk", &gcc, 0x108 },
	{ "gcc_ufs_card_unipro_core_clk", &gcc, 0x10c },
	{ "gcc_ufs_phy_ahb_clk", &gcc, 0x113 },
	{ "gcc_ufs_phy_axi_clk", &gcc, 0x112 },
	{ "gcc_ufs_phy_ice_core_clk", &gcc, 0x119 },
	{ "gcc_ufs_phy_phy_aux_clk", &gcc, 0x11a },
	{ "gcc_ufs_phy_rx_symbol_0_clk", &gcc, 0x115 },
	{ "gcc_ufs_phy_rx_symbol_1_clk", &gcc, 0x11b },
	{ "gcc_ufs_phy_tx_symbol_0_clk", &gcc, 0x114 },
	{ "gcc_ufs_phy_unipro_core_clk", &gcc, 0x118 },
	{ "gcc_usb30_prim_master_clk", &gcc, 0x6b },
	{ "gcc_usb30_prim_mock_utmi_clk", &gcc, 0x6d },
	{ "gcc_usb30_sec_master_clk", &gcc, 0x72 },
	{ "gcc_usb30_sec_mock_utmi_clk", &gcc, 0x74 },
	{ "gcc_usb3_prim_phy_aux_clk", &gcc, 0x6e },
	{ "gcc_usb3_prim_phy_com_aux_clk", &gcc, 0x6f },
	{ "gcc_usb3_prim_phy_pipe_clk", &gcc, 0x70 },
	{ "gcc_usb3_sec_phy_aux_clk", &gcc, 0x75 },
	{ "gcc_usb3_sec_phy_com_aux_clk", &gcc, 0x76 },
	{ "gcc_usb3_sec_phy_pipe_clk", &gcc, 0x77 },
	{ "gcc_video_ahb_clk", &gcc, 0x42 },
	{ "gcc_video_axi0_clk", &gcc, 0x4a },
	{ "gcc_video_axi1_clk", &gcc, 0x4b },
	{ "gcc_video_axic_clk", &gcc, 0x4c },
	{ "gcc_video_xo_clk", &gcc, 0x51 },
	{ "gpu_cc_ahb_clk", &gcc, 0x162, &gpu_cc, 0x10 },
	{ "gpu_cc_cx_apb_clk", &gcc, 0x162, &gpu_cc, 0x14 },
	{ "gpu_cc_cx_gmu_clk", &gcc, 0x162, &gpu_cc, 0x18 },
	{ "gpu_cc_cx_qdss_at_clk", &gcc, 0x162, &gpu_cc, 0x12 },
	{ "gpu_cc_cx_qdss_trig_clk", &gcc, 0x162, &gpu_cc, 0x17 },
	{ "gpu_cc_cx_snoc_dvm_clk", &gcc, 0x162, &gpu_cc, 0x15 },
	{ "gpu_cc_cxo_aon_clk", &gcc, 0x162, &gpu_cc, 0xa },
	{ "gpu_cc_cxo_clk", &gcc, 0x162, &gpu_cc, 0x19 },
	{ "gpu_cc_gx_gmu_clk", &gcc, 0x162, &gpu_cc, 0xf },
	{ "gpu_cc_gx_vsense_clk", &gcc, 0x162, &gpu_cc, 0xc },
	{ "measure_only_gpu_cc_cx_gfx3d_clk", &gcc, 0x162, &gpu_cc, 0x1a },
	{ "measure_only_gpu_cc_cx_gfx3d_slv_clk", &gcc, 0x162, &gpu_cc, 0x1b },
	{ "measure_only_gpu_cc_gx_gfx3d_clk", &gcc, 0x162, &gpu_cc, 0xb },
	{ "npu_cc_armwic_core_clk", &gcc, 0x180, &npu_cc, 0x4 },
	{ "npu_cc_bto_core_clk", &gcc, 0x180, &npu_cc, 0x12 },
	{ "npu_cc_bwmon_clk", &gcc, 0x180, &npu_cc, 0xf },
	{ "npu_cc_cal_dp_cdc_clk", &gcc, 0x180, &npu_cc, 0x8 },
	{ "npu_cc_cal_dp_clk", &gcc, 0x180, &npu_cc, 0x1 },
	{ "npu_cc_comp_noc_axi_clk", &gcc, 0x180, &npu_cc, 0x9 },
	{ "npu_cc_conf_noc_ahb_clk", &gcc, 0x180, &npu_cc, 0xa },
	{ "npu_cc_npu_core_apb_clk", &gcc, 0x180, &npu_cc, 0xe },
	{ "npu_cc_npu_core_atb_clk", &gcc, 0x180, &npu_cc, 0xb },
	{ "npu_cc_npu_core_clk", &gcc, 0x180, &npu_cc, 0x2 },
	{ "npu_cc_npu_core_cti_clk", &gcc, 0x180, &npu_cc, 0xc },
	{ "npu_cc_npu_cpc_clk", &gcc, 0x180, &npu_cc, 0x3 },
	{ "npu_cc_perf_cnt_clk", &gcc, 0x180, &npu_cc, 0x10 },
	{ "npu_cc_xo_clk", &gcc, 0x180, &npu_cc, 0x11 },
	{ "video_cc_iris_ahb_clk", &gcc, 0x57, &video_cc, 0x7 },
	{ "video_cc_mvs0_core_clk", &gcc, 0x57, &video_cc, 0x3 },
	{ "video_cc_mvs1_core_clk", &gcc, 0x57, &video_cc, 0x5 },
	{ "video_cc_mvsc_core_clk", &gcc, 0x57, &video_cc, 0x1 },
	{ "video_cc_xo_clk", &gcc, 0x57, &video_cc, 0x8 },
	{ "l3_clk", &gcc, 0xe8, &cpu_cc, 0x46, 16 },
	{ "pwrcl_clk", &gcc, 0xe8, &cpu_cc, 0x44, 16 },
	{ "perfcl_clk", &gcc, 0xe8, &cpu_cc, 0x45, 16 },
	{ "perfpcl_clk", &gcc, 0xe8, &cpu_cc, 0x47, 16 },
	{}
};

struct debugcc_platform sm8150_debugcc = {
	"sm8150",
	sm8150_clocks,
};
