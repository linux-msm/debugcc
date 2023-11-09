/*
 * Copyright (c) 2022, Linaro Ltd.
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
		.phys =	0x00100000,
		.size = 0x1f0000,

		.measure = measure_gcc,

		.enable_reg = 0x62004,
		.enable_mask = BIT(0),

		.mux_reg = 0x62024,
		.mux_mask = 0x1fff,

		.div_reg = 0x62000,
		.div_mask = 0xf,
		.div_val = 2,
	},

	.xo_div4_reg = 0x62008,
	.debug_ctl_reg = 0x62038,
	.debug_status_reg = 0x6203c,
};

static struct debug_mux disp0_cc = {
	.phys = 0xaf00000,
	.size = 0x20000,
	.block_name = "disp0",

	.measure = measure_leaf,
	.parent = &gcc.mux,
	.parent_mux_val = 0x79,

	.enable_reg = 0x500c,
	.enable_mask = BIT(0),

	.mux_reg = 0x7000,
	.mux_mask = 0xff,

	.div_reg = 0x5008,
	.div_mask = 0xf,
	.div_val = 4,
};

static struct debug_mux disp1_cc = {
	.phys = 0x22100000,
	.size = 0x20000,
	.block_name = "disp1",

	.measure = measure_leaf,
	.parent = &gcc.mux,
	.parent_mux_val = 0x82,

	.enable_reg = 0x500c,
	.enable_mask = BIT(0),

	.mux_reg = 0x7000,
	.mux_mask = 0xff,

	.div_reg = 0x5008,
	.div_mask = 0xf,
	.div_val = 4,
};

static struct debug_mux mc_cc = {
	.phys =	0x90ba000,
	.size = /* 0x54 */ 0x1000,
	.block_name = "mc",

	.measure = measure_mccc,
};

