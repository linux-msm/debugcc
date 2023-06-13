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
#ifndef __DEBUGCC_H__
#define __DEBUGCC_H__

#define BIT(x) (1 << (x))

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
