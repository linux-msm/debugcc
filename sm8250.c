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

static struct gcc_mux gcc = {
	.mux = {
		.phys =	0x100000,
		.size = 0x1f0000,

		.measure = measure_gcc,

		.enable_reg = 0x62008,
		.enable_mask = BIT(0),

		.mux_reg = 0x62000,
		.mux_mask = 0x3ff,

		.div_reg = 0x62004,
		.div_mask = 0xf,
		.div_val = 2,
	},

	.xo_div4_reg = 0x4300c,
	.debug_ctl_reg = 0x62038,
	.debug_status_reg = 0x6203c,
};

static struct debug_mux cam_cc = {
	.phys = 0xad00000,
	.size = 0x10000,
	.block_name = "cam",

	.measure = measure_leaf,
	.parent = &gcc.mux,
	.parent_mux_val = 0x55,

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
	.size = 0x20000,
	.block_name = "disp",

	.measure = measure_leaf,
	.parent = &gcc.mux,
	.parent_mux_val = 0x56,

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
	.parent_mux_val = 0x163,

	.enable_reg = 0x1100,
	.enable_mask = BIT(0),

	.mux_reg = 0x1568,
	.mux_mask = 0xff,

	.div_reg = 0x10fc,
	.div_mask = 0x3,
	.div_val = 2,
};

static struct debug_mux npu_cc = {
	.phys = 0x9980000,
	.size = 0x10000,
	.block_name = "npu",

	.measure = measure_leaf,
	.parent = &gcc.mux,
	.parent_mux_val = 0x180,

	.enable_reg = 0x3008,
	.enable_mask = BIT(0),

	.mux_reg = 0x3000,
	.mux_mask = 0xff,

	.div_reg = 0x3004,
	.div_mask = 0x3,
	.div_val = 2,
};

static struct debug_mux video_cc = {
	.phys = 0xabf0000,
	.size = 0x10000,
	.block_name = "video",

	.measure = measure_leaf,
	.parent = &gcc.mux,
	.parent_mux_val = 0x57,

	.enable_reg = 0xebc,
	.enable_mask = BIT(0),

	.mux_reg = 0xa4c,
	.mux_mask = 0x3f,

	.div_reg = 0x39c,
	.div_mask = 0x7,
	.div_val = 3,
};

static struct debug_mux mc_cc = {
	.phys =	0x90ba000,
	.size = /* 0x54 */ 0x1000,
	.block_name = "mc",

	.measure = measure_mccc,
	.parent = &gcc.mux,
	.parent_mux_val = 0xd1,
};

static struct debug_mux apss_cc = {
	.phys =	0x182a0000,
	.size = /* 0x1c */ 0x1000,
	.block_name = "apss",

	.measure = measure_leaf,
	.parent = &gcc.mux,
	.parent_mux_val = 0xe7,

	.enable_reg = 0x0,
	.enable_mask = BIT(0),

	.mux_reg = 0x18,
	.mux_mask = 0x7f << 4,
	.mux_shift = 4,

	.div_reg = 0x18,
	.div_mask = 0xf << 11,
	.div_shift = 11,
	.div_val = 1,
};

