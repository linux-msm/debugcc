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
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "debugcc.h"

#define GCC_BASE			0x300000
#define GCC_SIZE			0x8f014

#define GCC_DEBUG_CLK_CTL		0x62000
#define GCC_DEBUG_CTL			0x62004
#define GCC_DEBUG_STATUS		0x62008
#define GCC_XO_DIV4_CBCR		0x43008

static struct gcc_mux gcc = {
	.mux = {
		.phys =	GCC_BASE,
		.size = GCC_SIZE,

		.measure = measure_gcc,

		.enable_reg = GCC_DEBUG_CLK_CTL,
		.enable_mask = BIT(16),

		.mux_reg = GCC_DEBUG_CLK_CTL,
		.mux_mask = 0x3ff,

		.div_reg = GCC_DEBUG_CLK_CTL,
		.div_shift = 12,
		.div_mask = 0xf << 12,
		.div_val = 4,
	},

	.xo_div4_reg = GCC_XO_DIV4_CBCR,
	.debug_ctl_reg = GCC_DEBUG_CTL,
	.debug_status_reg = GCC_DEBUG_STATUS,
};

static struct debug_mux mmss_cc = {
	.phys = 0x8c0000,
	.size = 0xb00c,
	.block_name = "mmss",

	.measure = measure_leaf,
	.parent = &gcc.mux,
	.parent_mux_val = 0x1b,

	.enable_reg = 0x900,
	.enable_mask = BIT(16),

	.mux_reg = 0x900,
	.mux_mask = 0x3ff,
};

/* rudimentary muxes to enable APC debug clocks */
static struct debug_mux apc0_mux = {
	.phys = 0x06400000,
	.size = 0x1000,

	.measure = measure_leaf,
	.parent = &gcc.mux,
	.parent_mux_val = 0xbb,

	.enable_reg = 0x48,
	.enable_mask = 0xf00,
};

static struct debug_mux apc1_mux = {
	.phys = 0x06480000,
	.size = 0x1000,

	.measure = measure_leaf,
	.parent = &apc0_mux,

	.enable_reg = 0x48,
	.enable_mask = 0xf00,
};

static struct debug_mux cpu_cc = {
	.phys = 0x09820000,
	.size = 0x1000,
	.block_name = "cpu",

	.measure = measure_leaf,
	.parent = &apc1_mux,

	.mux_reg = 0x78,
	.mux_mask = 0xff << 8,
	.mux_shift = 8,
};