static struct measure_clk sc8280xp_clocks[] = {
	{ .name = "gcc_aggre_noc_pcie0_tunnel_axi_clk",			.clk_mux = &gcc.mux, .mux = 0x217 },
	{ .name = "gcc_aggre_noc_pcie1_tunnel_axi_clk",			.clk_mux = &gcc.mux, .mux = 0x218 },
	{ .name = "gcc_aggre_noc_pcie_4_axi_clk",			.clk_mux = &gcc.mux, .mux = 0x214 },
	{ .name = "gcc_aggre_noc_pcie_south_sf_axi_clk",		.clk_mux = &gcc.mux, .mux = 0x215 },
	{ .name = "gcc_aggre_ufs_card_axi_clk",				.clk_mux = &gcc.mux, .mux = 0x222 },
	{ .name = "gcc_aggre_ufs_phy_axi_clk",				.clk_mux = &gcc.mux, .mux = 0x221 },
	{ .name = "gcc_aggre_usb3_mp_axi_clk",				.clk_mux = &gcc.mux, .mux = 0x21b },
	{ .name = "gcc_aggre_usb3_prim_axi_clk",			.clk_mux = &gcc.mux, .mux = 0x219 },
	{ .name = "gcc_aggre_usb3_sec_axi_clk",				.clk_mux = &gcc.mux, .mux = 0x21a },
	{ .name = "gcc_aggre_usb4_1_axi_clk",				.clk_mux = &gcc.mux, .mux = 0x21d },
	{ .name = "gcc_aggre_usb4_axi_clk",				.clk_mux = &gcc.mux, .mux = 0x21c },
	{ .name = "gcc_aggre_usb_noc_axi_clk",				.clk_mux = &gcc.mux, .mux = 0x220 },
	{ .name = "gcc_aggre_usb_noc_north_axi_clk",			.clk_mux = &gcc.mux, .mux = 0x21f },
	{ .name = "gcc_aggre_usb_noc_south_axi_clk",			.clk_mux = &gcc.mux, .mux = 0x21e },
	{ .name = "gcc_ahb2phy0_clk",					.clk_mux = &gcc.mux, .mux = 0xfe },
	{ .name = "gcc_ahb2phy2_clk",					.clk_mux = &gcc.mux, .mux = 0xff },
	{ .name = "gcc_camera_hf_axi_clk",				.clk_mux = &gcc.mux, .mux = 0x6a },
	{ .name = "gcc_camera_sf_axi_clk",				.clk_mux = &gcc.mux, .mux = 0x6b },
	{ .name = "gcc_camera_throttle_nrt_axi_clk",			.clk_mux = &gcc.mux, .mux = 0x6d },
	{ .name = "gcc_camera_throttle_rt_axi_clk",			.clk_mux = &gcc.mux, .mux = 0x6c },
	{ .name = "gcc_camera_throttle_xo_clk",				.clk_mux = &gcc.mux, .mux = 0x6f },
	{ .name = "gcc_cfg_noc_usb3_mp_axi_clk",			.clk_mux = &gcc.mux, .mux = 0x2c },
	{ .name = "gcc_cfg_noc_usb3_prim_axi_clk",			.clk_mux = &gcc.mux, .mux = 0x2a },
	{ .name = "gcc_cfg_noc_usb3_sec_axi_clk",			.clk_mux = &gcc.mux, .mux = 0x2b },
	{ .name = "gcc_cnoc_pcie0_tunnel_clk",				.clk_mux = &gcc.mux, .mux = 0x1c },
	{ .name = "gcc_cnoc_pcie1_tunnel_clk",				.clk_mux = &gcc.mux, .mux = 0x1d },
	{ .name = "gcc_cnoc_pcie4_qx_clk",				.clk_mux = &gcc.mux, .mux = 0x18 },
	{ .name = "gcc_ddrss_gpu_axi_clk",				.clk_mux = &gcc.mux, .mux = 0x155 },
	{ .name = "gcc_ddrss_pcie_sf_tbu_clk",				.clk_mux = &gcc.mux, .mux = 0x156 },
	{ .name = "gcc_disp1_hf_axi_clk",				.clk_mux = &gcc.mux, .mux = 0x7d },
	{ .name = "gcc_disp1_sf_axi_clk",				.clk_mux = &gcc.mux, .mux = 0x7e },
	{ .name = "gcc_disp1_throttle_nrt_axi_clk",			.clk_mux = &gcc.mux, .mux = 0x80 },
	{ .name = "gcc_disp1_throttle_rt_axi_clk",			.clk_mux = &gcc.mux, .mux = 0x7f },
	{ .name = "gcc_disp_hf_axi_clk",				.clk_mux = &gcc.mux, .mux = 0x74 },
	{ .name = "gcc_disp_sf_axi_clk",				.clk_mux = &gcc.mux, .mux = 0x75 },
	{ .name = "gcc_disp_throttle_nrt_axi_clk",			.clk_mux = &gcc.mux, .mux = 0x77 },
	{ .name = "gcc_disp_throttle_rt_axi_clk",			.clk_mux = &gcc.mux, .mux = 0x76 },
	{ .name = "gcc_emac0_axi_clk",					.clk_mux = &gcc.mux, .mux = 0x246 },
	{ .name = "gcc_emac0_ptp_clk",					.clk_mux = &gcc.mux, .mux = 0x248 },
	{ .name = "gcc_emac0_rgmii_clk",				.clk_mux = &gcc.mux, .mux = 0x249 },
	{ .name = "gcc_emac0_slv_ahb_clk",				.clk_mux = &gcc.mux, .mux = 0x247 },
	{ .name = "gcc_emac1_axi_clk",					.clk_mux = &gcc.mux, .mux = 0x24a },
	{ .name = "gcc_emac1_ptp_clk",					.clk_mux = &gcc.mux, .mux = 0x24c },
	{ .name = "gcc_emac1_rgmii_clk",				.clk_mux = &gcc.mux, .mux = 0x24d },
	{ .name = "gcc_emac1_slv_ahb_clk",				.clk_mux = &gcc.mux, .mux = 0x24b },
	{ .name = "gcc_gp1_clk",					.clk_mux = &gcc.mux, .mux = 0x18c },
	{ .name = "gcc_gp2_clk",					.clk_mux = &gcc.mux, .mux = 0x18d },
	{ .name = "gcc_gp3_clk",					.clk_mux = &gcc.mux, .mux = 0x18e },
	{ .name = "gcc_gp4_clk",					.clk_mux = &gcc.mux, .mux = 0x290 },
	{ .name = "gcc_gp5_clk",					.clk_mux = &gcc.mux, .mux = 0x291 },
	{ .name = "gcc_gpu_gpll0_clk_src",				.clk_mux = &gcc.mux, .mux = 0x232 },
	{ .name = "gcc_gpu_gpll0_div_clk_src",				.clk_mux = &gcc.mux, .mux = 0x233 },
	{ .name = "gcc_gpu_memnoc_gfx_clk",				.clk_mux = &gcc.mux, .mux = 0x22e },
	{ .name = "gcc_gpu_snoc_dvm_gfx_clk",				.clk_mux = &gcc.mux, .mux = 0x231 },
	{ .name = "gcc_gpu_tcu_throttle_ahb_clk",			.clk_mux = &gcc.mux, .mux = 0x22b },
	{ .name = "gcc_gpu_tcu_throttle_clk",				.clk_mux = &gcc.mux, .mux = 0x22f },
	{ .name = "gcc_pcie0_phy_rchng_clk",				.clk_mux = &gcc.mux, .mux = 0x1d0 },
	{ .name = "gcc_pcie1_phy_rchng_clk",				.clk_mux = &gcc.mux, .mux = 0x19f },
	{ .name = "gcc_pcie2a_phy_rchng_clk",				.clk_mux = &gcc.mux, .mux = 0x1a9 },
	{ .name = "gcc_pcie2b_phy_rchng_clk",				.clk_mux = &gcc.mux, .mux = 0x1b3 },
	{ .name = "gcc_pcie3a_phy_rchng_clk",				.clk_mux = &gcc.mux, .mux = 0x1bd },
	{ .name = "gcc_pcie3b_phy_rchng_clk",				.clk_mux = &gcc.mux, .mux = 0x1c7 },
	{ .name = "gcc_pcie4_phy_rchng_clk",				.clk_mux = &gcc.mux, .mux = 0x196 },
	{ .name = "gcc_pcie_0_aux_clk",					.clk_mux = &gcc.mux, .mux = 0x1ce },
	{ .name = "gcc_pcie_0_cfg_ahb_clk",				.clk_mux = &gcc.mux, .mux = 0x1cd },
	{ .name = "gcc_pcie_0_mstr_axi_clk",				.clk_mux = &gcc.mux, .mux = 0x1cc },
	{ .name = "gcc_pcie_0_pipe_clk",				.clk_mux = &gcc.mux, .mux = 0x1cf },
	{ .name = "gcc_pcie_0_slv_axi_clk",				.clk_mux = &gcc.mux, .mux = 0x1cb },
	{ .name = "gcc_pcie_0_slv_q2a_axi_clk",				.clk_mux = &gcc.mux, .mux = 0x1ca },
	{ .name = "gcc_pcie_1_aux_clk",					.clk_mux = &gcc.mux, .mux = 0x19d },
	{ .name = "gcc_pcie_1_cfg_ahb_clk",				.clk_mux = &gcc.mux, .mux = 0x19c },
	{ .name = "gcc_pcie_1_mstr_axi_clk",				.clk_mux = &gcc.mux, .mux = 0x19b },
	{ .name = "gcc_pcie_1_pipe_clk",				.clk_mux = &gcc.mux, .mux = 0x19e },
	{ .name = "gcc_pcie_1_slv_axi_clk",				.clk_mux = &gcc.mux, .mux = 0x19a },
	{ .name = "gcc_pcie_1_slv_q2a_axi_clk",				.clk_mux = &gcc.mux, .mux = 0x199 },
	{ .name = "gcc_pcie_2a_aux_clk",				.clk_mux = &gcc.mux, .mux = 0x1a6 },
	{ .name = "gcc_pcie_2a_cfg_ahb_clk",				.clk_mux = &gcc.mux, .mux = 0x1a5 },
	{ .name = "gcc_pcie_2a_mstr_axi_clk",				.clk_mux = &gcc.mux, .mux = 0x1a4 },
	{ .name = "gcc_pcie_2a_pipe_clk",				.clk_mux = &gcc.mux, .mux = 0x1a7 },
	{ .name = "gcc_pcie_2a_pipediv2_clk",				.clk_mux = &gcc.mux, .mux = 0x1a8 },
	{ .name = "gcc_pcie_2a_slv_axi_clk",				.clk_mux = &gcc.mux, .mux = 0x1a3 },
	{ .name = "gcc_pcie_2a_slv_q2a_axi_clk",			.clk_mux = &gcc.mux, .mux = 0x1a2 },
	{ .name = "gcc_pcie_2b_aux_clk",				.clk_mux = &gcc.mux, .mux = 0x1b0 },
	{ .name = "gcc_pcie_2b_cfg_ahb_clk",				.clk_mux = &gcc.mux, .mux = 0x1af },
	{ .name = "gcc_pcie_2b_mstr_axi_clk",				.clk_mux = &gcc.mux, .mux = 0x1ae },
	{ .name = "gcc_pcie_2b_pipe_clk",				.clk_mux = &gcc.mux, .mux = 0x1b1 },
	{ .name = "gcc_pcie_2b_pipediv2_clk",				.clk_mux = &gcc.mux, .mux = 0x1b2 },
	{ .name = "gcc_pcie_2b_slv_axi_clk",				.clk_mux = &gcc.mux, .mux = 0x1ad },
	{ .name = "gcc_pcie_2b_slv_q2a_axi_clk",			.clk_mux = &gcc.mux, .mux = 0x1ac },
	{ .name = "gcc_pcie_3a_aux_clk",				.clk_mux = &gcc.mux, .mux = 0x1ba },
	{ .name = "gcc_pcie_3a_cfg_ahb_clk",				.clk_mux = &gcc.mux, .mux = 0x1b9 },
	{ .name = "gcc_pcie_3a_mstr_axi_clk",				.clk_mux = &gcc.mux, .mux = 0x1b8 },
	{ .name = "gcc_pcie_3a_pipe_clk",				.clk_mux = &gcc.mux, .mux = 0x1bb },
	{ .name = "gcc_pcie_3a_pipediv2_clk",				.clk_mux = &gcc.mux, .mux = 0x1bc },
	{ .name = "gcc_pcie_3a_slv_axi_clk",				.clk_mux = &gcc.mux, .mux = 0x1b7 },
	{ .name = "gcc_pcie_3a_slv_q2a_axi_clk",			.clk_mux = &gcc.mux, .mux = 0x1b6 },
	{ .name = "gcc_pcie_3b_aux_clk",				.clk_mux = &gcc.mux, .mux = 0x1c4 },
	{ .name = "gcc_pcie_3b_cfg_ahb_clk",				.clk_mux = &gcc.mux, .mux = 0x1c3 },
	{ .name = "gcc_pcie_3b_mstr_axi_clk",				.clk_mux = &gcc.mux, .mux = 0x1c2 },
	{ .name = "gcc_pcie_3b_pipe_clk",				.clk_mux = &gcc.mux, .mux = 0x1c5 },
	{ .name = "gcc_pcie_3b_pipediv2_clk",				.clk_mux = &gcc.mux, .mux = 0x1c6 },
	{ .name = "gcc_pcie_3b_slv_axi_clk",				.clk_mux = &gcc.mux, .mux = 0x1c1 },
	{ .name = "gcc_pcie_3b_slv_q2a_axi_clk",			.clk_mux = &gcc.mux, .mux = 0x1c0 },
	{ .name = "gcc_pcie_4_aux_clk",					.clk_mux = &gcc.mux, .mux = 0x193 },
	{ .name = "gcc_pcie_4_cfg_ahb_clk",				.clk_mux = &gcc.mux, .mux = 0x192 },
	{ .name = "gcc_pcie_4_mstr_axi_clk",				.clk_mux = &gcc.mux, .mux = 0x191 },
	{ .name = "gcc_pcie_4_pipe_clk",				.clk_mux = &gcc.mux, .mux = 0x194 },
	{ .name = "gcc_pcie_4_pipediv2_clk",				.clk_mux = &gcc.mux, .mux = 0x195 },
	{ .name = "gcc_pcie_4_slv_axi_clk",				.clk_mux = &gcc.mux, .mux = 0x190 },
	{ .name = "gcc_pcie_4_slv_q2a_axi_clk",				.clk_mux = &gcc.mux, .mux = 0x18f },
	{ .name = "gcc_pcie_rscc_ahb_clk",				.clk_mux = &gcc.mux, .mux = 0x8f },
	{ .name = "gcc_pcie_rscc_xo_clk",				.clk_mux = &gcc.mux, .mux = 0x8e },
	{ .name = "gcc_pcie_throttle_cfg_clk",				.clk_mux = &gcc.mux, .mux = 0x46 },
	{ .name = "gcc_pdm2_clk",					.clk_mux = &gcc.mux, .mux = 0x122 },
	{ .name = "gcc_pdm_ahb_clk",					.clk_mux = &gcc.mux, .mux = 0x120 },
	{ .name = "gcc_pdm_xo4_clk",					.clk_mux = &gcc.mux, .mux = 0x121 },
	{ .name = "gcc_qmip_camera_nrt_ahb_clk",			.clk_mux = &gcc.mux, .mux = 0x68 },
	{ .name = "gcc_qmip_camera_rt_ahb_clk",				.clk_mux = &gcc.mux, .mux = 0x69 },
	{ .name = "gcc_qmip_disp1_ahb_clk",				.clk_mux = &gcc.mux, .mux = 0x7b },
	{ .name = "gcc_qmip_disp1_rot_ahb_clk",				.clk_mux = &gcc.mux, .mux = 0x7c },
	{ .name = "gcc_qmip_disp_ahb_clk",				.clk_mux = &gcc.mux, .mux = 0x72 },
	{ .name = "gcc_qmip_disp_rot_ahb_clk",				.clk_mux = &gcc.mux, .mux = 0x73 },
	{ .name = "gcc_qmip_video_cvp_ahb_clk",				.clk_mux = &gcc.mux, .mux = 0x86 },
	{ .name = "gcc_qmip_video_vcodec_ahb_clk",			.clk_mux = &gcc.mux, .mux = 0x87 },
	{ .name = "gcc_qupv3_wrap0_core_2x_clk",			.clk_mux = &gcc.mux, .mux = 0x109 },
	{ .name = "gcc_qupv3_wrap0_core_clk",				.clk_mux = &gcc.mux, .mux = 0x108 },
	{ .name = "gcc_qupv3_wrap0_qspi0_clk",				.clk_mux = &gcc.mux, .mux = 0x112 },
	{ .name = "gcc_qupv3_wrap0_s0_clk",				.clk_mux = &gcc.mux, .mux = 0x10a },
	{ .name = "gcc_qupv3_wrap0_s1_clk",				.clk_mux = &gcc.mux, .mux = 0x10b },
	{ .name = "gcc_qupv3_wrap0_s2_clk",				.clk_mux = &gcc.mux, .mux = 0x10c },
	{ .name = "gcc_qupv3_wrap0_s3_clk",				.clk_mux = &gcc.mux, .mux = 0x10d },
	{ .name = "gcc_qupv3_wrap0_s4_clk",				.clk_mux = &gcc.mux, .mux = 0x10e },
	{ .name = "gcc_qupv3_wrap0_s5_clk",				.clk_mux = &gcc.mux, .mux = 0x10f },
	{ .name = "gcc_qupv3_wrap0_s6_clk",				.clk_mux = &gcc.mux, .mux = 0x110 },
	{ .name = "gcc_qupv3_wrap0_s7_clk",				.clk_mux = &gcc.mux, .mux = 0x111 },
	{ .name = "gcc_qupv3_wrap1_core_2x_clk",			.clk_mux = &gcc.mux, .mux = 0x116 },
	{ .name = "gcc_qupv3_wrap1_core_clk",				.clk_mux = &gcc.mux, .mux = 0x115 },
	{ .name = "gcc_qupv3_wrap1_qspi0_clk",				.clk_mux = &gcc.mux, .mux = 0x11f },
	{ .name = "gcc_qupv3_wrap1_s0_clk",				.clk_mux = &gcc.mux, .mux = 0x117 },
	{ .name = "gcc_qupv3_wrap1_s1_clk",				.clk_mux = &gcc.mux, .mux = 0x118 },
	{ .name = "gcc_qupv3_wrap1_s2_clk",				.clk_mux = &gcc.mux, .mux = 0x119 },
	{ .name = "gcc_qupv3_wrap1_s3_clk",				.clk_mux = &gcc.mux, .mux = 0x11a },
	{ .name = "measure_only_gcc_qupv3_wrap1_s4_clk",		.clk_mux = &gcc.mux, .mux = 0x11b },
	{ .name = "gcc_qupv3_wrap1_s5_clk",				.clk_mux = &gcc.mux, .mux = 0x11c },
	{ .name = "gcc_qupv3_wrap1_s6_clk",				.clk_mux = &gcc.mux, .mux = 0x11d },
	{ .name = "gcc_qupv3_wrap1_s7_clk",				.clk_mux = &gcc.mux, .mux = 0x11e },
	{ .name = "gcc_qupv3_wrap2_core_2x_clk",			.clk_mux = &gcc.mux, .mux = 0x251 },
	{ .name = "gcc_qupv3_wrap2_core_clk",				.clk_mux = &gcc.mux, .mux = 0x250 },
	{ .name = "gcc_qupv3_wrap2_qspi0_clk",				.clk_mux = &gcc.mux, .mux = 0x25a },
	{ .name = "gcc_qupv3_wrap2_s0_clk",				.clk_mux = &gcc.mux, .mux = 0x252 },
	{ .name = "gcc_qupv3_wrap2_s1_clk",				.clk_mux = &gcc.mux, .mux = 0x253 },
	{ .name = "gcc_qupv3_wrap2_s2_clk",				.clk_mux = &gcc.mux, .mux = 0x254 },
	{ .name = "gcc_qupv3_wrap2_s3_clk",				.clk_mux = &gcc.mux, .mux = 0x255 },
	{ .name = "gcc_qupv3_wrap2_s4_clk",				.clk_mux = &gcc.mux, .mux = 0x256 },
	{ .name = "gcc_qupv3_wrap2_s5_clk",				.clk_mux = &gcc.mux, .mux = 0x257 },
	{ .name = "gcc_qupv3_wrap2_s6_clk",				.clk_mux = &gcc.mux, .mux = 0x258 },
	{ .name = "gcc_qupv3_wrap2_s7_clk",				.clk_mux = &gcc.mux, .mux = 0x259 },
	{ .name = "gcc_qupv3_wrap_0_m_ahb_clk",				.clk_mux = &gcc.mux, .mux = 0x106 },
	{ .name = "gcc_qupv3_wrap_0_s_ahb_clk",				.clk_mux = &gcc.mux, .mux = 0x107 },
	{ .name = "measure_only_gcc_qupv3_wrap_1_m_ahb_clk",		.clk_mux = &gcc.mux, .mux = 0x113 },
	{ .name = "measure_only_gcc_qupv3_wrap_1_s_ahb_clk",		.clk_mux = &gcc.mux, .mux = 0x114 },
	{ .name = "gcc_qupv3_wrap_2_m_ahb_clk",				.clk_mux = &gcc.mux, .mux = 0x24e },
	{ .name = "gcc_qupv3_wrap_2_s_ahb_clk",				.clk_mux = &gcc.mux, .mux = 0x24f },
	{ .name = "gcc_sdcc2_ahb_clk",					.clk_mux = &gcc.mux, .mux = 0x101 },
	{ .name = "gcc_sdcc2_apps_clk",					.clk_mux = &gcc.mux, .mux = 0x100 },
	{ .name = "gcc_sdcc4_ahb_clk",					.clk_mux = &gcc.mux, .mux = 0x104 },
	{ .name = "gcc_sdcc4_apps_clk",					.clk_mux = &gcc.mux, .mux = 0x103 },
	{ .name = "gcc_sys_noc_usb_axi_clk",				.clk_mux = &gcc.mux, .mux = 0x14 },
	{ .name = "gcc_ufs_card_ahb_clk",				.clk_mux = &gcc.mux, .mux = 0x1d4 },
	{ .name = "gcc_ufs_card_axi_clk",				.clk_mux = &gcc.mux, .mux = 0x1d3 },
	{ .name = "gcc_ufs_card_ice_core_clk",				.clk_mux = &gcc.mux, .mux = 0x1da },
	{ .name = "gcc_ufs_card_phy_aux_clk",				.clk_mux = &gcc.mux, .mux = 0x1db },
	{ .name = "gcc_ufs_card_rx_symbol_0_clk",			.clk_mux = &gcc.mux, .mux = 0x1d6 },
	{ .name = "gcc_ufs_card_rx_symbol_1_clk",			.clk_mux = &gcc.mux, .mux = 0x1dc },
	{ .name = "gcc_ufs_card_tx_symbol_0_clk",			.clk_mux = &gcc.mux, .mux = 0x1d5 },
	{ .name = "gcc_ufs_card_unipro_core_clk",			.clk_mux = &gcc.mux, .mux = 0x1d9 },
	{ .name = "gcc_ufs_phy_ahb_clk",				.clk_mux = &gcc.mux, .mux = 0x1e0 },
	{ .name = "gcc_ufs_phy_axi_clk",				.clk_mux = &gcc.mux, .mux = 0x1df },
	{ .name = "gcc_ufs_phy_ice_core_clk",				.clk_mux = &gcc.mux, .mux = 0x1e6 },
	{ .name = "gcc_ufs_phy_phy_aux_clk",				.clk_mux = &gcc.mux, .mux = 0x1e7 },
	{ .name = "gcc_ufs_phy_rx_symbol_0_clk",			.clk_mux = &gcc.mux, .mux = 0x1e2 },
	{ .name = "gcc_ufs_phy_rx_symbol_1_clk",			.clk_mux = &gcc.mux, .mux = 0x1e8 },
	{ .name = "gcc_ufs_phy_tx_symbol_0_clk",			.clk_mux = &gcc.mux, .mux = 0x1e1 },
	{ .name = "gcc_ufs_phy_unipro_core_clk",			.clk_mux = &gcc.mux, .mux = 0x1e5 },
	{ .name = "gcc_usb30_mp_master_clk",				.clk_mux = &gcc.mux, .mux = 0xc5 },
	{ .name = "gcc_usb30_mp_mock_utmi_clk",				.clk_mux = &gcc.mux, .mux = 0xc7 },
	{ .name = "gcc_usb30_prim_master_clk",				.clk_mux = &gcc.mux, .mux = 0xb6 },
	{ .name = "gcc_usb30_prim_mock_utmi_clk",			.clk_mux = &gcc.mux, .mux = 0xb8 },
	{ .name = "gcc_usb30_sec_master_clk",				.clk_mux = &gcc.mux, .mux = 0xbf },
	{ .name = "gcc_usb30_sec_mock_utmi_clk",			.clk_mux = &gcc.mux, .mux = 0xc1 },
	{ .name = "gcc_usb3_mp_phy_aux_clk",				.clk_mux = &gcc.mux, .mux = 0xc8 },
	{ .name = "gcc_usb3_mp_phy_com_aux_clk",			.clk_mux = &gcc.mux, .mux = 0xc9 },
	{ .name = "gcc_usb3_mp_phy_pipe_0_clk",				.clk_mux = &gcc.mux, .mux = 0xca },
	{ .name = "gcc_usb3_mp_phy_pipe_1_clk",				.clk_mux = &gcc.mux, .mux = 0xcb },
	{ .name = "gcc_usb3_prim_phy_aux_clk",				.clk_mux = &gcc.mux, .mux = 0xb9 },
	{ .name = "gcc_usb3_prim_phy_com_aux_clk",			.clk_mux = &gcc.mux, .mux = 0xba },
	{ .name = "gcc_usb3_prim_phy_pipe_clk",				.clk_mux = &gcc.mux, .mux = 0xbb },
	{ .name = "gcc_usb3_sec_phy_aux_clk",				.clk_mux = &gcc.mux, .mux = 0xc2 },
	{ .name = "gcc_usb3_sec_phy_com_aux_clk",			.clk_mux = &gcc.mux, .mux = 0xc3 },
	{ .name = "gcc_usb3_sec_phy_pipe_clk",				.clk_mux = &gcc.mux, .mux = 0xc4 },
	{ .name = "gcc_usb4_1_cfg_ahb_clk",				.clk_mux = &gcc.mux, .mux = 0xf3 },
	{ .name = "gcc_usb4_1_dp_clk",					.clk_mux = &gcc.mux, .mux = 0xf0 },
	{ .name = "gcc_usb4_1_master_clk",				.clk_mux = &gcc.mux, .mux = 0xec },
	{ .name = "gcc_usb4_1_phy_p2rr2p_pipe_clk",			.clk_mux = &gcc.mux, .mux = 0xf8 },
	{ .name = "gcc_usb4_1_phy_pcie_pipe_clk",			.clk_mux = &gcc.mux, .mux = 0xee },
	{ .name = "gcc_usb4_1_phy_rx0_clk",				.clk_mux = &gcc.mux, .mux = 0xf4 },
	{ .name = "gcc_usb4_1_phy_rx1_clk",				.clk_mux = &gcc.mux, .mux = 0xf5 },
	{ .name = "gcc_usb4_1_phy_usb_pipe_clk",			.clk_mux = &gcc.mux, .mux = 0xf2 },
	{ .name = "gcc_usb4_1_sb_if_clk",				.clk_mux = &gcc.mux, .mux = 0xed },
	{ .name = "gcc_usb4_1_sys_clk",					.clk_mux = &gcc.mux, .mux = 0xef },
	{ .name = "gcc_usb4_1_tmu_clk",					.clk_mux = &gcc.mux, .mux = 0xf1 },
	{ .name = "gcc_usb4_cfg_ahb_clk",				.clk_mux = &gcc.mux, .mux = 0xe1 },
	{ .name = "gcc_usb4_dp_clk",					.clk_mux = &gcc.mux, .mux = 0xde },
	{ .name = "gcc_usb4_master_clk",				.clk_mux = &gcc.mux, .mux = 0xda },
	{ .name = "gcc_usb4_phy_p2rr2p_pipe_clk",			.clk_mux = &gcc.mux, .mux = 0xe6 },
	{ .name = "gcc_usb4_phy_pcie_pipe_clk",				.clk_mux = &gcc.mux, .mux = 0xdc },
	{ .name = "gcc_usb4_phy_rx0_clk",				.clk_mux = &gcc.mux, .mux = 0xe2 },
	{ .name = "gcc_usb4_phy_rx1_clk",				.clk_mux = &gcc.mux, .mux = 0xe3 },
	{ .name = "gcc_usb4_phy_usb_pipe_clk",				.clk_mux = &gcc.mux, .mux = 0xe0 },
	{ .name = "gcc_usb4_sb_if_clk",					.clk_mux = &gcc.mux, .mux = 0xdb },
	{ .name = "gcc_usb4_sys_clk",					.clk_mux = &gcc.mux, .mux = 0xdd },
	{ .name = "gcc_usb4_tmu_clk",					.clk_mux = &gcc.mux, .mux = 0xdf },
	{ .name = "gcc_video_axi0_clk",					.clk_mux = &gcc.mux, .mux = 0x88 },
	{ .name = "gcc_video_axi1_clk",					.clk_mux = &gcc.mux, .mux = 0x89 },
	{ .name = "gcc_video_cvp_throttle_clk",				.clk_mux = &gcc.mux, .mux = 0x8b },
	{ .name = "gcc_video_vcodec_throttle_clk",			.clk_mux = &gcc.mux, .mux = 0x8a },
	{ .name = "measure_only_cnoc_clk",				.clk_mux = &gcc.mux, .mux = 0x22 },
	{ .name = "measure_only_gcc_camera_ahb_clk",			.clk_mux = &gcc.mux, .mux = 0x67 },
	{ .name = "measure_only_gcc_camera_xo_clk",			.clk_mux = &gcc.mux, .mux = 0x6e },
	{ .name = "measure_only_gcc_disp1_ahb_clk",			.clk_mux = &gcc.mux, .mux = 0x7a },
	{ .name = "measure_only_gcc_disp1_xo_clk",			.clk_mux = &gcc.mux, .mux = 0x81 },
	{ .name = "measure_only_gcc_disp_ahb_clk",			.clk_mux = &gcc.mux, .mux = 0x71 },
	{ .name = "measure_only_gcc_disp_xo_clk",			.clk_mux = &gcc.mux, .mux = 0x78 },
	{ .name = "measure_only_gcc_gpu_cfg_ahb_clk",			.clk_mux = &gcc.mux, .mux = 0x22a },
	{ .name = "measure_only_gcc_video_ahb_clk",			.clk_mux = &gcc.mux, .mux = 0x85 },
	{ .name = "measure_only_gcc_video_xo_clk",			.clk_mux = &gcc.mux, .mux = 0x8c },
	{ .name = "measure_only_ipa_2x_clk",				.clk_mux = &gcc.mux, .mux = 0x225 },
	{ .name = "measure_only_memnoc_clk",				.clk_mux = &gcc.mux, .mux = 0x15b },
	{ .name = "measure_only_snoc_clk",				.clk_mux = &gcc.mux, .mux = 0xc },
	{ .name = "disp0_cc_mdss_ahb1_clk",				.clk_mux = &disp0_cc, .mux = 0x39 },
	{ .name = "disp0_cc_mdss_ahb_clk",				.clk_mux = &disp0_cc, .mux = 0x38 },
	{ .name = "disp0_cc_mdss_byte0_clk",				.clk_mux = &disp0_cc, .mux = 0x19 },
	{ .name = "disp0_cc_mdss_byte0_intf_clk",			.clk_mux = &disp0_cc, .mux = 0x1a },
	{ .name = "disp0_cc_mdss_byte1_clk",				.clk_mux = &disp0_cc, .mux = 0x1b },
	{ .name = "disp0_cc_mdss_byte1_intf_clk",			.clk_mux = &disp0_cc, .mux = 0x1c },
	{ .name = "disp0_cc_mdss_dptx0_aux_clk",			.clk_mux = &disp0_cc, .mux = 0x23 },
	{ .name = "disp0_cc_mdss_dptx0_link_clk",			.clk_mux = &disp0_cc, .mux = 0x1f },
	{ .name = "disp0_cc_mdss_dptx0_link_intf_clk",			.clk_mux = &disp0_cc, .mux = 0x20 },
	{ .name = "disp0_cc_mdss_dptx0_pixel0_clk",			.clk_mux = &disp0_cc, .mux = 0x24 },
	{ .name = "disp0_cc_mdss_dptx0_pixel1_clk",			.clk_mux = &disp0_cc, .mux = 0x25 },
	{ .name = "disp0_cc_mdss_dptx0_usb_router_link_intf_clk",	.clk_mux = &disp0_cc, .mux = 0x21 },
	{ .name = "disp0_cc_mdss_dptx1_aux_clk",			.clk_mux = &disp0_cc, .mux = 0x31 },
	{ .name = "disp0_cc_mdss_dptx1_link_clk",			.clk_mux = &disp0_cc, .mux = 0x2a },
	{ .name = "disp0_cc_mdss_dptx1_link_intf_clk",			.clk_mux = &disp0_cc, .mux = 0x2b },
	{ .name = "disp0_cc_mdss_dptx1_pixel0_clk",			.clk_mux = &disp0_cc, .mux = 0x26 },
	{ .name = "disp0_cc_mdss_dptx1_pixel1_clk",			.clk_mux = &disp0_cc, .mux = 0x27 },
	{ .name = "disp0_cc_mdss_dptx1_usb_router_link_intf_clk",	.clk_mux = &disp0_cc, .mux = 0x2c },
	{ .name = "disp0_cc_mdss_dptx2_aux_clk",			.clk_mux = &disp0_cc, .mux = 0x32 },
	{ .name = "disp0_cc_mdss_dptx2_link_clk",			.clk_mux = &disp0_cc, .mux = 0x2d },
	{ .name = "disp0_cc_mdss_dptx2_link_intf_clk",			.clk_mux = &disp0_cc, .mux = 0x2e },
	{ .name = "disp0_cc_mdss_dptx2_pixel0_clk",			.clk_mux = &disp0_cc, .mux = 0x28 },
	{ .name = "disp0_cc_mdss_dptx2_pixel1_clk",			.clk_mux = &disp0_cc, .mux = 0x29 },
	{ .name = "disp0_cc_mdss_dptx3_aux_clk",			.clk_mux = &disp0_cc, .mux = 0x37 },
	{ .name = "disp0_cc_mdss_dptx3_link_clk",			.clk_mux = &disp0_cc, .mux = 0x34 },
	{ .name = "disp0_cc_mdss_dptx3_link_intf_clk",			.clk_mux = &disp0_cc, .mux = 0x35 },
	{ .name = "disp0_cc_mdss_dptx3_pixel0_clk",			.clk_mux = &disp0_cc, .mux = 0x33 },
	{ .name = "disp0_cc_mdss_esc0_clk",				.clk_mux = &disp0_cc, .mux = 0x1d },
	{ .name = "disp0_cc_mdss_esc1_clk",				.clk_mux = &disp0_cc, .mux = 0x1e },
	{ .name = "disp0_cc_mdss_mdp1_clk",				.clk_mux = &disp0_cc, .mux = 0x12 },
	{ .name = "disp0_cc_mdss_mdp_clk",				.clk_mux = &disp0_cc, .mux = 0x11 },
	{ .name = "disp0_cc_mdss_mdp_lut1_clk",				.clk_mux = &disp0_cc, .mux = 0x16 },
	{ .name = "disp0_cc_mdss_mdp_lut_clk",				.clk_mux = &disp0_cc, .mux = 0x15 },
	{ .name = "disp0_cc_mdss_non_gdsc_ahb_clk",			.clk_mux = &disp0_cc, .mux = 0x3a },
	{ .name = "disp0_cc_mdss_pclk0_clk",				.clk_mux = &disp0_cc, .mux = 0xf },
	{ .name = "disp0_cc_mdss_pclk1_clk",				.clk_mux = &disp0_cc, .mux = 0x10 },
	{ .name = "disp0_cc_mdss_rot1_clk",				.clk_mux = &disp0_cc, .mux = 0x14 },
	{ .name = "disp0_cc_mdss_rot_clk",				.clk_mux = &disp0_cc, .mux = 0x13 },
	{ .name = "disp0_cc_mdss_rscc_ahb_clk",				.clk_mux = &disp0_cc, .mux = 0x3c },
	{ .name = "disp0_cc_mdss_rscc_vsync_clk",			.clk_mux = &disp0_cc, .mux = 0x3b },
	{ .name = "disp0_cc_mdss_vsync1_clk",				.clk_mux = &disp0_cc, .mux = 0x18 },
	{ .name = "disp0_cc_mdss_vsync_clk",				.clk_mux = &disp0_cc, .mux = 0x17 },
	{ .name = "disp0_cc_sleep_clk",					.clk_mux = &disp0_cc, .mux = 0x46 },
	{ .name = "disp0_cc_xo_clk",					.clk_mux = &disp0_cc, .mux = 0x45 },
	{ .name = "disp1_cc_mdss_ahb1_clk",				.clk_mux = &disp1_cc, .mux = 0x39 },
	{ .name = "disp1_cc_mdss_ahb_clk",				.clk_mux = &disp1_cc, .mux = 0x38 },
	{ .name = "disp1_cc_mdss_byte0_clk",				.clk_mux = &disp1_cc, .mux = 0x19 },
	{ .name = "disp1_cc_mdss_byte0_intf_clk",			.clk_mux = &disp1_cc, .mux = 0x1a },
	{ .name = "disp1_cc_mdss_byte1_clk",				.clk_mux = &disp1_cc, .mux = 0x1b },
	{ .name = "disp1_cc_mdss_byte1_intf_clk",			.clk_mux = &disp1_cc, .mux = 0x1c },
	{ .name = "disp1_cc_mdss_dptx0_aux_clk",			.clk_mux = &disp1_cc, .mux = 0x23 },
	{ .name = "disp1_cc_mdss_dptx0_link_clk",			.clk_mux = &disp1_cc, .mux = 0x1f },
	{ .name = "disp1_cc_mdss_dptx0_link_intf_clk",			.clk_mux = &disp1_cc, .mux = 0x20 },
	{ .name = "disp1_cc_mdss_dptx0_pixel0_clk",			.clk_mux = &disp1_cc, .mux = 0x24 },
	{ .name = "disp1_cc_mdss_dptx0_pixel1_clk",			.clk_mux = &disp1_cc, .mux = 0x25 },
	{ .name = "disp1_cc_mdss_dptx0_usb_router_link_intf_clk",	.clk_mux = &disp1_cc, .mux = 0x21 },
	{ .name = "disp1_cc_mdss_dptx1_aux_clk",			.clk_mux = &disp1_cc, .mux = 0x31 },
	{ .name = "disp1_cc_mdss_dptx1_link_clk",			.clk_mux = &disp1_cc, .mux = 0x2a },
	{ .name = "disp1_cc_mdss_dptx1_link_intf_clk",			.clk_mux = &disp1_cc, .mux = 0x2b },
	{ .name = "disp1_cc_mdss_dptx1_pixel0_clk",			.clk_mux = &disp1_cc, .mux = 0x26 },
	{ .name = "disp1_cc_mdss_dptx1_pixel1_clk",			.clk_mux = &disp1_cc, .mux = 0x27 },
	{ .name = "disp1_cc_mdss_dptx1_usb_router_link_intf_clk",	.clk_mux = &disp1_cc, .mux = 0x2c },
	{ .name = "disp1_cc_mdss_dptx2_aux_clk",			.clk_mux = &disp1_cc, .mux = 0x32 },
	{ .name = "disp1_cc_mdss_dptx2_link_clk",			.clk_mux = &disp1_cc, .mux = 0x2d },
	{ .name = "disp1_cc_mdss_dptx2_link_intf_clk",			.clk_mux = &disp1_cc, .mux = 0x2e },
	{ .name = "disp1_cc_mdss_dptx2_pixel0_clk",			.clk_mux = &disp1_cc, .mux = 0x28 },
	{ .name = "disp1_cc_mdss_dptx2_pixel1_clk",			.clk_mux = &disp1_cc, .mux = 0x29 },
	{ .name = "disp1_cc_mdss_dptx3_aux_clk",			.clk_mux = &disp1_cc, .mux = 0x37 },
	{ .name = "disp1_cc_mdss_dptx3_link_clk",			.clk_mux = &disp1_cc, .mux = 0x34 },
	{ .name = "disp1_cc_mdss_dptx3_link_intf_clk",			.clk_mux = &disp1_cc, .mux = 0x35 },
	{ .name = "disp1_cc_mdss_dptx3_pixel0_clk",			.clk_mux = &disp1_cc, .mux = 0x33 },
	{ .name = "disp1_cc_mdss_esc0_clk",				.clk_mux = &disp1_cc, .mux = 0x1d },
	{ .name = "disp1_cc_mdss_esc1_clk",				.clk_mux = &disp1_cc, .mux = 0x1e },
	{ .name = "disp1_cc_mdss_mdp1_clk",				.clk_mux = &disp1_cc, .mux = 0x12 },
	{ .name = "disp1_cc_mdss_mdp_clk",				.clk_mux = &disp1_cc, .mux = 0x11 },
	{ .name = "disp1_cc_mdss_mdp_lut1_clk",				.clk_mux = &disp1_cc, .mux = 0x16 },
	{ .name = "disp1_cc_mdss_mdp_lut_clk",				.clk_mux = &disp1_cc, .mux = 0x15 },
	{ .name = "disp1_cc_mdss_non_gdsc_ahb_clk",			.clk_mux = &disp1_cc, .mux = 0x3a },
	{ .name = "disp1_cc_mdss_pclk0_clk",				.clk_mux = &disp1_cc, .mux = 0xf },
	{ .name = "disp1_cc_mdss_pclk1_clk",				.clk_mux = &disp1_cc, .mux = 0x10 },
	{ .name = "disp1_cc_mdss_rot1_clk",				.clk_mux = &disp1_cc, .mux = 0x14 },
	{ .name = "disp1_cc_mdss_rot_clk",				.clk_mux = &disp1_cc, .mux = 0x13 },
	{ .name = "disp1_cc_mdss_rscc_ahb_clk",				.clk_mux = &disp1_cc, .mux = 0x3c },
	{ .name = "disp1_cc_mdss_rscc_vsync_clk",			.clk_mux = &disp1_cc, .mux = 0x3b },
	{ .name = "disp1_cc_mdss_vsync1_clk",				.clk_mux = &disp1_cc, .mux = 0x18 },
	{ .name = "disp1_cc_mdss_vsync_clk",				.clk_mux = &disp1_cc, .mux = 0x17 },
	{ .name = "disp1_cc_sleep_clk",					.clk_mux = &disp1_cc, .mux = 0x46 },
	{ .name = "disp1_cc_xo_clk",					.clk_mux = &disp1_cc, .mux = 0x45 },
	{ .name = "measure_only_mccc_clk", 				.clk_mux = &mc_cc, .mux = 0x50 },
	{}
};

struct debugcc_platform sc8280xp_debugcc = {
	.name = "sc8280xp",
	.clocks = sc8280xp_clocks,
};