static struct measure_clk sm8250_clocks[] = {
	{ .name = "cam_cc_bps_ahb_clk", .clk_mux = &cam_cc, .mux = 0x18 },
	{ .name = "cam_cc_bps_areg_clk", .clk_mux = &cam_cc, .mux = 0x17 },
	{ .name = "cam_cc_bps_axi_clk", .clk_mux = &cam_cc, .mux = 0x16 },
	{ .name = "cam_cc_bps_clk", .clk_mux = &cam_cc, .mux = 0x14 },
	{ .name = "cam_cc_camnoc_axi_clk", .clk_mux = &cam_cc, .mux = 0x3c },
	{ .name = "cam_cc_camnoc_dcd_xo_clk", .clk_mux = &cam_cc, .mux = 0x3d },
	{ .name = "cam_cc_cci_0_clk", .clk_mux = &cam_cc, .mux = 0x39 },
	{ .name = "cam_cc_cci_1_clk", .clk_mux = &cam_cc, .mux = 0x3a },
	{ .name = "cam_cc_core_ahb_clk", .clk_mux = &cam_cc, .mux = 0x40 },
	{ .name = "cam_cc_cpas_ahb_clk", .clk_mux = &cam_cc, .mux = 0x3b },
	{ .name = "cam_cc_csi0phytimer_clk", .clk_mux = &cam_cc, .mux = 0x8 },
	{ .name = "cam_cc_csi1phytimer_clk", .clk_mux = &cam_cc, .mux = 0xa },
	{ .name = "cam_cc_csi2phytimer_clk", .clk_mux = &cam_cc, .mux = 0xc },
	{ .name = "cam_cc_csi3phytimer_clk", .clk_mux = &cam_cc, .mux = 0xe },
	{ .name = "cam_cc_csi4phytimer_clk", .clk_mux = &cam_cc, .mux = 0x10 },
	{ .name = "cam_cc_csi5phytimer_clk", .clk_mux = &cam_cc, .mux = 0x12 },
	{ .name = "cam_cc_csiphy0_clk", .clk_mux = &cam_cc, .mux = 0x9 },
	{ .name = "cam_cc_csiphy1_clk", .clk_mux = &cam_cc, .mux = 0xb },
	{ .name = "cam_cc_csiphy2_clk", .clk_mux = &cam_cc, .mux = 0xd },
	{ .name = "cam_cc_csiphy3_clk", .clk_mux = &cam_cc, .mux = 0xf },
	{ .name = "cam_cc_csiphy4_clk", .clk_mux = &cam_cc, .mux = 0x11 },
	{ .name = "cam_cc_csiphy5_clk", .clk_mux = &cam_cc, .mux = 0x13 },
	{ .name = "cam_cc_fd_core_clk", .clk_mux = &cam_cc, .mux = 0x37 },
	{ .name = "cam_cc_fd_core_uar_clk", .clk_mux = &cam_cc, .mux = 0x38 },
	{ .name = "cam_cc_gdsc_clk", .clk_mux = &cam_cc, .mux = 0x41 },
	{ .name = "cam_cc_icp_ahb_clk", .clk_mux = &cam_cc, .mux = 0x36 },
	{ .name = "cam_cc_icp_clk", .clk_mux = &cam_cc, .mux = 0x35 },
	{ .name = "cam_cc_ife_0_ahb_clk", .clk_mux = &cam_cc, .mux = 0x26 },
	{ .name = "cam_cc_ife_0_areg_clk", .clk_mux = &cam_cc, .mux = 0x1f },
	{ .name = "cam_cc_ife_0_axi_clk", .clk_mux = &cam_cc, .mux = 0x25 },
	{ .name = "cam_cc_ife_0_clk", .clk_mux = &cam_cc, .mux = 0x1e },
	{ .name = "cam_cc_ife_0_cphy_rx_clk", .clk_mux = &cam_cc, .mux = 0x24 },
	{ .name = "cam_cc_ife_0_csid_clk", .clk_mux = &cam_cc, .mux = 0x22 },
	{ .name = "cam_cc_ife_0_dsp_clk", .clk_mux = &cam_cc, .mux = 0x21 },
	{ .name = "cam_cc_ife_1_ahb_clk", .clk_mux = &cam_cc, .mux = 0x2e },
	{ .name = "cam_cc_ife_1_areg_clk", .clk_mux = &cam_cc, .mux = 0x29 },
	{ .name = "cam_cc_ife_1_axi_clk", .clk_mux = &cam_cc, .mux = 0x2d },
	{ .name = "cam_cc_ife_1_clk", .clk_mux = &cam_cc, .mux = 0x27 },
	{ .name = "cam_cc_ife_1_cphy_rx_clk", .clk_mux = &cam_cc, .mux = 0x2c },
	{ .name = "cam_cc_ife_1_csid_clk", .clk_mux = &cam_cc, .mux = 0x2b },
	{ .name = "cam_cc_ife_1_dsp_clk", .clk_mux = &cam_cc, .mux = 0x2a },
	{ .name = "cam_cc_ife_lite_ahb_clk", .clk_mux = &cam_cc, .mux = 0x32 },
	{ .name = "cam_cc_ife_lite_axi_clk", .clk_mux = &cam_cc, .mux = 0x49 },
	{ .name = "cam_cc_ife_lite_clk", .clk_mux = &cam_cc, .mux = 0x2f },
	{ .name = "cam_cc_ife_lite_cphy_rx_clk", .clk_mux = &cam_cc, .mux = 0x31 },
	{ .name = "cam_cc_ife_lite_csid_clk", .clk_mux = &cam_cc, .mux = 0x30 },
	{ .name = "cam_cc_ipe_0_ahb_clk", .clk_mux = &cam_cc, .mux = 0x1d },
	{ .name = "cam_cc_ipe_0_areg_clk", .clk_mux = &cam_cc, .mux = 0x1c },
	{ .name = "cam_cc_ipe_0_axi_clk", .clk_mux = &cam_cc, .mux = 0x1b },
	{ .name = "cam_cc_ipe_0_clk", .clk_mux = &cam_cc, .mux = 0x19 },
	{ .name = "cam_cc_jpeg_clk", .clk_mux = &cam_cc, .mux = 0x33 },
	{ .name = "cam_cc_mclk0_clk", .clk_mux = &cam_cc, .mux = 0x1 },
	{ .name = "cam_cc_mclk1_clk", .clk_mux = &cam_cc, .mux = 0x2 },
	{ .name = "cam_cc_mclk2_clk", .clk_mux = &cam_cc, .mux = 0x3 },
	{ .name = "cam_cc_mclk3_clk", .clk_mux = &cam_cc, .mux = 0x4 },
	{ .name = "cam_cc_mclk4_clk", .clk_mux = &cam_cc, .mux = 0x5 },
	{ .name = "cam_cc_mclk5_clk", .clk_mux = &cam_cc, .mux = 0x6 },
	{ .name = "cam_cc_mclk6_clk", .clk_mux = &cam_cc, .mux = 0x7 },
	{ .name = "cam_cc_sbi_ahb_clk", .clk_mux = &cam_cc, .mux = 0x4e },
	{ .name = "cam_cc_sbi_axi_clk", .clk_mux = &cam_cc, .mux = 0x4d },
	{ .name = "cam_cc_sbi_clk", .clk_mux = &cam_cc, .mux = 0x4a },
	{ .name = "cam_cc_sbi_cphy_rx_clk", .clk_mux = &cam_cc, .mux = 0x4c },
	{ .name = "cam_cc_sbi_csid_clk", .clk_mux = &cam_cc, .mux = 0x4b },
	{ .name = "cam_cc_sbi_ife_0_clk", .clk_mux = &cam_cc, .mux = 0x4f },
	{ .name = "cam_cc_sbi_ife_1_clk", .clk_mux = &cam_cc, .mux = 0x50 },
	{ .name = "cam_cc_sleep_clk", .clk_mux = &cam_cc, .mux = 0x42 },

