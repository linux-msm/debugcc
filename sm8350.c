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

#define GCC_DEBUG_POST_DIV		0x62004
#define GCC_DEBUG_CBCR			0x62008
#define GCC_DEBUG_OFFSET		0x62000
#define GCC_DEBUG_CTL			0x62038
#define GCC_DEBUG_STATUS		0x6203C
#define GCC_XO_DIV4_CBCR		0x4300C

static struct debug_mux gcc = {
	.phys =	0x162000,
	.size = GCC_SIZE,

	.enable_reg = 0x8,
	.enable_mask = BIT(0),

	.mux_reg = 0x0,
	.mux_mask = 0x3ff,

	.div_reg = 0x4,
	.div_mask = 0xf,

	.xo_div4_reg = 0xc,
	.debug_ctl_reg = 0x38,
	.debug_status_reg = 0x3c,
};

static struct debug_mux cam_cc = {
	.phys = 0xad00000,
	.size = 0x10000,

	.enable_reg = 0xd008,
	.enable_mask = BIT(0),

	.mux_reg = 0xd000,
	.mux_mask = 0xff,

	.div_reg = 0xd004,
	.div_mask = 0x3,
};

static struct debug_mux disp_cc = {
	.phys = 0xaf00000,
	.size = 0x10000,

	.enable_reg = 0x500c,
	.enable_mask = BIT(0),

	.mux_reg = 0x7000,
	.mux_mask = 0xff,

	.div_reg = 0x5008,
	.div_mask = 0x3,
};

static struct debug_mux gpu_cc = {
	.phys = 0x3d90000,
	.size = 0x9000,

	.enable_reg = 0x1100,
	.enable_mask = BIT(0),

	.mux_reg = 0x1568,
	.mux_mask = 0xff,

	.div_reg = 0x10fc,
	.div_mask = 0xf,
};

static struct debug_mux video_cc = {
	.phys = 0xaaf0000,
	.size = 0x10000,

	.enable_reg = 0xebc,
	.enable_mask = BIT(0),

	.mux_reg = 0xa4c,
	.mux_mask = 0x3f,

