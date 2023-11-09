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
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "debugcc.h"

#define GCC_BASE	0x01800000
#define GCC_SIZE	0x80000

#define GCC_DEBUG_CLK_CTL		0x74000
#define GCC_CLOCK_FREQ_MEASURE_CTL	0x74004
#define GCC_CLOCK_FREQ_MEASURE_STATUS	0x74008
#define GCC_XO_DIV4_CBCR		0x30034

struct turing_mux {
	struct debug_mux mux;

	unsigned int ahb_reg;
	unsigned int ahb_mask;
};

unsigned long measure_turing(const struct measure_clk *clk,
			     const struct debug_mux *mux);

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

static struct turing_mux turing_cc = {
	.mux = {
		.phys = 0x800000,
		.size = 0x30000,

		.measure = measure_turing,
		.parent = &gcc.mux,
		.parent_mux_val = 50,

		.enable_reg = 0x22008,
		.enable_mask = BIT(0),

		.mux_reg = 0x22000,
		.mux_mask = 0xffff,
	},

	.ahb_reg = 0x5e004,
	.ahb_mask = BIT(31),
};

static bool leaf_enabled(struct turing_mux *leaf)
{
	uint32_t val;

	/* If no AHB clock is specified, we assume it's clocked */
	if (!leaf || !leaf->ahb_mask)
		return true;

	/* we know that the parent is GCC, we read AHB reg from GCC */
	val = readl(leaf->mux.parent->base + leaf->ahb_reg);
	val &= leaf->ahb_mask;

	/* CLK_OFF will be set if block is not clocked, so inverse */
	return !val;
}

unsigned long measure_turing(const struct measure_clk *clk,
			     const struct debug_mux *mux)
{
	struct turing_mux *turing = container_of(mux, struct turing_mux, mux);

	if (!leaf_enabled(turing))
		return 0;

	return measure_leaf(clk, mux);
}

