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

static struct debug_mux gcc = {
	.phys =	GCC_BASE,
	.size = GCC_SIZE,

	.enable_reg = GCC_DEBUG_CLK_CTL,
	.enable_mask = BIT(16),

	.mux_reg = GCC_DEBUG_CLK_CTL,
	.mux_mask = 0x3ff,

	.div_reg = GCC_DEBUG_CLK_CTL,
	.div_shift = 12,
	.div_mask = 0xf << 12,
	.div_val = 4,

	.xo_div4_reg = GCC_XO_DIV4_CBCR,
	.debug_ctl_reg = GCC_DEBUG_CTL,
	.debug_status_reg = GCC_DEBUG_STATUS,
};

static struct debug_mux mmss_cc = {
	.phys = 0x8c0000,
	.size = 0xb00c,

	.enable_reg = 0x900,
	.enable_mask = BIT(16),

	.mux_reg = 0x900,
	.mux_mask = 0x3ff,
};

static struct measure_clk msm8996_clocks[] = {
	{ "snoc_clk", &gcc, 0x0000 },
	{ "gcc_sys_noc_usb3_axi_clk", &gcc, 0x0006 },
	{ "gcc_sys_noc_ufs_axi_clk", &gcc, 0x0007 },
	{ "cnoc_clk", &gcc, 0x000e },
	{ "pnoc_clk", &gcc, 0x0011 },
	{ "gcc_periph_noc_usb20_ahb_clk", &gcc, 0x0014 },
	{ "gcc_mmss_noc_cfg_ahb_clk", &gcc, 0x0019 },
	//{ "mmss_gcc_dbg_clk", &gcc, 0x001b },
	{ "mmss_mmagic_ahb_clk", &gcc, 0x1b, &mmss_cc, 0x0001 },
	{ "mmss_misc_ahb_clk", &gcc, 0x1b, &mmss_cc, 0x0003 },
	{ "vmem_maxi_clk", &gcc, 0x1b, &mmss_cc, 0x0009 },
	{ "vmem_ahb_clk", &gcc, 0x1b, &mmss_cc, 0x000a },
	{ "gpu_ahb_clk", &gcc, 0x1b, &mmss_cc, 0x000c },
	{ "gpu_gx_gfx3d_clk", &gcc, 0x1b, &mmss_cc, 0x000d },
	{ "video_core_clk", &gcc, 0x1b, &mmss_cc, 0x000e },
	{ "video_axi_clk", &gcc, 0x1b, &mmss_cc, 0x000f },
	{ "video_maxi_clk", &gcc, 0x1b, &mmss_cc, 0x0010 },
	{ "video_ahb_clk", &gcc, 0x1b, &mmss_cc, 0x0011 },
	{ "mmss_rbcpr_clk", &gcc, 0x1b, &mmss_cc, 0x0012 },
	{ "mmss_rbcpr_ahb_clk", &gcc, 0x1b, &mmss_cc, 0x0013 },
	{ "mdss_mdp_clk", &gcc, 0x1b, &mmss_cc, 0x0014 },
	{ "mdss_pclk0_clk", &gcc, 0x1b, &mmss_cc, 0x0016 },
	{ "mdss_pclk1_clk", &gcc, 0x1b, &mmss_cc, 0x0017 },
	{ "mdss_extpclk_clk", &gcc, 0x1b, &mmss_cc, 0x0018 },
	{ "video_subcore0_clk", &gcc, 0x1b, &mmss_cc, 0x001a },
	{ "video_subcore1_clk", &gcc, 0x1b, &mmss_cc, 0x001b },
	{ "mdss_vsync_clk", &gcc, 0x1b, &mmss_cc, 0x001c },
	{ "mdss_hdmi_clk", &gcc, 0x1b, &mmss_cc, 0x001d },
	{ "mdss_byte0_clk", &gcc, 0x1b, &mmss_cc, 0x001e },
	{ "mdss_byte1_clk", &gcc, 0x1b, &mmss_cc, 0x001f },
	{ "mdss_esc0_clk", &gcc, 0x1b, &mmss_cc, 0x0020 },
	{ "mdss_esc1_clk", &gcc, 0x1b, &mmss_cc, 0x0021 },
	{ "mdss_ahb_clk", &gcc, 0x1b, &mmss_cc, 0x0022 },
	{ "mdss_hdmi_ahb_clk", &gcc, 0x1b, &mmss_cc, 0x0023 },
	{ "mdss_axi_clk", &gcc, 0x1b, &mmss_cc, 0x0024 },
	{ "camss_top_ahb_clk", &gcc, 0x1b, &mmss_cc, 0x0025 },
	{ "camss_micro_ahb_clk", &gcc, 0x1b, &mmss_cc, 0x0026 },
	{ "camss_gp0_clk", &gcc, 0x1b, &mmss_cc, 0x0027 },
	{ "camss_gp1_clk", &gcc, 0x1b, &mmss_cc, 0x0028 },
	{ "camss_mclk0_clk", &gcc, 0x1b, &mmss_cc, 0x0029 },
	{ "camss_mclk1_clk", &gcc, 0x1b, &mmss_cc, 0x002a },
	{ "camss_mclk2_clk", &gcc, 0x1b, &mmss_cc, 0x002b },
	{ "camss_mclk3_clk", &gcc, 0x1b, &mmss_cc, 0x002c },
	{ "camss_cci_clk", &gcc, 0x1b, &mmss_cc, 0x002d },
	{ "camss_cci_ahb_clk", &gcc, 0x1b, &mmss_cc, 0x002e },
	{ "camss_csi0phytimer_clk", &gcc, 0x1b, &mmss_cc, 0x002f },
	{ "camss_csi1phytimer_clk", &gcc, 0x1b, &mmss_cc, 0x0030 },
	{ "camss_csi2phytimer_clk", &gcc, 0x1b, &mmss_cc, 0x0031 },
	{ "camss_jpeg0_clk", &gcc, 0x1b, &mmss_cc, 0x0032 },
	{ "camss_ispif_ahb_clk", &gcc, 0x1b, &mmss_cc, 0x0033 },
	{ "camss_jpeg2_clk", &gcc, 0x1b, &mmss_cc, 0x0034 },
	{ "camss_jpeg_ahb_clk", &gcc, 0x1b, &mmss_cc, 0x0035 },
	{ "camss_jpeg_axi_clk", &gcc, 0x1b, &mmss_cc, 0x0036 },
	{ "camss_ahb_clk", &gcc, 0x1b, &mmss_cc, 0x0037 },
	{ "camss_vfe0_clk", &gcc, 0x1b, &mmss_cc, 0x0038 },
	{ "camss_vfe1_clk", &gcc, 0x1b, &mmss_cc, 0x0039 },
	{ "camss_cpp_clk", &gcc, 0x1b, &mmss_cc, 0x003a },
	{ "camss_cpp_ahb_clk", &gcc, 0x1b, &mmss_cc, 0x003b },
	{ "camss_vfe_ahb_clk", &gcc, 0x1b, &mmss_cc, 0x003c },
	{ "camss_vfe_axi_clk", &gcc, 0x1b, &mmss_cc, 0x003d },
	{ "gpu_gx_rbbmtimer_clk", &gcc, 0x1b, &mmss_cc, 0x003e },
	{ "camss_csi_vfe0_clk", &gcc, 0x1b, &mmss_cc, 0x003f },
	{ "camss_csi_vfe1_clk", &gcc, 0x1b, &mmss_cc, 0x0040 },
	{ "camss_csi0_clk", &gcc, 0x1b, &mmss_cc, 0x0041 },
	{ "camss_csi0_ahb_clk", &gcc, 0x1b, &mmss_cc, 0x0042 },
	{ "camss_csi0phy_clk", &gcc, 0x1b, &mmss_cc, 0x0043 },
	{ "camss_csi0rdi_clk", &gcc, 0x1b, &mmss_cc, 0x0044 },
	{ "camss_csi0pix_clk", &gcc, 0x1b, &mmss_cc, 0x0045 },
	{ "camss_csi1_clk", &gcc, 0x1b, &mmss_cc, 0x0046 },
	{ "camss_csi1_ahb_clk", &gcc, 0x1b, &mmss_cc, 0x0047 },
	{ "camss_csi1phy_clk", &gcc, 0x1b, &mmss_cc, 0x0048 },
	{ "camss_csi1rdi_clk", &gcc, 0x1b, &mmss_cc, 0x0049 },
	{ "camss_csi1pix_clk", &gcc, 0x1b, &mmss_cc, 0x004a },
	{ "camss_csi2_clk", &gcc, 0x1b, &mmss_cc, 0x004b },
	{ "camss_csi2_ahb_clk", &gcc, 0x1b, &mmss_cc, 0x004c },
	{ "camss_csi2phy_clk", &gcc, 0x1b, &mmss_cc, 0x004d },
	{ "camss_csi2rdi_clk", &gcc, 0x1b, &mmss_cc, 0x004e },
	{ "camss_csi2pix_clk", &gcc, 0x1b, &mmss_cc, 0x004f },
	{ "camss_csi3_clk", &gcc, 0x1b, &mmss_cc, 0x0050 },
	{ "camss_csi3_ahb_clk", &gcc, 0x1b, &mmss_cc, 0x0051 },
	{ "camss_csi3phy_clk", &gcc, 0x1b, &mmss_cc, 0x0052 },
	{ "camss_csi3rdi_clk", &gcc, 0x1b, &mmss_cc, 0x0053 },
	{ "camss_csi3pix_clk", &gcc, 0x1b, &mmss_cc, 0x0054 },
	{ "mmss_mmagic_maxi_clk", &gcc, 0x1b, &mmss_cc, 0x0070 },
	{ "camss_vfe0_stream_clk", &gcc, 0x1b, &mmss_cc, 0x0071 },
	{ "camss_vfe1_stream_clk", &gcc, 0x1b, &mmss_cc, 0x0072 },
	{ "camss_cpp_vbif_ahb_clk", &gcc, 0x1b, &mmss_cc, 0x0073 },
	{ "mmss_mmagic_cfg_ahb_clk", &gcc, 0x1b, &mmss_cc, 0x0074 },
	{ "mmss_misc_cxo_clk", &gcc, 0x1b, &mmss_cc, 0x0077 },
	{ "camss_cpp_axi_clk", &gcc, 0x1b, &mmss_cc, 0x007a },
	{ "camss_jpeg_dma_clk", &gcc, 0x1b, &mmss_cc, 0x007b },
	{ "camss_vfe0_ahb_clk", &gcc, 0x1b, &mmss_cc, 0x0086 },
	{ "camss_vfe1_ahb_clk", &gcc, 0x1b, &mmss_cc, 0x0087 },
	{ "gpu_aon_isense_clk", &gcc, 0x1b, &mmss_cc, 0x0088 },
	{ "fd_core_clk", &gcc, 0x1b, &mmss_cc, 0x0089 },
	{ "fd_core_uar_clk", &gcc, 0x1b, &mmss_cc, 0x008a },
	{ "fd_ahb_clk", &gcc, 0x1b, &mmss_cc, 0x008c },
	{ "camss_csiphy0_3p_clk", &gcc, 0x1b, &mmss_cc, 0x0091 },
	{ "camss_csiphy1_3p_clk", &gcc, 0x1b, &mmss_cc, 0x0092 },
	{ "camss_csiphy2_3p_clk", &gcc, 0x1b, &mmss_cc, 0x0093 },
	{ "smmu_vfe_ahb_clk", &gcc, 0x1b, &mmss_cc, 0x0094 },
	{ "smmu_vfe_axi_clk", &gcc, 0x1b, &mmss_cc, 0x0095 },
	{ "smmu_cpp_ahb_clk", &gcc, 0x1b, &mmss_cc, 0x0096 },
	{ "smmu_cpp_axi_clk", &gcc, 0x1b, &mmss_cc, 0x0097 },
	{ "smmu_jpeg_ahb_clk", &gcc, 0x1b, &mmss_cc, 0x0098 },
	{ "smmu_jpeg_axi_clk", &gcc, 0x1b, &mmss_cc, 0x0099 },
	{ "mmagic_camss_axi_clk", &gcc, 0x1b, &mmss_cc, 0x009a },
	{ "smmu_rot_ahb_clk", &gcc, 0x1b, &mmss_cc, 0x009b },
	{ "smmu_rot_axi_clk", &gcc, 0x1b, &mmss_cc, 0x009c },
	{ "smmu_mdp_ahb_clk", &gcc, 0x1b, &mmss_cc, 0x009d },
	{ "smmu_mdp_axi_clk", &gcc, 0x1b, &mmss_cc, 0x009e },
	{ "mmagic_mdss_axi_clk", &gcc, 0x1b, &mmss_cc, 0x009f },
	{ "smmu_video_ahb_clk", &gcc, 0x1b, &mmss_cc, 0x00a0 },
	{ "smmu_video_axi_clk", &gcc, 0x1b, &mmss_cc, 0x00a1 },
	{ "mmagic_video_axi_clk", &gcc, 0x1b, &mmss_cc, 0x00a2 },
	{ "mmagic_camss_noc_cfg_ahb_clk", &gcc, 0x1b, &mmss_cc, 0x00ad },
	{ "mmagic_mdss_noc_cfg_ahb_clk", &gcc, 0x1b, &mmss_cc, 0x00ae },
	{ "mmagic_video_noc_cfg_ahb_clk", &gcc, 0x1b, &mmss_cc, 0x00af },
	{ "mmagic_bimc_noc_cfg_ahb_clk", &gcc, 0x1b, &mmss_cc, 0x00b0 },
	{ "gcc_mmss_bimc_gfx_clk", &gcc, 0x001c},
	{ "gcc_usb30_master_clk", &gcc, 0x002d },
	{ "gcc_usb30_sleep_clk", &gcc, 0x002e },
	{ "gcc_usb30_mock_utmi_clk", &gcc, 0x002f },
	{ "gcc_usb3_phy_aux_clk", &gcc, 0x0030 },
	{ "gcc_usb3_phy_pipe_clk", &gcc, 0x0031 },
	{ "gcc_usb20_master_clk", &gcc, 0x0035 },
	{ "gcc_usb20_sleep_clk", &gcc, 0x0036 },
	{ "gcc_usb20_mock_utmi_clk", &gcc, 0x0037 },
	{ "gcc_usb_phy_cfg_ahb2phy_clk", &gcc, 0x0038 },
	{ "gcc_sdcc1_apps_clk", &gcc, 0x0039 },
	{ "gcc_sdcc1_ahb_clk", &gcc, 0x003a },
	{ "gcc_sdcc2_apps_clk", &gcc, 0x003b },
	{ "gcc_sdcc2_ahb_clk", &gcc, 0x003c },
	{ "gcc_sdcc3_apps_clk", &gcc, 0x003d },
	{ "gcc_sdcc3_ahb_clk", &gcc, 0x003e },
	{ "gcc_sdcc4_apps_clk", &gcc, 0x003f },
	{ "gcc_sdcc4_ahb_clk", &gcc, 0x0040 },
	{ "gcc_blsp1_ahb_clk", &gcc, 0x0041 },
	{ "gcc_blsp1_qup1_spi_apps_clk", &gcc, 0x0043 },
	{ "gcc_blsp1_qup1_i2c_apps_clk", &gcc, 0x0044 },
	{ "gcc_blsp1_uart1_apps_clk", &gcc, 0x0045 },
	{ "gcc_blsp1_qup2_spi_apps_clk", &gcc, 0x0047 },
	{ "gcc_blsp1_qup2_i2c_apps_clk", &gcc, 0x0048 },
	{ "gcc_blsp1_uart2_apps_clk", &gcc, 0x0049 },
	{ "gcc_blsp1_qup3_spi_apps_clk", &gcc, 0x004b },
	{ "gcc_blsp1_qup3_i2c_apps_clk", &gcc, 0x004c },
	{ "gcc_blsp1_uart3_apps_clk", &gcc, 0x004d },
	{ "gcc_blsp1_qup4_spi_apps_clk", &gcc, 0x004f },
	{ "gcc_blsp1_qup4_i2c_apps_clk", &gcc, 0x0050 },
	{ "gcc_blsp1_uart4_apps_clk", &gcc, 0x0051 },
	{ "gcc_blsp1_qup5_spi_apps_clk", &gcc, 0x0053 },
	{ "gcc_blsp1_qup5_i2c_apps_clk", &gcc, 0x0054 },
	{ "gcc_blsp1_uart5_apps_clk", &gcc, 0x0055 },
	{ "gcc_blsp1_qup6_spi_apps_clk", &gcc, 0x0057 },
	{ "gcc_blsp1_qup6_i2c_apps_clk", &gcc, 0x0058 },
	{ "gcc_blsp1_uart6_apps_clk", &gcc, 0x0059 },
	{ "gcc_blsp2_ahb_clk", &gcc, 0x005b },
	{ "gcc_blsp2_qup1_spi_apps_clk", &gcc, 0x005d },
	{ "gcc_blsp2_qup1_i2c_apps_clk", &gcc, 0x005e },
	{ "gcc_blsp2_uart1_apps_clk", &gcc, 0x005f },
	{ "gcc_blsp2_qup2_spi_apps_clk", &gcc, 0x0061 },
	{ "gcc_blsp2_qup2_i2c_apps_clk", &gcc, 0x0062 },
	{ "gcc_blsp2_uart2_apps_clk", &gcc, 0x0063 },
	{ "gcc_blsp2_qup3_spi_apps_clk", &gcc, 0x0065 },
	{ "gcc_blsp2_qup3_i2c_apps_clk", &gcc, 0x0066 },
	{ "gcc_blsp2_uart3_apps_clk", &gcc, 0x0067 },
	{ "gcc_blsp2_qup4_spi_apps_clk", &gcc, 0x0069 },
	{ "gcc_blsp2_qup4_i2c_apps_clk", &gcc, 0x006a },
	{ "gcc_blsp2_uart4_apps_clk", &gcc, 0x006b },
	{ "gcc_blsp2_qup5_spi_apps_clk", &gcc, 0x006d },
	{ "gcc_blsp2_qup5_i2c_apps_clk", &gcc, 0x006e },
	{ "gcc_blsp2_uart5_apps_clk", &gcc, 0x006f },
	{ "gcc_blsp2_qup6_spi_apps_clk", &gcc, 0x0071 },
	{ "gcc_blsp2_qup6_i2c_apps_clk", &gcc, 0x0072 },
	{ "gcc_blsp2_uart6_apps_clk", &gcc, 0x0073 },
	{ "gcc_pdm_ahb_clk", &gcc, 0x0076 },
	{ "gcc_pdm2_clk", &gcc, 0x0078 },
	{ "gcc_prng_ahb_clk", &gcc, 0x0079 },
	{ "gcc_tsif_ahb_clk", &gcc, 0x007a },
	{ "gcc_tsif_ref_clk", &gcc, 0x007b },
	{ "gcc_boot_rom_ahb_clk", &gcc, 0x007e },
	{ "ce1_clk", &gcc, 0x0099 },
	{ "gcc_ce1_axi_m_clk", &gcc, 0x009a },
	{ "gcc_ce1_ahb_m_clk", &gcc, 0x009b },
	{ "measure_only_bimc_hmss_axi_clk", &gcc, 0x00a5 },
	{ "bimc_clk", &gcc, 0x00ad },
	{ "gcc_bimc_gfx_clk", &gcc, 0x00af},
	{ "gcc_hmss_rbcpr_clk", &gcc, 0x00ba },
	//{ "cpu_dbg_clk", &gcc, 0x00bb },
	{ "gcc_gp1_clk", &gcc, 0x00e3 },
	{ "gcc_gp2_clk", &gcc, 0x00e4 },
	{ "gcc_gp3_clk", &gcc, 0x00e5 },
	{ "gcc_pcie_0_slv_axi_clk", &gcc, 0x00e6 },
	{ "gcc_pcie_0_mstr_axi_clk", &gcc, 0x00e7 },
	{ "gcc_pcie_0_cfg_ahb_clk", &gcc, 0x00e8 },
	{ "gcc_pcie_0_aux_clk", &gcc, 0x00e9 },
	{ "gcc_pcie_0_pipe_clk", &gcc, 0x00ea },
	{ "gcc_pcie_1_slv_axi_clk", &gcc, 0x00ec },
	{ "gcc_pcie_1_mstr_axi_clk", &gcc, 0x00ed },
	{ "gcc_pcie_1_cfg_ahb_clk", &gcc, 0x00ee },
	{ "gcc_pcie_1_aux_clk", &gcc, 0x00ef },
	{ "gcc_pcie_1_pipe_clk", &gcc, 0x00f0 },
	{ "gcc_pcie_2_slv_axi_clk", &gcc, 0x00f2 },
	{ "gcc_pcie_2_mstr_axi_clk", &gcc, 0x00f3 },
	{ "gcc_pcie_2_cfg_ahb_clk", &gcc, 0x00f4 },
	{ "gcc_pcie_2_aux_clk", &gcc, 0x00f5 },
	{ "gcc_pcie_2_pipe_clk", &gcc, 0x00f6 },
	{ "gcc_pcie_phy_cfg_ahb_clk", &gcc, 0x00f8 },
	{ "gcc_pcie_phy_aux_clk", &gcc, 0x00f9 },
	{ "gcc_ufs_axi_clk", &gcc, 0x00fc },
	{ "gcc_ufs_ahb_clk", &gcc, 0x00fd },
	{ "gcc_ufs_tx_cfg_clk", &gcc, 0x00fe },
	{ "gcc_ufs_rx_cfg_clk", &gcc, 0x00ff },
	{ "gcc_ufs_tx_symbol_0_clk", &gcc, 0x0100 },
	{ "gcc_ufs_rx_symbol_0_clk", &gcc, 0x0101 },
	{ "gcc_ufs_rx_symbol_1_clk", &gcc, 0x0102 },
	{ "gcc_ufs_unipro_core_clk", &gcc, 0x0106 },
	{ "gcc_ufs_ice_core_clk", &gcc, 0x0107 },
	{ "gcc_ufs_sys_clk_core_clk", &gcc, 0x108},
	{ "gcc_ufs_tx_symbol_clk_core_clk", &gcc, 0x0109 },
	{ "gcc_aggre0_snoc_axi_clk", &gcc, 0x0116 },
	{ "gcc_aggre0_cnoc_ahb_clk", &gcc, 0x0117 },
	{ "gcc_smmu_aggre0_axi_clk", &gcc, 0x0119 },
	{ "gcc_smmu_aggre0_ahb_clk", &gcc, 0x011a },
	{ "gcc_aggre0_noc_qosgen_extref_clk", &gcc, 0x011b },
	{ "gcc_aggre2_ufs_axi_clk", &gcc, 0x0126 },
	{ "gcc_aggre2_usb3_axi_clk", &gcc, 0x0127 },
	{ "gcc_dcc_ahb_clk", &gcc, 0x012b },
	{ "gcc_aggre0_noc_mpu_cfg_ahb_clk", &gcc, 0x012c},
	{ "ipa_clk", &gcc, 0x12f },
	{ "gcc_mss_cfg_ahb_clk", &gcc, 0x0133 },
	{ "gcc_mss_mnoc_bimc_axi_clk", &gcc, 0x0134 },
	{ "gcc_mss_snoc_axi_clk", &gcc, 0x0135 },
	{ "gcc_mss_q6_bimc_axi_clk", &gcc, 0x0136 },
	{}
};

struct debugcc_platform msm8996_debugcc = {
	"msm8996",
	msm8996_clocks,
};