	{ .name = "disp_cc_mdss_ahb_clk", .clk_mux = &disp_cc, .mux = 0x2b },
	{ .name = "disp_cc_mdss_byte0_clk", .clk_mux = &disp_cc, .mux = 0x15 },
	{ .name = "disp_cc_mdss_byte0_intf_clk", .clk_mux = &disp_cc, .mux = 0x16 },
	{ .name = "disp_cc_mdss_byte1_clk", .clk_mux = &disp_cc, .mux = 0x17 },
	{ .name = "disp_cc_mdss_byte1_intf_clk", .clk_mux = &disp_cc, .mux = 0x18 },
	{ .name = "disp_cc_mdss_dp_aux1_clk", .clk_mux = &disp_cc, .mux = 0x25 },
	{ .name = "disp_cc_mdss_dp_aux_clk", .clk_mux = &disp_cc, .mux = 0x20 },
	{ .name = "disp_cc_mdss_dp_link1_clk", .clk_mux = &disp_cc, .mux = 0x22 },
	{ .name = "disp_cc_mdss_dp_link1_intf_clk", .clk_mux = &disp_cc, .mux = 0x23 },
	{ .name = "disp_cc_mdss_dp_link_clk", .clk_mux = &disp_cc, .mux = 0x1b },
	{ .name = "disp_cc_mdss_dp_link_intf_clk", .clk_mux = &disp_cc, .mux = 0x1c },
	{ .name = "disp_cc_mdss_dp_pixel1_clk", .clk_mux = &disp_cc, .mux = 0x1f },
	{ .name = "disp_cc_mdss_dp_pixel2_clk", .clk_mux = &disp_cc, .mux = 0x21 },
	{ .name = "disp_cc_mdss_dp_pixel_clk", .clk_mux = &disp_cc, .mux = 0x1e },
	{ .name = "disp_cc_mdss_edp_aux_clk", .clk_mux = &disp_cc, .mux = 0x29 },
	{ .name = "disp_cc_mdss_edp_gtc_clk", .clk_mux = &disp_cc, .mux = 0x2a },
	{ .name = "disp_cc_mdss_edp_link_clk", .clk_mux = &disp_cc, .mux = 0x27 },
	{ .name = "disp_cc_mdss_edp_link_intf_clk", .clk_mux = &disp_cc, .mux = 0x28 },
	{ .name = "disp_cc_mdss_edp_pixel_clk", .clk_mux = &disp_cc, .mux = 0x26 },
	{ .name = "disp_cc_mdss_esc0_clk", .clk_mux = &disp_cc, .mux = 0x19 },
	{ .name = "disp_cc_mdss_esc1_clk", .clk_mux = &disp_cc, .mux = 0x1a },
	{ .name = "disp_cc_mdss_mdp_clk", .clk_mux = &disp_cc, .mux = 0x11 },
	{ .name = "disp_cc_mdss_mdp_lut_clk", .clk_mux = &disp_cc, .mux = 0x13 },
	{ .name = "disp_cc_mdss_non_gdsc_ahb_clk", .clk_mux = &disp_cc, .mux = 0x2c },
	{ .name = "disp_cc_mdss_pclk0_clk", .clk_mux = &disp_cc, .mux = 0xf },
	{ .name = "disp_cc_mdss_pclk1_clk", .clk_mux = &disp_cc, .mux = 0x10 },
	{ .name = "disp_cc_mdss_rot_clk", .clk_mux = &disp_cc, .mux = 0x12 },
	{ .name = "disp_cc_mdss_rscc_ahb_clk", .clk_mux = &disp_cc, .mux = 0x2e },
	{ .name = "disp_cc_mdss_rscc_vsync_clk", .clk_mux = &disp_cc, .mux = 0x2d },
	{ .name = "disp_cc_mdss_vsync_clk", .clk_mux = &disp_cc, .mux = 0x14 },
	{ .name = "disp_cc_sleep_clk", .clk_mux = &disp_cc, .mux = 0x37 },
	{ .name = "disp_cc_xo_clk", .clk_mux = &disp_cc, .mux = 0x36 },

