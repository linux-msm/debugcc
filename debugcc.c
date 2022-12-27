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
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "debugcc.h"

static const struct debugcc_platform *platforms[] = {
	&msm8936_debugcc,
	&msm8994_debugcc,
	&msm8996_debugcc,
	&qcs404_debugcc,
	&sc8280xp_debugcc,
	&sdm845_debugcc,
	&sm6115_debugcc,
	&sm6125_debugcc,
	&sm6350_debugcc,
	&sm6375_debugcc,
	&sm8150_debugcc,
	&sm8250_debugcc,
	&sm8350_debugcc,
	&sm8450_debugcc,
	&sm8550_debugcc,
	NULL
};

static uint32_t readl(void *ptr)
{
	return *((volatile uint32_t*)ptr);
}

static void writel(uint32_t val, void *ptr)
{
	*((volatile uint32_t*)ptr) = val;
}

static unsigned int measure_ticks(struct debug_mux *gcc, unsigned int ticks)
{
	uint32_t val;

	writel(ticks, gcc->base + gcc->debug_ctl_reg);
	do {
		val = readl(gcc->base + gcc->debug_status_reg);
	} while (val & BIT(25));

	writel(ticks | BIT(20), gcc->base + gcc->debug_ctl_reg);
	do {
		val = readl(gcc->base + gcc->debug_status_reg);
	} while (!(val & BIT(25)));

	val &= 0x1ffffff;

	writel(ticks, gcc->base + gcc->debug_ctl_reg);

	return val;
}

static void mux_prepare_enable(struct debug_mux *mux, int selector)
{
	uint32_t val;

	if (mux->mux_mask) {
		val = readl(mux->base + mux->mux_reg);
		val &= ~mux->mux_mask;
		val |= selector << mux->mux_shift;
		writel(val, mux->base + mux->mux_reg);
	}

	if (mux->div_mask) {
		val = readl(mux->base + mux->div_reg);
		val &= ~mux->div_mask;
		val |= (mux->div_val - 1) << mux->div_shift;
		writel(val, mux->base + mux->div_reg);
	}

	mux_enable(mux);
}

void mux_enable(struct debug_mux *mux)
{
	uint32_t val;

	if (mux->enable_mask) {
		val = readl(mux->base + mux->enable_reg);
		val |= mux->enable_mask;
		writel(val, mux->base + mux->enable_reg);
	}

	if (mux->premeasure)
		mux->premeasure(mux);
}

void mux_disable(struct debug_mux *mux)
{
	uint32_t val;

	if (mux->postmeasure)
		mux->postmeasure(mux);

	if (mux->enable_mask) {
		val = readl(mux->base + mux->enable_reg);
		val &= ~mux->enable_mask;
		writel(val, mux->base + mux->enable_reg);
	}
}

static bool leaf_enabled(struct debug_mux *mux, struct debug_mux *leaf)
{
	uint32_t val;

	/* If no AHB clock is specified, we assume it's clocked */
	if (!leaf || !leaf->ahb_mask)
		return true;

	val = readl(mux->base + leaf->ahb_reg);
	val &= leaf->ahb_mask;

	/* CLK_OFF will be set if block is not clocked, so inverse */
	return !val;
}

static unsigned long measure_default(const struct measure_clk *clk)
{
	unsigned long raw_count_short;
	unsigned long raw_count_full;
	struct debug_mux *gcc = clk->primary;
	unsigned long xo_div4;

	xo_div4 = readl(gcc->base + gcc->xo_div4_reg);
	writel(xo_div4 | 1, gcc->base + gcc->xo_div4_reg);

	raw_count_short = measure_ticks(gcc, 0x1000);
	raw_count_full = measure_ticks(gcc, 0x10000);

	writel(xo_div4, gcc->base + gcc->xo_div4_reg);

	if (raw_count_full == raw_count_short) {
		return 0;
	}

	raw_count_full = ((raw_count_full * 10) + 15) * 4800000;
	raw_count_full = raw_count_full / ((0x10000 * 10) + 35);

	if (clk->leaf && clk->leaf->div_val)
		raw_count_full *= clk->leaf->div_val;

	if (clk->primary->div_val)
		raw_count_full *= clk->primary->div_val;

	if (clk->fixed_div)
		raw_count_full *= clk->fixed_div;


	return raw_count_full;
}

unsigned long measure_mccc(const struct measure_clk *clk)
{
	/* MCCC is always on, just read the rate and return. */
	return 1000000000000ULL / readl(clk->leaf->base + clk->leaf_mux);
}

static void measure(const struct measure_clk *clk)
{
	unsigned long clk_rate;
	struct debug_mux *gcc = clk->primary;

	if (!leaf_enabled(gcc, clk->leaf)) {
		printf("%50s: skipping\n", clk->name);
		return;
	}

	if (clk->leaf)
		mux_prepare_enable(clk->leaf, clk->leaf_mux);

	mux_prepare_enable(clk->primary, clk->mux);

	if (clk->leaf && clk->leaf->measure)
		clk_rate = clk->leaf->measure(clk);
	else
		clk_rate = measure_default(clk);

	mux_disable(clk->primary);

	if (clk->leaf)
		mux_disable(clk->leaf);

	if (clk_rate == 0) {
		printf("%50s: off\n", clk->name);
		return;
	}

	printf("%50s: %fMHz (%ldHz)\n", clk->name, clk_rate / 1000000.0, clk_rate);
}

