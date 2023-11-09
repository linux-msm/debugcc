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
	{ .name = "cam_cc_bps_ahb_clk", .clk_mux = &cam_cc, .mux = 0x18 },
	{ .name = "cam_cc_bps_areg_clk", .clk_mux = &cam_cc, .mux = 0x17 },
	{ .name = "cam_cc_bps_axi_clk", .clk_mux = &cam_cc, .mux = 0x16 },
	{ .name = "cam_cc_bps_clk", .clk_mux = &cam_cc, .mux = 0x14 },
	{ .name = "cam_cc_camnoc_axi_clk", .clk_mux = &cam_cc, .mux = 0x3C },
	{ .name = "cam_cc_camnoc_dcd_xo_clk", .clk_mux = &cam_cc, .mux = 0x3D },
	{ .name = "cam_cc_cci_0_clk", .clk_mux = &cam_cc, .mux = 0x39 },
	{ .name = "cam_cc_cci_1_clk", .clk_mux = &cam_cc, .mux = 0x3A },
	{ .name = "cam_cc_core_ahb_clk", .clk_mux = &cam_cc, .mux = 0x40 },
	{ .name = "cam_cc_cpas_ahb_clk", .clk_mux = &cam_cc, .mux = 0x3B },
	{ .name = "cam_cc_csi0phytimer_clk", .clk_mux = &cam_cc, .mux = 0x8 },
	{ .name = "cam_cc_csi1phytimer_clk", .clk_mux = &cam_cc, .mux = 0xA },
	{ .name = "cam_cc_csi2phytimer_clk", .clk_mux = &cam_cc, .mux = 0xC },
	{ .name = "cam_cc_csi3phytimer_clk", .clk_mux = &cam_cc, .mux = 0xE },
	{ .name = "cam_cc_csi4phytimer_clk", .clk_mux = &cam_cc, .mux = 0x10 },
	{ .name = "cam_cc_csi5phytimer_clk", .clk_mux = &cam_cc, .mux = 0x12 },
	{ .name = "cam_cc_csiphy0_clk", .clk_mux = &cam_cc, .mux = 0x9 },
	{ .name = "cam_cc_csiphy1_clk", .clk_mux = &cam_cc, .mux = 0xB },
	{ .name = "cam_cc_csiphy2_clk", .clk_mux = &cam_cc, .mux = 0xD },
	{ .name = "cam_cc_csiphy3_clk", .clk_mux = &cam_cc, .mux = 0xF },
	{ .name = "cam_cc_csiphy4_clk", .clk_mux = &cam_cc, .mux = 0x11 },
	{ .name = "cam_cc_csiphy5_clk", .clk_mux = &cam_cc, .mux = 0x13 },
	{ .name = "cam_cc_icp_ahb_clk", .clk_mux = &cam_cc, .mux = 0x36 },
	{ .name = "cam_cc_icp_clk", .clk_mux = &cam_cc, .mux = 0x35 },
	{ .name = "cam_cc_ife_0_ahb_clk", .clk_mux = &cam_cc, .mux = 0x26 },
	{ .name = "cam_cc_ife_0_areg_clk", .clk_mux = &cam_cc, .mux = 0x1F },
	{ .name = "cam_cc_ife_0_axi_clk", .clk_mux = &cam_cc, .mux = 0x25 },
	{ .name = "cam_cc_ife_0_clk", .clk_mux = &cam_cc, .mux = 0x1E },
	{ .name = "cam_cc_ife_0_cphy_rx_clk", .clk_mux = &cam_cc, .mux = 0x24 },
	{ .name = "cam_cc_ife_0_csid_clk", .clk_mux = &cam_cc, .mux = 0x22 },
	{ .name = "cam_cc_ife_0_dsp_clk", .clk_mux = &cam_cc, .mux = 0x21 },
	{ .name = "cam_cc_ife_1_ahb_clk", .clk_mux = &cam_cc, .mux = 0x2E },
	{ .name = "cam_cc_ife_1_areg_clk", .clk_mux = &cam_cc, .mux = 0x29 },
	{ .name = "cam_cc_ife_1_axi_clk", .clk_mux = &cam_cc, .mux = 0x2D },
	{ .name = "cam_cc_ife_1_clk", .clk_mux = &cam_cc, .mux = 0x27 },
	{ .name = "cam_cc_ife_1_cphy_rx_clk", .clk_mux = &cam_cc, .mux = 0x2C },
	{ .name = "cam_cc_ife_1_csid_clk", .clk_mux = &cam_cc, .mux = 0x2B },
	{ .name = "cam_cc_ife_1_dsp_clk", .clk_mux = &cam_cc, .mux = 0x2A },
	{ .name = "cam_cc_ife_2_ahb_clk", .clk_mux = &cam_cc, .mux = 0x54 },
	{ .name = "cam_cc_ife_2_areg_clk", .clk_mux = &cam_cc, .mux = 0x37 },
	{ .name = "cam_cc_ife_2_axi_clk", .clk_mux = &cam_cc, .mux = 0x53 },
	{ .name = "cam_cc_ife_2_clk", .clk_mux = &cam_cc, .mux = 0x7 },
	{ .name = "cam_cc_ife_2_cphy_rx_clk", .clk_mux = &cam_cc, .mux = 0x52 },
	{ .name = "cam_cc_ife_2_csid_clk", .clk_mux = &cam_cc, .mux = 0x51 },
	{ .name = "cam_cc_ife_lite_ahb_clk", .clk_mux = &cam_cc, .mux = 0x32 },
	{ .name = "cam_cc_ife_lite_axi_clk", .clk_mux = &cam_cc, .mux = 0x49 },
	{ .name = "cam_cc_ife_lite_clk", .clk_mux = &cam_cc, .mux = 0x2F },
	{ .name = "cam_cc_ife_lite_cphy_rx_clk", .clk_mux = &cam_cc, .mux = 0x31 },
	{ .name = "cam_cc_ife_lite_csid_clk", .clk_mux = &cam_cc, .mux = 0x30 },
	{ .name = "cam_cc_ipe_0_ahb_clk", .clk_mux = &cam_cc, .mux = 0x1D },
	{ .name = "cam_cc_ipe_0_areg_clk", .clk_mux = &cam_cc, .mux = 0x1C },
	{ .name = "cam_cc_ipe_0_axi_clk", .clk_mux = &cam_cc, .mux = 0x1B },
	{ .name = "cam_cc_ipe_0_clk", .clk_mux = &cam_cc, .mux = 0x19 },
	{ .name = "cam_cc_jpeg_clk", .clk_mux = &cam_cc, .mux = 0x33 },
	{ .name = "cam_cc_mclk0_clk", .clk_mux = &cam_cc, .mux = 0x1 },
	{ .name = "cam_cc_mclk1_clk", .clk_mux = &cam_cc, .mux = 0x2 },
	{ .name = "cam_cc_mclk2_clk", .clk_mux = &cam_cc, .mux = 0x3 },
	{ .name = "cam_cc_mclk3_clk", .clk_mux = &cam_cc, .mux = 0x4 },
	{ .name = "cam_cc_mclk4_clk", .clk_mux = &cam_cc, .mux = 0x5 },
	{ .name = "cam_cc_mclk5_clk", .clk_mux = &cam_cc, .mux = 0x6 },
	{ .name = "cam_cc_sbi_ahb_clk", .clk_mux = &cam_cc, .mux = 0x4E },
	{ .name = "cam_cc_sbi_axi_clk", .clk_mux = &cam_cc, .mux = 0x4D },
	{ .name = "cam_cc_sbi_clk", .clk_mux = &cam_cc, .mux = 0x4A },
	{ .name = "cam_cc_sbi_cphy_rx_0_clk", .clk_mux = &cam_cc, .mux = 0x4C },
	{ .name = "cam_cc_sbi_cphy_rx_1_clk", .clk_mux = &cam_cc, .mux = 0x56 },
	{ .name = "cam_cc_sbi_csid_0_clk", .clk_mux = &cam_cc, .mux = 0x4B },
	{ .name = "cam_cc_sbi_csid_1_clk", .clk_mux = &cam_cc, .mux = 0x57 },
	{ .name = "cam_cc_sbi_ife_0_clk", .clk_mux = &cam_cc, .mux = 0x4F },
	{ .name = "cam_cc_sbi_ife_1_clk", .clk_mux = &cam_cc, .mux = 0x50 },
	{ .name = "cam_cc_sbi_ife_2_clk", .clk_mux = &cam_cc, .mux = 0x55 },
	{ .name = "cam_cc_sleep_clk", .clk_mux = &cam_cc, .mux = 0x42 },
