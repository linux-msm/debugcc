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

#define GCC_BASE			0x01800000
#define GCC_SIZE			0x80000

#define GCC_DEBUG_CLK_CTL		0x74000
#define GCC_CLOCK_FREQ_MEASURE_CTL	0x74004
#define GCC_CLOCK_FREQ_MEASURE_STATUS	0x74008
#define GCC_XO_DIV4_CBCR		0x30034

static struct gcc_mux gcc = {
	.mux = {
		.phys =	GCC_BASE,
		.size = GCC_SIZE,

		.measure = measure_gcc,

		.enable_reg = GCC_DEBUG_CLK_CTL,
		.enable_mask = BIT(16),

		.mux_reg = GCC_DEBUG_CLK_CTL,
		.mux_mask = 0x1ff,

		.div_reg = GCC_DEBUG_CLK_CTL,
		.div_shift = 12,
		.div_mask = 0xf << 12,
		.div_val = 4,
	},

	.xo_div4_reg = GCC_XO_DIV4_CBCR,
	.debug_ctl_reg = GCC_CLOCK_FREQ_MEASURE_CTL,
	.debug_status_reg = GCC_CLOCK_FREQ_MEASURE_STATUS,
};

static struct measure_clk msm8936_clocks[] = {
	{ .name = "gcc_gp1_clk", .clk_mux = &gcc.mux, .mux = 16 },
	{ .name = "gcc_gp2_clk", .clk_mux = &gcc.mux, .mux = 17 },
	{ .name = "gcc_gp3_clk", .clk_mux = &gcc.mux, .mux = 18 },
	{ .name = "gcc_bimc_gfx_clk", .clk_mux = &gcc.mux, .mux = 45 },
	{ .name = "gcc_mss_cfg_ahb_clk", .clk_mux = &gcc.mux, .mux = 48 },
	{ .name = "gcc_mss_q6_bimc_axi_clk", .clk_mux = &gcc.mux, .mux = 49 },
	{ .name = "gcc_apss_tcu_clk", .clk_mux = &gcc.mux, .mux = 80 },
	{ .name = "gcc_mdp_tbu_clk", .clk_mux = &gcc.mux, .mux = 81 },
	{ .name = "gcc_gfx_tbu_clk", .clk_mux = &gcc.mux, .mux = 82 },
	{ .name = "gcc_gfx_tcu_clk", .clk_mux = &gcc.mux, .mux = 83 },
	{ .name = "gcc_venus_tbu_clk", .clk_mux = &gcc.mux, .mux = 84 },
	{ .name = "gcc_gtcu_ahb_clk", .clk_mux = &gcc.mux, .mux = 88 },
	{ .name = "gcc_vfe_tbu_clk", .clk_mux = &gcc.mux, .mux = 90 },
	{ .name = "gcc_smmu_cfg_clk", .clk_mux = &gcc.mux, .mux = 91 },
	{ .name = "gcc_jpeg_tbu_clk", .clk_mux = &gcc.mux, .mux = 92 },
	{ .name = "gcc_usb_hs_system_clk", .clk_mux = &gcc.mux, .mux = 96 },
	{ .name = "gcc_usb_hs_ahb_clk", .clk_mux = &gcc.mux, .mux = 97 },
	{ .name = "gcc_usb_fs_ahb_clk", .clk_mux = &gcc.mux, .mux = 241 },
	{ .name = "gcc_usb_fs_ic_clk", .clk_mux = &gcc.mux, .mux = 244 },
	{ .name = "gcc_usb2a_phy_sleep_clk", .clk_mux = &gcc.mux, .mux = 99 },
	{ .name = "gcc_sdcc1_apps_clk", .clk_mux = &gcc.mux, .mux = 104 },
	{ .name = "gcc_sdcc1_ahb_clk", .clk_mux = &gcc.mux, .mux = 105 },
	{ .name = "gcc_sdcc2_apps_clk", .clk_mux = &gcc.mux, .mux = 112 },
	{ .name = "gcc_sdcc2_ahb_clk", .clk_mux = &gcc.mux, .mux = 113 },
	{ .name = "gcc_blsp1_ahb_clk", .clk_mux = &gcc.mux, .mux = 136 },
	{ .name = "gcc_blsp1_qup1_spi_apps_clk", .clk_mux = &gcc.mux, .mux = 138 },
	{ .name = "gcc_blsp1_qup1_i2c_apps_clk", .clk_mux = &gcc.mux, .mux = 139 },
	{ .name = "gcc_blsp1_uart1_apps_clk", .clk_mux = &gcc.mux, .mux = 140 },
	{ .name = "gcc_blsp1_qup2_spi_apps_clk", .clk_mux = &gcc.mux, .mux = 142 },
	{ .name = "gcc_blsp1_qup2_i2c_apps_clk", .clk_mux = &gcc.mux, .mux = 144 },
	{ .name = "gcc_blsp1_uart2_apps_clk", .clk_mux = &gcc.mux, .mux = 145 },
	{ .name = "gcc_blsp1_qup3_spi_apps_clk", .clk_mux = &gcc.mux, .mux = 147 },
	{ .name = "gcc_blsp1_qup3_i2c_apps_clk", .clk_mux = &gcc.mux, .mux = 148 },
	{ .name = "gcc_blsp1_qup4_spi_apps_clk", .clk_mux = &gcc.mux, .mux = 152 },
	{ .name = "gcc_blsp1_qup4_i2c_apps_clk", .clk_mux = &gcc.mux, .mux = 153 },
	{ .name = "gcc_blsp1_qup5_spi_apps_clk", .clk_mux = &gcc.mux, .mux = 156 },
	{ .name = "gcc_blsp1_qup5_i2c_apps_clk", .clk_mux = &gcc.mux, .mux = 157 },
	{ .name = "gcc_blsp1_qup6_spi_apps_clk", .clk_mux = &gcc.mux, .mux = 161 },
	{ .name = "gcc_blsp1_qup6_i2c_apps_clk", .clk_mux = &gcc.mux, .mux = 162 },
	{ .name = "gcc_camss_ahb_clk", .clk_mux = &gcc.mux, .mux = 168 },
	{ .name = "gcc_camss_top_ahb_clk", .clk_mux = &gcc.mux, .mux = 169 },
	{ .name = "gcc_camss_micro_ahb_clk", .clk_mux = &gcc.mux, .mux = 170 },
	{ .name = "gcc_camss_gp0_clk", .clk_mux = &gcc.mux, .mux = 171 },
	{ .name = "gcc_camss_gp1_clk", .clk_mux = &gcc.mux, .mux = 172 },
	{ .name = "gcc_camss_mclk0_clk", .clk_mux = &gcc.mux, .mux = 173 },
	{ .name = "gcc_camss_mclk1_clk", .clk_mux = &gcc.mux, .mux = 174 },
	{ .name = "gcc_camss_mclk2_clk", .clk_mux = &gcc.mux, .mux = 445 },
	{ .name = "gcc_camss_cci_clk", .clk_mux = &gcc.mux, .mux = 175 },
	{ .name = "gcc_camss_cci_ahb_clk", .clk_mux = &gcc.mux, .mux = 176 },
	{ .name = "gcc_camss_csi0phytimer_clk", .clk_mux = &gcc.mux, .mux = 177 },
	{ .name = "gcc_camss_csi1phytimer_clk", .clk_mux = &gcc.mux, .mux = 178 },
	{ .name = "gcc_camss_jpeg0_clk", .clk_mux = &gcc.mux, .mux = 179 },
	{ .name = "gcc_camss_jpeg_ahb_clk", .clk_mux = &gcc.mux, .mux = 180 },
	{ .name = "gcc_camss_jpeg_axi_clk", .clk_mux = &gcc.mux, .mux = 181 },
	{ .name = "gcc_camss_vfe0_clk", .clk_mux = &gcc.mux, .mux = 184 },
	{ .name = "gcc_camss_cpp_clk", .clk_mux = &gcc.mux, .mux = 185 },
	{ .name = "gcc_camss_cpp_ahb_clk", .clk_mux = &gcc.mux, .mux = 186 },
	{ .name = "gcc_camss_vfe_ahb_clk", .clk_mux = &gcc.mux, .mux = 187 },
	{ .name = "gcc_camss_vfe_axi_clk", .clk_mux = &gcc.mux, .mux = 188 },
	{ .name = "gcc_camss_csi_vfe0_clk", .clk_mux = &gcc.mux, .mux = 191 },
	{ .name = "gcc_camss_csi0_clk", .clk_mux = &gcc.mux, .mux = 192 },
	{ .name = "gcc_camss_csi0_ahb_clk", .clk_mux = &gcc.mux, .mux = 193 },
	{ .name = "gcc_camss_csi0phy_clk", .clk_mux = &gcc.mux, .mux = 194 },
	{ .name = "gcc_camss_csi0rdi_clk", .clk_mux = &gcc.mux, .mux = 195 },
	{ .name = "gcc_camss_csi0pix_clk", .clk_mux = &gcc.mux, .mux = 196 },
	{ .name = "gcc_camss_csi1_clk", .clk_mux = &gcc.mux, .mux = 197 },
	{ .name = "gcc_camss_csi1_ahb_clk", .clk_mux = &gcc.mux, .mux = 198 },
	{ .name = "gcc_camss_csi1phy_clk", .clk_mux = &gcc.mux, .mux = 199 },
	{ .name = "gcc_camss_csi2_clk", .clk_mux = &gcc.mux, .mux = 227 },
	{ .name = "gcc_camss_csi2_ahb_clk", .clk_mux = &gcc.mux, .mux = 228 },
	{ .name = "gcc_camss_csi2phy_clk", .clk_mux = &gcc.mux, .mux = 229 },
	{ .name = "gcc_camss_csi2rdi_clk", .clk_mux = &gcc.mux, .mux = 230 },
	{ .name = "gcc_camss_csi2pix_clk", .clk_mux = &gcc.mux, .mux = 231 },
	{ .name = "gcc_pdm_ahb_clk", .clk_mux = &gcc.mux, .mux = 208 },
	{ .name = "gcc_pdm2_clk", .clk_mux = &gcc.mux, .mux = 210 },
	{ .name = "gcc_prng_ahb_clk", .clk_mux = &gcc.mux, .mux = 216 },
	{ .name = "gcc_camss_csi1rdi_clk", .clk_mux = &gcc.mux, .mux = 224 },
	{ .name = "gcc_camss_csi1pix_clk", .clk_mux = &gcc.mux, .mux = 225 },
	{ .name = "gcc_camss_ispif_ahb_clk", .clk_mux = &gcc.mux, .mux = 226 },
	{ .name = "gcc_boot_rom_ahb_clk", .clk_mux = &gcc.mux, .mux = 248 },
	{ .name = "gcc_crypto_clk", .clk_mux = &gcc.mux, .mux = 312 },
	{ .name = "gcc_crypto_axi_clk", .clk_mux = &gcc.mux, .mux = 313 },
	{ .name = "gcc_crypto_ahb_clk", .clk_mux = &gcc.mux, .mux = 314 },
	{ .name = "gcc_oxili_timer_clk", .clk_mux = &gcc.mux, .mux = 489 },
	{ .name = "gcc_oxili_gfx3d_clk", .clk_mux = &gcc.mux, .mux = 490 },
	{ .name = "gcc_oxili_ahb_clk", .clk_mux = &gcc.mux, .mux = 491 },
	{ .name = "gcc_oxili_gmem_clk", .clk_mux = &gcc.mux, .mux = 496 },
	{ .name = "gcc_venus0_vcodec0_clk", .clk_mux = &gcc.mux, .mux = 497 },
	{ .name = "gcc_venus0_core0_vcodec0_clk", .clk_mux = &gcc.mux, .mux = 440 },
	{ .name = "gcc_venus0_core1_vcodec0_clk", .clk_mux = &gcc.mux, .mux = 441 },
	{ .name = "gcc_venus0_axi_clk", .clk_mux = &gcc.mux, .mux = 498 },
	{ .name = "gcc_venus0_ahb_clk", .clk_mux = &gcc.mux, .mux = 499 },
	{ .name = "gcc_mdss_ahb_clk", .clk_mux = &gcc.mux, .mux = 502 },
	{ .name = "gcc_mdss_axi_clk", .clk_mux = &gcc.mux, .mux = 503 },
	{ .name = "gcc_mdss_pclk0_clk", .clk_mux = &gcc.mux, .mux = 504 },
	{ .name = "gcc_mdss_pclk1_clk", .clk_mux = &gcc.mux, .mux = 442 },
	{ .name = "gcc_mdss_mdp_clk", .clk_mux = &gcc.mux, .mux = 505 },
	{ .name = "gcc_mdss_vsync_clk", .clk_mux = &gcc.mux, .mux = 507 },
	{ .name = "gcc_mdss_byte0_clk", .clk_mux = &gcc.mux, .mux = 508 },
	{ .name = "gcc_mdss_byte1_clk", .clk_mux = &gcc.mux, .mux = 443 },
	{ .name = "gcc_mdss_esc0_clk", .clk_mux = &gcc.mux, .mux = 509 },
	{ .name = "gcc_mdss_esc1_clk", .clk_mux = &gcc.mux, .mux = 444 },
	{ .name = "gcc_bimc_clk", .clk_mux = &gcc.mux, .mux = 340 },
	{ .name = "gcc_bimc_gpu_clk", .clk_mux = &gcc.mux, .mux = 343 },
	{ .name = "gcc_bimc_ddr_ch0_clk", .clk_mux = &gcc.mux, .mux = 346 },
	{ .name = "gcc_cpp_tbu_clk", .clk_mux = &gcc.mux, .mux = 233 },
	{ .name = "gcc_mdp_rt_tbu_clk", .clk_mux = &gcc.mux, .mux = 238 },
	{ .name = "wcnss_m_clk", .clk_mux = &gcc.mux, .mux = 408 },
	{},
};

struct debugcc_platform msm8936_debugcc = {
	.name = "msm8936",
	.clocks = msm8936_clocks,
};
