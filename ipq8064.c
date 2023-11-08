// SPDX-License-Identifier: BSD-3-Clause

#include <sys/mman.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "debugcc.h"

#define GCC_PHYS		0x900000

#define PDM_CLK_NS_REG		0x2cc0
#define XO4_CLK_DIV_4		BIT(3) | BIT(4)
#define XO4_CLK_BRANCH_ENA	BIT(7)
#define CLK_ROOT_ENA		BIT(11)
#define CXO_SRC_BRANCH_ENA	BIT(13)

#define RINGOSC_NS_REG		0x2dc0
#define RO_CLK_BRANCH_ENA	BIT(9)
#define RO_ROOT_ENA		BIT(11)

#define RINGOSC_TCXO_CTL_REG	0x2dc4
#define RINGOSC_STATUS_REG	0x2dcc

#define CLK_TEST_REG		0x2fa0
#define RING_OSC_DBG_SEL	BIT(26) // Select cc_dbg_hs_clk instead of default value cc_ringosc_clk
#define TEST_BUS_ENA		BIT(23)
#define TEST_BUS_SEL_MASK	GENMASK(22, 19)
#define HS_DBG_CLK_BRANCH_ENA	BIT(17)
#define DBG_CLK_HS_SEL_MASK	GENMASK(16, 10)
#define DBG_CLK_HS_SEL_SHIFT	10
#define LS_DBG_CLK_BRANCH_ENA	BIT(8)
#define DBG_CLK_LS_SEL_MASK	GENMASK(7, 0)

#define APCS_GCC_PHYS		0x2011000

#define APCS_CLK_DIAG_REG	0x1c 
#define FAST_CLK_EN		BIT(7)
#define FAST_CLK_SEL_MASK	GENMASK(5, 3)
#define FAST_CLK_SEL_SHIFT	3
#define SLOW_CLK_SEL_MASK	GENMASK(2, 0)

static struct debug_mux ringosc_mux;

static struct gcc_mux gcc = {
	.mux = {
		.phys = GCC_PHYS,
		.size = 0x4000,

		.measure = measure_gcc,

		.enable_reg = CLK_TEST_REG,
		.enable_mask = RING_OSC_DBG_SEL,

		.parent = &ringosc_mux,
	},

	// PXO == CXO == 25MHz
	.xo_rate = (25 * 1000 * 1000) / 4,

	.xo_div4_reg = PDM_CLK_NS_REG,
	.xo_div4_val = XO4_CLK_DIV_4 | XO4_CLK_BRANCH_ENA | \
		       CLK_ROOT_ENA | CXO_SRC_BRANCH_ENA,

	.debug_ctl_reg = RINGOSC_TCXO_CTL_REG,
	.debug_status_reg = RINGOSC_STATUS_REG,
};

static struct debug_mux ringosc_mux = {
	.phys = GCC_PHYS,
	.size = 0x4000,

	.enable_reg = RINGOSC_NS_REG,
	.enable_mask = RO_CLK_BRANCH_ENA | RO_ROOT_ENA,
};

static struct debug_mux hs_mux = {
	.phys = GCC_PHYS,
	.size = 0x4000,
	.block_name = "hs",

	.measure = measure_leaf,
	.parent = &gcc.mux,

	.enable_reg = CLK_TEST_REG,
	.enable_mask = HS_DBG_CLK_BRANCH_ENA,

	.mux_reg = CLK_TEST_REG,
	.mux_mask = DBG_CLK_HS_SEL_MASK,
	.mux_shift = DBG_CLK_HS_SEL_SHIFT,
};

static struct debug_mux ls_mux = {
	.phys = GCC_PHYS,
	.size = 0x4000,
	.block_name = "ls",

	.measure = measure_leaf,
	.parent = &hs_mux,
	// cc_dbg_ls_out_clk
	.parent_mux_val = 0x43,

	.enable_reg = CLK_TEST_REG,
	.enable_mask = LS_DBG_CLK_BRANCH_ENA,

	.mux_reg = CLK_TEST_REG,
	.mux_mask = DBG_CLK_LS_SEL_MASK,
};

static struct debug_mux cpul2_mux = {
	.phys = APCS_GCC_PHYS,
	.size = 0x1000,
	.block_name = "cpul2",

	.measure = measure_leaf,
	.parent = &hs_mux,
	// sc_dbg_hs1_clk
	.parent_mux_val = 0x41,

	.enable_reg = APCS_CLK_DIAG_REG,
	.enable_mask = FAST_CLK_EN,

	.mux_reg = APCS_CLK_DIAG_REG,
	.mux_mask = FAST_CLK_SEL_MASK,
	.mux_shift = FAST_CLK_SEL_SHIFT,
};

