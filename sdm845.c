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

static struct debug_mux gcc = {
	.phys =	GCC_BASE,
	.size = GCC_SIZE,

	.enable_reg = GCC_DEBUG_CBCR,
	.enable_mask = BIT(0),

	.mux_reg = GCC_DEBUG_OFFSET,
	.mux_mask = 0x3ff,

	.div_reg = GCC_DEBUG_POST_DIV,
	.div_mask = 0xf,

	.xo_div4_reg = GCC_XO_DIV4_CBCR,
	.debug_ctl_reg = GCC_DEBUG_CTL,
	.debug_status_reg = GCC_DEBUG_STATUS,
};

static struct debug_mux cam_cc = {
	.phys = 0xad00000,
	.size = 0x10000,

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

	.enable_reg = 0xa58,
	.enable_mask = BIT(0),

	.mux_reg = 0xa4c,
	.mux_mask = 0x3f,

	.div_reg = 0xa50,
	.div_mask = 0x7,
};

static struct measure_clk sdm845_clocks[] = {
	{ "measure_only_snoc_clk", &gcc, 7, 4 },
	{ "gcc_sys_noc_cpuss_ahb_clk", &gcc, 12, 4 },
	{ "measure_only_cnoc_clk", &gcc, 21, 4 },
	{ "gcc_cfg_noc_usb3_prim_axi_clk", &gcc, 29, 4 },
	{ "gcc_cfg_noc_usb3_sec_axi_clk", &gcc, 30, 4 },
	{ "gcc_aggre_noc_pcie_tbu_clk", &gcc, 45, 4 },
	{ "gcc_video_ahb_clk", &gcc, 57, 4 },
	{ "gcc_camera_ahb_clk", &gcc, 58, 4 },
	{ "gcc_disp_ahb_clk", &gcc, 59, 4 },
	{ "gcc_qmip_video_ahb_clk", &gcc, 60, 4 },
	{ "gcc_qmip_camera_ahb_clk", &gcc, 61, 4 },
	{ "gcc_qmip_disp_ahb_clk", &gcc, 62, 4 },
	{ "gcc_video_axi_clk", &gcc, 63, 4 },
	{ "gcc_camera_axi_clk", &gcc, 64, 4 },
	{ "gcc_disp_axi_clk", &gcc, 65, 4 },
	{ "gcc_video_xo_clk", &gcc, 66, 4 },
	{ "gcc_camera_xo_clk", &gcc, 67, 4 },
	{ "gcc_disp_xo_clk", &gcc, 68, 4 },
	{ "cam_cc_mclk0_clk", &gcc, 70, 4, &cam_cc, 1, 1 },
	{ "cam_cc_mclk1_clk", &gcc, 70, 4, &cam_cc, 2, 1 },
	{ "cam_cc_mclk2_clk", &gcc, 70, 4, &cam_cc, 3, 1 },
	{ "cam_cc_mclk3_clk", &gcc, 70, 4, &cam_cc, 4, 1 },
	{ "cam_cc_csi0phytimer_clk", &gcc, 70, 4, &cam_cc, 5, 1 },
	{ "cam_cc_csiphy0_clk", &gcc, 70, 4, &cam_cc, 6, 1 },
	{ "cam_cc_csi1phytimer_clk", &gcc, 70, 4, &cam_cc, 7, 1 },
	{ "cam_cc_csiphy1_clk", &gcc, 70, 4, &cam_cc, 8, 1 },
	{ "cam_cc_csi2phytimer_clk", &gcc, 70, 4, &cam_cc, 9, 1 },
	{ "cam_cc_csiphy2_clk", &gcc, 70, 4, &cam_cc, 10, 1 },
	{ "cam_cc_bps_clk", &gcc, 70, 4, &cam_cc, 11, 1 },
	{ "cam_cc_bps_axi_clk", &gcc, 70, 4, &cam_cc, 12, 1 },
	{ "cam_cc_bps_areg_clk", &gcc, 70, 4, &cam_cc, 13, 1 },
	{ "cam_cc_bps_ahb_clk", &gcc, 70, 4, &cam_cc, 14, 1 },
	{ "cam_cc_ipe_0_clk", &gcc, 70, 4, &cam_cc, 15, 1 },
	{ "cam_cc_ipe_0_axi_clk", &gcc, 70, 4, &cam_cc, 16, 1 },
	{ "cam_cc_ipe_0_areg_clk", &gcc, 70, 4, &cam_cc, 17, 1 },
	{ "cam_cc_ipe_0_ahb_clk", &gcc, 70, 4, &cam_cc, 18, 1 },
	{ "cam_cc_ipe_1_clk", &gcc, 70, 4, &cam_cc, 19, 1 },
	{ "cam_cc_ipe_1_axi_clk", &gcc, 70, 4, &cam_cc, 20, 1 },
	{ "cam_cc_ipe_1_areg_clk", &gcc, 70, 4, &cam_cc, 21, 1 },
	{ "cam_cc_ipe_1_ahb_clk", &gcc, 70, 4, &cam_cc, 22, 1 },
	{ "cam_cc_ife_0_clk", &gcc, 70, 4, &cam_cc, 23, 1 },
	{ "cam_cc_ife_0_dsp_clk", &gcc, 70, 4, &cam_cc, 24, 1 },
	{ "cam_cc_ife_0_csid_clk", &gcc, 70, 4, &cam_cc, 25, 1 },
	{ "cam_cc_ife_0_cphy_rx_clk", &gcc, 70, 4, &cam_cc, 26, 1 },
	{ "cam_cc_ife_0_axi_clk", &gcc, 70, 4, &cam_cc, 27, 1 },
	{ "cam_cc_ife_1_clk", &gcc, 70, 4, &cam_cc, 29, 1 },
	{ "cam_cc_ife_1_dsp_clk", &gcc, 70, 4, &cam_cc, 30, 1 },
	{ "cam_cc_ife_1_csid_clk", &gcc, 70, 4, &cam_cc, 31, 1 },
	{ "cam_cc_ife_1_cphy_rx_clk", &gcc, 70, 4, &cam_cc, 32, 1 },
	{ "cam_cc_ife_1_axi_clk", &gcc, 70, 4, &cam_cc, 33, 1 },
	{ "cam_cc_ife_lite_clk", &gcc, 70, 4, &cam_cc, 34, 1 },
	{ "cam_cc_ife_lite_csid_clk", &gcc, 70, 4, &cam_cc, 35, 1 },
	{ "cam_cc_ife_lite_cphy_rx_clk", &gcc, 70, 4, &cam_cc, 36, 1 },
	{ "cam_cc_jpeg_clk", &gcc, 70, 4, &cam_cc, 37, 1 },
	{ "cam_cc_icp_clk", &gcc, 70, 4, &cam_cc, 38, 1 },
	{ "cam_cc_fd_core_clk", &gcc, 70, 4, &cam_cc, 40, 1 },
	{ "cam_cc_fd_core_uar_clk", &gcc, 70, 4, &cam_cc, 41, 1 },
	{ "cam_cc_cci_clk", &gcc, 70, 4, &cam_cc, 42, 1 },
	{ "cam_cc_lrme_clk", &gcc, 70, 4, &cam_cc, 43, 1 },
	{ "cam_cc_cpas_ahb_clk", &gcc, 70, 4, &cam_cc, 44, 1 },
	{ "cam_cc_camnoc_axi_clk", &gcc, 70, 4, &cam_cc, 45, 1 },
	{ "cam_cc_soc_ahb_clk", &gcc, 70, 4, &cam_cc, 46, 1 },
	{ "cam_cc_icp_atb_clk", &gcc, 70, 4, &cam_cc, 47, 1 },
	{ "cam_cc_icp_cti_clk", &gcc, 70, 4, &cam_cc, 48, 1 },
	{ "cam_cc_icp_ts_clk", &gcc, 70, 4, &cam_cc, 49, 1 },
	{ "cam_cc_icp_apb_clk", &gcc, 70, 4, &cam_cc, 50, 1 },
	{ "cam_cc_sys_tmr_clk", &gcc, 70, 4, &cam_cc, 51, 1 },
	{ "cam_cc_camnoc_atb_clk", &gcc, 70, 4, &cam_cc, 52, 1 },
	{ "cam_cc_csiphy3_clk", &gcc, 70, 4, &cam_cc, 54, 1 },
	{ "disp_cc_mdss_pclk0_clk", &gcc, 71, 4, &disp_cc, 1, 1 },
	{ "disp_cc_mdss_pclk1_clk", &gcc, 71, 4, &disp_cc, 2, 1 },
	{ "disp_cc_mdss_mdp_clk", &gcc, 71, 4, &disp_cc, 3, 1 },
	{ "disp_cc_mdss_rot_clk", &gcc, 71, 4, &disp_cc, 4, 1 },
	{ "disp_cc_mdss_mdp_lut_clk", &gcc, 71, 4, &disp_cc, 5, 1 },
	{ "disp_cc_mdss_vsync_clk", &gcc, 71, 4, &disp_cc, 6, 1 },
	{ "disp_cc_mdss_byte0_clk", &gcc, 71, 4, &disp_cc, 7, 1 },
	{ "disp_cc_mdss_byte0_intf_clk", &gcc, 71, 4, &disp_cc, 8, 1 },
	{ "disp_cc_mdss_byte1_clk", &gcc, 71, 4, &disp_cc, 9, 1 },
	{ "disp_cc_mdss_byte1_intf_clk", &gcc, 71, 4, &disp_cc, 10, 1 },
	{ "disp_cc_mdss_esc0_clk", &gcc, 71, 4, &disp_cc, 11, 1 },
	{ "disp_cc_mdss_esc1_clk", &gcc, 71, 4, &disp_cc, 12, 1 },
	{ "disp_cc_mdss_dp_link_clk", &gcc, 71, 4, &disp_cc, 13, 1 },
	{ "disp_cc_mdss_dp_link_intf_clk", &gcc, 71, 4, &disp_cc, 14, 1 },
	{ "disp_cc_mdss_dp_crypto_clk", &gcc, 71, 4, &disp_cc, 15, 1 },
	{ "disp_cc_mdss_dp_pixel_clk", &gcc, 71, 4, &disp_cc, 16, 1 },
	{ "disp_cc_mdss_dp_pixel1_clk", &gcc, 71, 4, &disp_cc, 17, 1 },
	{ "disp_cc_mdss_dp_aux_clk", &gcc, 71, 4, &disp_cc, 18, 1 },
	{ "disp_cc_mdss_ahb_clk", &gcc, 71, 4, &disp_cc, 19, 1 },
	{ "disp_cc_mdss_axi_clk", &gcc, 71, 4, &disp_cc, 20, 1 },
	{ "disp_cc_mdss_qdss_at_clk", &gcc, 71, 4, &disp_cc, 21, 1 },
	{ "disp_cc_mdss_qdss_tsctr_div8_clk", &gcc, 71, 4, &disp_cc, 22, 1 },
	{ "disp_cc_mdss_rscc_ahb_clk", &gcc, 71, 4, &disp_cc, 23, 1 },
	{ "disp_cc_mdss_rscc_vsync_clk", &gcc, 71, 4, &disp_cc, 24, 1 },
	{ "video_cc_venus_ctl_core_clk", &gcc, 72, 4, &video_cc, 1, 1 },
	{ "video_cc_vcodec0_core_clk", &gcc, 72, 4, &video_cc, 2, 1 },
	{ "video_cc_vcodec1_core_clk", &gcc, 72, 4, &video_cc, 3, 1 },
	{ "video_cc_venus_ctl_axi_clk", &gcc, 72, 4, &video_cc, 4, 1 },
	{ "video_cc_vcodec0_axi_clk", &gcc, 72, 4, &video_cc, 5, 1 },
	{ "video_cc_vcodec1_axi_clk", &gcc, 72, 4, &video_cc, 6, 1 },
	{ "video_cc_qdss_trig_clk", &gcc, 72, 4, &video_cc, 7, 1 },
	{ "video_cc_apb_clk", &gcc, 72, 4, &video_cc, 8, 1 },
	{ "video_cc_venus_ahb_clk", &gcc, 72, 4, &video_cc, 9, 1 },
	{ "video_cc_qdss_tsctr_div8_clk", &gcc, 72, 4, &video_cc, 10, 1 },
	{ "video_cc_at_clk", &gcc, 72, 4, &video_cc, 11, 1 },
	{ "gcc_disp_gpll0_clk_src", &gcc, 76, 4 },
	{ "gcc_disp_gpll0_div_clk_src", &gcc, 77, 4 },
	{ "gcc_usb30_prim_master_clk", &gcc, 95, 4 },
	{ "gcc_usb30_prim_sleep_clk", &gcc, 96, 4 },
	{ "gcc_usb30_prim_mock_utmi_clk", &gcc, 97, 4 },
	{ "gcc_usb3_prim_phy_aux_clk", &gcc, 98, 4 },
	{ "gcc_usb3_prim_phy_com_aux_clk", &gcc, 99, 4 },
	{ "gcc_usb3_prim_phy_pipe_clk", &gcc, 100, 4 },
	{ "gcc_usb30_sec_master_clk", &gcc, 101, 4 },
	{ "gcc_usb30_sec_sleep_clk", &gcc, 102, 4 },
	{ "gcc_usb30_sec_mock_utmi_clk", &gcc, 103, 4 },
	{ "gcc_usb3_sec_phy_aux_clk", &gcc, 104, 4 },
	{ "gcc_usb3_sec_phy_com_aux_clk", &gcc, 105, 4 },
	{ "gcc_usb3_sec_phy_pipe_clk", &gcc, 106, 4 },
	{ "gcc_usb_phy_cfg_ahb2phy_clk", &gcc, 111, 4 },
	{ "gcc_sdcc2_apps_clk", &gcc, 112, 4 },
	{ "gcc_sdcc2_ahb_clk", &gcc, 113, 4 },
	{ "gcc_sdcc4_apps_clk", &gcc, 114, 4 },
	{ "gcc_sdcc4_ahb_clk", &gcc, 115, 4 },
	{ "gcc_qupv3_wrap_0_m_ahb_clk", &gcc, 116, 4 },
	{ "gcc_qupv3_wrap_0_s_ahb_clk", &gcc, 117, 4 },
	{ "gcc_qupv3_wrap0_core_clk", &gcc, 118, 4 },
	{ "gcc_qupv3_wrap0_core_2x_clk", &gcc, 119, 4 },
	{ "gcc_qupv3_wrap0_s0_clk", &gcc, 120, 4 },
	{ "gcc_qupv3_wrap0_s1_clk", &gcc, 121, 4 },
	{ "gcc_qupv3_wrap0_s2_clk", &gcc, 122, 4 },
	{ "gcc_qupv3_wrap0_s3_clk", &gcc, 123, 4 },
	{ "gcc_qupv3_wrap0_s4_clk", &gcc, 124, 4 },
	{ "gcc_qupv3_wrap0_s5_clk", &gcc, 125, 4 },
	{ "gcc_qupv3_wrap0_s6_clk", &gcc, 126, 4 },
	{ "gcc_qupv3_wrap0_s7_clk", &gcc, 127, 4 },
	{ "gcc_qupv3_wrap1_core_2x_clk", &gcc, 128, 4 },
	{ "gcc_qupv3_wrap1_core_clk", &gcc, 129, 4 },
	{ "gcc_qupv3_wrap_1_m_ahb_clk", &gcc, 130, 4 },
	{ "gcc_qupv3_wrap_1_s_ahb_clk", &gcc, 131, 4 },
	{ "gcc_qupv3_wrap1_s0_clk", &gcc, 132, 4 },
	{ "gcc_qupv3_wrap1_s1_clk", &gcc, 133, 4 },
	{ "gcc_qupv3_wrap1_s2_clk", &gcc, 134, 4 },
	{ "gcc_qupv3_wrap1_s3_clk", &gcc, 135, 4 },
	{ "gcc_qupv3_wrap1_s4_clk", &gcc, 136, 4 },
	{ "gcc_qupv3_wrap1_s5_clk", &gcc, 137, 4 },
	{ "gcc_qupv3_wrap1_s6_clk", &gcc, 138, 4 },
	{ "gcc_qupv3_wrap1_s7_clk", &gcc, 139, 4 },
	{ "gcc_pdm_ahb_clk", &gcc, 140, 4 },
	{ "gcc_pdm_xo4_clk", &gcc, 141, 4 },
	{ "gcc_pdm2_clk", &gcc, 142, 4 },
	{ "gcc_prng_ahb_clk", &gcc, 143, 4 },
	{ "gcc_tsif_ahb_clk", &gcc, 144, 4 },
	{ "gcc_tsif_ref_clk", &gcc, 145, 4 },
	{ "gcc_tsif_inactivity_timers_clk", &gcc, 146, 4 },
	{ "gcc_boot_rom_ahb_clk", &gcc, 148, 4 },
	{ "gcc_ce1_clk", &gcc, 167, 4 },
	{ "gcc_ce1_axi_clk", &gcc, 168, 4 },
	{ "gcc_ce1_ahb_clk", &gcc, 169, 4 },
	{ "gcc_ddrss_gpu_axi_clk", &gcc, 187, 4 },
	{ "measure_only_bimc_clk", &gcc, 194, 4 },
	{ "gcc_cpuss_ahb_clk", &gcc, 206, 4 },
	{ "gcc_cpuss_gnoc_clk", &gcc, 207, 4 },
	{ "gcc_cpuss_rbcpr_clk", &gcc, 208, 4 },
	{ "gcc_cpuss_dvm_bus_clk", &gcc, 211, 4 },
	{ "pwrcl_clk", &gcc, 214, 4, &cpu, 68, 1, 16 },
	{ "perfcl_clk", &gcc, 214, 4, &cpu, 69, 1, 16 },
	{ "l3_clk", &gcc, 214, 4, &cpu, 70, 1, 16 },
	{ "gcc_gp1_clk", &gcc, 222, 4 },
	{ "gcc_gp2_clk", &gcc, 223, 4 },
	{ "gcc_gp3_clk", &gcc, 224, 4 },
	{ "gcc_pcie_0_slv_q2a_axi_clk", &gcc, 225, 4 },
	{ "gcc_pcie_0_slv_axi_clk", &gcc, 226, 4 },
	{ "gcc_pcie_0_mstr_axi_clk", &gcc, 227, 4 },
	{ "gcc_pcie_0_cfg_ahb_clk", &gcc, 228, 4 },
	{ "gcc_pcie_0_aux_clk", &gcc, 229, 4 },
	{ "gcc_pcie_0_pipe_clk", &gcc, 230, 4 },
	{ "gcc_pcie_1_slv_q2a_axi_clk", &gcc, 232, 4 },
	{ "gcc_pcie_1_slv_axi_clk", &gcc, 233, 4 },
	{ "gcc_pcie_1_mstr_axi_clk", &gcc, 234, 4 },
	{ "gcc_pcie_1_cfg_ahb_clk", &gcc, 235, 4 },
	{ "gcc_pcie_1_aux_clk", &gcc, 236, 4 },
	{ "gcc_pcie_1_pipe_clk", &gcc, 237, 4 },
	{ "gcc_pcie_phy_aux_clk", &gcc, 239, 4 },
	{ "gcc_ufs_card_axi_clk", &gcc, 240, 4 },
	{ "gcc_ufs_card_ahb_clk", &gcc, 241, 4 },
	{ "gcc_ufs_card_tx_symbol_0_clk", &gcc, 242, 4 },
	{ "gcc_ufs_card_rx_symbol_0_clk", &gcc, 243, 4 },
	{ "gcc_ufs_card_unipro_core_clk", &gcc, 246, 4 },
	{ "gcc_ufs_card_ice_core_clk", &gcc, 247, 4 },
	{ "gcc_ufs_card_phy_aux_clk", &gcc, 248, 4 },
	{ "gcc_ufs_card_rx_symbol_1_clk", &gcc, 249, 4 },
	{ "gcc_ufs_phy_axi_clk", &gcc, 251, 4 },
	{ "gcc_ufs_phy_ahb_clk", &gcc, 252, 4 },
	{ "gcc_ufs_phy_tx_symbol_0_clk", &gcc, 253, 4 },
	{ "gcc_ufs_phy_rx_symbol_0_clk", &gcc, 254, 4 },
	{ "gcc_ufs_phy_unipro_core_clk", &gcc, 257, 4 },
	{ "gcc_ufs_phy_ice_core_clk", &gcc, 258, 4 },
	{ "gcc_ufs_phy_phy_aux_clk", &gcc, 259, 4 },
	{ "gcc_ufs_phy_rx_symbol_1_clk", &gcc, 260, 4 },
	{ "gcc_vddcx_vs_clk", &gcc, 268, 4 },
	{ "gcc_vddmx_vs_clk", &gcc, 269, 4 },
	{ "gcc_vdda_vs_clk", &gcc, 270, 4 },
	{ "gcc_vs_ctrl_clk", &gcc, 271, 4 },
	{ "gcc_vs_ctrl_ahb_clk", &gcc, 272, 4 },
	{ "gcc_mss_vs_clk", &gcc, 273, 4 },
	{ "gcc_gpu_vs_clk", &gcc, 274, 4 },
	{ "gcc_apc_vs_clk", &gcc, 275, 4 },
	{ "gcc_aggre_usb3_prim_axi_clk", &gcc, 283, 4 },
	{ "gcc_aggre_usb3_sec_axi_clk", &gcc, 284, 4 },
	{ "gcc_aggre_ufs_phy_axi_clk", &gcc, 285, 4 },
	{ "gcc_aggre_ufs_card_axi_clk", &gcc, 286, 4 },
	{ "measure_only_ipa_2x_clk", &gcc, 296, 4 },
	{ "gcc_mss_cfg_ahb_clk", &gcc, 301, 4 },
	{ "gcc_mss_mfab_axis_clk", &gcc, 302, 4 },
	{ "gcc_mss_axis2_clk", &gcc, 303, 4 },
	{ "gcc_mss_gpll0_div_clk_src", &gcc, 307, 4 },
	{ "gcc_mss_snoc_axi_clk", &gcc, 308, 4 },
	{ "gcc_mss_q6_memnoc_axi_clk", &gcc, 309, 4 },
	{ "gcc_gpu_cfg_ahb_clk", &gcc, 322, 4 },
	{ "gpu_cc_cxo_clk", &gcc, 324, 4, &gpu_cc, 10, 1 },
	{ "gpu_cc_cxo_aon_clk", &gcc, 324, 4, &gpu_cc, 11, 1 },
	{ "gpu_cc_gx_gfx3d_clk", &gcc, 324, 4, &gpu_cc, 12, 1 },
	{ "gpu_cc_gx_vsense_clk", &gcc, 324, 4, &gpu_cc, 13, 1 },
	{ "gpu_cc_gx_qdss_tsctr_clk", &gcc, 324, 4, &gpu_cc, 14, 1 },
	{ "gpu_cc_gx_gmu_clk", &gcc, 324, 4, &gpu_cc, 16, 1 },
	{ "gpu_cc_crc_ahb_clk", &gcc, 324, 4, &gpu_cc, 18, 1 },
	{ "gpu_cc_cx_qdss_at_clk", &gcc, 324, 4, &gpu_cc, 19, 1 },
	{ "gpu_cc_cx_qdss_tsctr_clk", &gcc, 324, 4, &gpu_cc, 20, 1 },
	{ "gpu_cc_cx_apb_clk", &gcc, 324, 4, &gpu_cc, 21, 1 },
	{ "gpu_cc_cx_snoc_dvm_clk", &gcc, 324, 4, &gpu_cc, 22, 1 },
	{ "gpu_cc_sleep_clk", &gcc, 324, 4, &gpu_cc, 23, 1 },
	{ "gpu_cc_cx_qdss_trig_clk", &gcc, 324, 4, &gpu_cc, 24, 1 },
	{ "gpu_cc_cx_gmu_clk", &gcc, 324, 4, &gpu_cc, 25, 1 },
	{ "gpu_cc_cx_gfx3d_clk", &gcc, 324, 4, &gpu_cc, 26, 1 },
	{ "gpu_cc_cx_gfx3d_slv_clk", &gcc, 324, 4, &gpu_cc, 27, 1 },
	{ "gpu_cc_rbcpr_clk", &gcc, 324, 4, &gpu_cc, 28, 1 },
	{ "gpu_cc_rbcpr_ahb_clk", &gcc, 324, 4, &gpu_cc, 29, 1 },
	{ "gpu_cc_acd_cxo_clk", &gcc, 324, 4, &gpu_cc, 31, 1 },
	{ "gcc_gpu_memnoc_gfx_clk", &gcc, 325, 4 },
	{ "gcc_gpu_snoc_dvm_gfx_clk", &gcc, 327, 4 },
	{ "gcc_gpu_gpll0_clk_src", &gcc, 328, 4 },
	{ "gcc_gpu_gpll0_div_clk_src", &gcc, 329, 4 },
	{ "gcc_sdcc1_apps_clk", &gcc, 347, 4 },
	{ "gcc_sdcc1_ahb_clk", &gcc, 348, 4 },
	{ "gcc_sdcc1_ice_core_clk", &gcc, 349, 4 },
	{ "gcc_pcie_phy_refgen_clk", &gcc, 352, 4 },
	{}
};

struct debugcc_platform sdm845_debugcc = {
	"sdm845",
	sdm845_clocks,
};