/* disp_cc_debug_mux is 0x53 */
	{ .name = "disp_cc_mdss_ahb_clk", .clk_mux = &disp_cc, .mux = 0x2A },
	{ .name = "disp_cc_mdss_byte0_clk", .clk_mux = &disp_cc, .mux = 0x15 },
	{ .name = "disp_cc_mdss_byte0_intf_clk", .clk_mux = &disp_cc, .mux = 0x16 },
	{ .name = "disp_cc_mdss_byte1_clk", .clk_mux = &disp_cc, .mux = 0x17 },
	{ .name = "disp_cc_mdss_byte1_intf_clk", .clk_mux = &disp_cc, .mux = 0x18 },
	{ .name = "disp_cc_mdss_dp_aux1_clk", .clk_mux = &disp_cc, .mux = 0x25 },
	{ .name = "disp_cc_mdss_dp_aux_clk", .clk_mux = &disp_cc, .mux = 0x20 },
	{ .name = "disp_cc_mdss_dp_link1_clk", .clk_mux = &disp_cc, .mux = 0x22 },
	{ .name = "disp_cc_mdss_dp_link1_intf_clk", .clk_mux = &disp_cc, .mux = 0x23 },
	{ .name = "disp_cc_mdss_dp_link_clk", .clk_mux = &disp_cc, .mux = 0x1B },
	{ .name = "disp_cc_mdss_dp_link_intf_clk", .clk_mux = &disp_cc, .mux = 0x1C },
	{ .name = "disp_cc_mdss_dp_pixel1_clk", .clk_mux = &disp_cc, .mux = 0x1F },
	{ .name = "disp_cc_mdss_dp_pixel2_clk", .clk_mux = &disp_cc, .mux = 0x21 },
	{ .name = "disp_cc_mdss_dp_pixel_clk", .clk_mux = &disp_cc, .mux = 0x1E },
	{ .name = "disp_cc_mdss_edp_aux_clk", .clk_mux = &disp_cc, .mux = 0x29 },
	{ .name = "disp_cc_mdss_edp_link_clk", .clk_mux = &disp_cc, .mux = 0x27 },
	{ .name = "disp_cc_mdss_edp_link_intf_clk", .clk_mux = &disp_cc, .mux = 0x28 },
	{ .name = "disp_cc_mdss_edp_pixel_clk", .clk_mux = &disp_cc, .mux = 0x26 },
	{ .name = "disp_cc_mdss_esc0_clk", .clk_mux = &disp_cc, .mux = 0x19 },
	{ .name = "disp_cc_mdss_esc1_clk", .clk_mux = &disp_cc, .mux = 0x1A },
	{ .name = "disp_cc_mdss_mdp_clk", .clk_mux = &disp_cc, .mux = 0x11 },
	{ .name = "disp_cc_mdss_mdp_lut_clk", .clk_mux = &disp_cc, .mux = 0x13 },
	{ .name = "disp_cc_mdss_non_gdsc_ahb_clk", .clk_mux = &disp_cc, .mux = 0x2B },
	{ .name = "disp_cc_mdss_pclk0_clk", .clk_mux = &disp_cc, .mux = 0xF },
	{ .name = "disp_cc_mdss_pclk1_clk", .clk_mux = &disp_cc, .mux = 0x10 },
	{ .name = "disp_cc_mdss_rot_clk", .clk_mux = &disp_cc, .mux = 0x12 },
	{ .name = "disp_cc_mdss_rscc_ahb_clk", .clk_mux = &disp_cc, .mux = 0x2D },
	{ .name = "disp_cc_mdss_rscc_vsync_clk", .clk_mux = &disp_cc, .mux = 0x2C },
	{ .name = "disp_cc_mdss_vsync_clk", .clk_mux = &disp_cc, .mux = 0x14 },
	{ .name = "disp_cc_sleep_clk", .clk_mux = &disp_cc, .mux = 0x36 },

	{ .name = "core_bi_pll_test_se", .clk_mux = &gcc.mux, .mux = 0x5 },
	{ .name = "gcc_aggre_noc_pcie_0_axi_clk", .clk_mux = &gcc.mux, .mux = 0x138 },
	{ .name = "gcc_aggre_noc_pcie_1_axi_clk", .clk_mux = &gcc.mux, .mux = 0x139 },
	{ .name = "gcc_aggre_noc_pcie_tbu_clk", .clk_mux = &gcc.mux, .mux = 0x34 },
	{ .name = "gcc_aggre_ufs_card_axi_clk", .clk_mux = &gcc.mux, .mux = 0x13D },
	{ .name = "gcc_aggre_ufs_phy_axi_clk", .clk_mux = &gcc.mux, .mux = 0x13C },
	{ .name = "gcc_aggre_usb3_prim_axi_clk", .clk_mux = &gcc.mux, .mux = 0x13A },
	{ .name = "gcc_aggre_usb3_sec_axi_clk", .clk_mux = &gcc.mux, .mux = 0x13B },
	{ .name = "gcc_boot_rom_ahb_clk", .clk_mux = &gcc.mux, .mux = 0xA8 },
	{ .name = "gcc_camera_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x47 },
	{ .name = "gcc_camera_hf_axi_clk", .clk_mux = &gcc.mux, .mux = 0x4A },
	{ .name = "gcc_camera_sf_axi_clk", .clk_mux = &gcc.mux, .mux = 0x4B },
	{ .name = "gcc_camera_xo_clk", .clk_mux = &gcc.mux, .mux = 0x4C },
	{ .name = "gcc_cfg_noc_usb3_prim_axi_clk", .clk_mux = &gcc.mux, .mux = 0x1F },
	{ .name = "gcc_cfg_noc_usb3_sec_axi_clk", .clk_mux = &gcc.mux, .mux = 0x20 },
	{ .name = "gcc_ddrss_gpu_axi_clk", .clk_mux = &gcc.mux, .mux = 0xC9 },
	{ .name = "gcc_ddrss_pcie_sf_tbu_clk", .clk_mux = &gcc.mux, .mux = 0xCA },
	{ .name = "gcc_disp_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x4E },
	{ .name = "gcc_disp_hf_axi_clk", .clk_mux = &gcc.mux, .mux = 0x50 },
	{ .name = "gcc_disp_sf_axi_clk", .clk_mux = &gcc.mux, .mux = 0x51 },
	{ .name = "gcc_disp_xo_clk", .clk_mux = &gcc.mux, .mux = 0x52 },
	{ .name = "gcc_gp1_clk", .clk_mux = &gcc.mux, .mux = 0xF1 },
	{ .name = "gcc_gp2_clk", .clk_mux = &gcc.mux, .mux = 0xF2 },
	{ .name = "gcc_gp3_clk", .clk_mux = &gcc.mux, .mux = 0xF3 },
	{ .name = "gcc_gpu_cfg_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x151 },
	{ .name = "gcc_gpu_gpll0_clk_src", .clk_mux = &gcc.mux, .mux = 0x158 },
	{ .name = "gcc_gpu_gpll0_div_clk_src", .clk_mux = &gcc.mux, .mux = 0x159 },
	{ .name = "gcc_gpu_memnoc_gfx_clk", .clk_mux = &gcc.mux, .mux = 0x154 },
	{ .name = "gcc_gpu_snoc_dvm_gfx_clk", .clk_mux = &gcc.mux, .mux = 0x157 },
	{ .name = "gcc_pcie0_phy_rchng_clk", .clk_mux = &gcc.mux, .mux = 0xFA },
	{ .name = "gcc_pcie1_phy_rchng_clk", .clk_mux = &gcc.mux, .mux = 0x103 },
	{ .name = "gcc_pcie_0_aux_clk", .clk_mux = &gcc.mux, .mux = 0xF8 },
	{ .name = "gcc_pcie_0_cfg_ahb_clk", .clk_mux = &gcc.mux, .mux = 0xF7 },
	{ .name = "gcc_pcie_0_mstr_axi_clk", .clk_mux = &gcc.mux, .mux = 0xF6 },
	{ .name = "gcc_pcie_0_pipe_clk", .clk_mux = &gcc.mux, .mux = 0xF9 },
	{ .name = "gcc_pcie_0_slv_axi_clk", .clk_mux = &gcc.mux, .mux = 0xF5 },
	{ .name = "gcc_pcie_0_slv_q2a_axi_clk", .clk_mux = &gcc.mux, .mux = 0xF4 },
	{ .name = "gcc_pcie_1_aux_clk", .clk_mux = &gcc.mux, .mux = 0x101 },
	{ .name = "gcc_pcie_1_cfg_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x100 },
	{ .name = "gcc_pcie_1_mstr_axi_clk", .clk_mux = &gcc.mux, .mux = 0xFF },
	{ .name = "gcc_pcie_1_pipe_clk", .clk_mux = &gcc.mux, .mux = 0x102 },
	{ .name = "gcc_pcie_1_slv_axi_clk", .clk_mux = &gcc.mux, .mux = 0xFE },
	{ .name = "gcc_pcie_1_slv_q2a_axi_clk", .clk_mux = &gcc.mux, .mux = 0xFD },
	{ .name = "gcc_pdm2_clk", .clk_mux = &gcc.mux, .mux = 0x9E },
	{ .name = "gcc_pdm_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x9C },
	{ .name = "gcc_pdm_xo4_clk", .clk_mux = &gcc.mux, .mux = 0x9D },
	{ .name = "gcc_qmip_camera_nrt_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x48 },
	{ .name = "gcc_qmip_camera_rt_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x49 },
	{ .name = "gcc_qmip_disp_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x4F },
	{ .name = "gcc_qmip_video_cvp_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x55 },
	{ .name = "gcc_qmip_video_vcodec_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x56 },
	{ .name = "gcc_qupv3_wrap0_core_2x_clk", .clk_mux = &gcc.mux, .mux = 0x89 },
	{ .name = "gcc_qupv3_wrap0_core_clk", .clk_mux = &gcc.mux, .mux = 0x88 },
	{ .name = "gcc_qupv3_wrap0_s0_clk", .clk_mux = &gcc.mux, .mux = 0x8A },
	{ .name = "gcc_qupv3_wrap0_s1_clk", .clk_mux = &gcc.mux, .mux = 0x8B },
	{ .name = "gcc_qupv3_wrap0_s2_clk", .clk_mux = &gcc.mux, .mux = 0x8C },
	{ .name = "gcc_qupv3_wrap0_s3_clk", .clk_mux = &gcc.mux, .mux = 0x8D },
	{ .name = "gcc_qupv3_wrap0_s4_clk", .clk_mux = &gcc.mux, .mux = 0x8E },
	{ .name = "gcc_qupv3_wrap0_s5_clk", .clk_mux = &gcc.mux, .mux = 0x8F },
	{ .name = "gcc_qupv3_wrap0_s6_clk", .clk_mux = &gcc.mux, .mux = 0x90 },
	{ .name = "gcc_qupv3_wrap0_s7_clk", .clk_mux = &gcc.mux, .mux = 0x91 },
	{ .name = "gcc_qupv3_wrap1_core_2x_clk", .clk_mux = &gcc.mux, .mux = 0x95 },
	{ .name = "gcc_qupv3_wrap1_core_clk", .clk_mux = &gcc.mux, .mux = 0x94 },
	{ .name = "gcc_qupv3_wrap1_s0_clk", .clk_mux = &gcc.mux, .mux = 0x96 },
	{ .name = "gcc_qupv3_wrap1_s1_clk", .clk_mux = &gcc.mux, .mux = 0x97 },
	{ .name = "gcc_qupv3_wrap1_s2_clk", .clk_mux = &gcc.mux, .mux = 0x98 },
	{ .name = "gcc_qupv3_wrap1_s3_clk", .clk_mux = &gcc.mux, .mux = 0x99 },
	{ .name = "gcc_qupv3_wrap1_s4_clk", .clk_mux = &gcc.mux, .mux = 0x9A },
	{ .name = "gcc_qupv3_wrap2_core_2x_clk", .clk_mux = &gcc.mux, .mux = 0x16E },
	{ .name = "gcc_qupv3_wrap2_core_clk", .clk_mux = &gcc.mux, .mux = 0x16D },
	{ .name = "gcc_qupv3_wrap2_s0_clk", .clk_mux = &gcc.mux, .mux = 0x16F },
	{ .name = "gcc_qupv3_wrap2_s1_clk", .clk_mux = &gcc.mux, .mux = 0x170 },
	{ .name = "gcc_qupv3_wrap2_s2_clk", .clk_mux = &gcc.mux, .mux = 0x171 },
	{ .name = "gcc_qupv3_wrap2_s3_clk", .clk_mux = &gcc.mux, .mux = 0x172 },
	{ .name = "gcc_qupv3_wrap2_s4_clk", .clk_mux = &gcc.mux, .mux = 0x173 },
	{ .name = "gcc_qupv3_wrap2_s5_clk", .clk_mux = &gcc.mux, .mux = 0x174 },
	{ .name = "gcc_qupv3_wrap_0_m_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x86 },
	{ .name = "gcc_qupv3_wrap_0_s_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x87 },
	{ .name = "gcc_qupv3_wrap_2_m_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x16B },
	{ .name = "gcc_qupv3_wrap_2_s_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x16C },
	{ .name = "gcc_sdcc2_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x83 },
	{ .name = "gcc_sdcc2_apps_clk", .clk_mux = &gcc.mux, .mux = 0x82 },
	{ .name = "gcc_sdcc4_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x85 },
	{ .name = "gcc_sdcc4_apps_clk", .clk_mux = &gcc.mux, .mux = 0x84 },
	{ .name = "gcc_throttle_pcie_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x40 },
	{ .name = "gcc_ufs_card_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x107 },
	{ .name = "gcc_ufs_card_axi_clk", .clk_mux = &gcc.mux, .mux = 0x106 },
	{ .name = "gcc_ufs_card_ice_core_clk", .clk_mux = &gcc.mux, .mux = 0x10D },
	{ .name = "gcc_ufs_card_phy_aux_clk", .clk_mux = &gcc.mux, .mux = 0x10E },
	{ .name = "gcc_ufs_card_rx_symbol_0_clk", .clk_mux = &gcc.mux, .mux = 0x109 },
	{ .name = "gcc_ufs_card_rx_symbol_1_clk", .clk_mux = &gcc.mux, .mux = 0x10F },
	{ .name = "gcc_ufs_card_tx_symbol_0_clk", .clk_mux = &gcc.mux, .mux = 0x108 },
	{ .name = "gcc_ufs_card_unipro_core_clk", .clk_mux = &gcc.mux, .mux = 0x10C },
	{ .name = "gcc_ufs_phy_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x113 },
	{ .name = "gcc_ufs_phy_axi_clk", .clk_mux = &gcc.mux, .mux = 0x112 },
	{ .name = "gcc_ufs_phy_ice_core_clk", .clk_mux = &gcc.mux, .mux = 0x119 },
	{ .name = "gcc_ufs_phy_phy_aux_clk", .clk_mux = &gcc.mux, .mux = 0x11A },
	{ .name = "gcc_ufs_phy_rx_symbol_0_clk", .clk_mux = &gcc.mux, .mux = 0x115 },
	{ .name = "gcc_ufs_phy_rx_symbol_1_clk", .clk_mux = &gcc.mux, .mux = 0x11B },
	{ .name = "gcc_ufs_phy_tx_symbol_0_clk", .clk_mux = &gcc.mux, .mux = 0x114 },
	{ .name = "gcc_ufs_phy_unipro_core_clk", .clk_mux = &gcc.mux, .mux = 0x118 },
	{ .name = "gcc_usb30_prim_master_clk", .clk_mux = &gcc.mux, .mux = 0x6D },
	{ .name = "gcc_usb30_prim_mock_utmi_clk", .clk_mux = &gcc.mux, .mux = 0x6F },
	{ .name = "gcc_usb30_prim_sleep_clk", .clk_mux = &gcc.mux, .mux = 0x6E },
	{ .name = "gcc_usb30_sec_master_clk", .clk_mux = &gcc.mux, .mux = 0x76 },
	{ .name = "gcc_usb30_sec_mock_utmi_clk", .clk_mux = &gcc.mux, .mux = 0x78 },
	{ .name = "gcc_usb30_sec_sleep_clk", .clk_mux = &gcc.mux, .mux = 0x77 },
	{ .name = "gcc_usb3_prim_phy_aux_clk", .clk_mux = &gcc.mux, .mux = 0x70 },
	{ .name = "gcc_usb3_prim_phy_com_aux_clk", .clk_mux = &gcc.mux, .mux = 0x71 },
	{ .name = "gcc_usb3_prim_phy_pipe_clk", .clk_mux = &gcc.mux, .mux = 0x72 },
	{ .name = "gcc_usb3_sec_phy_aux_clk", .clk_mux = &gcc.mux, .mux = 0x79 },
	{ .name = "gcc_usb3_sec_phy_com_aux_clk", .clk_mux = &gcc.mux, .mux = 0x7A },
	{ .name = "gcc_usb3_sec_phy_pipe_clk", .clk_mux = &gcc.mux, .mux = 0x7B },
	{ .name = "gcc_video_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x54 },
	{ .name = "gcc_video_axi0_clk", .clk_mux = &gcc.mux, .mux = 0x57 },
	{ .name = "gcc_video_axi1_clk", .clk_mux = &gcc.mux, .mux = 0x58 },
	{ .name = "gcc_video_xo_clk", .clk_mux = &gcc.mux, .mux = 0x59 },
	{ .name = "gpu_cc_debug_mux", .clk_mux = &gcc.mux, .mux = 0x153 },
	{ .name = "measure_only_cnoc_clk", .clk_mux = &gcc.mux, .mux = 0x18 },
	{ .name = "measure_only_ipa_2x_clk", .clk_mux = &gcc.mux, .mux = 0x140 },
	{ .name = "measure_only_memnoc_clk", .clk_mux = &gcc.mux, .mux = 0xCF },
	{ .name = "measure_only_snoc_clk", .clk_mux = &gcc.mux, .mux = 0x9 },
	{ .name = "pcie_0_pipe_clk", .clk_mux = &gcc.mux, .mux = 0xFB },
	{ .name = "pcie_1_pipe_clk", .clk_mux = &gcc.mux, .mux = 0x104 },
	{ .name = "ufs_card_rx_symbol_0_clk", .clk_mux = &gcc.mux, .mux = 0x10B },
	{ .name = "ufs_card_rx_symbol_1_clk", .clk_mux = &gcc.mux, .mux = 0x110 },
	{ .name = "ufs_card_tx_symbol_0_clk", .clk_mux = &gcc.mux, .mux = 0x10A },
	{ .name = "ufs_phy_rx_symbol_0_clk", .clk_mux = &gcc.mux, .mux = 0x117 },
	{ .name = "ufs_phy_rx_symbol_1_clk", .clk_mux = &gcc.mux, .mux = 0x11C },
	{ .name = "ufs_phy_tx_symbol_0_clk", .clk_mux = &gcc.mux, .mux = 0x116 },
	{ .name = "usb3_phy_wrapper_gcc_usb30_pipe_clk", .clk_mux = &gcc.mux, .mux = 0x7C },
	{ .name = "usb3_uni_phy_sec_gcc_usb30_pipe_clk", .clk_mux = &gcc.mux, .mux = 0x7D },
	{ .name = "mc_cc_debug_mux", .clk_mux = &gcc.mux, .mux = 0xD3 },
/* gpu_cc_debug_mux is 0x153 */
	{ .name = "gpu_cc_ahb_clk", .clk_mux = &gpu_cc, .mux = 0x12 },
	{ .name = "gpu_cc_cb_clk", .clk_mux = &gpu_cc, .mux = 0x26 },
	{ .name = "gpu_cc_crc_ahb_clk", .clk_mux = &gpu_cc, .mux = 0x13 },
	{ .name = "gpu_cc_cx_apb_clk", .clk_mux = &gpu_cc, .mux = 0x16 },
	{ .name = "gpu_cc_cx_gmu_clk", .clk_mux = &gpu_cc, .mux = 0x1A },
	{ .name = "gpu_cc_cx_qdss_at_clk", .clk_mux = &gpu_cc, .mux = 0x14 },
	{ .name = "gpu_cc_cx_qdss_trig_clk", .clk_mux = &gpu_cc, .mux = 0x19 },
	{ .name = "gpu_cc_cx_qdss_tsctr_clk", .clk_mux = &gpu_cc, .mux = 0x15 },
	{ .name = "gpu_cc_cx_snoc_dvm_clk", .clk_mux = &gpu_cc, .mux = 0x17 },
	{ .name = "gpu_cc_cxo_aon_clk", .clk_mux = &gpu_cc, .mux = 0xB },
	{ .name = "gpu_cc_cxo_clk", .clk_mux = &gpu_cc, .mux = 0x1B },
	{ .name = "gpu_cc_freq_measure_clk", .clk_mux = &gpu_cc, .mux = 0xC },
	{ .name = "gpu_cc_gx_gmu_clk", .clk_mux = &gpu_cc, .mux = 0x11 },
	{ .name = "gpu_cc_gx_qdss_tsctr_clk", .clk_mux = &gpu_cc, .mux = 0xF },
	{ .name = "gpu_cc_gx_vsense_clk", .clk_mux = &gpu_cc, .mux = 0xE },
	{ .name = "gpu_cc_hub_aon_clk", .clk_mux = &gpu_cc, .mux = 0x27 },
	{ .name = "gpu_cc_hub_cx_int_clk", .clk_mux = &gpu_cc, .mux = 0x1C },
	{ .name = "gpu_cc_mnd1x_0_gfx3d_clk", .clk_mux = &gpu_cc, .mux = 0x21 },
	{ .name = "gpu_cc_mnd1x_1_gfx3d_clk", .clk_mux = &gpu_cc, .mux = 0x22 },
	{ .name = "gpu_cc_sleep_clk", .clk_mux = &gpu_cc, .mux = 0x18 },
	{ .name = "measure_only_gpu_cc_cx_gfx3d_clk", .clk_mux = &gpu_cc, .mux = 0x1D },
	{ .name = "measure_only_gpu_cc_cx_gfx3d_slv_clk", .clk_mux = &gpu_cc, .mux = 0x1E },
	{ .name = "measure_only_gpu_cc_gx_gfx3d_clk", .clk_mux = &gpu_cc, .mux = 0xD },
/* video_cc_debug_mux is 0x5A */
	{ .name = "video_cc_mvs0_clk", .clk_mux = &video_cc, .mux = 0x3 },
	{ .name = "video_cc_mvs0c_clk", .clk_mux = &video_cc, .mux = 0x1 },
	{ .name = "video_cc_mvs1_clk", .clk_mux = &video_cc, .mux = 0x5 },
	{ .name = "video_cc_mvs1_div2_clk", .clk_mux = &video_cc, .mux = 0x8 },
	{ .name = "video_cc_mvs1c_clk", .clk_mux = &video_cc, .mux = 0x9 },
	{ .name = "video_cc_sleep_clk", .clk_mux = &video_cc, .mux = 0xC },
	{}
};

struct debugcc_platform sm8350_debugcc = {
	.name = "sm8350",
	.clocks = sm8350_clocks,
};