static struct measure_clk ipq8064_clocks[] = {
	{ "sdc1_p_clk", &ls_mux, 0x12 },
	{ "sdc1_clk", &ls_mux, 0x13 },
	{ "sdc3_p_clk", &ls_mux, 0x16 },
	{ "sdc3_clk", &ls_mux, 0x17 },
	{ "gp0_clk", &ls_mux, 0x1F },
	{ "gp1_clk", &ls_mux, 0x20 },
	{ "gp2_clk", &ls_mux, 0x21 },
	{ "dfab_clk", &ls_mux, 0x25 },
	{ "dfab_a_clk", &ls_mux, 0x25 },
	{ "pmem_clk", &ls_mux, 0x26 },
	{ "dma_bam_p_clk", &ls_mux, 0x32 },
	{ "cfpb_clk", &ls_mux, 0x33 },
	{ "cfpb_a_clk", &ls_mux, 0x33 },
	{ "gsbi1_p_clk", &ls_mux, 0x3D },
	{ "gsbi1_uart_clk", &ls_mux, 0x3E },
	{ "gsbi1_qup_clk", &ls_mux, 0x3F },
	{ "gsbi2_p_clk", &ls_mux, 0x41 },
	{ "gsbi2_uart_clk", &ls_mux, 0x42 },
	{ "gsbi2_qup_clk", &ls_mux, 0x44 },
	{ "gsbi4_p_clk", &ls_mux, 0x49 },
	{ "gsbi4_uart_clk", &ls_mux, 0x4A },
	{ "gsbi5_p_clk", &ls_mux, 0x4D },
	{ "gsbi5_uart_clk", &ls_mux, 0x4E },
	{ "gsbi5_qup_clk", &ls_mux, 0x50 },
	{ "gsbi6_p_clk", &ls_mux, 0x51 },
	{ "gsbi6_uart_clk", &ls_mux, 0x52 },
	{ "gsbi6_qup_clk", &ls_mux, 0x54 },
	{ "gsbi7_p_clk", &ls_mux, 0x55 },
	{ "gsbi7_uart_clk", &ls_mux, 0x56 },
	{ "gsbi7_qup_clk", &ls_mux, 0x58 },
	{ "sfab_sata_s_p_clk", &ls_mux, 0x59 },
	{ "sata_p_clk", &ls_mux, 0x5A },
	{ "sata_rxoob_clk", &ls_mux, 0x5B },
	{ "sata_pmalive_clk", &ls_mux, 0x5C },
	{ "pcie_src_clk", &ls_mux, 0x5D },
	{ "pcie_p_clk", &ls_mux, 0x5E },
	{ "ce5_p_clk", &ls_mux, 0x5F },
	{ "ce5_core_clk", &ls_mux, 0x60 },
	{ "sata_phy_ref_clk", &ls_mux, 0x6B },
	{ "sata_phy_cfg_clk", &ls_mux, 0x6C },
	{ "sfpb_clk", &ls_mux, 0x78 },
	{ "sfpb_a_clk", &ls_mux, 0x78 },
	{ "pmic_ssbi2_clk", &ls_mux, 0x7A },
	{ "pmic_arb0_p_clk", &ls_mux, 0x7B },
	{ "pmic_arb1_p_clk", &ls_mux, 0x7C },
	{ "prng_clk", &ls_mux, 0x7D },
	{ "rpm_msg_ram_p_clk", &ls_mux, 0x7F },
	{ "adm0_p_clk", &ls_mux, 0x80 },
	{ "usb_hs1_p_clk", &ls_mux, 0x84 },
	{ "usb_hs1_xcvr_clk", &ls_mux, 0x85 },
	{ "usb_hsic_p_clk", &ls_mux, 0x86 },
	{ "usb_hsic_system_clk", &ls_mux, 0x87 },
	{ "usb_hsic_xcvr_fs_clk", &ls_mux, 0x88 },
	{ "usb_fs1_p_clk", &ls_mux, 0x89 },
	{ "usb_fs1_sys_clk", &ls_mux, 0x8A },
	{ "usb_fs1_xcvr_clk", &ls_mux, 0x8B },
	{ "tsif_p_clk", &ls_mux, 0x8F },
	{ "tsif_ref_clk", &ls_mux, 0x91 },
	{ "ce1_p_clk", &ls_mux, 0x92 },
	{ "tssc_clk", &ls_mux, 0x94 },
	{ "usb_hsic_hsio_cal_clk", &ls_mux, 0x9D },
	{ "ce1_core_clk", &ls_mux, 0xA4 },
	{ "pcie1_p_clk", &ls_mux, 0xB0 },
	{ "pcie1_src_clk", &ls_mux, 0xB1 },
	{ "pcie2_p_clk", &ls_mux, 0xB2 },
	{ "pcie2_src_clk", &ls_mux, 0xB3 },

	{ "afab_clk", &hs_mux, 0x07 },
	{ "afab_a_clk", &hs_mux, 0x07 },
	{ "sfab_clk", &hs_mux, 0x18 },
	{ "sfab_a_clk", &hs_mux, 0x18 },
	{ "adm0_clk", &hs_mux, 0x2A },
	{ "sata_a_clk", &hs_mux, 0x31 },
	{ "pcie_aux_clk", &hs_mux, 0x2B },
	{ "pcie_phy_ref_clk", &hs_mux, 0x2D },
	{ "pcie_a_clk", &hs_mux, 0x32 },
	{ "ebi1_clk", &hs_mux, 0x34 },
	{ "ebi1_a_clk", &hs_mux, 0x34 },
	{ "usb_hsic_hsic_clk", &hs_mux, 0x50 },
	{ "pcie1_aux_clk", &hs_mux, 0x55 },
	{ "pcie1_phy_ref_clk", &hs_mux, 0x56 },
	{ "pcie2_aux_clk", &hs_mux, 0x57 },
	{ "pcie2_phy_ref_clk", &hs_mux, 0x58 },
	{ "pcie1_a_clk", &hs_mux, 0x66 },
	{ "pcie2_a_clk", &hs_mux, 0x67 },

	{ "l2_m_clk", &cpul2_mux, 0x2, 8 },
	{ "krait0_m_clk", &cpul2_mux, 0x0, 8 },
	{ "krait1_m_clk", &cpul2_mux, 0x1, 8 },
	{}
};

struct debugcc_platform ipq8064_debugcc = {
	"ipq8064",
	ipq8064_clocks,
};
