// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2022, Linaro Ltd. */

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
	 { "gcc_gp1_clk", &gcc.mux, 16 },
	 { "gcc_gp2_clk", &gcc.mux, 17 },
	 { "gcc_gp3_clk", &gcc.mux, 18 },
	 { "gcc_bimc_gfx_clk", &gcc.mux, 45 },
	 { "gcc_mss_cfg_ahb_clk", &gcc.mux, 48 },
	 { "gcc_mss_q6_bimc_axi_clk", &gcc.mux, 49 },
	 { "gcc_apss_tcu_clk", &gcc.mux, 80 },
	 { "gcc_mdp_tbu_clk", &gcc.mux, 81 },
	 { "gcc_gfx_tbu_clk", &gcc.mux, 82 },
	 { "gcc_gfx_tcu_clk", &gcc.mux, 83 },
	 { "gcc_venus_tbu_clk", &gcc.mux, 84 },
	 { "gcc_gtcu_ahb_clk", &gcc.mux, 88 },
	 { "gcc_vfe_tbu_clk", &gcc.mux, 90 },
	 { "gcc_smmu_cfg_clk", &gcc.mux, 91 },
	 { "gcc_jpeg_tbu_clk", &gcc.mux, 92 },
	 { "gcc_usb_hs_system_clk", &gcc.mux, 96 },
	 { "gcc_usb_hs_ahb_clk", &gcc.mux, 97 },
	 { "gcc_usb_fs_ahb_clk", &gcc.mux, 241 },
	 { "gcc_usb_fs_ic_clk", &gcc.mux, 244 },
	 { "gcc_usb2a_phy_sleep_clk", &gcc.mux, 99 },
	 { "gcc_sdcc1_apps_clk", &gcc.mux, 104 },
	 { "gcc_sdcc1_ahb_clk", &gcc.mux, 105 },
	 { "gcc_sdcc2_apps_clk", &gcc.mux, 112 },
	 { "gcc_sdcc2_ahb_clk", &gcc.mux, 113 },
	 { "gcc_blsp1_ahb_clk", &gcc.mux, 136 },
	 { "gcc_blsp1_qup1_spi_apps_clk", &gcc.mux, 138 },
	 { "gcc_blsp1_qup1_i2c_apps_clk", &gcc.mux, 139 },
	 { "gcc_blsp1_uart1_apps_clk", &gcc.mux, 140 },
	 { "gcc_blsp1_qup2_spi_apps_clk", &gcc.mux, 142 },
	 { "gcc_blsp1_qup2_i2c_apps_clk", &gcc.mux, 144 },
	 { "gcc_blsp1_uart2_apps_clk", &gcc.mux, 145 },
	 { "gcc_blsp1_qup3_spi_apps_clk", &gcc.mux, 147 },
	 { "gcc_blsp1_qup3_i2c_apps_clk", &gcc.mux, 148 },
	 { "gcc_blsp1_qup4_spi_apps_clk", &gcc.mux, 152 },
	 { "gcc_blsp1_qup4_i2c_apps_clk", &gcc.mux, 153 },
	 { "gcc_blsp1_qup5_spi_apps_clk", &gcc.mux, 156 },
	 { "gcc_blsp1_qup5_i2c_apps_clk", &gcc.mux, 157 },
	 { "gcc_blsp1_qup6_spi_apps_clk", &gcc.mux, 161 },
	 { "gcc_blsp1_qup6_i2c_apps_clk", &gcc.mux, 162 },
	 { "gcc_camss_ahb_clk", &gcc.mux, 168 },
	 { "gcc_camss_top_ahb_clk", &gcc.mux, 169 },
	 { "gcc_camss_micro_ahb_clk", &gcc.mux, 170 },
	 { "gcc_camss_gp0_clk", &gcc.mux, 171 },
	 { "gcc_camss_gp1_clk", &gcc.mux, 172 },
	 { "gcc_camss_mclk0_clk", &gcc.mux, 173 },
	 { "gcc_camss_mclk1_clk", &gcc.mux, 174 },
	 { "gcc_camss_mclk2_clk", &gcc.mux, 445 },
	 { "gcc_camss_cci_clk", &gcc.mux, 175 },
	 { "gcc_camss_cci_ahb_clk", &gcc.mux, 176 },
	 { "gcc_camss_csi0phytimer_clk", &gcc.mux, 177 },
	 { "gcc_camss_csi1phytimer_clk", &gcc.mux, 178 },
	 { "gcc_camss_jpeg0_clk", &gcc.mux, 179 },
	 { "gcc_camss_jpeg_ahb_clk", &gcc.mux, 180 },
	 { "gcc_camss_jpeg_axi_clk", &gcc.mux, 181 },
	 { "gcc_camss_vfe0_clk", &gcc.mux, 184 },
	 { "gcc_camss_cpp_clk", &gcc.mux, 185 },
	 { "gcc_camss_cpp_ahb_clk", &gcc.mux, 186 },
	 { "gcc_camss_vfe_ahb_clk", &gcc.mux, 187 },
	 { "gcc_camss_vfe_axi_clk", &gcc.mux, 188 },
	 { "gcc_camss_csi_vfe0_clk", &gcc.mux, 191 },
	 { "gcc_camss_csi0_clk", &gcc.mux, 192 },
	 { "gcc_camss_csi0_ahb_clk", &gcc.mux, 193 },
	 { "gcc_camss_csi0phy_clk", &gcc.mux, 194 },
	 { "gcc_camss_csi0rdi_clk", &gcc.mux, 195 },
	 { "gcc_camss_csi0pix_clk", &gcc.mux, 196 },
	 { "gcc_camss_csi1_clk", &gcc.mux, 197 },
	 { "gcc_camss_csi1_ahb_clk", &gcc.mux, 198 },
	 { "gcc_camss_csi1phy_clk", &gcc.mux, 199 },
	 { "gcc_camss_csi2_clk", &gcc.mux, 227 },
	 { "gcc_camss_csi2_ahb_clk", &gcc.mux, 228 },
	 { "gcc_camss_csi2phy_clk", &gcc.mux, 229 },
	 { "gcc_camss_csi2rdi_clk", &gcc.mux, 230 },
	 { "gcc_camss_csi2pix_clk", &gcc.mux, 231 },
	 { "gcc_pdm_ahb_clk", &gcc.mux, 208 },
	 { "gcc_pdm2_clk", &gcc.mux, 210 },
	 { "gcc_prng_ahb_clk", &gcc.mux, 216 },
	 { "gcc_camss_csi1rdi_clk", &gcc.mux, 224 },
	 { "gcc_camss_csi1pix_clk", &gcc.mux, 225 },
	 { "gcc_camss_ispif_ahb_clk", &gcc.mux, 226 },
	 { "gcc_boot_rom_ahb_clk", &gcc.mux, 248 },
	 { "gcc_crypto_clk", &gcc.mux, 312 },
	 { "gcc_crypto_axi_clk", &gcc.mux, 313 },
	 { "gcc_crypto_ahb_clk", &gcc.mux, 314 },
	 { "gcc_oxili_timer_clk", &gcc.mux, 489 },
	 { "gcc_oxili_gfx3d_clk", &gcc.mux, 490 },
	 { "gcc_oxili_ahb_clk", &gcc.mux, 491 },
	 { "gcc_oxili_gmem_clk", &gcc.mux, 496 },
	 { "gcc_venus0_vcodec0_clk", &gcc.mux, 497 },
	 { "gcc_venus0_core0_vcodec0_clk", &gcc.mux, 440 },
	 { "gcc_venus0_core1_vcodec0_clk", &gcc.mux, 441 },
	 { "gcc_venus0_axi_clk", &gcc.mux, 498 },
	 { "gcc_venus0_ahb_clk", &gcc.mux, 499 },
	 { "gcc_mdss_ahb_clk", &gcc.mux, 502 },
	 { "gcc_mdss_axi_clk", &gcc.mux, 503 },
	 { "gcc_mdss_pclk0_clk", &gcc.mux, 504 },
	 { "gcc_mdss_pclk1_clk", &gcc.mux, 442 },
	 { "gcc_mdss_mdp_clk", &gcc.mux, 505 },
	 { "gcc_mdss_vsync_clk", &gcc.mux, 507 },
	 { "gcc_mdss_byte0_clk", &gcc.mux, 508 },
	 { "gcc_mdss_byte1_clk", &gcc.mux, 443 },
	 { "gcc_mdss_esc0_clk", &gcc.mux, 509 },
	 { "gcc_mdss_esc1_clk", &gcc.mux, 444 },
	 { "gcc_bimc_clk", &gcc.mux, 340 },
	 { "gcc_bimc_gpu_clk", &gcc.mux, 343 },
	 { "gcc_bimc_ddr_ch0_clk", &gcc.mux, 346 },
	 { "gcc_cpp_tbu_clk", &gcc.mux, 233 },
	 { "gcc_mdp_rt_tbu_clk", &gcc.mux, 238 },
	 { "wcnss_m_clk", &gcc.mux, 408 },
	{},
};

struct debugcc_platform msm8936_debugcc = {
	"msm8936",
	msm8936_clocks,
};