	{ .name = "gcc_aggre_noc_pcie_tbu_clk", .clk_mux = &gcc.mux, .mux = 0x36 },
	{ .name = "gcc_aggre_ufs_card_axi_clk", .clk_mux = &gcc.mux, .mux = 0x142 },
	{ .name = "gcc_aggre_ufs_phy_axi_clk", .clk_mux = &gcc.mux, .mux = 0x141 },
	{ .name = "gcc_aggre_usb3_prim_axi_clk", .clk_mux = &gcc.mux, .mux = 0x13f },
	{ .name = "gcc_aggre_usb3_sec_axi_clk", .clk_mux = &gcc.mux, .mux = 0x140 },
	{ .name = "gcc_boot_rom_ahb_clk", .clk_mux = &gcc.mux, .mux = 0xa3 },
	{ .name = "gcc_camera_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x44 },
	{ .name = "gcc_camera_hf_axi_clk", .clk_mux = &gcc.mux, .mux = 0x4d },
	{ .name = "gcc_camera_sf_axi_clk", .clk_mux = &gcc.mux, .mux = 0x4e },
	{ .name = "gcc_camera_xo_clk", .clk_mux = &gcc.mux, .mux = 0x52 },
	{ .name = "gcc_cfg_noc_usb3_prim_axi_clk", .clk_mux = &gcc.mux, .mux = 0x21 },
	{ .name = "gcc_cfg_noc_usb3_sec_axi_clk", .clk_mux = &gcc.mux, .mux = 0x22 },
	{ .name = "gcc_cpuss_ahb_clk", .clk_mux = &gcc.mux, .mux = 0xe0 },
	{ .name = "gcc_cpuss_dvm_bus_clk", .clk_mux = &gcc.mux, .mux = 0xe4 },
	{ .name = "gcc_cpuss_rbcpr_clk", .clk_mux = &gcc.mux, .mux = 0xe1 },
	{ .name = "gcc_ddrss_gpu_axi_clk", .clk_mux = &gcc.mux, .mux = 0xc4 },
	{ .name = "gcc_ddrss_pcie_sf_tbu_clk", .clk_mux = &gcc.mux, .mux = 0xc5 },
	{ .name = "gcc_disp_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x45 },
	{ .name = "gcc_disp_hf_axi_clk", .clk_mux = &gcc.mux, .mux = 0x4f },
	{ .name = "gcc_disp_sf_axi_clk", .clk_mux = &gcc.mux, .mux = 0x50 },
	{ .name = "gcc_disp_xo_clk", .clk_mux = &gcc.mux, .mux = 0x53 },
	{ .name = "gcc_gp1_clk", .clk_mux = &gcc.mux, .mux = 0xef },
	{ .name = "gcc_gp2_clk", .clk_mux = &gcc.mux, .mux = 0xf0 },
	{ .name = "gcc_gp3_clk", .clk_mux = &gcc.mux, .mux = 0xf1 },
	{ .name = "gcc_gpu_cfg_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x161 },
	{ .name = "gcc_gpu_gpll0_clk_src", .clk_mux = &gcc.mux, .mux = 0x167 },
	{ .name = "gcc_gpu_gpll0_div_clk_src", .clk_mux = &gcc.mux, .mux = 0x168 },
	{ .name = "gcc_gpu_memnoc_gfx_clk", .clk_mux = &gcc.mux, .mux = 0x164 },
	{ .name = "gcc_gpu_snoc_dvm_gfx_clk", .clk_mux = &gcc.mux, .mux = 0x166 },
	{ .name = "gcc_npu_axi_clk", .clk_mux = &gcc.mux, .mux = 0x17a },
	{ .name = "gcc_npu_bwmon_axi_clk", .clk_mux = &gcc.mux, .mux = 0x19a },
	{ .name = "gcc_npu_bwmon_cfg_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x199 },
	{ .name = "gcc_npu_cfg_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x179 },
	{ .name = "gcc_npu_dma_clk", .clk_mux = &gcc.mux, .mux = 0x17b },
	{ .name = "gcc_npu_gpll0_clk_src", .clk_mux = &gcc.mux, .mux = 0x17e },
	{ .name = "gcc_npu_gpll0_div_clk_src", .clk_mux = &gcc.mux, .mux = 0x17f },
	{ .name = "gcc_pcie0_phy_refgen_clk", .clk_mux = &gcc.mux, .mux = 0x103 },
	{ .name = "gcc_pcie1_phy_refgen_clk", .clk_mux = &gcc.mux, .mux = 0x104 },
	{ .name = "gcc_pcie2_phy_refgen_clk", .clk_mux = &gcc.mux, .mux = 0x105 },
	{ .name = "gcc_pcie_0_aux_clk", .clk_mux = &gcc.mux, .mux = 0xf6 },
	{ .name = "gcc_pcie_0_cfg_ahb_clk", .clk_mux = &gcc.mux, .mux = 0xf5 },
	{ .name = "gcc_pcie_0_mstr_axi_clk", .clk_mux = &gcc.mux, .mux = 0xf4 },
	{ .name = "gcc_pcie_0_pipe_clk", .clk_mux = &gcc.mux, .mux = 0xf7 },
	{ .name = "gcc_pcie_0_slv_axi_clk", .clk_mux = &gcc.mux, .mux = 0xf3 },
	{ .name = "gcc_pcie_0_slv_q2a_axi_clk", .clk_mux = &gcc.mux, .mux = 0xf2 },
	{ .name = "gcc_pcie_1_aux_clk", .clk_mux = &gcc.mux, .mux = 0xfe },
	{ .name = "gcc_pcie_1_cfg_ahb_clk", .clk_mux = &gcc.mux, .mux = 0xfd },
	{ .name = "gcc_pcie_1_mstr_axi_clk", .clk_mux = &gcc.mux, .mux = 0xfc },
	{ .name = "gcc_pcie_1_pipe_clk", .clk_mux = &gcc.mux, .mux = 0xff },
	{ .name = "gcc_pcie_1_slv_axi_clk", .clk_mux = &gcc.mux, .mux = 0xfb },
	{ .name = "gcc_pcie_1_slv_q2a_axi_clk", .clk_mux = &gcc.mux, .mux = 0xfa },
	{ .name = "gcc_pcie_2_aux_clk", .clk_mux = &gcc.mux, .mux = 0x191 },
	{ .name = "gcc_pcie_2_cfg_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x190 },
	{ .name = "gcc_pcie_2_mstr_axi_clk", .clk_mux = &gcc.mux, .mux = 0x18f },
	{ .name = "gcc_pcie_2_pipe_clk", .clk_mux = &gcc.mux, .mux = 0x192 },
	{ .name = "gcc_pcie_2_slv_axi_clk", .clk_mux = &gcc.mux, .mux = 0x18e },
	{ .name = "gcc_pcie_2_slv_q2a_axi_clk", .clk_mux = &gcc.mux, .mux = 0x18d },
	{ .name = "gcc_pcie_phy_aux_clk", .clk_mux = &gcc.mux, .mux = 0x102 },
	{ .name = "gcc_pdm2_clk", .clk_mux = &gcc.mux, .mux = 0x9d },
	{ .name = "gcc_pdm_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x9b },
	{ .name = "gcc_pdm_xo4_clk", .clk_mux = &gcc.mux, .mux = 0x9c },
	{ .name = "gcc_prng_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x9e },
	{ .name = "gcc_qmip_camera_nrt_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x48 },
	{ .name = "gcc_qmip_camera_rt_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x49 },
	{ .name = "gcc_qmip_disp_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x4a },
	{ .name = "gcc_qmip_video_cvp_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x46 },
	{ .name = "gcc_qmip_video_vcodec_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x47 },
	{ .name = "gcc_qupv3_wrap0_core_2x_clk", .clk_mux = &gcc.mux, .mux = 0x88 },
	{ .name = "gcc_qupv3_wrap0_core_clk", .clk_mux = &gcc.mux, .mux = 0x87 },
	{ .name = "gcc_qupv3_wrap0_s0_clk", .clk_mux = &gcc.mux, .mux = 0x89 },
	{ .name = "gcc_qupv3_wrap0_s1_clk", .clk_mux = &gcc.mux, .mux = 0x8a },
	{ .name = "gcc_qupv3_wrap0_s2_clk", .clk_mux = &gcc.mux, .mux = 0x8b },
	{ .name = "gcc_qupv3_wrap0_s3_clk", .clk_mux = &gcc.mux, .mux = 0x8c },
	{ .name = "gcc_qupv3_wrap0_s4_clk", .clk_mux = &gcc.mux, .mux = 0x8d },
	{ .name = "gcc_qupv3_wrap0_s5_clk", .clk_mux = &gcc.mux, .mux = 0x8e },
	{ .name = "gcc_qupv3_wrap0_s6_clk", .clk_mux = &gcc.mux, .mux = 0x8f },
	{ .name = "gcc_qupv3_wrap0_s7_clk", .clk_mux = &gcc.mux, .mux = 0x90 },
	{ .name = "gcc_qupv3_wrap1_core_2x_clk", .clk_mux = &gcc.mux, .mux = 0x94 },
	{ .name = "gcc_qupv3_wrap1_core_clk", .clk_mux = &gcc.mux, .mux = 0x93 },
	{ .name = "gcc_qupv3_wrap1_s0_clk", .clk_mux = &gcc.mux, .mux = 0x95 },
	{ .name = "gcc_qupv3_wrap1_s1_clk", .clk_mux = &gcc.mux, .mux = 0x96 },
	{ .name = "gcc_qupv3_wrap1_s2_clk", .clk_mux = &gcc.mux, .mux = 0x97 },
	{ .name = "gcc_qupv3_wrap1_s3_clk", .clk_mux = &gcc.mux, .mux = 0x98 },
	{ .name = "gcc_qupv3_wrap1_s4_clk", .clk_mux = &gcc.mux, .mux = 0x99 },
	{ .name = "gcc_qupv3_wrap1_s5_clk", .clk_mux = &gcc.mux, .mux = 0x9a },
	{ .name = "gcc_qupv3_wrap2_core_2x_clk", .clk_mux = &gcc.mux, .mux = 0x184 },
	{ .name = "gcc_qupv3_wrap2_core_clk", .clk_mux = &gcc.mux, .mux = 0x183 },
	{ .name = "gcc_qupv3_wrap2_s0_clk", .clk_mux = &gcc.mux, .mux = 0x185 },
	{ .name = "gcc_qupv3_wrap2_s1_clk", .clk_mux = &gcc.mux, .mux = 0x186 },
	{ .name = "gcc_qupv3_wrap2_s2_clk", .clk_mux = &gcc.mux, .mux = 0x187 },
	{ .name = "gcc_qupv3_wrap2_s3_clk", .clk_mux = &gcc.mux, .mux = 0x188 },
	{ .name = "gcc_qupv3_wrap2_s4_clk", .clk_mux = &gcc.mux, .mux = 0x189 },
	{ .name = "gcc_qupv3_wrap2_s5_clk", .clk_mux = &gcc.mux, .mux = 0x18a },
	{ .name = "gcc_qupv3_wrap_0_m_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x85 },
	{ .name = "gcc_qupv3_wrap_0_s_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x86 },
	{ .name = "gcc_qupv3_wrap_1_m_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x91 },
	{ .name = "gcc_qupv3_wrap_1_s_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x92 },
	{ .name = "gcc_qupv3_wrap_2_m_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x181 },
	{ .name = "gcc_qupv3_wrap_2_s_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x182 },
	{ .name = "gcc_sdcc2_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x82 },
	{ .name = "gcc_sdcc2_apps_clk", .clk_mux = &gcc.mux, .mux = 0x81 },
	{ .name = "gcc_sdcc4_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x84 },
	{ .name = "gcc_sdcc4_apps_clk", .clk_mux = &gcc.mux, .mux = 0x83 },
	{ .name = "gcc_sys_noc_cpuss_ahb_clk", .clk_mux = &gcc.mux, .mux = 0xc },
	{ .name = "gcc_tsif_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x9f },
	{ .name = "gcc_tsif_inactivity_timers_clk", .clk_mux = &gcc.mux, .mux = 0xa1 },
	{ .name = "gcc_tsif_ref_clk", .clk_mux = &gcc.mux, .mux = 0xa0 },
	{ .name = "gcc_ufs_card_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x107 },
	{ .name = "gcc_ufs_card_axi_clk", .clk_mux = &gcc.mux, .mux = 0x106 },
	{ .name = "gcc_ufs_card_ice_core_clk", .clk_mux = &gcc.mux, .mux = 0x10d },
	{ .name = "gcc_ufs_card_phy_aux_clk", .clk_mux = &gcc.mux, .mux = 0x10e },
	{ .name = "gcc_ufs_card_rx_symbol_0_clk", .clk_mux = &gcc.mux, .mux = 0x109 },
	{ .name = "gcc_ufs_card_rx_symbol_1_clk", .clk_mux = &gcc.mux, .mux = 0x10f },
	{ .name = "gcc_ufs_card_tx_symbol_0_clk", .clk_mux = &gcc.mux, .mux = 0x108 },
	{ .name = "gcc_ufs_card_unipro_core_clk", .clk_mux = &gcc.mux, .mux = 0x10c },
	{ .name = "gcc_ufs_phy_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x113 },
	{ .name = "gcc_ufs_phy_axi_clk", .clk_mux = &gcc.mux, .mux = 0x112 },
	{ .name = "gcc_ufs_phy_ice_core_clk", .clk_mux = &gcc.mux, .mux = 0x119 },
	{ .name = "gcc_ufs_phy_phy_aux_clk", .clk_mux = &gcc.mux, .mux = 0x11a },
	{ .name = "gcc_ufs_phy_rx_symbol_0_clk", .clk_mux = &gcc.mux, .mux = 0x115 },
	{ .name = "gcc_ufs_phy_rx_symbol_1_clk", .clk_mux = &gcc.mux, .mux = 0x11b },
	{ .name = "gcc_ufs_phy_tx_symbol_0_clk", .clk_mux = &gcc.mux, .mux = 0x114 },
	{ .name = "gcc_ufs_phy_unipro_core_clk", .clk_mux = &gcc.mux, .mux = 0x118 },
	{ .name = "gcc_usb30_prim_master_clk", .clk_mux = &gcc.mux, .mux = 0x6e },
	{ .name = "gcc_usb30_prim_mock_utmi_clk", .clk_mux = &gcc.mux, .mux = 0x70 },
	{ .name = "gcc_usb30_prim_sleep_clk", .clk_mux = &gcc.mux, .mux = 0x6f },
	{ .name = "gcc_usb30_sec_master_clk", .clk_mux = &gcc.mux, .mux = 0x75 },
	{ .name = "gcc_usb30_sec_mock_utmi_clk", .clk_mux = &gcc.mux, .mux = 0x77 },
	{ .name = "gcc_usb30_sec_sleep_clk", .clk_mux = &gcc.mux, .mux = 0x76 },
	{ .name = "gcc_usb3_prim_phy_aux_clk", .clk_mux = &gcc.mux, .mux = 0x71 },
	{ .name = "gcc_usb3_prim_phy_com_aux_clk", .clk_mux = &gcc.mux, .mux = 0x72 },
	{ .name = "gcc_usb3_prim_phy_pipe_clk", .clk_mux = &gcc.mux, .mux = 0x73 },
	{ .name = "gcc_usb3_sec_phy_aux_clk", .clk_mux = &gcc.mux, .mux = 0x78 },
	{ .name = "gcc_usb3_sec_phy_com_aux_clk", .clk_mux = &gcc.mux, .mux = 0x79 },
	{ .name = "gcc_usb3_sec_phy_pipe_clk", .clk_mux = &gcc.mux, .mux = 0x7a },
	{ .name = "gcc_video_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x43 },
	{ .name = "gcc_video_axi0_clk", .clk_mux = &gcc.mux, .mux = 0x4b },
	{ .name = "gcc_video_axi1_clk", .clk_mux = &gcc.mux, .mux = 0x4c },
	{ .name = "gcc_video_xo_clk", .clk_mux = &gcc.mux, .mux = 0x51 },
	{ .name = "measure_only_cnoc_clk", .clk_mux = &gcc.mux, .mux = 0x19 },
	{ .name = "measure_only_ipa_2x_clk", .clk_mux = &gcc.mux, .mux = 0x147 },
	{ .name = "measure_only_memnoc_clk", .clk_mux = &gcc.mux, .mux = 0xcc },
	{ .name = "measure_only_snoc_clk", .clk_mux = &gcc.mux, .mux = 0x7 },

	{ .name = "gpu_cc_ahb_clk", .clk_mux = &gpu_cc, .mux = 0x10 },
	{ .name = "gpu_cc_crc_ahb_clk", .clk_mux = &gpu_cc, .mux = 0x11 },
	{ .name = "gpu_cc_cx_apb_clk", .clk_mux = &gpu_cc, .mux = 0x14 },
	{ .name = "gpu_cc_cx_gmu_clk", .clk_mux = &gpu_cc, .mux = 0x18 },
	{ .name = "gpu_cc_cx_qdss_at_clk", .clk_mux = &gpu_cc, .mux = 0x12 },
	{ .name = "gpu_cc_cx_qdss_trig_clk", .clk_mux = &gpu_cc, .mux = 0x17 },
	{ .name = "gpu_cc_cx_qdss_tsctr_clk", .clk_mux = &gpu_cc, .mux = 0x13 },
	{ .name = "gpu_cc_cx_snoc_dvm_clk", .clk_mux = &gpu_cc, .mux = 0x15 },
	{ .name = "gpu_cc_cxo_aon_clk", .clk_mux = &gpu_cc, .mux = 0xa },
	{ .name = "gpu_cc_cxo_clk", .clk_mux = &gpu_cc, .mux = 0x19 },
	{ .name = "gpu_cc_gx_gmu_clk", .clk_mux = &gpu_cc, .mux = 0xf },
	{ .name = "gpu_cc_gx_qdss_tsctr_clk", .clk_mux = &gpu_cc, .mux = 0xd },
	{ .name = "gpu_cc_gx_vsense_clk", .clk_mux = &gpu_cc, .mux = 0xc },
	{ .name = "gpu_cc_sleep_clk", .clk_mux = &gpu_cc, .mux = 0x16 },
	{ .name = "measure_only_gpu_cc_cx_gfx3d_clk", .clk_mux = &gpu_cc, .mux = 0x1a },
	{ .name = "measure_only_gpu_cc_cx_gfx3d_slv_clk", .clk_mux = &gpu_cc, .mux = 0x1b },
	{ .name = "measure_only_gpu_cc_gx_gfx3d_clk", .clk_mux = &gpu_cc, .mux = 0xb },

	{ .name = "npu_cc_atb_clk", .clk_mux = &npu_cc, .mux = 0x17 },
	{ .name = "npu_cc_bto_core_clk", .clk_mux = &npu_cc, .mux = 0x19 },
	{ .name = "npu_cc_bwmon_clk", .clk_mux = &npu_cc, .mux = 0x18 },
	{ .name = "npu_cc_cal_hm0_cdc_clk", .clk_mux = &npu_cc, .mux = 0xb },
	{ .name = "npu_cc_cal_hm0_clk", .clk_mux = &npu_cc, .mux = 0x2 },
	{ .name = "npu_cc_cal_hm0_dpm_ip_clk", .clk_mux = &npu_cc, .mux = 0xc },
	{ .name = "npu_cc_cal_hm0_perf_cnt_clk", .clk_mux = &npu_cc, .mux = 0xd },
	{ .name = "npu_cc_cal_hm1_cdc_clk", .clk_mux = &npu_cc, .mux = 0xe },
	{ .name = "npu_cc_cal_hm1_clk", .clk_mux = &npu_cc, .mux = 0x3 },
	{ .name = "npu_cc_cal_hm1_dpm_ip_clk", .clk_mux = &npu_cc, .mux = 0xf },
	{ .name = "npu_cc_cal_hm1_perf_cnt_clk", .clk_mux = &npu_cc, .mux = 0x10 },
	{ .name = "npu_cc_core_clk", .clk_mux = &npu_cc, .mux = 0x4 },
	{ .name = "npu_cc_dl_dpm_clk", .clk_mux = &npu_cc, .mux = 0x23 },
	{ .name = "npu_cc_dl_llm_clk", .clk_mux = &npu_cc, .mux = 0x22 },
	{ .name = "npu_cc_dpm_clk", .clk_mux = &npu_cc, .mux = 0x8 },
	{ .name = "npu_cc_dpm_temp_clk", .clk_mux = &npu_cc, .mux = 0x14 },
	{ .name = "npu_cc_dpm_xo_clk", .clk_mux = &npu_cc, .mux = 0xa },
	{ .name = "npu_cc_dsp_ahbm_clk", .clk_mux = &npu_cc, .mux = 0x1c },
	{ .name = "npu_cc_dsp_ahbs_clk", .clk_mux = &npu_cc, .mux = 0x1b },
	{ .name = "npu_cc_dsp_axi_clk", .clk_mux = &npu_cc, .mux = 0x1e },
	{ .name = "npu_cc_dsp_bwmon_ahb_clk", .clk_mux = &npu_cc, .mux = 0x1d },
	{ .name = "npu_cc_dsp_bwmon_clk", .clk_mux = &npu_cc, .mux = 0x1f },
	{ .name = "npu_cc_isense_clk", .clk_mux = &npu_cc, .mux = 0x7 },
	{ .name = "npu_cc_llm_clk", .clk_mux = &npu_cc, .mux = 0x6 },
	{ .name = "npu_cc_llm_curr_clk", .clk_mux = &npu_cc, .mux = 0x21 },
	{ .name = "npu_cc_llm_temp_clk", .clk_mux = &npu_cc, .mux = 0x15 },
	{ .name = "npu_cc_llm_xo_clk", .clk_mux = &npu_cc, .mux = 0x9 },
	{ .name = "npu_cc_noc_ahb_clk", .clk_mux = &npu_cc, .mux = 0x13 },
	{ .name = "npu_cc_noc_axi_clk", .clk_mux = &npu_cc, .mux = 0x12 },
	{ .name = "npu_cc_noc_dma_clk", .clk_mux = &npu_cc, .mux = 0x11 },
	{ .name = "npu_cc_rsc_xo_clk", .clk_mux = &npu_cc, .mux = 0x1a },
	{ .name = "npu_cc_s2p_clk", .clk_mux = &npu_cc, .mux = 0x16 },
	{ .name = "npu_cc_xo_clk", .clk_mux = &npu_cc, .mux = 0x1 },

	{ .name = "video_cc_ahb_clk", .clk_mux = &video_cc, .mux = 0x7 },
	{ .name = "video_cc_mvs0_clk", .clk_mux = &video_cc, .mux = 0x3 },
	{ .name = "video_cc_mvs0c_clk", .clk_mux = &video_cc, .mux = 0x1 },
	{ .name = "video_cc_mvs1_clk", .clk_mux = &video_cc, .mux = 0x5 },
	{ .name = "video_cc_mvs1_div2_clk", .clk_mux = &video_cc, .mux = 0x8 },
	{ .name = "video_cc_mvs1c_clk", .clk_mux = &video_cc, .mux = 0x9 },
	{ .name = "video_cc_sleep_clk", .clk_mux = &video_cc, .mux = 0xc },
	{ .name = "video_cc_xo_clk", .clk_mux = &video_cc, .mux = 0xb },

	{ .name = "measure_only_mccc_clk", .clk_mux = &mc_cc, .mux = 0x50 },

	{ .name = "measure_only_apcs_gold_post_acd_clk", .clk_mux = &apss_cc, .mux = 0x25, /* TODO: Are these pre_div_vals? */ 8 },
	{ .name = "measure_only_apcs_goldplus_post_acd_clk", .clk_mux = &apss_cc, .mux = 0x61, /* TODO: Are these pre_div_vals? */ 8 },
	{ .name = "measure_only_apcs_l3_post_acd_clk", .clk_mux = &apss_cc, .mux = 0x41, /* TODO: Are these pre_div_vals? */ 4 },
	{ .name = "measure_only_apcs_silver_post_acd_clk", .clk_mux = &apss_cc, .mux = 0x21, /* TODO: Are these pre_div_vals? */ 4 },

	{}
};

struct debugcc_platform sm8250_debugcc = {
	.name = "sm8250",
	.clocks = sm8250_clocks,
};