static struct measure_clk msm8996_clocks[] = {
	{ .name = "snoc_clk", .clk_mux = &gcc.mux, .mux = 0x0000 },
	{ .name = "gcc_sys_noc_usb3_axi_clk", .clk_mux = &gcc.mux, .mux = 0x0006 },
	{ .name = "gcc_sys_noc_ufs_axi_clk", .clk_mux = &gcc.mux, .mux = 0x0007 },
	{ .name = "cnoc_clk", .clk_mux = &gcc.mux, .mux = 0x000e },
	{ .name = "pnoc_clk", .clk_mux = &gcc.mux, .mux = 0x0011 },
	{ .name = "gcc_periph_noc_usb20_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x0014 },
	{ .name = "gcc_mmss_noc_cfg_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x0019 },
	{ .name = "mmss_mmagic_ahb_clk", .clk_mux = &mmss_cc, .mux = 0x0001 },
	{ .name = "mmss_misc_ahb_clk", .clk_mux = &mmss_cc, .mux = 0x0003 },
	{ .name = "vmem_maxi_clk", .clk_mux = &mmss_cc, .mux = 0x0009 },
	{ .name = "vmem_ahb_clk", .clk_mux = &mmss_cc, .mux = 0x000a },
	{ .name = "gpu_ahb_clk", .clk_mux = &mmss_cc, .mux = 0x000c },
	{ .name = "gpu_gx_gfx3d_clk", .clk_mux = &mmss_cc, .mux = 0x000d },
	{ .name = "video_core_clk", .clk_mux = &mmss_cc, .mux = 0x000e },
	{ .name = "video_axi_clk", .clk_mux = &mmss_cc, .mux = 0x000f },
	{ .name = "video_maxi_clk", .clk_mux = &mmss_cc, .mux = 0x0010 },
	{ .name = "video_ahb_clk", .clk_mux = &mmss_cc, .mux = 0x0011 },
	{ .name = "mmss_rbcpr_clk", .clk_mux = &mmss_cc, .mux = 0x0012 },
	{ .name = "mmss_rbcpr_ahb_clk", .clk_mux = &mmss_cc, .mux = 0x0013 },
	{ .name = "mdss_mdp_clk", .clk_mux = &mmss_cc, .mux = 0x0014 },
	{ .name = "mdss_pclk0_clk", .clk_mux = &mmss_cc, .mux = 0x0016 },
	{ .name = "mdss_pclk1_clk", .clk_mux = &mmss_cc, .mux = 0x0017 },
	{ .name = "mdss_extpclk_clk", .clk_mux = &mmss_cc, .mux = 0x0018 },
	{ .name = "video_subcore0_clk", .clk_mux = &mmss_cc, .mux = 0x001a },
	{ .name = "video_subcore1_clk", .clk_mux = &mmss_cc, .mux = 0x001b },
	{ .name = "mdss_vsync_clk", .clk_mux = &mmss_cc, .mux = 0x001c },
	{ .name = "mdss_hdmi_clk", .clk_mux = &mmss_cc, .mux = 0x001d },
	{ .name = "mdss_byte0_clk", .clk_mux = &mmss_cc, .mux = 0x001e },
	{ .name = "mdss_byte1_clk", .clk_mux = &mmss_cc, .mux = 0x001f },
	{ .name = "mdss_esc0_clk", .clk_mux = &mmss_cc, .mux = 0x0020 },
	{ .name = "mdss_esc1_clk", .clk_mux = &mmss_cc, .mux = 0x0021 },
	{ .name = "mdss_ahb_clk", .clk_mux = &mmss_cc, .mux = 0x0022 },
	{ .name = "mdss_hdmi_ahb_clk", .clk_mux = &mmss_cc, .mux = 0x0023 },
	{ .name = "mdss_axi_clk", .clk_mux = &mmss_cc, .mux = 0x0024 },
	{ .name = "camss_top_ahb_clk", .clk_mux = &mmss_cc, .mux = 0x0025 },
	{ .name = "camss_micro_ahb_clk", .clk_mux = &mmss_cc, .mux = 0x0026 },
	{ .name = "camss_gp0_clk", .clk_mux = &mmss_cc, .mux = 0x0027 },
	{ .name = "camss_gp1_clk", .clk_mux = &mmss_cc, .mux = 0x0028 },
	{ .name = "camss_mclk0_clk", .clk_mux = &mmss_cc, .mux = 0x0029 },
	{ .name = "camss_mclk1_clk", .clk_mux = &mmss_cc, .mux = 0x002a },
	{ .name = "camss_mclk2_clk", .clk_mux = &mmss_cc, .mux = 0x002b },
	{ .name = "camss_mclk3_clk", .clk_mux = &mmss_cc, .mux = 0x002c },
	{ .name = "camss_cci_clk", .clk_mux = &mmss_cc, .mux = 0x002d },
	{ .name = "camss_cci_ahb_clk", .clk_mux = &mmss_cc, .mux = 0x002e },
	{ .name = "camss_csi0phytimer_clk", .clk_mux = &mmss_cc, .mux = 0x002f },
	{ .name = "camss_csi1phytimer_clk", .clk_mux = &mmss_cc, .mux = 0x0030 },
	{ .name = "camss_csi2phytimer_clk", .clk_mux = &mmss_cc, .mux = 0x0031 },
	{ .name = "camss_jpeg0_clk", .clk_mux = &mmss_cc, .mux = 0x0032 },
	{ .name = "camss_ispif_ahb_clk", .clk_mux = &mmss_cc, .mux = 0x0033 },
	{ .name = "camss_jpeg2_clk", .clk_mux = &mmss_cc, .mux = 0x0034 },
	{ .name = "camss_jpeg_ahb_clk", .clk_mux = &mmss_cc, .mux = 0x0035 },
	{ .name = "camss_jpeg_axi_clk", .clk_mux = &mmss_cc, .mux = 0x0036 },
	{ .name = "camss_ahb_clk", .clk_mux = &mmss_cc, .mux = 0x0037 },
	{ .name = "camss_vfe0_clk", .clk_mux = &mmss_cc, .mux = 0x0038 },
	{ .name = "camss_vfe1_clk", .clk_mux = &mmss_cc, .mux = 0x0039 },
	{ .name = "camss_cpp_clk", .clk_mux = &mmss_cc, .mux = 0x003a },
	{ .name = "camss_cpp_ahb_clk", .clk_mux = &mmss_cc, .mux = 0x003b },
	{ .name = "camss_vfe_ahb_clk", .clk_mux = &mmss_cc, .mux = 0x003c },
	{ .name = "camss_vfe_axi_clk", .clk_mux = &mmss_cc, .mux = 0x003d },
	{ .name = "gpu_gx_rbbmtimer_clk", .clk_mux = &mmss_cc, .mux = 0x003e },
	{ .name = "camss_csi_vfe0_clk", .clk_mux = &mmss_cc, .mux = 0x003f },
	{ .name = "camss_csi_vfe1_clk", .clk_mux = &mmss_cc, .mux = 0x0040 },
	{ .name = "camss_csi0_clk", .clk_mux = &mmss_cc, .mux = 0x0041 },
	{ .name = "camss_csi0_ahb_clk", .clk_mux = &mmss_cc, .mux = 0x0042 },
	{ .name = "camss_csi0phy_clk", .clk_mux = &mmss_cc, .mux = 0x0043 },
	{ .name = "camss_csi0rdi_clk", .clk_mux = &mmss_cc, .mux = 0x0044 },
	{ .name = "camss_csi0pix_clk", .clk_mux = &mmss_cc, .mux = 0x0045 },
	{ .name = "camss_csi1_clk", .clk_mux = &mmss_cc, .mux = 0x0046 },
	{ .name = "camss_csi1_ahb_clk", .clk_mux = &mmss_cc, .mux = 0x0047 },
	{ .name = "camss_csi1phy_clk", .clk_mux = &mmss_cc, .mux = 0x0048 },
	{ .name = "camss_csi1rdi_clk", .clk_mux = &mmss_cc, .mux = 0x0049 },
	{ .name = "camss_csi1pix_clk", .clk_mux = &mmss_cc, .mux = 0x004a },
	{ .name = "camss_csi2_clk", .clk_mux = &mmss_cc, .mux = 0x004b },
	{ .name = "camss_csi2_ahb_clk", .clk_mux = &mmss_cc, .mux = 0x004c },
	{ .name = "camss_csi2phy_clk", .clk_mux = &mmss_cc, .mux = 0x004d },
	{ .name = "camss_csi2rdi_clk", .clk_mux = &mmss_cc, .mux = 0x004e },
	{ .name = "camss_csi2pix_clk", .clk_mux = &mmss_cc, .mux = 0x004f },
	{ .name = "camss_csi3_clk", .clk_mux = &mmss_cc, .mux = 0x0050 },
	{ .name = "camss_csi3_ahb_clk", .clk_mux = &mmss_cc, .mux = 0x0051 },
	{ .name = "camss_csi3phy_clk", .clk_mux = &mmss_cc, .mux = 0x0052 },
	{ .name = "camss_csi3rdi_clk", .clk_mux = &mmss_cc, .mux = 0x0053 },
	{ .name = "camss_csi3pix_clk", .clk_mux = &mmss_cc, .mux = 0x0054 },
	{ .name = "mmss_mmagic_maxi_clk", .clk_mux = &mmss_cc, .mux = 0x0070 },
	{ .name = "camss_vfe0_stream_clk", .clk_mux = &mmss_cc, .mux = 0x0071 },
	{ .name = "camss_vfe1_stream_clk", .clk_mux = &mmss_cc, .mux = 0x0072 },
	{ .name = "camss_cpp_vbif_ahb_clk", .clk_mux = &mmss_cc, .mux = 0x0073 },
	{ .name = "mmss_mmagic_cfg_ahb_clk", .clk_mux = &mmss_cc, .mux = 0x0074 },
	{ .name = "mmss_misc_cxo_clk", .clk_mux = &mmss_cc, .mux = 0x0077 },
	{ .name = "camss_cpp_axi_clk", .clk_mux = &mmss_cc, .mux = 0x007a },
	{ .name = "camss_jpeg_dma_clk", .clk_mux = &mmss_cc, .mux = 0x007b },
	{ .name = "camss_vfe0_ahb_clk", .clk_mux = &mmss_cc, .mux = 0x0086 },
	{ .name = "camss_vfe1_ahb_clk", .clk_mux = &mmss_cc, .mux = 0x0087 },
	{ .name = "gpu_aon_isense_clk", .clk_mux = &mmss_cc, .mux = 0x0088 },
	{ .name = "fd_core_clk", .clk_mux = &mmss_cc, .mux = 0x0089 },
	{ .name = "fd_core_uar_clk", .clk_mux = &mmss_cc, .mux = 0x008a },
	{ .name = "fd_ahb_clk", .clk_mux = &mmss_cc, .mux = 0x008c },
	{ .name = "camss_csiphy0_3p_clk", .clk_mux = &mmss_cc, .mux = 0x0091 },
	{ .name = "camss_csiphy1_3p_clk", .clk_mux = &mmss_cc, .mux = 0x0092 },
	{ .name = "camss_csiphy2_3p_clk", .clk_mux = &mmss_cc, .mux = 0x0093 },
	{ .name = "smmu_vfe_ahb_clk", .clk_mux = &mmss_cc, .mux = 0x0094 },
	{ .name = "smmu_vfe_axi_clk", .clk_mux = &mmss_cc, .mux = 0x0095 },
	{ .name = "smmu_cpp_ahb_clk", .clk_mux = &mmss_cc, .mux = 0x0096 },
	{ .name = "smmu_cpp_axi_clk", .clk_mux = &mmss_cc, .mux = 0x0097 },
	{ .name = "smmu_jpeg_ahb_clk", .clk_mux = &mmss_cc, .mux = 0x0098 },
	{ .name = "smmu_jpeg_axi_clk", .clk_mux = &mmss_cc, .mux = 0x0099 },
	{ .name = "mmagic_camss_axi_clk", .clk_mux = &mmss_cc, .mux = 0x009a },
	{ .name = "smmu_rot_ahb_clk", .clk_mux = &mmss_cc, .mux = 0x009b },
	{ .name = "smmu_rot_axi_clk", .clk_mux = &mmss_cc, .mux = 0x009c },
	{ .name = "smmu_mdp_ahb_clk", .clk_mux = &mmss_cc, .mux = 0x009d },
	{ .name = "smmu_mdp_axi_clk", .clk_mux = &mmss_cc, .mux = 0x009e },
	{ .name = "mmagic_mdss_axi_clk", .clk_mux = &mmss_cc, .mux = 0x009f },
	{ .name = "smmu_video_ahb_clk", .clk_mux = &mmss_cc, .mux = 0x00a0 },
	{ .name = "smmu_video_axi_clk", .clk_mux = &mmss_cc, .mux = 0x00a1 },
	{ .name = "mmagic_video_axi_clk", .clk_mux = &mmss_cc, .mux = 0x00a2 },
	{ .name = "mmagic_camss_noc_cfg_ahb_clk", .clk_mux = &mmss_cc, .mux = 0x00ad },
	{ .name = "mmagic_mdss_noc_cfg_ahb_clk", .clk_mux = &mmss_cc, .mux = 0x00ae },
	{ .name = "mmagic_video_noc_cfg_ahb_clk", .clk_mux = &mmss_cc, .mux = 0x00af },
	{ .name = "mmagic_bimc_noc_cfg_ahb_clk", .clk_mux = &mmss_cc, .mux = 0x00b0 },
	{ .name = "gcc_mmss_bimc_gfx_clk", .clk_mux = &gcc.mux, .mux = 0x001c},
	{ .name = "gcc_usb30_master_clk", .clk_mux = &gcc.mux, .mux = 0x002d },
	{ .name = "gcc_usb30_sleep_clk", .clk_mux = &gcc.mux, .mux = 0x002e },
	{ .name = "gcc_usb30_mock_utmi_clk", .clk_mux = &gcc.mux, .mux = 0x002f },
	{ .name = "gcc_usb3_phy_aux_clk", .clk_mux = &gcc.mux, .mux = 0x0030 },
	{ .name = "gcc_usb3_phy_pipe_clk", .clk_mux = &gcc.mux, .mux = 0x0031 },
	{ .name = "gcc_usb20_master_clk", .clk_mux = &gcc.mux, .mux = 0x0035 },
	{ .name = "gcc_usb20_sleep_clk", .clk_mux = &gcc.mux, .mux = 0x0036 },
	{ .name = "gcc_usb20_mock_utmi_clk", .clk_mux = &gcc.mux, .mux = 0x0037 },
	{ .name = "gcc_usb_phy_cfg_ahb2phy_clk", .clk_mux = &gcc.mux, .mux = 0x0038 },
	{ .name = "gcc_sdcc1_apps_clk", .clk_mux = &gcc.mux, .mux = 0x0039 },
	{ .name = "gcc_sdcc1_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x003a },
	{ .name = "gcc_sdcc2_apps_clk", .clk_mux = &gcc.mux, .mux = 0x003b },
	{ .name = "gcc_sdcc2_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x003c },
	{ .name = "gcc_sdcc3_apps_clk", .clk_mux = &gcc.mux, .mux = 0x003d },
	{ .name = "gcc_sdcc3_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x003e },
	{ .name = "gcc_sdcc4_apps_clk", .clk_mux = &gcc.mux, .mux = 0x003f },
	{ .name = "gcc_sdcc4_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x0040 },
	{ .name = "gcc_blsp1_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x0041 },
	{ .name = "gcc_blsp1_qup1_spi_apps_clk", .clk_mux = &gcc.mux, .mux = 0x0043 },
	{ .name = "gcc_blsp1_qup1_i2c_apps_clk", .clk_mux = &gcc.mux, .mux = 0x0044 },
	{ .name = "gcc_blsp1_uart1_apps_clk", .clk_mux = &gcc.mux, .mux = 0x0045 },
	{ .name = "gcc_blsp1_qup2_spi_apps_clk", .clk_mux = &gcc.mux, .mux = 0x0047 },
	{ .name = "gcc_blsp1_qup2_i2c_apps_clk", .clk_mux = &gcc.mux, .mux = 0x0048 },
	{ .name = "gcc_blsp1_uart2_apps_clk", .clk_mux = &gcc.mux, .mux = 0x0049 },
	{ .name = "gcc_blsp1_qup3_spi_apps_clk", .clk_mux = &gcc.mux, .mux = 0x004b },
	{ .name = "gcc_blsp1_qup3_i2c_apps_clk", .clk_mux = &gcc.mux, .mux = 0x004c },
	{ .name = "gcc_blsp1_uart3_apps_clk", .clk_mux = &gcc.mux, .mux = 0x004d },
	{ .name = "gcc_blsp1_qup4_spi_apps_clk", .clk_mux = &gcc.mux, .mux = 0x004f },
	{ .name = "gcc_blsp1_qup4_i2c_apps_clk", .clk_mux = &gcc.mux, .mux = 0x0050 },
	{ .name = "gcc_blsp1_uart4_apps_clk", .clk_mux = &gcc.mux, .mux = 0x0051 },
	{ .name = "gcc_blsp1_qup5_spi_apps_clk", .clk_mux = &gcc.mux, .mux = 0x0053 },
	{ .name = "gcc_blsp1_qup5_i2c_apps_clk", .clk_mux = &gcc.mux, .mux = 0x0054 },
	{ .name = "gcc_blsp1_uart5_apps_clk", .clk_mux = &gcc.mux, .mux = 0x0055 },
	{ .name = "gcc_blsp1_qup6_spi_apps_clk", .clk_mux = &gcc.mux, .mux = 0x0057 },
	{ .name = "gcc_blsp1_qup6_i2c_apps_clk", .clk_mux = &gcc.mux, .mux = 0x0058 },
	{ .name = "gcc_blsp1_uart6_apps_clk", .clk_mux = &gcc.mux, .mux = 0x0059 },
	{ .name = "gcc_blsp2_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x005b },
	{ .name = "gcc_blsp2_qup1_spi_apps_clk", .clk_mux = &gcc.mux, .mux = 0x005d },
	{ .name = "gcc_blsp2_qup1_i2c_apps_clk", .clk_mux = &gcc.mux, .mux = 0x005e },
	{ .name = "gcc_blsp2_uart1_apps_clk", .clk_mux = &gcc.mux, .mux = 0x005f },
	{ .name = "gcc_blsp2_qup2_spi_apps_clk", .clk_mux = &gcc.mux, .mux = 0x0061 },
	{ .name = "gcc_blsp2_qup2_i2c_apps_clk", .clk_mux = &gcc.mux, .mux = 0x0062 },
	{ .name = "gcc_blsp2_uart2_apps_clk", .clk_mux = &gcc.mux, .mux = 0x0063 },
	{ .name = "gcc_blsp2_qup3_spi_apps_clk", .clk_mux = &gcc.mux, .mux = 0x0065 },
	{ .name = "gcc_blsp2_qup3_i2c_apps_clk", .clk_mux = &gcc.mux, .mux = 0x0066 },
	{ .name = "gcc_blsp2_uart3_apps_clk", .clk_mux = &gcc.mux, .mux = 0x0067 },
	{ .name = "gcc_blsp2_qup4_spi_apps_clk", .clk_mux = &gcc.mux, .mux = 0x0069 },
	{ .name = "gcc_blsp2_qup4_i2c_apps_clk", .clk_mux = &gcc.mux, .mux = 0x006a },
	{ .name = "gcc_blsp2_uart4_apps_clk", .clk_mux = &gcc.mux, .mux = 0x006b },
	{ .name = "gcc_blsp2_qup5_spi_apps_clk", .clk_mux = &gcc.mux, .mux = 0x006d },
	{ .name = "gcc_blsp2_qup5_i2c_apps_clk", .clk_mux = &gcc.mux, .mux = 0x006e },
	{ .name = "gcc_blsp2_uart5_apps_clk", .clk_mux = &gcc.mux, .mux = 0x006f },
	{ .name = "gcc_blsp2_qup6_spi_apps_clk", .clk_mux = &gcc.mux, .mux = 0x0071 },
	{ .name = "gcc_blsp2_qup6_i2c_apps_clk", .clk_mux = &gcc.mux, .mux = 0x0072 },
	{ .name = "gcc_blsp2_uart6_apps_clk", .clk_mux = &gcc.mux, .mux = 0x0073 },
	{ .name = "gcc_pdm_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x0076 },
	{ .name = "gcc_pdm2_clk", .clk_mux = &gcc.mux, .mux = 0x0078 },
	{ .name = "gcc_prng_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x0079 },
	{ .name = "gcc_tsif_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x007a },
	{ .name = "gcc_tsif_ref_clk", .clk_mux = &gcc.mux, .mux = 0x007b },
	{ .name = "gcc_boot_rom_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x007e },
	{ .name = "ce1_clk", .clk_mux = &gcc.mux, .mux = 0x0099 },
	{ .name = "gcc_ce1_axi_m_clk", .clk_mux = &gcc.mux, .mux = 0x009a },
	{ .name = "gcc_ce1_ahb_m_clk", .clk_mux = &gcc.mux, .mux = 0x009b },
	{ .name = "measure_only_bimc_hmss_axi_clk", .clk_mux = &gcc.mux, .mux = 0x00a5 },
	{ .name = "bimc_clk", .clk_mux = &gcc.mux, .mux = 0x00ad },
	{ .name = "gcc_bimc_gfx_clk", .clk_mux = &gcc.mux, .mux = 0x00af},
	{ .name = "gcc_hmss_rbcpr_clk", .clk_mux = &gcc.mux, .mux = 0x00ba },
	{ .name = "cpu_cbf_clk", .clk_mux = &cpu_cc, .mux = 0x01 },
	{ .name = "cpu_pwr_clk", .clk_mux = &cpu_cc, .mux = 0x11, .fixed_div = 16 },
	{ .name = "cpu_perf_clk", .clk_mux = &cpu_cc, .mux = 0x21, .fixed_div = 16 },
	{ .name = "gcc_gp1_clk", .clk_mux = &gcc.mux, .mux = 0x00e3 },
	{ .name = "gcc_gp2_clk", .clk_mux = &gcc.mux, .mux = 0x00e4 },
	{ .name = "gcc_gp3_clk", .clk_mux = &gcc.mux, .mux = 0x00e5 },
	{ .name = "gcc_pcie_0_slv_axi_clk", .clk_mux = &gcc.mux, .mux = 0x00e6 },
	{ .name = "gcc_pcie_0_mstr_axi_clk", .clk_mux = &gcc.mux, .mux = 0x00e7 },
	{ .name = "gcc_pcie_0_cfg_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x00e8 },
	{ .name = "gcc_pcie_0_aux_clk", .clk_mux = &gcc.mux, .mux = 0x00e9 },
	{ .name = "gcc_pcie_0_pipe_clk", .clk_mux = &gcc.mux, .mux = 0x00ea },
	{ .name = "gcc_pcie_1_slv_axi_clk", .clk_mux = &gcc.mux, .mux = 0x00ec },
	{ .name = "gcc_pcie_1_mstr_axi_clk", .clk_mux = &gcc.mux, .mux = 0x00ed },
	{ .name = "gcc_pcie_1_cfg_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x00ee },
	{ .name = "gcc_pcie_1_aux_clk", .clk_mux = &gcc.mux, .mux = 0x00ef },
	{ .name = "gcc_pcie_1_pipe_clk", .clk_mux = &gcc.mux, .mux = 0x00f0 },
	{ .name = "gcc_pcie_2_slv_axi_clk", .clk_mux = &gcc.mux, .mux = 0x00f2 },
	{ .name = "gcc_pcie_2_mstr_axi_clk", .clk_mux = &gcc.mux, .mux = 0x00f3 },
	{ .name = "gcc_pcie_2_cfg_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x00f4 },
	{ .name = "gcc_pcie_2_aux_clk", .clk_mux = &gcc.mux, .mux = 0x00f5 },
	{ .name = "gcc_pcie_2_pipe_clk", .clk_mux = &gcc.mux, .mux = 0x00f6 },
	{ .name = "gcc_pcie_phy_cfg_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x00f8 },
	{ .name = "gcc_pcie_phy_aux_clk", .clk_mux = &gcc.mux, .mux = 0x00f9 },
	{ .name = "gcc_ufs_axi_clk", .clk_mux = &gcc.mux, .mux = 0x00fc },
	{ .name = "gcc_ufs_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x00fd },
	{ .name = "gcc_ufs_tx_cfg_clk", .clk_mux = &gcc.mux, .mux = 0x00fe },
	{ .name = "gcc_ufs_rx_cfg_clk", .clk_mux = &gcc.mux, .mux = 0x00ff },
	{ .name = "gcc_ufs_tx_symbol_0_clk", .clk_mux = &gcc.mux, .mux = 0x0100 },
	{ .name = "gcc_ufs_rx_symbol_0_clk", .clk_mux = &gcc.mux, .mux = 0x0101 },
	{ .name = "gcc_ufs_rx_symbol_1_clk", .clk_mux = &gcc.mux, .mux = 0x0102 },
	{ .name = "gcc_ufs_unipro_core_clk", .clk_mux = &gcc.mux, .mux = 0x0106 },
	{ .name = "gcc_ufs_ice_core_clk", .clk_mux = &gcc.mux, .mux = 0x0107 },
	{ .name = "gcc_ufs_sys_clk_core_clk", .clk_mux = &gcc.mux, .mux = 0x108},
	{ .name = "gcc_ufs_tx_symbol_clk_core_clk", .clk_mux = &gcc.mux, .mux = 0x0109 },
	{ .name = "gcc_aggre0_snoc_axi_clk", .clk_mux = &gcc.mux, .mux = 0x0116 },
	{ .name = "gcc_aggre0_cnoc_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x0117 },
	{ .name = "gcc_smmu_aggre0_axi_clk", .clk_mux = &gcc.mux, .mux = 0x0119 },
	{ .name = "gcc_smmu_aggre0_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x011a },
	{ .name = "gcc_aggre0_noc_qosgen_extref_clk", .clk_mux = &gcc.mux, .mux = 0x011b },
	{ .name = "gcc_aggre2_ufs_axi_clk", .clk_mux = &gcc.mux, .mux = 0x0126 },
	{ .name = "gcc_aggre2_usb3_axi_clk", .clk_mux = &gcc.mux, .mux = 0x0127 },
	{ .name = "gcc_dcc_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x012b },
	{ .name = "gcc_aggre0_noc_mpu_cfg_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x012c},
	{ .name = "ipa_clk", .clk_mux = &gcc.mux, .mux = 0x12f },
	{ .name = "gcc_mss_cfg_ahb_clk", .clk_mux = &gcc.mux, .mux = 0x0133 },
	{ .name = "gcc_mss_mnoc_bimc_axi_clk", .clk_mux = &gcc.mux, .mux = 0x0134 },
	{ .name = "gcc_mss_snoc_axi_clk", .clk_mux = &gcc.mux, .mux = 0x0135 },
	{ .name = "gcc_mss_q6_bimc_axi_clk", .clk_mux = &gcc.mux, .mux = 0x0136 },
	{}
};

static int msm8996_premap(int devmem)
{
	if (mmap_mux(devmem, &apc0_mux))
		return -1;

	if (mmap_mux(devmem, &apc1_mux))
		return -1;

	return 0;
}

struct debugcc_platform msm8996_debugcc = {
	.name = "msm8996",
	.clocks = msm8996_clocks,
	.premap = msm8996_premap,
};