	.div_reg = 0xe9c,
	.div_mask = 0x7,
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
	{ "cam_cc_bps_ahb_clk", &gcc, 0x4D, 2, &cam_cc, 0x18, 4 },
	{ "cam_cc_bps_areg_clk", &gcc, 0x4D, 2, &cam_cc, 0x17, 4 },
	{ "cam_cc_bps_axi_clk", &gcc, 0x4D, 2, &cam_cc, 0x16, 4 },
	{ "cam_cc_bps_clk", &gcc, 0x4D, 2, &cam_cc, 0x14, 4 },
	{ "cam_cc_camnoc_axi_clk", &gcc, 0x4D, 2, &cam_cc, 0x3C, 4 },
	{ "cam_cc_camnoc_dcd_xo_clk", &gcc, 0x4D, 2, &cam_cc, 0x3D, 4 },
	{ "cam_cc_cci_0_clk", &gcc, 0x4D, 2, &cam_cc, 0x39, 4 },
	{ "cam_cc_cci_1_clk", &gcc, 0x4D, 2, &cam_cc, 0x3A, 4 },
	{ "cam_cc_core_ahb_clk", &gcc, 0x4D, 2, &cam_cc, 0x40, 4 },
	{ "cam_cc_cpas_ahb_clk", &gcc, 0x4D, 2, &cam_cc, 0x3B, 4 },
	{ "cam_cc_csi0phytimer_clk", &gcc, 0x4D, 2, &cam_cc, 0x8, 4 },
	{ "cam_cc_csi1phytimer_clk", &gcc, 0x4D, 2, &cam_cc, 0xA, 4 },
	{ "cam_cc_csi2phytimer_clk", &gcc, 0x4D, 2, &cam_cc, 0xC, 4 },
	{ "cam_cc_csi3phytimer_clk", &gcc, 0x4D, 2, &cam_cc, 0xE, 4 },
	{ "cam_cc_csi4phytimer_clk", &gcc, 0x4D, 2, &cam_cc, 0x10, 4 },
	{ "cam_cc_csi5phytimer_clk", &gcc, 0x4D, 2, &cam_cc, 0x12, 4 },
	{ "cam_cc_csiphy0_clk", &gcc, 0x4D, 2, &cam_cc, 0x9, 4 },
	{ "cam_cc_csiphy1_clk", &gcc, 0x4D, 2, &cam_cc, 0xB, 4 },
	{ "cam_cc_csiphy2_clk", &gcc, 0x4D, 2, &cam_cc, 0xD, 4 },
	{ "cam_cc_csiphy3_clk", &gcc, 0x4D, 2, &cam_cc, 0xF, 4 },
	{ "cam_cc_csiphy4_clk", &gcc, 0x4D, 2, &cam_cc, 0x11, 4 },
	{ "cam_cc_csiphy5_clk", &gcc, 0x4D, 2, &cam_cc, 0x13, 4 },
	{ "cam_cc_icp_ahb_clk", &gcc, 0x4D, 2, &cam_cc, 0x36, 4 },
	{ "cam_cc_icp_clk", &gcc, 0x4D, 2, &cam_cc, 0x35, 4 },
	{ "cam_cc_ife_0_ahb_clk", &gcc, 0x4D, 2, &cam_cc, 0x26, 4 },
	{ "cam_cc_ife_0_areg_clk", &gcc, 0x4D, 2, &cam_cc, 0x1F, 4 },
	{ "cam_cc_ife_0_axi_clk", &gcc, 0x4D, 2, &cam_cc, 0x25, 4 },
	{ "cam_cc_ife_0_clk", &gcc, 0x4D, 2, &cam_cc, 0x1E, 4 },
	{ "cam_cc_ife_0_cphy_rx_clk", &gcc, 0x4D, 2, &cam_cc, 0x24, 4 },
	{ "cam_cc_ife_0_csid_clk", &gcc, 0x4D, 2, &cam_cc, 0x22, 4 },
	{ "cam_cc_ife_0_dsp_clk", &gcc, 0x4D, 2, &cam_cc, 0x21, 4 },
	{ "cam_cc_ife_1_ahb_clk", &gcc, 0x4D, 2, &cam_cc, 0x2E, 4 },
	{ "cam_cc_ife_1_areg_clk", &gcc, 0x4D, 2, &cam_cc, 0x29, 4 },
	{ "cam_cc_ife_1_axi_clk", &gcc, 0x4D, 2, &cam_cc, 0x2D, 4 },
	{ "cam_cc_ife_1_clk", &gcc, 0x4D, 2, &cam_cc, 0x27, 4 },
	{ "cam_cc_ife_1_cphy_rx_clk", &gcc, 0x4D, 2, &cam_cc, 0x2C, 4 },
	{ "cam_cc_ife_1_csid_clk", &gcc, 0x4D, 2, &cam_cc, 0x2B, 4 },
	{ "cam_cc_ife_1_dsp_clk", &gcc, 0x4D, 2, &cam_cc, 0x2A, 4 },
	{ "cam_cc_ife_2_ahb_clk", &gcc, 0x4D, 2, &cam_cc, 0x54, 4 },
	{ "cam_cc_ife_2_areg_clk", &gcc, 0x4D, 2, &cam_cc, 0x37, 4 },
	{ "cam_cc_ife_2_axi_clk", &gcc, 0x4D, 2, &cam_cc, 0x53, 4 },
	{ "cam_cc_ife_2_clk", &gcc, 0x4D, 2, &cam_cc, 0x7, 4 },
	{ "cam_cc_ife_2_cphy_rx_clk", &gcc, 0x4D, 2, &cam_cc, 0x52, 4 },
	{ "cam_cc_ife_2_csid_clk", &gcc, 0x4D, 2, &cam_cc, 0x51, 4 },
	{ "cam_cc_ife_lite_ahb_clk", &gcc, 0x4D, 2, &cam_cc, 0x32, 4 },
	{ "cam_cc_ife_lite_axi_clk", &gcc, 0x4D, 2, &cam_cc, 0x49, 4 },
	{ "cam_cc_ife_lite_clk", &gcc, 0x4D, 2, &cam_cc, 0x2F, 4 },
	{ "cam_cc_ife_lite_cphy_rx_clk", &gcc, 0x4D, 2, &cam_cc, 0x31, 4 },
	{ "cam_cc_ife_lite_csid_clk", &gcc, 0x4D, 2, &cam_cc, 0x30, 4 },
	{ "cam_cc_ipe_0_ahb_clk", &gcc, 0x4D, 2, &cam_cc, 0x1D, 4 },
	{ "cam_cc_ipe_0_areg_clk", &gcc, 0x4D, 2, &cam_cc, 0x1C, 4 },
	{ "cam_cc_ipe_0_axi_clk", &gcc, 0x4D, 2, &cam_cc, 0x1B, 4 },
	{ "cam_cc_ipe_0_clk", &gcc, 0x4D, 2, &cam_cc, 0x19, 4 },
	{ "cam_cc_jpeg_clk", &gcc, 0x4D, 2, &cam_cc, 0x33, 4 },
	{ "cam_cc_mclk0_clk", &gcc, 0x4D, 2, &cam_cc, 0x1, 4 },
	{ "cam_cc_mclk1_clk", &gcc, 0x4D, 2, &cam_cc, 0x2, 4 },
	{ "cam_cc_mclk2_clk", &gcc, 0x4D, 2, &cam_cc, 0x3, 4 },
	{ "cam_cc_mclk3_clk", &gcc, 0x4D, 2, &cam_cc, 0x4, 4 },
	{ "cam_cc_mclk4_clk", &gcc, 0x4D, 2, &cam_cc, 0x5, 4 },
	{ "cam_cc_mclk5_clk", &gcc, 0x4D, 2, &cam_cc, 0x6, 4 },
	{ "cam_cc_sbi_ahb_clk", &gcc, 0x4D, 2, &cam_cc, 0x4E, 4 },
	{ "cam_cc_sbi_axi_clk", &gcc, 0x4D, 2, &cam_cc, 0x4D, 4 },
	{ "cam_cc_sbi_clk", &gcc, 0x4D, 2, &cam_cc, 0x4A, 4 },
	{ "cam_cc_sbi_cphy_rx_0_clk", &gcc, 0x4D, 2, &cam_cc, 0x4C, 4 },
	{ "cam_cc_sbi_cphy_rx_1_clk", &gcc, 0x4D, 2, &cam_cc, 0x56, 4 },
	{ "cam_cc_sbi_csid_0_clk", &gcc, 0x4D, 2, &cam_cc, 0x4B, 4 },
	{ "cam_cc_sbi_csid_1_clk", &gcc, 0x4D, 2, &cam_cc, 0x57, 4 },
	{ "cam_cc_sbi_ife_0_clk", &gcc, 0x4D, 2, &cam_cc, 0x4F, 4 },
	{ "cam_cc_sbi_ife_1_clk", &gcc, 0x4D, 2, &cam_cc, 0x50, 4 },
	{ "cam_cc_sbi_ife_2_clk", &gcc, 0x4D, 2, &cam_cc, 0x55, 4 },
	{ "cam_cc_sleep_clk", &gcc, 0x4D, 2, &cam_cc, 0x42, 4 },
/* disp_cc_debug_mux is 0x53 */
	{ "disp_cc_mdss_ahb_clk", &gcc, 0x53, 2, &disp_cc, 0x2A, 4 },
	{ "disp_cc_mdss_byte0_clk", &gcc, 0x53, 2, &disp_cc, 0x15, 4 },
	{ "disp_cc_mdss_byte0_intf_clk", &gcc, 0x53, 2, &disp_cc, 0x16, 4 },
	{ "disp_cc_mdss_byte1_clk", &gcc, 0x53, 2, &disp_cc, 0x17, 4 },
	{ "disp_cc_mdss_byte1_intf_clk", &gcc, 0x53, 2, &disp_cc, 0x18, 4 },
	{ "disp_cc_mdss_dp_aux1_clk", &gcc, 0x53, 2, &disp_cc, 0x25, 4 },
	{ "disp_cc_mdss_dp_aux_clk", &gcc, 0x53, 2, &disp_cc, 0x20, 4 },
	{ "disp_cc_mdss_dp_link1_clk", &gcc, 0x53, 2, &disp_cc, 0x22, 4 },
	{ "disp_cc_mdss_dp_link1_intf_clk", &gcc, 0x53, 2, &disp_cc, 0x23, 4 },
	{ "disp_cc_mdss_dp_link_clk", &gcc, 0x53, 2, &disp_cc, 0x1B, 4 },
	{ "disp_cc_mdss_dp_link_intf_clk", &gcc, 0x53, 2, &disp_cc, 0x1C, 4 },
	{ "disp_cc_mdss_dp_pixel1_clk", &gcc, 0x53, 2, &disp_cc, 0x1F, 4 },
	{ "disp_cc_mdss_dp_pixel2_clk", &gcc, 0x53, 2, &disp_cc, 0x21, 4 },
	{ "disp_cc_mdss_dp_pixel_clk", &gcc, 0x53, 2, &disp_cc, 0x1E, 4 },
	{ "disp_cc_mdss_edp_aux_clk", &gcc, 0x53, 2, &disp_cc, 0x29, 4 },
	{ "disp_cc_mdss_edp_link_clk", &gcc, 0x53, 2, &disp_cc, 0x27, 4 },
	{ "disp_cc_mdss_edp_link_intf_clk", &gcc, 0x53, 2, &disp_cc, 0x28, 4 },
	{ "disp_cc_mdss_edp_pixel_clk", &gcc, 0x53, 2, &disp_cc, 0x26, 4 },
	{ "disp_cc_mdss_esc0_clk", &gcc, 0x53, 2, &disp_cc, 0x19, 4 },
	{ "disp_cc_mdss_esc1_clk", &gcc, 0x53, 2, &disp_cc, 0x1A, 4 },
	{ "disp_cc_mdss_mdp_clk", &gcc, 0x53, 2, &disp_cc, 0x11, 4 },
	{ "disp_cc_mdss_mdp_lut_clk", &gcc, 0x53, 2, &disp_cc, 0x13, 4 },
	{ "disp_cc_mdss_non_gdsc_ahb_clk", &gcc, 0x53, 2, &disp_cc, 0x2B, 4 },
	{ "disp_cc_mdss_pclk0_clk", &gcc, 0x53, 2, &disp_cc, 0xF, 4 },
	{ "disp_cc_mdss_pclk1_clk", &gcc, 0x53, 2, &disp_cc, 0x10, 4 },
	{ "disp_cc_mdss_rot_clk", &gcc, 0x53, 2, &disp_cc, 0x12, 4 },
	{ "disp_cc_mdss_rscc_ahb_clk", &gcc, 0x53, 2, &disp_cc, 0x2D, 4 },
	{ "disp_cc_mdss_rscc_vsync_clk", &gcc, 0x53, 2, &disp_cc, 0x2C, 4 },
	{ "disp_cc_mdss_vsync_clk", &gcc, 0x53, 2, &disp_cc, 0x14, 4 },
	{ "disp_cc_sleep_clk", &gcc, 0x53, 2, &disp_cc, 0x36, 4 },
// gcc
	{ "core_bi_pll_test_se", &gcc, 0x5, 2 },
	{ "gcc_aggre_noc_pcie_0_axi_clk", &gcc, 0x138, 2 },
	{ "gcc_aggre_noc_pcie_1_axi_clk", &gcc, 0x139, 2 },
	{ "gcc_aggre_noc_pcie_tbu_clk", &gcc, 0x34, 2 },
	{ "gcc_aggre_ufs_card_axi_clk", &gcc, 0x13D, 2 },
	{ "gcc_aggre_ufs_phy_axi_clk", &gcc, 0x13C, 2 },
	{ "gcc_aggre_usb3_prim_axi_clk", &gcc, 0x13A, 2 },
	{ "gcc_aggre_usb3_sec_axi_clk", &gcc, 0x13B, 2 },
	{ "gcc_boot_rom_ahb_clk", &gcc, 0xA8, 2 },
	{ "gcc_camera_ahb_clk", &gcc, 0x47, 2 },
	{ "gcc_camera_hf_axi_clk", &gcc, 0x4A, 2 },
	{ "gcc_camera_sf_axi_clk", &gcc, 0x4B, 2 },
	{ "gcc_camera_xo_clk", &gcc, 0x4C, 2 },
	{ "gcc_cfg_noc_usb3_prim_axi_clk", &gcc, 0x1F, 2 },
	{ "gcc_cfg_noc_usb3_sec_axi_clk", &gcc, 0x20, 2 },
	{ "gcc_ddrss_gpu_axi_clk", &gcc, 0xC9, 2 },
	{ "gcc_ddrss_pcie_sf_tbu_clk", &gcc, 0xCA, 2 },
	{ "gcc_disp_ahb_clk", &gcc, 0x4E, 2 },
	{ "gcc_disp_hf_axi_clk", &gcc, 0x50, 2 },
	{ "gcc_disp_sf_axi_clk", &gcc, 0x51, 2 },
	{ "gcc_disp_xo_clk", &gcc, 0x52, 2 },
	{ "gcc_gp1_clk", &gcc, 0xF1, 2 },
	{ "gcc_gp2_clk", &gcc, 0xF2, 2 },
	{ "gcc_gp3_clk", &gcc, 0xF3, 2 },
	{ "gcc_gpu_cfg_ahb_clk", &gcc, 0x151, 2 },
	{ "gcc_gpu_gpll0_clk_src", &gcc, 0x158, 2 },
	{ "gcc_gpu_gpll0_div_clk_src", &gcc, 0x159, 2 },
	{ "gcc_gpu_memnoc_gfx_clk", &gcc, 0x154, 2 },
	{ "gcc_gpu_snoc_dvm_gfx_clk", &gcc, 0x157, 2 },
	{ "gcc_pcie0_phy_rchng_clk", &gcc, 0xFA, 2 },
	{ "gcc_pcie1_phy_rchng_clk", &gcc, 0x103, 2 },
	{ "gcc_pcie_0_aux_clk", &gcc, 0xF8, 2 },
	{ "gcc_pcie_0_cfg_ahb_clk", &gcc, 0xF7, 2 },
	{ "gcc_pcie_0_mstr_axi_clk", &gcc, 0xF6, 2 },
	{ "gcc_pcie_0_pipe_clk", &gcc, 0xF9, 2 },
	{ "gcc_pcie_0_slv_axi_clk", &gcc, 0xF5, 2 },
	{ "gcc_pcie_0_slv_q2a_axi_clk", &gcc, 0xF4, 2 },
	{ "gcc_pcie_1_aux_clk", &gcc, 0x101, 2 },
	{ "gcc_pcie_1_cfg_ahb_clk", &gcc, 0x100, 2 },
	{ "gcc_pcie_1_mstr_axi_clk", &gcc, 0xFF, 2 },
	{ "gcc_pcie_1_pipe_clk", &gcc, 0x102, 2 },
	{ "gcc_pcie_1_slv_axi_clk", &gcc, 0xFE, 2 },
	{ "gcc_pcie_1_slv_q2a_axi_clk", &gcc, 0xFD, 2 },
	{ "gcc_pdm2_clk", &gcc, 0x9E, 2 },
	{ "gcc_pdm_ahb_clk", &gcc, 0x9C, 2 },
	{ "gcc_pdm_xo4_clk", &gcc, 0x9D, 2 },
	{ "gcc_qmip_camera_nrt_ahb_clk", &gcc, 0x48, 2 },
	{ "gcc_qmip_camera_rt_ahb_clk", &gcc, 0x49, 2 },
	{ "gcc_qmip_disp_ahb_clk", &gcc, 0x4F, 2 },
	{ "gcc_qmip_video_cvp_ahb_clk", &gcc, 0x55, 2 },
	{ "gcc_qmip_video_vcodec_ahb_clk", &gcc, 0x56, 2 },
	{ "gcc_qupv3_wrap0_core_2x_clk", &gcc, 0x89, 2 },
	{ "gcc_qupv3_wrap0_core_clk", &gcc, 0x88, 2 },
	{ "gcc_qupv3_wrap0_s0_clk", &gcc, 0x8A, 2 },
	{ "gcc_qupv3_wrap0_s1_clk", &gcc, 0x8B, 2 },
	{ "gcc_qupv3_wrap0_s2_clk", &gcc, 0x8C, 2 },
	{ "gcc_qupv3_wrap0_s3_clk", &gcc, 0x8D, 2 },
	{ "gcc_qupv3_wrap0_s4_clk", &gcc, 0x8E, 2 },
	{ "gcc_qupv3_wrap0_s5_clk", &gcc, 0x8F, 2 },
	{ "gcc_qupv3_wrap0_s6_clk", &gcc, 0x90, 2 },
	{ "gcc_qupv3_wrap0_s7_clk", &gcc, 0x91, 2 },
	{ "gcc_qupv3_wrap1_core_2x_clk", &gcc, 0x95, 2 },
	{ "gcc_qupv3_wrap1_core_clk", &gcc, 0x94, 2 },
	{ "gcc_qupv3_wrap1_s0_clk", &gcc, 0x96, 2 },
	{ "gcc_qupv3_wrap1_s1_clk", &gcc, 0x97, 2 },
	{ "gcc_qupv3_wrap1_s2_clk", &gcc, 0x98, 2 },
	{ "gcc_qupv3_wrap1_s3_clk", &gcc, 0x99, 2 },
	{ "gcc_qupv3_wrap1_s4_clk", &gcc, 0x9A, 2 },
	{ "gcc_qupv3_wrap2_core_2x_clk", &gcc, 0x16E, 2 },
	{ "gcc_qupv3_wrap2_core_clk", &gcc, 0x16D, 2 },
	{ "gcc_qupv3_wrap2_s0_clk", &gcc, 0x16F, 2 },
	{ "gcc_qupv3_wrap2_s1_clk", &gcc, 0x170, 2 },
	{ "gcc_qupv3_wrap2_s2_clk", &gcc, 0x171, 2 },
	{ "gcc_qupv3_wrap2_s3_clk", &gcc, 0x172, 2 },
	{ "gcc_qupv3_wrap2_s4_clk", &gcc, 0x173, 2 },
	{ "gcc_qupv3_wrap2_s5_clk", &gcc, 0x174, 2 },
	{ "gcc_qupv3_wrap_0_m_ahb_clk", &gcc, 0x86, 2 },
	{ "gcc_qupv3_wrap_0_s_ahb_clk", &gcc, 0x87, 2 },
	{ "gcc_qupv3_wrap_2_m_ahb_clk", &gcc, 0x16B, 2 },
	{ "gcc_qupv3_wrap_2_s_ahb_clk", &gcc, 0x16C, 2 },
	{ "gcc_sdcc2_ahb_clk", &gcc, 0x83, 2 },
	{ "gcc_sdcc2_apps_clk", &gcc, 0x82, 2 },
	{ "gcc_sdcc4_ahb_clk", &gcc, 0x85, 2 },
	{ "gcc_sdcc4_apps_clk", &gcc, 0x84, 2 },
	{ "gcc_throttle_pcie_ahb_clk", &gcc, 0x40, 2 },
	{ "gcc_ufs_card_ahb_clk", &gcc, 0x107, 2 },
	{ "gcc_ufs_card_axi_clk", &gcc, 0x106, 2 },
	{ "gcc_ufs_card_ice_core_clk", &gcc, 0x10D, 2 },
	{ "gcc_ufs_card_phy_aux_clk", &gcc, 0x10E, 2 },
	{ "gcc_ufs_card_rx_symbol_0_clk", &gcc, 0x109, 2 },
	{ "gcc_ufs_card_rx_symbol_1_clk", &gcc, 0x10F, 2 },
	{ "gcc_ufs_card_tx_symbol_0_clk", &gcc, 0x108, 2 },
	{ "gcc_ufs_card_unipro_core_clk", &gcc, 0x10C, 2 },
	{ "gcc_ufs_phy_ahb_clk", &gcc, 0x113, 2 },
	{ "gcc_ufs_phy_axi_clk", &gcc, 0x112, 2 },
	{ "gcc_ufs_phy_ice_core_clk", &gcc, 0x119, 2 },
	{ "gcc_ufs_phy_phy_aux_clk", &gcc, 0x11A, 2 },
	{ "gcc_ufs_phy_rx_symbol_0_clk", &gcc, 0x115, 2 },
	{ "gcc_ufs_phy_rx_symbol_1_clk", &gcc, 0x11B, 2 },
	{ "gcc_ufs_phy_tx_symbol_0_clk", &gcc, 0x114, 2 },
	{ "gcc_ufs_phy_unipro_core_clk", &gcc, 0x118, 2 },
	{ "gcc_usb30_prim_master_clk", &gcc, 0x6D, 2 },
	{ "gcc_usb30_prim_mock_utmi_clk", &gcc, 0x6F, 2 },
	{ "gcc_usb30_prim_sleep_clk", &gcc, 0x6E, 2 },
	{ "gcc_usb30_sec_master_clk", &gcc, 0x76, 2 },
	{ "gcc_usb30_sec_mock_utmi_clk", &gcc, 0x78, 2 },
	{ "gcc_usb30_sec_sleep_clk", &gcc, 0x77, 2 },
	{ "gcc_usb3_prim_phy_aux_clk", &gcc, 0x70, 2 },
	{ "gcc_usb3_prim_phy_com_aux_clk", &gcc, 0x71, 2 },
	{ "gcc_usb3_prim_phy_pipe_clk", &gcc, 0x72, 2 },
	{ "gcc_usb3_sec_phy_aux_clk", &gcc, 0x79, 2 },
	{ "gcc_usb3_sec_phy_com_aux_clk", &gcc, 0x7A, 2 },
	{ "gcc_usb3_sec_phy_pipe_clk", &gcc, 0x7B, 2 },
	{ "gcc_video_ahb_clk", &gcc, 0x54, 2 },
	{ "gcc_video_axi0_clk", &gcc, 0x57, 2 },
	{ "gcc_video_axi1_clk", &gcc, 0x58, 2 },
	{ "gcc_video_xo_clk", &gcc, 0x59, 2 },
	{ "gpu_cc_debug_mux", &gcc, 0x153, 2 },
	{ "measure_only_cnoc_clk", &gcc, 0x18, 2 },
	{ "measure_only_ipa_2x_clk", &gcc, 0x140, 2 },
	{ "measure_only_memnoc_clk", &gcc, 0xCF, 2 },
	{ "measure_only_snoc_clk", &gcc, 0x9, 2 },
	{ "pcie_0_pipe_clk", &gcc, 0xFB, 2 },
	{ "pcie_1_pipe_clk", &gcc, 0x104, 2 },
	{ "ufs_card_rx_symbol_0_clk", &gcc, 0x10B, 2 },
	{ "ufs_card_rx_symbol_1_clk", &gcc, 0x110, 2 },
	{ "ufs_card_tx_symbol_0_clk", &gcc, 0x10A, 2 },
	{ "ufs_phy_rx_symbol_0_clk", &gcc, 0x117, 2 },
	{ "ufs_phy_rx_symbol_1_clk", &gcc, 0x11C, 2 },
	{ "ufs_phy_tx_symbol_0_clk", &gcc, 0x116, 2 },
	{ "usb3_phy_wrapper_gcc_usb30_pipe_clk", &gcc, 0x7C, 2 },
	{ "usb3_uni_phy_sec_gcc_usb30_pipe_clk", &gcc, 0x7D, 2 },
	{ "mc_cc_debug_mux", &gcc, 0xD3, 2 },
/* gpu_cc_debug_mux is 0x153 */
	{ "gpu_cc_ahb_clk", &gcc, 0x153, 2, &gpu_cc, 0x12, 2 },
	{ "gpu_cc_cb_clk", &gcc, 0x153, 2, &gpu_cc, 0x26, 2 },
	{ "gpu_cc_crc_ahb_clk", &gcc, 0x153, 2, &gpu_cc, 0x13, 2 },
	{ "gpu_cc_cx_apb_clk", &gcc, 0x153, 2, &gpu_cc, 0x16, 2 },
	{ "gpu_cc_cx_gmu_clk", &gcc, 0x153, 2, &gpu_cc, 0x1A, 2 },
	{ "gpu_cc_cx_qdss_at_clk", &gcc, 0x153, 2, &gpu_cc, 0x14, 2 },
	{ "gpu_cc_cx_qdss_trig_clk", &gcc, 0x153, 2, &gpu_cc, 0x19, 2 },
	{ "gpu_cc_cx_qdss_tsctr_clk", &gcc, 0x153, 2, &gpu_cc, 0x15, 2 },
	{ "gpu_cc_cx_snoc_dvm_clk", &gcc, 0x153, 2, &gpu_cc, 0x17, 2 },
	{ "gpu_cc_cxo_aon_clk", &gcc, 0x153, 2, &gpu_cc, 0xB, 2 },
	{ "gpu_cc_cxo_clk", &gcc, 0x153, 2, &gpu_cc, 0x1B, 2 },
	{ "gpu_cc_freq_measure_clk", &gcc, 0x153, 2, &gpu_cc, 0xC, 2 },
	{ "gpu_cc_gx_gmu_clk", &gcc, 0x153, 2, &gpu_cc, 0x11, 2 },
	{ "gpu_cc_gx_qdss_tsctr_clk", &gcc, 0x153, 2, &gpu_cc, 0xF, 2 },
	{ "gpu_cc_gx_vsense_clk", &gcc, 0x153, 2, &gpu_cc, 0xE, 2 },
	{ "gpu_cc_hub_aon_clk", &gcc, 0x153, 2, &gpu_cc, 0x27, 2 },
	{ "gpu_cc_hub_cx_int_clk", &gcc, 0x153, 2, &gpu_cc, 0x1C, 2 },
	{ "gpu_cc_mnd1x_0_gfx3d_clk", &gcc, 0x153, 2, &gpu_cc, 0x21, 2 },
	{ "gpu_cc_mnd1x_1_gfx3d_clk", &gcc, 0x153, 2, &gpu_cc, 0x22, 2 },
	{ "gpu_cc_sleep_clk", &gcc, 0x153, 2, &gpu_cc, 0x18, 2 },
	{ "measure_only_gpu_cc_cx_gfx3d_clk", &gcc, 0x153, 2, &gpu_cc, 0x1D, 2 },
	{ "measure_only_gpu_cc_cx_gfx3d_slv_clk", &gcc, 0x153, 2, &gpu_cc, 0x1E, 2 },
	{ "measure_only_gpu_cc_gx_gfx3d_clk", &gcc, 0x153, 2, &gpu_cc, 0xD, 2 },
/* video_cc_debug_mux is 0x5A */
	{ "video_cc_mvs0_clk", &gcc, 0x5A, 2, &video_cc, 0x3, 3 },
	{ "video_cc_mvs0c_clk", &gcc, 0x5A, 2, &video_cc, 0x1, 3 },
	{ "video_cc_mvs1_clk", &gcc, 0x5A, 2, &video_cc, 0x5, 3 },
	{ "video_cc_mvs1_div2_clk", &gcc, 0x5A, 2, &video_cc, 0x8, 3 },
	{ "video_cc_mvs1c_clk", &gcc, 0x5A, 2, &video_cc, 0x9, 3 },
	{ "video_cc_sleep_clk", &gcc, 0x5A, 2, &video_cc, 0xC, 3 },
	{}
};

struct debugcc_platform sm8350_debugcc = {
	"sm8350",
	sm8350_clocks,
};
