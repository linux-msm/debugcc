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

	.xo_div4_reg = GCC_XO_DIV4_CBCR,
	.debug_ctl_reg = GCC_CLOCK_FREQ_MEASURE_CTL,
	.debug_status_reg = GCC_CLOCK_FREQ_MEASURE_STATUS,
};

static struct debug_mux turing = {
	.phys = 0x800000,
	.size = 0x30000,

	.enable_reg = 0x22008,
	.enable_mask = BIT(0),

	.mux_reg = 0x22000,
	.mux_mask = 0xffff,

	.div_mask = 0,

	.ahb_reg = 0x5e004,
	.ahb_mask = BIT(31),
};

static struct measure_clk qcs404_clocks[] = {
	{ "snoc_clk", &gcc, 0, 4 },
	{ "gcc_sys_noc_usb3_clk", &gcc, 1, 4 },
	{ "pnoc_clk", &gcc, 8, 4 },
	{ "gcc_pcnoc_usb2_clk", &gcc, 9, 4 },
	{ "gcc_pcnoc_usb3_clk", &gcc, 10, 4 },
	{ "gcc_gp1_clk", &gcc, 16, 4 },
	{ "gcc_gp2_clk", &gcc, 17, 4 },
	{ "gcc_gp3_clk", &gcc, 18, 4 },
	{ "gcc_bimc_gfx_clk", &gcc, 45, 4 },
	{ "aon_clk_src", &gcc, 50, 1, &turing, 1, 4 },
	{ "turing_wrapper_aon_clk", &gcc, 50, 1, &turing, 2, 4 },
	{ "turing_wrapper_cnoc_sway_aon_clk", &gcc, 50, 1, &turing, 3, 4 },
	{ "turing_wrapper_qos_ahbs_aon_clk", &gcc, 50, 1, &turing, 4, 4 },
	{ "q6ss_ahbm_aon_clk", &gcc, 50, 1, &turing, 5, 4 },
	{ "q6ss_ahbs_aon_clk", &gcc, 50, 1, &turing, 6, 4},
	{ "turing_wrapper_bus_timeout_aon_clk", &gcc, 50, 1, &turing, 7, 4 },
	{ "turing_wrapper_rscc_aon_clk", &gcc, 50, 1, &turing, 8, 4 },
	{ "q6ss_alt_reset_aon_clk", &gcc, 50, 1, &turing, 10, 4 },
	{ "qos_fixed_lat_counter_clk_src", &gcc, 50, 1, &turing, 11, 4 },
	{ "turing_wrapper_qos_dmonitor_fixed_lat_counter_clk", &gcc, 50, 1, &turing, 12, 4 },
	{ "turing_wrapper_qos_danger_fixed_lat_counter_clk", &gcc, 50, 1, &turing, 13, 4 },
	{ "q6_xo_clk_src", &gcc, 50, 1, &turing, 14, 4 },
	{ "qos_xo_clk_src", &gcc, 50, 1, &turing, 15, 4 },
	{ "turing_wrapper_qos_xo_lat_counter_clk", &gcc, 50, 1, &turing, 16, 4 },
	{ "bcr_slp_clk_src", &gcc, 50, 1, &turing, 19, 4 },
	{ "q6ss_bcr_slp_clk", &gcc, 50, 1, &turing, 20, 4 },
	{ "turing_wrapper_cnoc_ahbs_clk", &gcc, 50, 1, &turing, 28, 4 },
	{ "q6ss_q6_axim_clk", &gcc, 50, 1, &turing, 29, 4 },
	{ "q6ss_sleep_clk_src", &gcc, 50, 1, &turing, 33, 4 },
	{ "qdsp6ss_xo_clk", &gcc, 50, 1, &turing, 36, 4 },
	{ "qdsp6ss_sleep_clk", &gcc, 50, 1, &turing, 37, 4 },
	{ "q6ss_dbg_in_clk", &gcc, 50, 1, &turing, 39, 4 },
	{ "gcc_usb_hs_system_clk", &gcc, 96, 4 },
	{ "gcc_usb_hs_inactivity_timers_clk", &gcc, 98, 4 },
	{ "gcc_usb2a_phy_sleep_clk", &gcc, 99, 4 },
	{ "gcc_usb_hs_phy_cfg_ahb_clk", &gcc, 100, 4 },
	{ "gcc_usb20_mock_utmi_clk", &gcc, 101, 4 },
	{ "gcc_sdcc1_apps_clk", &gcc, 104, 4 },
	{ "gcc_sdcc1_ahb_clk", &gcc, 105, 4 },
	{ "gcc_sdcc1_ice_core_clk", &gcc, 106, 4 },
	{ "gcc_sdcc2_apps_clk", &gcc, 112, 4 },
	{ "gcc_sdcc2_ahb_clk", &gcc, 113, 4 },
	{ "gcc_usb30_master_clk", &gcc, 120, 4 },
	{ "gcc_usb30_sleep_clk", &gcc, 121, 4 },
	{ "gcc_usb30_mock_utmi_clk", &gcc, 122, 4 },
	{ "gcc_usb3_phy_pipe_clk", &gcc, 123, 4 },
	{ "gcc_usb3_phy_aux_clk", &gcc, 124, 4 },
	{ "gcc_eth_axi_clk", &gcc, 128, 4 },
	{ "gcc_eth_rgmii_clk", &gcc, 129, 4 },
	{ "gcc_eth_slave_ahb_clk", &gcc, 130, 4 },
	{ "gcc_eth_ptp_clk", &gcc, 131, 4 },
	{ "gcc_blsp1_ahb_clk", &gcc, 136, 4 },
	{ "gcc_blsp1_qup1_spi_apps_clk", &gcc, 138, 4 },
	{ "wcnss_m_clk", &gcc, 138, 4 },
	{ "gcc_blsp1_qup1_i2c_apps_clk", &gcc, 139, 4 },
	{ "gcc_blsp1_uart1_apps_clk", &gcc, 140, 4 },
	{ "gcc_blsp1_qup2_spi_apps_clk", &gcc, 142, 4 },
	{ "gcc_blsp1_qup2_i2c_apps_clk", &gcc, 143, 4 },
	{ "gcc_blsp1_uart2_apps_clk", &gcc, 144, 4 },
	{ "gcc_blsp1_qup3_spi_apps_clk", &gcc, 146, 4 },
	{ "gcc_blsp1_qup3_i2c_apps_clk", &gcc, 147, 4 },
	{ "gcc_blsp1_qup4_spi_apps_clk", &gcc, 148, 4 },
	{ "gcc_blsp1_qup4_i2c_apps_clk", &gcc, 149, 4 },
	{ "gcc_blsp1_uart3_apps_clk", &gcc, 150, 4 },
	{ "gcc_blsp1_qup0_spi_apps_clk", &gcc, 152, 4 },
	{ "gcc_blsp1_qup0_i2c_apps_clk", &gcc, 153, 4 },
	{ "gcc_blsp1_uart0_apps_clk", &gcc, 154, 4 },
	{ "gcc_blsp2_ahb_clk", &gcc, 160, 4 },
	{ "gcc_blsp2_qup0_i2c_apps_clk", &gcc, 162, 4 },
	{ "gcc_blsp2_qup0_spi_apps_clk", &gcc, 163, 4 },
	{ "gcc_blsp2_uart0_apps_clk", &gcc, 164, 4 },
	{ "gcc_pcie_0_slv_axi_clk", &gcc, 168, 4 },
	{ "gcc_pcie_0_mstr_axi_clk", &gcc, 169, 4 },
	{ "gcc_pcie_0_cfg_ahb_clk", &gcc, 170, 4 },
	{ "gcc_pcie_0_aux_clk", &gcc, 171, 4 },
	{ "gcc_pcie_0_pipe_clk", &gcc, 172, 4 },
	{ "pcie0_pipe_clk", &gcc, 173, 1 },
	{ "qpic_clk", &gcc, 192, 4 },
	{ "gcc_pdm_ahb_clk", &gcc, 208, 4 },
	{ "gcc_pdm2_clk", &gcc, 210, 4 },
	{ "gcc_pwm0_xo512_clk", &gcc, 211, 4 },
	{ "gcc_pwm1_xo512_clk", &gcc, 212, 4 },
	{ "gcc_pwm2_xo512_clk", &gcc, 213, 4 },
	{ "gcc_prng_ahb_clk", &gcc, 216, 4 },
	{ "gcc_geni_ir_s_clk", &gcc, 238, 4 },
	{ "gcc_boot_rom_ahb_clk", &gcc, 248, 4 },
	{ "ce1_clk", &gcc, 312, 4 },
	{ "bimc_clk", &gcc, 346, 4 },
	{ "bimc_fsm_ddr_clk", &gcc, 350, 1 },
	{ "gcc_apss_ahb_clk", &gcc, 360, 4 },
	{ "gcc_dcc_clk", &gcc, 441, 4 },
	{ "gcc_oxili_gfx3d_clk", &gcc, 490, 4 },
	{ "gcc_oxili_ahb_clk", &gcc, 491, 4 },
	{ "gcc_mdss_hdmi_pclk_clk", &gcc, 497, 4 },
	{ "gcc_mdss_hdmi_app_clk", &gcc, 498, 4 },
	{ "gcc_mdss_ahb_clk", &gcc, 502, 4 },
	{ "gcc_mdss_axi_clk", &gcc, 503, 4 },
	{ "gcc_mdss_pclk0_clk", &gcc, 504, 4 },
	{ "gcc_mdss_mdp_clk", &gcc, 505, 4 },
	{ "gcc_mdss_vsync_clk", &gcc, 507, 4 },
	{ "gcc_mdss_byte0_clk", &gcc, 508, 4 },
	{ "gcc_mdss_esc0_clk", &gcc, 509, 4 },
	{}
};

struct debugcc_platform qcs404_debugcc = {
	"qcs404",
	qcs404_clocks,
};