static struct measure_clk qcs404_clocks[] = {
	{ .name = "snoc_clk", .clk_mux = &gcc.mux, .mux = 0 },
	{ .name = "gcc_sys_noc_usb3_clk", .clk_mux = &gcc.mux, .mux = 1 },
	{ .name = "pnoc_clk", .clk_mux = &gcc.mux, .mux = 8 },
	{ .name = "gcc_pcnoc_usb2_clk", .clk_mux = &gcc.mux, .mux = 9 },
	{ .name = "gcc_pcnoc_usb3_clk", .clk_mux = &gcc.mux, .mux = 10 },
	{ .name = "gcc_gp1_clk", .clk_mux = &gcc.mux, .mux = 16 },
	{ .name = "gcc_gp2_clk", .clk_mux = &gcc.mux, .mux = 17 },
	{ .name = "gcc_gp3_clk", .clk_mux = &gcc.mux, .mux = 18 },
	{ .name = "gcc_bimc_gfx_clk", .clk_mux = &gcc.mux, .mux = 45 },
	{ .name = "aon_clk_src", .clk_mux = &turing_cc.mux, .mux = 1},
	{ .name = "turing_wrapper_aon_clk", .clk_mux = &turing_cc.mux, .mux = 2},
	{ .name = "turing_wrapper_cnoc_sway_aon_clk", .clk_mux = &turing_cc.mux, .mux = 3},
	{ .name = "turing_wrapper_qos_ahbs_aon_clk", .clk_mux = &turing_cc.mux, .mux = 4},
	{ .name = "q6ss_ahbm_aon_clk", .clk_mux = &turing_cc.mux, .mux = 5},
	{ .name = "q6ss_ahbs_aon_clk", .clk_mux = &turing_cc.mux, .mux = 6},
	{ .name = "turing_wrapper_bus_timeout_aon_clk", .clk_mux = &turing_cc.mux, .mux = 7},
	{ .name = "turing_wrapper_rscc_aon_clk", .clk_mux = &turing_cc.mux, .mux = 8},
	{ .name = "q6ss_alt_reset_aon_clk", .clk_mux = &turing_cc.mux, .mux = 10},
	{ .name = "qos_fixed_lat_counter_clk_src", .clk_mux = &turing_cc.mux, .mux = 11},
	{ .name = "turing_wrapper_qos_dmonitor_fixed_lat_counter_clk", .clk_mux = &turing_cc.mux, .mux = 12},
	{ .name = "turing_wrapper_qos_danger_fixed_lat_counter_clk", .clk_mux = &turing_cc.mux, .mux = 13},
	{ .name = "q6_xo_clk_src", .clk_mux = &turing_cc.mux, .mux = 14},
	{ .name = "qos_xo_clk_src", .clk_mux = &turing_cc.mux, .mux = 15},
	{ .name = "turing_wrapper_qos_xo_lat_counter_clk", .clk_mux = &turing_cc.mux, .mux = 16},
	{ .name = "bcr_slp_clk_src", .clk_mux = &turing_cc.mux, .mux = 19},
	{ .name = "q6ss_bcr_slp_clk", .clk_mux = &turing_cc.mux, .mux = 20},
	{ .name = "turing_wrapper_cnoc_ahbs_clk", .clk_mux = &turing_cc.mux, .mux = 28},
	{ .name = "q6ss_q6_axim_clk", .clk_mux = &turing_cc.mux, .mux = 29},
	{ .name = "q6ss_sleep_clk_src", .clk_mux = &turing_cc.mux, .mux = 33},
	{ .name = "qdsp6ss_xo_clk", .clk_mux = &turing_cc.mux, .mux = 36},
	{ .name = "qdsp6ss_sleep_clk", .clk_mux = &turing_cc.mux, .mux = 37},
	{ .name = "q6ss_dbg_in_clk", .clk_mux = &turing_cc.mux, .mux = 39},
	{ .name = "gcc_usb_hs_system_clk", .clk_mux = &gcc.mux, .mux = 96 },
	{ .name = "gcc_usb_hs_inactivity_timers_clk", .clk_mux = &gcc.mux, .mux = 98 },
	{ .name = "gcc_usb2a_phy_sleep_clk", .clk_mux = &gcc.mux, .mux = 99 },
	{ .name = "gcc_usb_hs_phy_cfg_ahb_clk", .clk_mux = &gcc.mux, .mux = 100 },
	{ .name = "gcc_usb20_mock_utmi_clk", .clk_mux = &gcc.mux, .mux = 101 },
	{ .name = "gcc_sdcc1_apps_clk", .clk_mux = &gcc.mux, .mux = 104 },
	{ .name = "gcc_sdcc1_ahb_clk", .clk_mux = &gcc.mux, .mux = 105 },
	{ .name = "gcc_sdcc1_ice_core_clk", .clk_mux = &gcc.mux, .mux = 106 },
	{ .name = "gcc_sdcc2_apps_clk", .clk_mux = &gcc.mux, .mux = 112 },
	{ .name = "gcc_sdcc2_ahb_clk", .clk_mux = &gcc.mux, .mux = 113 },
	{ .name = "gcc_usb30_master_clk", .clk_mux = &gcc.mux, .mux = 120 },
	{ .name = "gcc_usb30_sleep_clk", .clk_mux = &gcc.mux, .mux = 121 },
	{ .name = "gcc_usb30_mock_utmi_clk", .clk_mux = &gcc.mux, .mux = 122 },
	{ .name = "gcc_usb3_phy_pipe_clk", .clk_mux = &gcc.mux, .mux = 123 },
	{ .name = "gcc_usb3_phy_aux_clk", .clk_mux = &gcc.mux, .mux = 124 },
	{ .name = "gcc_eth_axi_clk", .clk_mux = &gcc.mux, .mux = 128 },
	{ .name = "gcc_eth_rgmii_clk", .clk_mux = &gcc.mux, .mux = 129 },
	{ .name = "gcc_eth_slave_ahb_clk", .clk_mux = &gcc.mux, .mux = 130 },
	{ .name = "gcc_eth_ptp_clk", .clk_mux = &gcc.mux, .mux = 131 },
	{ .name = "gcc_blsp1_ahb_clk", .clk_mux = &gcc.mux, .mux = 136 },
	{ .name = "gcc_blsp1_qup1_spi_apps_clk", .clk_mux = &gcc.mux, .mux = 138 },
	{ .name = "wcnss_m_clk", .clk_mux = &gcc.mux, .mux = 138 },
	{ .name = "gcc_blsp1_qup1_i2c_apps_clk", .clk_mux = &gcc.mux, .mux = 139 },
	{ .name = "gcc_blsp1_uart1_apps_clk", .clk_mux = &gcc.mux, .mux = 140 },
	{ .name = "gcc_blsp1_qup2_spi_apps_clk", .clk_mux = &gcc.mux, .mux = 142 },
	{ .name = "gcc_blsp1_qup2_i2c_apps_clk", .clk_mux = &gcc.mux, .mux = 143 },
	{ .name = "gcc_blsp1_uart2_apps_clk", .clk_mux = &gcc.mux, .mux = 144 },
	{ .name = "gcc_blsp1_qup3_spi_apps_clk", .clk_mux = &gcc.mux, .mux = 146 },
	{ .name = "gcc_blsp1_qup3_i2c_apps_clk", .clk_mux = &gcc.mux, .mux = 147 },
	{ .name = "gcc_blsp1_qup4_spi_apps_clk", .clk_mux = &gcc.mux, .mux = 148 },
	{ .name = "gcc_blsp1_qup4_i2c_apps_clk", .clk_mux = &gcc.mux, .mux = 149 },
	{ .name = "gcc_blsp1_uart3_apps_clk", .clk_mux = &gcc.mux, .mux = 150 },
	{ .name = "gcc_blsp1_qup0_spi_apps_clk", .clk_mux = &gcc.mux, .mux = 152 },
	{ .name = "gcc_blsp1_qup0_i2c_apps_clk", .clk_mux = &gcc.mux, .mux = 153 },
	{ .name = "gcc_blsp1_uart0_apps_clk", .clk_mux = &gcc.mux, .mux = 154 },
	{ .name = "gcc_blsp2_ahb_clk", .clk_mux = &gcc.mux, .mux = 160 },
	{ .name = "gcc_blsp2_qup0_i2c_apps_clk", .clk_mux = &gcc.mux, .mux = 162 },
	{ .name = "gcc_blsp2_qup0_spi_apps_clk", .clk_mux = &gcc.mux, .mux = 163 },
	{ .name = "gcc_blsp2_uart0_apps_clk", .clk_mux = &gcc.mux, .mux = 164 },
	{ .name = "gcc_pcie_0_slv_axi_clk", .clk_mux = &gcc.mux, .mux = 168 },
	{ .name = "gcc_pcie_0_mstr_axi_clk", .clk_mux = &gcc.mux, .mux = 169 },
	{ .name = "gcc_pcie_0_cfg_ahb_clk", .clk_mux = &gcc.mux, .mux = 170 },
	{ .name = "gcc_pcie_0_aux_clk", .clk_mux = &gcc.mux, .mux = 171 },
	{ .name = "gcc_pcie_0_pipe_clk", .clk_mux = &gcc.mux, .mux = 172 },
	//{ "pcie0_pipe_clk", &gcc.mux, 173, 1 },
	{ .name = "qpic_clk", .clk_mux = &gcc.mux, .mux = 192 },
	{ .name = "gcc_pdm_ahb_clk", .clk_mux = &gcc.mux, .mux = 208 },
	{ .name = "gcc_pdm2_clk", .clk_mux = &gcc.mux, .mux = 210 },
	{ .name = "gcc_pwm0_xo512_clk", .clk_mux = &gcc.mux, .mux = 211 },
	{ .name = "gcc_pwm1_xo512_clk", .clk_mux = &gcc.mux, .mux = 212 },
	{ .name = "gcc_pwm2_xo512_clk", .clk_mux = &gcc.mux, .mux = 213 },
	{ .name = "gcc_prng_ahb_clk", .clk_mux = &gcc.mux, .mux = 216 },
	{ .name = "gcc_geni_ir_s_clk", .clk_mux = &gcc.mux, .mux = 238 },
	{ .name = "gcc_boot_rom_ahb_clk", .clk_mux = &gcc.mux, .mux = 248 },
	{ .name = "ce1_clk", .clk_mux = &gcc.mux, .mux = 312 },
	{ .name = "bimc_clk", .clk_mux = &gcc.mux, .mux = 346 },
	//{ "bimc_fsm_ddr_clk", &gcc.mux, 350, 1 },
	{ .name = "gcc_apss_ahb_clk", .clk_mux = &gcc.mux, .mux = 360 },
	{ .name = "gcc_dcc_clk", .clk_mux = &gcc.mux, .mux = 441 },
	{ .name = "gcc_oxili_gfx3d_clk", .clk_mux = &gcc.mux, .mux = 490 },
	{ .name = "gcc_oxili_ahb_clk", .clk_mux = &gcc.mux, .mux = 491 },
	{ .name = "gcc_mdss_hdmi_pclk_clk", .clk_mux = &gcc.mux, .mux = 497 },
	{ .name = "gcc_mdss_hdmi_app_clk", .clk_mux = &gcc.mux, .mux = 498 },
	{ .name = "gcc_mdss_ahb_clk", .clk_mux = &gcc.mux, .mux = 502 },
	{ .name = "gcc_mdss_axi_clk", .clk_mux = &gcc.mux, .mux = 503 },
	{ .name = "gcc_mdss_pclk0_clk", .clk_mux = &gcc.mux, .mux = 504 },
	{ .name = "gcc_mdss_mdp_clk", .clk_mux = &gcc.mux, .mux = 505 },
	{ .name = "gcc_mdss_vsync_clk", .clk_mux = &gcc.mux, .mux = 507 },
	{ .name = "gcc_mdss_byte0_clk", .clk_mux = &gcc.mux, .mux = 508 },
	{ .name = "gcc_mdss_esc0_clk", .clk_mux = &gcc.mux, .mux = 509 },
	{}
};

struct debugcc_platform qcs404_debugcc = {
	.name = "qcs404",
	.clocks = qcs404_clocks,
};
