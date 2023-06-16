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
	{ "measure_only_snoc_clk", &gcc.mux, 7 },
	{ "gcc_sys_noc_cpuss_ahb_clk", &gcc.mux, 12 },
	{ "measure_only_cnoc_clk", &gcc.mux, 21 },
	{ "gcc_cfg_noc_usb3_prim_axi_clk", &gcc.mux, 29 },
	{ "gcc_cfg_noc_usb3_sec_axi_clk", &gcc.mux, 30 },
	{ "gcc_aggre_noc_pcie_tbu_clk", &gcc.mux, 45 },
	{ "gcc_video_ahb_clk", &gcc.mux, 57 },
	{ "gcc_camera_ahb_clk", &gcc.mux, 58 },
	{ "gcc_disp_ahb_clk", &gcc.mux, 59 },
	{ "gcc_qmip_video_ahb_clk", &gcc.mux, 60 },
	{ "gcc_qmip_camera_ahb_clk", &gcc.mux, 61 },
	{ "gcc_qmip_disp_ahb_clk", &gcc.mux, 62 },
	{ "gcc_video_axi_clk", &gcc.mux, 63 },
	{ "gcc_camera_axi_clk", &gcc.mux, 64 },
	{ "gcc_disp_axi_clk", &gcc.mux, 65 },
	{ "gcc_video_xo_clk", &gcc.mux, 66 },
	{ "gcc_camera_xo_clk", &gcc.mux, 67 },
	{ "gcc_disp_xo_clk", &gcc.mux, 68 },
	{ "cam_cc_mclk0_clk", &cam_cc, 1 },
	{ "cam_cc_mclk1_clk", &cam_cc, 2 },
	{ "cam_cc_mclk2_clk", &cam_cc, 3 },
	{ "cam_cc_mclk3_clk", &cam_cc, 4 },
	{ "cam_cc_csi0phytimer_clk", &cam_cc, 5 },
	{ "cam_cc_csiphy0_clk", &cam_cc, 6 },
	{ "cam_cc_csi1phytimer_clk", &cam_cc, 7 },
	{ "cam_cc_csiphy1_clk", &cam_cc, 8 },
	{ "cam_cc_csi2phytimer_clk", &cam_cc, 9 },
	{ "cam_cc_csiphy2_clk", &cam_cc, 10 },
	{ "cam_cc_bps_clk", &cam_cc, 11 },
	{ "cam_cc_bps_axi_clk", &cam_cc, 12 },
	{ "cam_cc_bps_areg_clk", &cam_cc, 13 },
	{ "cam_cc_bps_ahb_clk", &cam_cc, 14 },
	{ "cam_cc_ipe_0_clk", &cam_cc, 15 },
	{ "cam_cc_ipe_0_axi_clk", &cam_cc, 16 },
	{ "cam_cc_ipe_0_areg_clk", &cam_cc, 17 },
	{ "cam_cc_ipe_0_ahb_clk", &cam_cc, 18 },
	{ "cam_cc_ipe_1_clk", &cam_cc, 19 },
	{ "cam_cc_ipe_1_axi_clk", &cam_cc, 20 },
	{ "cam_cc_ipe_1_areg_clk", &cam_cc, 21 },
	{ "cam_cc_ipe_1_ahb_clk", &cam_cc, 22 },
	{ "cam_cc_ife_0_clk", &cam_cc, 23 },
	{ "cam_cc_ife_0_dsp_clk", &cam_cc, 24 },
	{ "cam_cc_ife_0_csid_clk", &cam_cc, 25 },
	{ "cam_cc_ife_0_cphy_rx_clk", &cam_cc, 26 },
	{ "cam_cc_ife_0_axi_clk", &cam_cc, 27 },
	{ "cam_cc_ife_1_clk", &cam_cc, 29 },
	{ "cam_cc_ife_1_dsp_clk", &cam_cc, 30 },
	{ "cam_cc_ife_1_csid_clk", &cam_cc, 31 },
	{ "cam_cc_ife_1_cphy_rx_clk", &cam_cc, 32 },
	{ "cam_cc_ife_1_axi_clk", &cam_cc, 33 },
	{ "cam_cc_ife_lite_clk", &cam_cc, 34 },
	{ "cam_cc_ife_lite_csid_clk", &cam_cc, 35 },
	{ "cam_cc_ife_lite_cphy_rx_clk", &cam_cc, 36 },
	{ "cam_cc_jpeg_clk", &cam_cc, 37 },
	{ "cam_cc_icp_clk", &cam_cc, 38 },
	{ "cam_cc_fd_core_clk", &cam_cc, 40 },
	{ "cam_cc_fd_core_uar_clk", &cam_cc, 41 },
	{ "cam_cc_cci_clk", &cam_cc, 42 },
	{ "cam_cc_lrme_clk", &cam_cc, 43 },
	{ "cam_cc_cpas_ahb_clk", &cam_cc, 44 },
	{ "cam_cc_camnoc_axi_clk", &cam_cc, 45 },
	{ "cam_cc_soc_ahb_clk", &cam_cc, 46 },
	{ "cam_cc_icp_atb_clk", &cam_cc, 47 },
	{ "cam_cc_icp_cti_clk", &cam_cc, 48 },
	{ "cam_cc_icp_ts_clk", &cam_cc, 49 },
	{ "cam_cc_icp_apb_clk", &cam_cc, 50 },
	{ "cam_cc_sys_tmr_clk", &cam_cc, 51 },
	{ "cam_cc_camnoc_atb_clk", &cam_cc, 52 },
	{ "cam_cc_csiphy3_clk", &cam_cc, 54 },
	{ "disp_cc_mdss_pclk0_clk", &disp_cc, 1 },
	{ "disp_cc_mdss_pclk1_clk", &disp_cc, 2 },
	{ "disp_cc_mdss_mdp_clk", &disp_cc, 3 },
	{ "disp_cc_mdss_rot_clk", &disp_cc, 4 },
	{ "disp_cc_mdss_mdp_lut_clk", &disp_cc, 5 },
	{ "disp_cc_mdss_vsync_clk", &disp_cc, 6 },
	{ "disp_cc_mdss_byte0_clk", &disp_cc, 7 },
	{ "disp_cc_mdss_byte0_intf_clk", &disp_cc, 8 },
	{ "disp_cc_mdss_byte1_clk", &disp_cc, 9 },
	{ "disp_cc_mdss_byte1_intf_clk", &disp_cc, 10 },
	{ "disp_cc_mdss_esc0_clk", &disp_cc, 11 },
	{ "disp_cc_mdss_esc1_clk", &disp_cc, 12 },
	{ "disp_cc_mdss_dp_link_clk", &disp_cc, 13 },
	{ "disp_cc_mdss_dp_link_intf_clk", &disp_cc, 14 },
	{ "disp_cc_mdss_dp_crypto_clk", &disp_cc, 15 },
	{ "disp_cc_mdss_dp_pixel_clk", &disp_cc, 16 },
	{ "disp_cc_mdss_dp_pixel1_clk", &disp_cc, 17 },
	{ "disp_cc_mdss_dp_aux_clk", &disp_cc, 18 },
	{ "disp_cc_mdss_ahb_clk", &disp_cc, 19 },
	{ "disp_cc_mdss_axi_clk", &disp_cc, 20 },
	{ "disp_cc_mdss_qdss_at_clk", &disp_cc, 21 },
	{ "disp_cc_mdss_qdss_tsctr_div8_clk", &disp_cc, 22 },
	{ "disp_cc_mdss_rscc_ahb_clk", &disp_cc, 23 },
	{ "disp_cc_mdss_rscc_vsync_clk", &disp_cc, 24 },
	{ "video_cc_venus_ctl_core_clk", &video_cc, 1 },
	{ "video_cc_vcodec0_core_clk", &video_cc, 2 },
	{ "video_cc_vcodec1_core_clk", &video_cc, 3 },
	{ "video_cc_venus_ctl_axi_clk", &video_cc, 4 },
	{ "video_cc_vcodec0_axi_clk", &video_cc, 5 },
	{ "video_cc_vcodec1_axi_clk", &video_cc, 6 },
	{ "video_cc_qdss_trig_clk", &video_cc, 7 },
	{ "video_cc_apb_clk", &video_cc, 8 },
	{ "video_cc_venus_ahb_clk", &video_cc, 9 },
	{ "video_cc_qdss_tsctr_div8_clk", &video_cc, 10 },
	{ "video_cc_at_clk", &video_cc, 11 },
	{ "gcc_disp_gpll0_clk_src", &gcc.mux, 76 },
	{ "gcc_disp_gpll0_div_clk_src", &gcc.mux, 77 },
	{ "gcc_usb30_prim_master_clk", &gcc.mux, 95 },
	{ "gcc_usb30_prim_sleep_clk", &gcc.mux, 96 },
	{ "gcc_usb30_prim_mock_utmi_clk", &gcc.mux, 97 },
	{ "gcc_usb3_prim_phy_aux_clk", &gcc.mux, 98 },
	{ "gcc_usb3_prim_phy_com_aux_clk", &gcc.mux, 99 },
	{ "gcc_usb3_prim_phy_pipe_clk", &gcc.mux, 100 },
	{ "gcc_usb30_sec_master_clk", &gcc.mux, 101 },
	{ "gcc_usb30_sec_sleep_clk", &gcc.mux, 102 },
	{ "gcc_usb30_sec_mock_utmi_clk", &gcc.mux, 103 },
	{ "gcc_usb3_sec_phy_aux_clk", &gcc.mux, 104 },
	{ "gcc_usb3_sec_phy_com_aux_clk", &gcc.mux, 105 },
	{ "gcc_usb3_sec_phy_pipe_clk", &gcc.mux, 106 },
	{ "gcc_usb_phy_cfg_ahb2phy_clk", &gcc.mux, 111 },
	{ "gcc_sdcc2_apps_clk", &gcc.mux, 112 },
	{ "gcc_sdcc2_ahb_clk", &gcc.mux, 113 },
	{ "gcc_sdcc4_apps_clk", &gcc.mux, 114 },
	{ "gcc_sdcc4_ahb_clk", &gcc.mux, 115 },
	{ "gcc_qupv3_wrap_0_m_ahb_clk", &gcc.mux, 116 },
	{ "gcc_qupv3_wrap_0_s_ahb_clk", &gcc.mux, 117 },
	{ "gcc_qupv3_wrap0_core_clk", &gcc.mux, 118 },
	{ "gcc_qupv3_wrap0_core_2x_clk", &gcc.mux, 119 },
	{ "gcc_qupv3_wrap0_s0_clk", &gcc.mux, 120 },
	{ "gcc_qupv3_wrap0_s1_clk", &gcc.mux, 121 },
	{ "gcc_qupv3_wrap0_s2_clk", &gcc.mux, 122 },
	{ "gcc_qupv3_wrap0_s3_clk", &gcc.mux, 123 },
	{ "gcc_qupv3_wrap0_s4_clk", &gcc.mux, 124 },
	{ "gcc_qupv3_wrap0_s5_clk", &gcc.mux, 125 },
	{ "gcc_qupv3_wrap0_s6_clk", &gcc.mux, 126 },
	{ "gcc_qupv3_wrap0_s7_clk", &gcc.mux, 127 },
	{ "gcc_qupv3_wrap1_core_2x_clk", &gcc.mux, 128 },
	{ "gcc_qupv3_wrap1_core_clk", &gcc.mux, 129 },
	{ "gcc_qupv3_wrap_1_m_ahb_clk", &gcc.mux, 130 },
	{ "gcc_qupv3_wrap_1_s_ahb_clk", &gcc.mux, 131 },
	{ "gcc_qupv3_wrap1_s0_clk", &gcc.mux, 132 },
	{ "gcc_qupv3_wrap1_s1_clk", &gcc.mux, 133 },
	{ "gcc_qupv3_wrap1_s2_clk", &gcc.mux, 134 },
	{ "gcc_qupv3_wrap1_s3_clk", &gcc.mux, 135 },
	{ "gcc_qupv3_wrap1_s4_clk", &gcc.mux, 136 },
	{ "gcc_qupv3_wrap1_s5_clk", &gcc.mux, 137 },
	{ "gcc_qupv3_wrap1_s6_clk", &gcc.mux, 138 },
	{ "gcc_qupv3_wrap1_s7_clk", &gcc.mux, 139 },
	{ "gcc_pdm_ahb_clk", &gcc.mux, 140 },
	{ "gcc_pdm_xo4_clk", &gcc.mux, 141 },
	{ "gcc_pdm2_clk", &gcc.mux, 142 },
	{ "gcc_prng_ahb_clk", &gcc.mux, 143 },
	{ "gcc_tsif_ahb_clk", &gcc.mux, 144 },
	{ "gcc_tsif_ref_clk", &gcc.mux, 145 },
	{ "gcc_tsif_inactivity_timers_clk", &gcc.mux, 146 },
	{ "gcc_boot_rom_ahb_clk", &gcc.mux, 148 },
	{ "gcc_ce1_clk", &gcc.mux, 167 },
	{ "gcc_ce1_axi_clk", &gcc.mux, 168 },
	{ "gcc_ce1_ahb_clk", &gcc.mux, 169 },
	{ "gcc_ddrss_gpu_axi_clk", &gcc.mux, 187 },
	{ "measure_only_bimc_clk", &gcc.mux, 194 },
	{ "gcc_cpuss_ahb_clk", &gcc.mux, 206 },
	{ "gcc_cpuss_gnoc_clk", &gcc.mux, 207 },
	{ "gcc_cpuss_rbcpr_clk", &gcc.mux, 208 },
	{ "gcc_cpuss_dvm_bus_clk", &gcc.mux, 211 },
	{ "pwrcl_clk", &cpu, 68, 16 },
	{ "perfcl_clk", &cpu, 69, 16 },
	{ "l3_clk", &cpu, 70, 16 },
	{ "gcc_gp1_clk", &gcc.mux, 222 },
	{ "gcc_gp2_clk", &gcc.mux, 223 },
	{ "gcc_gp3_clk", &gcc.mux, 224 },
	{ "gcc_pcie_0_slv_q2a_axi_clk", &gcc.mux, 225 },
	{ "gcc_pcie_0_slv_axi_clk", &gcc.mux, 226 },
	{ "gcc_pcie_0_mstr_axi_clk", &gcc.mux, 227 },
	{ "gcc_pcie_0_cfg_ahb_clk", &gcc.mux, 228 },
	{ "gcc_pcie_0_aux_clk", &gcc.mux, 229 },
	{ "gcc_pcie_0_pipe_clk", &gcc.mux, 230 },
	{ "gcc_pcie_1_slv_q2a_axi_clk", &gcc.mux, 232 },
	{ "gcc_pcie_1_slv_axi_clk", &gcc.mux, 233 },
	{ "gcc_pcie_1_mstr_axi_clk", &gcc.mux, 234 },
	{ "gcc_pcie_1_cfg_ahb_clk", &gcc.mux, 235 },
	{ "gcc_pcie_1_aux_clk", &gcc.mux, 236 },
	{ "gcc_pcie_1_pipe_clk", &gcc.mux, 237 },
	{ "gcc_pcie_phy_aux_clk", &gcc.mux, 239 },
	{ "gcc_ufs_card_axi_clk", &gcc.mux, 240 },
	{ "gcc_ufs_card_ahb_clk", &gcc.mux, 241 },
	{ "gcc_ufs_card_tx_symbol_0_clk", &gcc.mux, 242 },
	{ "gcc_ufs_card_rx_symbol_0_clk", &gcc.mux, 243 },
	{ "gcc_ufs_card_unipro_core_clk", &gcc.mux, 246 },
	{ "gcc_ufs_card_ice_core_clk", &gcc.mux, 247 },
	{ "gcc_ufs_card_phy_aux_clk", &gcc.mux, 248 },
	{ "gcc_ufs_card_rx_symbol_1_clk", &gcc.mux, 249 },
	{ "gcc_ufs_phy_axi_clk", &gcc.mux, 251 },
	{ "gcc_ufs_phy_ahb_clk", &gcc.mux, 252 },
	{ "gcc_ufs_phy_tx_symbol_0_clk", &gcc.mux, 253 },
	{ "gcc_ufs_phy_rx_symbol_0_clk", &gcc.mux, 254 },
	{ "gcc_ufs_phy_unipro_core_clk", &gcc.mux, 257 },
	{ "gcc_ufs_phy_ice_core_clk", &gcc.mux, 258 },
	{ "gcc_ufs_phy_phy_aux_clk", &gcc.mux, 259 },
	{ "gcc_ufs_phy_rx_symbol_1_clk", &gcc.mux, 260 },
	{ "gcc_vddcx_vs_clk", &gcc.mux, 268 },
	{ "gcc_vddmx_vs_clk", &gcc.mux, 269 },
	{ "gcc_vdda_vs_clk", &gcc.mux, 270 },
	{ "gcc_vs_ctrl_clk", &gcc.mux, 271 },
	{ "gcc_vs_ctrl_ahb_clk", &gcc.mux, 272 },
	{ "gcc_mss_vs_clk", &gcc.mux, 273 },
	{ "gcc_gpu_vs_clk", &gcc.mux, 274 },
	{ "gcc_apc_vs_clk", &gcc.mux, 275 },
	{ "gcc_aggre_usb3_prim_axi_clk", &gcc.mux, 283 },
	{ "gcc_aggre_usb3_sec_axi_clk", &gcc.mux, 284 },
	{ "gcc_aggre_ufs_phy_axi_clk", &gcc.mux, 285 },
	{ "gcc_aggre_ufs_card_axi_clk", &gcc.mux, 286 },
	{ "measure_only_ipa_2x_clk", &gcc.mux, 296 },
	{ "gcc_mss_cfg_ahb_clk", &gcc.mux, 301 },
	{ "gcc_mss_mfab_axis_clk", &gcc.mux, 302 },
	{ "gcc_mss_axis2_clk", &gcc.mux, 303 },
	{ "gcc_mss_gpll0_div_clk_src", &gcc.mux, 307 },
	{ "gcc_mss_snoc_axi_clk", &gcc.mux, 308 },
	{ "gcc_mss_q6_memnoc_axi_clk", &gcc.mux, 309 },
	{ "gcc_gpu_cfg_ahb_clk", &gcc.mux, 322 },
	{ "gpu_cc_cxo_clk", &gpu_cc, 10 },
	{ "gpu_cc_cxo_aon_clk", &gpu_cc, 11 },
	{ "gpu_cc_gx_gfx3d_clk", &gpu_cc, 12 },
	{ "gpu_cc_gx_vsense_clk", &gpu_cc, 13 },
	{ "gpu_cc_gx_qdss_tsctr_clk", &gpu_cc, 14 },
	{ "gpu_cc_gx_gmu_clk", &gpu_cc, 16 },
	{ "gpu_cc_crc_ahb_clk", &gpu_cc, 18 },
	{ "gpu_cc_cx_qdss_at_clk", &gpu_cc, 19 },
	{ "gpu_cc_cx_qdss_tsctr_clk", &gpu_cc, 20 },
	{ "gpu_cc_cx_apb_clk", &gpu_cc, 21 },
	{ "gpu_cc_cx_snoc_dvm_clk", &gpu_cc, 22 },
	{ "gpu_cc_sleep_clk", &gpu_cc, 23 },
	{ "gpu_cc_cx_qdss_trig_clk", &gpu_cc, 24 },
	{ "gpu_cc_cx_gmu_clk", &gpu_cc, 25 },
	{ "gpu_cc_cx_gfx3d_clk", &gpu_cc, 26 },
	{ "gpu_cc_cx_gfx3d_slv_clk", &gpu_cc, 27 },
	{ "gpu_cc_rbcpr_clk", &gpu_cc, 28 },
	{ "gpu_cc_rbcpr_ahb_clk", &gpu_cc, 29 },
	{ "gpu_cc_acd_cxo_clk", &gpu_cc, 31 },
	{ "gcc_gpu_memnoc_gfx_clk", &gcc.mux, 325 },
	{ "gcc_gpu_snoc_dvm_gfx_clk", &gcc.mux, 327 },
	{ "gcc_gpu_gpll0_clk_src", &gcc.mux, 328 },
	{ "gcc_gpu_gpll0_div_clk_src", &gcc.mux, 329 },
	{ "gcc_sdcc1_apps_clk", &gcc.mux, 347 },
	{ "gcc_sdcc1_ahb_clk", &gcc.mux, 348 },
	{ "gcc_sdcc1_ice_core_clk", &gcc.mux, 349 },
	{ "gcc_pcie_phy_refgen_clk", &gcc.mux, 352 },
	{}
};

struct debugcc_platform sdm845_debugcc = {
	"sdm845",
	sdm845_clocks,
};
