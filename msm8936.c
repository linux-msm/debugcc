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

static struct debug_mux gcc;

static struct debug_mux gcc = {
	.phys =	GCC_BASE,
	.size = GCC_SIZE,

	.enable_reg = GCC_DEBUG_CLK_CTL,
	.enable_mask = BIT(16),

	.mux_reg = GCC_DEBUG_CLK_CTL,
	.mux_mask = 0x1ff,

	.div_reg = GCC_DEBUG_CLK_CTL,
	.div_shift = 12,
	.div_mask = 0xf << 12,
	.div_val = 4,

	.xo_div4_reg = GCC_XO_DIV4_CBCR,
	.debug_ctl_reg = GCC_CLOCK_FREQ_MEASURE_CTL,
	.debug_status_reg = GCC_CLOCK_FREQ_MEASURE_STATUS,
};

static struct measure_clk msm8936_clocks[] = {
	 { "gcc_gp1_clk", &gcc, 16 },
	 { "gcc_gp2_clk", &gcc, 17 },
	 { "gcc_gp3_clk", &gcc, 18 },
	 { "gcc_bimc_gfx_clk", &gcc, 45 },
	 { "gcc_mss_cfg_ahb_clk", &gcc, 48 },
	 { "gcc_mss_q6_bimc_axi_clk", &gcc, 49 },
	 { "gcc_apss_tcu_clk", &gcc, 80 },
	 { "gcc_mdp_tbu_clk", &gcc, 81 },
	 { "gcc_gfx_tbu_clk", &gcc, 82 },
	 { "gcc_gfx_tcu_clk", &gcc, 83 },
	 { "gcc_venus_tbu_clk", &gcc, 84 },
	 { "gcc_gtcu_ahb_clk", &gcc, 88 },
	 { "gcc_vfe_tbu_clk", &gcc, 90 },
	 { "gcc_smmu_cfg_clk", &gcc, 91 },
	 { "gcc_jpeg_tbu_clk", &gcc, 92 },
	 { "gcc_usb_hs_system_clk", &gcc, 96 },
	 { "gcc_usb_hs_ahb_clk", &gcc, 97 },
	 { "gcc_usb_fs_ahb_clk", &gcc, 241 },
	 { "gcc_usb_fs_ic_clk", &gcc, 244 },
	 { "gcc_usb2a_phy_sleep_clk", &gcc, 99 },
	 { "gcc_sdcc1_apps_clk", &gcc, 104 },
	 { "gcc_sdcc1_ahb_clk", &gcc, 105 },
	 { "gcc_sdcc2_apps_clk", &gcc, 112 },
	 { "gcc_sdcc2_ahb_clk", &gcc, 113 },
	 { "gcc_blsp1_ahb_clk", &gcc, 136 },
	 { "gcc_blsp1_qup1_spi_apps_clk", &gcc, 138 },
	 { "gcc_blsp1_qup1_i2c_apps_clk", &gcc, 139 },
	 { "gcc_blsp1_uart1_apps_clk", &gcc, 140 },
	 { "gcc_blsp1_qup2_spi_apps_clk", &gcc, 142 },
	 { "gcc_blsp1_qup2_i2c_apps_clk", &gcc, 144 },
	 { "gcc_blsp1_uart2_apps_clk", &gcc, 145 },
	 { "gcc_blsp1_qup3_spi_apps_clk", &gcc, 147 },
	 { "gcc_blsp1_qup3_i2c_apps_clk", &gcc, 148 },
	 { "gcc_blsp1_qup4_spi_apps_clk", &gcc, 152 },
	 { "gcc_blsp1_qup4_i2c_apps_clk", &gcc, 153 },
	 { "gcc_blsp1_qup5_spi_apps_clk", &gcc, 156 },
	 { "gcc_blsp1_qup5_i2c_apps_clk", &gcc, 157 },
	 { "gcc_blsp1_qup6_spi_apps_clk", &gcc, 161 },
	 { "gcc_blsp1_qup6_i2c_apps_clk", &gcc, 162 },
	 { "gcc_camss_ahb_clk", &gcc, 168 },
	 { "gcc_camss_top_ahb_clk", &gcc, 169 },
	 { "gcc_camss_micro_ahb_clk", &gcc, 170 },
	 { "gcc_camss_gp0_clk", &gcc, 171 },
	 { "gcc_camss_gp1_clk", &gcc, 172 },
	 { "gcc_camss_mclk0_clk", &gcc, 173 },
	 { "gcc_camss_mclk1_clk", &gcc, 174 },
	 { "gcc_camss_mclk2_clk", &gcc, 445 },
	 { "gcc_camss_cci_clk", &gcc, 175 },
	 { "gcc_camss_cci_ahb_clk", &gcc, 176 },
	 { "gcc_camss_csi0phytimer_clk", &gcc, 177 },
	 { "gcc_camss_csi1phytimer_clk", &gcc, 178 },
	 { "gcc_camss_jpeg0_clk", &gcc, 179 },
	 { "gcc_camss_jpeg_ahb_clk", &gcc, 180 },
	 { "gcc_camss_jpeg_axi_clk", &gcc, 181 },
	 { "gcc_camss_vfe0_clk", &gcc, 184 },
	 { "gcc_camss_cpp_clk", &gcc, 185 },
	 { "gcc_camss_cpp_ahb_clk", &gcc, 186 },
	 { "gcc_camss_vfe_ahb_clk", &gcc, 187 },
	 { "gcc_camss_vfe_axi_clk", &gcc, 188 },
	 { "gcc_camss_csi_vfe0_clk", &gcc, 191 },
	 { "gcc_camss_csi0_clk", &gcc, 192 },
	 { "gcc_camss_csi0_ahb_clk", &gcc, 193 },
	 { "gcc_camss_csi0phy_clk", &gcc, 194 },
	 { "gcc_camss_csi0rdi_clk", &gcc, 195 },
	 { "gcc_camss_csi0pix_clk", &gcc, 196 },
	 { "gcc_camss_csi1_clk", &gcc, 197 },
	 { "gcc_camss_csi1_ahb_clk", &gcc, 198 },
	 { "gcc_camss_csi1phy_clk", &gcc, 199 },
	 { "gcc_camss_csi2_clk", &gcc, 227 },
	 { "gcc_camss_csi2_ahb_clk", &gcc, 228 },
	 { "gcc_camss_csi2phy_clk", &gcc, 229 },
	 { "gcc_camss_csi2rdi_clk", &gcc, 230 },
	 { "gcc_camss_csi2pix_clk", &gcc, 231 },
	 { "gcc_pdm_ahb_clk", &gcc, 208 },
	 { "gcc_pdm2_clk", &gcc, 210 },
	 { "gcc_prng_ahb_clk", &gcc, 216 },
	 { "gcc_camss_csi1rdi_clk", &gcc, 224 },
	 { "gcc_camss_csi1pix_clk", &gcc, 225 },
	 { "gcc_camss_ispif_ahb_clk", &gcc, 226 },
	 { "gcc_boot_rom_ahb_clk", &gcc, 248 },
	 { "gcc_crypto_clk", &gcc, 312 },
	 { "gcc_crypto_axi_clk", &gcc, 313 },
	 { "gcc_crypto_ahb_clk", &gcc, 314 },
	 { "gcc_oxili_timer_clk", &gcc, 489 },
	 { "gcc_oxili_gfx3d_clk", &gcc, 490 },
	 { "gcc_oxili_ahb_clk", &gcc, 491 },
	 { "gcc_oxili_gmem_clk", &gcc, 496 },
	 { "gcc_venus0_vcodec0_clk", &gcc, 497 },
	 { "gcc_venus0_core0_vcodec0_clk", &gcc, 440 },
	 { "gcc_venus0_core1_vcodec0_clk", &gcc, 441 },
	 { "gcc_venus0_axi_clk", &gcc, 498 },
	 { "gcc_venus0_ahb_clk", &gcc, 499 },
	 { "gcc_mdss_ahb_clk", &gcc, 502 },
	 { "gcc_mdss_axi_clk", &gcc, 503 },
	 { "gcc_mdss_pclk0_clk", &gcc, 504 },
	 { "gcc_mdss_pclk1_clk", &gcc, 442 },
	 { "gcc_mdss_mdp_clk", &gcc, 505 },
	 { "gcc_mdss_vsync_clk", &gcc, 507 },
	 { "gcc_mdss_byte0_clk", &gcc, 508 },
	 { "gcc_mdss_byte1_clk", &gcc, 443 },
	 { "gcc_mdss_esc0_clk", &gcc, 509 },
	 { "gcc_mdss_esc1_clk", &gcc, 444 },
	 { "gcc_bimc_clk", &gcc, 340 },
	 { "gcc_bimc_gpu_clk", &gcc, 343 },
	 { "gcc_bimc_ddr_ch0_clk", &gcc, 346 },
	 { "gcc_cpp_tbu_clk", &gcc, 233 },
	 { "gcc_mdp_rt_tbu_clk", &gcc, 238 },
	 { "wcnss_m_clk", &gcc, 408 },
	{},
};

struct debugcc_platform msm8936_debugcc = {
	"msm8936",
	msm8936_clocks,
};