static const struct debugcc_platform *find_platform(const char *name)
{
	const struct debugcc_platform **p;

	for (p = platforms; *p; p++) {
		if (!strcmp((*p)->name, name))
			return *p;
	}

	return NULL;
}

/**
 * match_platform() - match platform with executable name
 * @argv: argv[0] of the executable
 *
 * Return: A debugcc_platform when a match is found, otherwise NULL
 *
 * Matches %s-debugcc against the registered platforms
 */
static const struct debugcc_platform *match_platform(const char *argv)
{
	const struct debugcc_platform **p;
	const char *name;
	size_t len;

	for (p = platforms; *p; p++) {
		name = (*p)->name;

		len = strlen(name);

		if (!strncmp(name, argv, len) && !strcmp(argv + len, "-debugcc"))
			return *p;
	}

	return NULL;
}

static const struct measure_clk *find_clock(const struct debugcc_platform *platform,
					    const char *name)
{
	const struct measure_clk *clk;

	for (clk = platform->clocks; clk->name; clk++) {
		if (!strcmp(clk->name, name))
			return clk;
	}

	return NULL;
}

static bool clock_from_block(const struct measure_clk *clk, const char *block_name)
{
	return  !block_name ||
		(!clk->leaf && !strcmp(block_name, CORE_CC_BLOCK)) ||
		(clk->leaf && clk->leaf->block_name && !strcmp(block_name, clk->leaf->block_name));
}

static void list_clocks_block(const struct debugcc_platform *platform, const char *block_name)
{
	const struct measure_clk *clk;

	for (clk = platform->clocks; clk->name; clk++) {
		if (!clock_from_block(clk, block_name))
			continue;

		if (clk->leaf && clk->leaf->block_name)
			printf("%-40s %s\n", clk->name, clk->leaf->block_name);
		else
			printf("%s\n", clk->name);
	}
}

int mmap_mux(int devmem, struct debug_mux *mux)
{
	/* Do nothing if this mux has already been mapped */
	if (!mux || mux->base)
		return 0;

	mux->base = mmap(0, mux->size, PROT_READ | PROT_WRITE, MAP_SHARED, devmem, mux->phys);
	if (mux->base == (void *)-1) {
		warn("failed to map %#zx", mux->phys);
		return -1;
	}

	return 0;
}

/**
 * mmap_hardware() - loop over all clock and make sure hardware is mmapped
 * @devmem: file descriptor to an opened /dev/mem
 * @platform: debugcc_platform with list of clocks to mmap
 *
 * Return: 0 on succees, -1 on failure
 */
static int mmap_hardware(int devmem, const struct debugcc_platform *platform)
{
	const struct measure_clk *clk;
	int ret;

	for (clk = platform->clocks; clk->name; clk++) {
		ret = mmap_mux(devmem, clk->primary);
		if (ret < 0)
			return ret;

		ret = mmap_mux(devmem, clk->leaf);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static void usage(void)
{
	const struct debugcc_platform **p;

	fprintf(stderr, "debugcc <-p platform> [-b blk] <-a | -l | clk>\n");
	fprintf(stderr, "<platform>-debugcc [-b blk] <-a | -l | clk>\n");

	fprintf(stderr, "available platforms:");
	for (p = platforms; *p; p++)
		fprintf(stderr, " %s", (*p)->name);
	fprintf(stderr, "\n");

	exit(0);
}

int main(int argc, char **argv)
{
	const struct debugcc_platform *platform = NULL;
	const struct measure_clk *clk = NULL;
	bool do_list_clocks = false;
	bool all_clocks = false;
	const char *block_name = NULL;
	int devmem;
	int opt;
	int ret;

	while ((opt = getopt(argc, argv, "ab:lp:")) != -1) {
		switch (opt) {
		case 'a':
			all_clocks = true;
			break;
		case 'b':
			block_name = strdup(optarg);
			break;
		case 'l':
			do_list_clocks = true;
			break;
		case 'p':
			platform = find_platform(optarg);
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}

	if (!platform) {
		platform = match_platform(argv[0]);
		if (!platform)
			usage();
	}

	if (do_list_clocks) {
		list_clocks_block(platform, block_name);
		exit(0);
	}

	if (!all_clocks) {
		if (optind >= argc)
			usage();

		clk = find_clock(platform, argv[optind]);
		if (!clk) {
			fprintf(stderr, "no clock named \"%s\"\n", argv[optind]);
			exit(1);
		}
	}

	devmem = open("/dev/mem", O_RDWR | O_SYNC);
	if (devmem < 0)
		err(1, "failed to open /dev/mem");

	if (platform->premap) {
		ret = platform->premap(devmem);
		if (ret < 0)
			exit (1);
	}

	ret = mmap_hardware(devmem, platform);
	if (ret < 0)
		exit(1);

	if (clk) {
		measure(clk);
	} else {
		for (clk = platform->clocks; clk->name; clk++) {
			if (clock_from_block(clk, block_name)) {
				measure(clk);
			}
		}
	}

	return 0;
}
