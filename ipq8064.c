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
	{ .name = "sdc1_p_clk", .clk_mux = &ls_mux, .mux = 0x12 },
	{ .name = "sdc1_clk", .clk_mux = &ls_mux, .mux = 0x13 },
	{ .name = "sdc3_p_clk", .clk_mux = &ls_mux, .mux = 0x16 },
	{ .name = "sdc3_clk", .clk_mux = &ls_mux, .mux = 0x17 },
	{ .name = "gp0_clk", .clk_mux = &ls_mux, .mux = 0x1F },
	{ .name = "gp1_clk", .clk_mux = &ls_mux, .mux = 0x20 },
	{ .name = "gp2_clk", .clk_mux = &ls_mux, .mux = 0x21 },
	{ .name = "dfab_clk", .clk_mux = &ls_mux, .mux = 0x25 },
	{ .name = "dfab_a_clk", .clk_mux = &ls_mux, .mux = 0x25 },
	{ .name = "pmem_clk", .clk_mux = &ls_mux, .mux = 0x26 },
	{ .name = "dma_bam_p_clk", .clk_mux = &ls_mux, .mux = 0x32 },
	{ .name = "cfpb_clk", .clk_mux = &ls_mux, .mux = 0x33 },
	{ .name = "cfpb_a_clk", .clk_mux = &ls_mux, .mux = 0x33 },
	{ .name = "gsbi1_p_clk", .clk_mux = &ls_mux, .mux = 0x3D },
	{ .name = "gsbi1_uart_clk", .clk_mux = &ls_mux, .mux = 0x3E },
	{ .name = "gsbi1_qup_clk", .clk_mux = &ls_mux, .mux = 0x3F },
	{ .name = "gsbi2_p_clk", .clk_mux = &ls_mux, .mux = 0x41 },
	{ .name = "gsbi2_uart_clk", .clk_mux = &ls_mux, .mux = 0x42 },
	{ .name = "gsbi2_qup_clk", .clk_mux = &ls_mux, .mux = 0x44 },
	{ .name = "gsbi4_p_clk", .clk_mux = &ls_mux, .mux = 0x49 },
	{ .name = "gsbi4_uart_clk", .clk_mux = &ls_mux, .mux = 0x4A },
	{ .name = "gsbi5_p_clk", .clk_mux = &ls_mux, .mux = 0x4D },
	{ .name = "gsbi5_uart_clk", .clk_mux = &ls_mux, .mux = 0x4E },
	{ .name = "gsbi5_qup_clk", .clk_mux = &ls_mux, .mux = 0x50 },
	{ .name = "gsbi6_p_clk", .clk_mux = &ls_mux, .mux = 0x51 },
	{ .name = "gsbi6_uart_clk", .clk_mux = &ls_mux, .mux = 0x52 },
	{ .name = "gsbi6_qup_clk", .clk_mux = &ls_mux, .mux = 0x54 },
	{ .name = "gsbi7_p_clk", .clk_mux = &ls_mux, .mux = 0x55 },
	{ .name = "gsbi7_uart_clk", .clk_mux = &ls_mux, .mux = 0x56 },
	{ .name = "gsbi7_qup_clk", .clk_mux = &ls_mux, .mux = 0x58 },
	{ .name = "sfab_sata_s_p_clk", .clk_mux = &ls_mux, .mux = 0x59 },
	{ .name = "sata_p_clk", .clk_mux = &ls_mux, .mux = 0x5A },
	{ .name = "sata_rxoob_clk", .clk_mux = &ls_mux, .mux = 0x5B },
	{ .name = "sata_pmalive_clk", .clk_mux = &ls_mux, .mux = 0x5C },
	{ .name = "pcie_src_clk", .clk_mux = &ls_mux, .mux = 0x5D },
	{ .name = "pcie_p_clk", .clk_mux = &ls_mux, .mux = 0x5E },
	{ .name = "ce5_p_clk", .clk_mux = &ls_mux, .mux = 0x5F },
	{ .name = "ce5_core_clk", .clk_mux = &ls_mux, .mux = 0x60 },
	{ .name = "sata_phy_ref_clk", .clk_mux = &ls_mux, .mux = 0x6B },
	{ .name = "sata_phy_cfg_clk", .clk_mux = &ls_mux, .mux = 0x6C },
	{ .name = "sfpb_clk", .clk_mux = &ls_mux, .mux = 0x78 },
	{ .name = "sfpb_a_clk", .clk_mux = &ls_mux, .mux = 0x78 },
	{ .name = "pmic_ssbi2_clk", .clk_mux = &ls_mux, .mux = 0x7A },
	{ .name = "pmic_arb0_p_clk", .clk_mux = &ls_mux, .mux = 0x7B },
	{ .name = "pmic_arb1_p_clk", .clk_mux = &ls_mux, .mux = 0x7C },
	{ .name = "prng_clk", .clk_mux = &ls_mux, .mux = 0x7D },
	{ .name = "rpm_msg_ram_p_clk", .clk_mux = &ls_mux, .mux = 0x7F },
	{ .name = "adm0_p_clk", .clk_mux = &ls_mux, .mux = 0x80 },
	{ .name = "usb_hs1_p_clk", .clk_mux = &ls_mux, .mux = 0x84 },
	{ .name = "usb_hs1_xcvr_clk", .clk_mux = &ls_mux, .mux = 0x85 },
	{ .name = "usb_hsic_p_clk", .clk_mux = &ls_mux, .mux = 0x86 },
	{ .name = "usb_hsic_system_clk", .clk_mux = &ls_mux, .mux = 0x87 },
	{ .name = "usb_hsic_xcvr_fs_clk", .clk_mux = &ls_mux, .mux = 0x88 },
	{ .name = "usb_fs1_p_clk", .clk_mux = &ls_mux, .mux = 0x89 },
	{ .name = "usb_fs1_sys_clk", .clk_mux = &ls_mux, .mux = 0x8A },
	{ .name = "usb_fs1_xcvr_clk", .clk_mux = &ls_mux, .mux = 0x8B },
	{ .name = "tsif_p_clk", .clk_mux = &ls_mux, .mux = 0x8F },
	{ .name = "tsif_ref_clk", .clk_mux = &ls_mux, .mux = 0x91 },
	{ .name = "ce1_p_clk", .clk_mux = &ls_mux, .mux = 0x92 },
	{ .name = "tssc_clk", .clk_mux = &ls_mux, .mux = 0x94 },
	{ .name = "usb_hsic_hsio_cal_clk", .clk_mux = &ls_mux, .mux = 0x9D },
	{ .name = "ce1_core_clk", .clk_mux = &ls_mux, .mux = 0xA4 },
	{ .name = "pcie1_p_clk", .clk_mux = &ls_mux, .mux = 0xB0 },
	{ .name = "pcie1_src_clk", .clk_mux = &ls_mux, .mux = 0xB1 },
	{ .name = "pcie2_p_clk", .clk_mux = &ls_mux, .mux = 0xB2 },
	{ .name = "pcie2_src_clk", .clk_mux = &ls_mux, .mux = 0xB3 },

	{ .name = "afab_clk", .clk_mux = &hs_mux, .mux = 0x07 },
	{ .name = "afab_a_clk", .clk_mux = &hs_mux, .mux = 0x07 },
	{ .name = "sfab_clk", .clk_mux = &hs_mux, .mux = 0x18 },
	{ .name = "sfab_a_clk", .clk_mux = &hs_mux, .mux = 0x18 },
	{ .name = "adm0_clk", .clk_mux = &hs_mux, .mux = 0x2A },
	{ .name = "sata_a_clk", .clk_mux = &hs_mux, .mux = 0x31 },
	{ .name = "pcie_aux_clk", .clk_mux = &hs_mux, .mux = 0x2B },
	{ .name = "pcie_phy_ref_clk", .clk_mux = &hs_mux, .mux = 0x2D },
	{ .name = "pcie_a_clk", .clk_mux = &hs_mux, .mux = 0x32 },
	{ .name = "ebi1_clk", .clk_mux = &hs_mux, .mux = 0x34 },
	{ .name = "ebi1_a_clk", .clk_mux = &hs_mux, .mux = 0x34 },
	{ .name = "usb_hsic_hsic_clk", .clk_mux = &hs_mux, .mux = 0x50 },
	{ .name = "pcie1_aux_clk", .clk_mux = &hs_mux, .mux = 0x55 },
	{ .name = "pcie1_phy_ref_clk", .clk_mux = &hs_mux, .mux = 0x56 },
	{ .name = "pcie2_aux_clk", .clk_mux = &hs_mux, .mux = 0x57 },
	{ .name = "pcie2_phy_ref_clk", .clk_mux = &hs_mux, .mux = 0x58 },
	{ .name = "pcie1_a_clk", .clk_mux = &hs_mux, .mux = 0x66 },
	{ .name = "pcie2_a_clk", .clk_mux = &hs_mux, .mux = 0x67 },

	{ .name = "l2_m_clk", .clk_mux = &cpul2_mux, .mux = 0x2, 8 },
	{ .name = "krait0_m_clk", .clk_mux = &cpul2_mux, .mux = 0x0, 8 },
	{ .name = "krait1_m_clk", .clk_mux = &cpul2_mux, .mux = 0x1, 8 },
	{}
};

struct debugcc_platform ipq8064_debugcc = {
	.name = "ipq8064",
	.clocks = ipq8064_clocks,
};
