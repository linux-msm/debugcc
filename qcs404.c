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
	{ "snoc_clk", &gcc.mux, 0 },
	{ "gcc_sys_noc_usb3_clk", &gcc.mux, 1 },
	{ "pnoc_clk", &gcc.mux, 8 },
	{ "gcc_pcnoc_usb2_clk", &gcc.mux, 9 },
	{ "gcc_pcnoc_usb3_clk", &gcc.mux, 10 },
	{ "gcc_gp1_clk", &gcc.mux, 16 },
	{ "gcc_gp2_clk", &gcc.mux, 17 },
	{ "gcc_gp3_clk", &gcc.mux, 18 },
	{ "gcc_bimc_gfx_clk", &gcc.mux, 45 },
	{ "aon_clk_src", &turing_cc.mux, 1},
	{ "turing_wrapper_aon_clk", &turing_cc.mux, 2},
	{ "turing_wrapper_cnoc_sway_aon_clk", &turing_cc.mux, 3},
	{ "turing_wrapper_qos_ahbs_aon_clk", &turing_cc.mux, 4},
	{ "q6ss_ahbm_aon_clk", &turing_cc.mux, 5},
	{ "q6ss_ahbs_aon_clk", &turing_cc.mux, 6},
	{ "turing_wrapper_bus_timeout_aon_clk", &turing_cc.mux, 7},
	{ "turing_wrapper_rscc_aon_clk", &turing_cc.mux, 8},
	{ "q6ss_alt_reset_aon_clk", &turing_cc.mux, 10},
	{ "qos_fixed_lat_counter_clk_src", &turing_cc.mux, 11},
	{ "turing_wrapper_qos_dmonitor_fixed_lat_counter_clk", &turing_cc.mux, 12},
	{ "turing_wrapper_qos_danger_fixed_lat_counter_clk", &turing_cc.mux, 13},
	{ "q6_xo_clk_src", &turing_cc.mux, 14},
	{ "qos_xo_clk_src", &turing_cc.mux, 15},
	{ "turing_wrapper_qos_xo_lat_counter_clk", &turing_cc.mux, 16},
	{ "bcr_slp_clk_src", &turing_cc.mux, 19},
	{ "q6ss_bcr_slp_clk", &turing_cc.mux, 20},
	{ "turing_wrapper_cnoc_ahbs_clk", &turing_cc.mux, 28},
	{ "q6ss_q6_axim_clk", &turing_cc.mux, 29},
	{ "q6ss_sleep_clk_src", &turing_cc.mux, 33},
	{ "qdsp6ss_xo_clk", &turing_cc.mux, 36},
	{ "qdsp6ss_sleep_clk", &turing_cc.mux, 37},
	{ "q6ss_dbg_in_clk", &turing_cc.mux, 39},
	{ "gcc_usb_hs_system_clk", &gcc.mux, 96 },
	{ "gcc_usb_hs_inactivity_timers_clk", &gcc.mux, 98 },
	{ "gcc_usb2a_phy_sleep_clk", &gcc.mux, 99 },
	{ "gcc_usb_hs_phy_cfg_ahb_clk", &gcc.mux, 100 },
	{ "gcc_usb20_mock_utmi_clk", &gcc.mux, 101 },
	{ "gcc_sdcc1_apps_clk", &gcc.mux, 104 },
	{ "gcc_sdcc1_ahb_clk", &gcc.mux, 105 },
	{ "gcc_sdcc1_ice_core_clk", &gcc.mux, 106 },
	{ "gcc_sdcc2_apps_clk", &gcc.mux, 112 },
	{ "gcc_sdcc2_ahb_clk", &gcc.mux, 113 },
	{ "gcc_usb30_master_clk", &gcc.mux, 120 },
	{ "gcc_usb30_sleep_clk", &gcc.mux, 121 },
	{ "gcc_usb30_mock_utmi_clk", &gcc.mux, 122 },
	{ "gcc_usb3_phy_pipe_clk", &gcc.mux, 123 },
	{ "gcc_usb3_phy_aux_clk", &gcc.mux, 124 },
	{ "gcc_eth_axi_clk", &gcc.mux, 128 },
	{ "gcc_eth_rgmii_clk", &gcc.mux, 129 },
	{ "gcc_eth_slave_ahb_clk", &gcc.mux, 130 },
	{ "gcc_eth_ptp_clk", &gcc.mux, 131 },
	{ "gcc_blsp1_ahb_clk", &gcc.mux, 136 },
	{ "gcc_blsp1_qup1_spi_apps_clk", &gcc.mux, 138 },
	{ "wcnss_m_clk", &gcc.mux, 138 },
	{ "gcc_blsp1_qup1_i2c_apps_clk", &gcc.mux, 139 },
	{ "gcc_blsp1_uart1_apps_clk", &gcc.mux, 140 },
	{ "gcc_blsp1_qup2_spi_apps_clk", &gcc.mux, 142 },
	{ "gcc_blsp1_qup2_i2c_apps_clk", &gcc.mux, 143 },
	{ "gcc_blsp1_uart2_apps_clk", &gcc.mux, 144 },
	{ "gcc_blsp1_qup3_spi_apps_clk", &gcc.mux, 146 },
	{ "gcc_blsp1_qup3_i2c_apps_clk", &gcc.mux, 147 },
	{ "gcc_blsp1_qup4_spi_apps_clk", &gcc.mux, 148 },
	{ "gcc_blsp1_qup4_i2c_apps_clk", &gcc.mux, 149 },
	{ "gcc_blsp1_uart3_apps_clk", &gcc.mux, 150 },
	{ "gcc_blsp1_qup0_spi_apps_clk", &gcc.mux, 152 },
	{ "gcc_blsp1_qup0_i2c_apps_clk", &gcc.mux, 153 },
	{ "gcc_blsp1_uart0_apps_clk", &gcc.mux, 154 },
	{ "gcc_blsp2_ahb_clk", &gcc.mux, 160 },
	{ "gcc_blsp2_qup0_i2c_apps_clk", &gcc.mux, 162 },
	{ "gcc_blsp2_qup0_spi_apps_clk", &gcc.mux, 163 },
	{ "gcc_blsp2_uart0_apps_clk", &gcc.mux, 164 },
	{ "gcc_pcie_0_slv_axi_clk", &gcc.mux, 168 },
	{ "gcc_pcie_0_mstr_axi_clk", &gcc.mux, 169 },
	{ "gcc_pcie_0_cfg_ahb_clk", &gcc.mux, 170 },
	{ "gcc_pcie_0_aux_clk", &gcc.mux, 171 },
	{ "gcc_pcie_0_pipe_clk", &gcc.mux, 172 },
	//{ "pcie0_pipe_clk", &gcc.mux, 173, 1 },
	{ "qpic_clk", &gcc.mux, 192 },
	{ "gcc_pdm_ahb_clk", &gcc.mux, 208 },
	{ "gcc_pdm2_clk", &gcc.mux, 210 },
	{ "gcc_pwm0_xo512_clk", &gcc.mux, 211 },
	{ "gcc_pwm1_xo512_clk", &gcc.mux, 212 },
	{ "gcc_pwm2_xo512_clk", &gcc.mux, 213 },
	{ "gcc_prng_ahb_clk", &gcc.mux, 216 },
	{ "gcc_geni_ir_s_clk", &gcc.mux, 238 },
	{ "gcc_boot_rom_ahb_clk", &gcc.mux, 248 },
	{ "ce1_clk", &gcc.mux, 312 },
	{ "bimc_clk", &gcc.mux, 346 },
	//{ "bimc_fsm_ddr_clk", &gcc.mux, 350, 1 },
	{ "gcc_apss_ahb_clk", &gcc.mux, 360 },
	{ "gcc_dcc_clk", &gcc.mux, 441 },
	{ "gcc_oxili_gfx3d_clk", &gcc.mux, 490 },
	{ "gcc_oxili_ahb_clk", &gcc.mux, 491 },
	{ "gcc_mdss_hdmi_pclk_clk", &gcc.mux, 497 },
	{ "gcc_mdss_hdmi_app_clk", &gcc.mux, 498 },
	{ "gcc_mdss_ahb_clk", &gcc.mux, 502 },
	{ "gcc_mdss_axi_clk", &gcc.mux, 503 },
	{ "gcc_mdss_pclk0_clk", &gcc.mux, 504 },
	{ "gcc_mdss_mdp_clk", &gcc.mux, 505 },
	{ "gcc_mdss_vsync_clk", &gcc.mux, 507 },
	{ "gcc_mdss_byte0_clk", &gcc.mux, 508 },
	{ "gcc_mdss_esc0_clk", &gcc.mux, 509 },
	{}
};

struct debugcc_platform qcs404_debugcc = {
	"qcs404",
	qcs404_clocks,
};
