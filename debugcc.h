// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2019, Linaro Ltd. */

#ifndef __DEBUGCC_H__
#define __DEBUGCC_H__

#define BIT(x) (1 << (x))
#define GENMASK(h, l) (((~0UL) << (l)) & (~0UL >> (sizeof(long) * 8 - 1 - (h))))

#define CORE_CC_BLOCK "core"

struct measure_clk;

struct debug_mux {
	unsigned long phys;
	void *base;
	const char *block_name;
	size_t size;

	struct debug_mux *parent;
	unsigned long parent_mux_val;

	unsigned int enable_reg;
	unsigned int enable_mask;

	unsigned int mux_reg;
	unsigned int mux_mask;
	unsigned int mux_shift;

	unsigned int div_reg;
	unsigned int div_shift;
	unsigned int div_mask;
	unsigned int div_val;

	unsigned long (*measure)(const struct measure_clk *clk,
				 const struct debug_mux *mux);
};

struct gcc_mux {
	struct debug_mux mux;

	unsigned int xo_rate;

	unsigned int xo_div4_reg;
	unsigned int xo_div4_val;

	unsigned int debug_ctl_reg;
	unsigned int debug_status_reg;
};

struct measure_clk {
	char *name;
	struct debug_mux *clk_mux;
	unsigned long mux;

	unsigned int fixed_div;
};

struct debugcc_platform {
	const char *name;
	const struct measure_clk *clocks;
	int (*premap)(int devmem);
};

#define container_of(ptr, type, member) \
	((type *) ((char *)(ptr) - offsetof(type, member)))

static inline uint32_t readl(void *ptr)
{
	return *((volatile uint32_t*)ptr);
}

static inline void writel(uint32_t val, void *ptr)
{
	*((volatile uint32_t*)ptr) = val;
}

int mmap_mux(int devmem, struct debug_mux *mux);
void mux_enable(struct debug_mux *mux);
void mux_disable(struct debug_mux *mux);

unsigned long measure_gcc(const struct measure_clk *clk,
			  const struct debug_mux *mux);
unsigned long measure_leaf(const struct measure_clk *clk,
			   const struct debug_mux *mux);
unsigned long measure_mccc(const struct measure_clk *clk,
			   const struct debug_mux *mux);

extern const struct debugcc_platform *platforms[];

#endif
