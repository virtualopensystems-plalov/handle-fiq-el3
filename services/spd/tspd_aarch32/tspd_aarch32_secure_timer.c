/*
 * Copyright (c) 2015, ARM Limited and Contributors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Neither the name of ARM nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific
 * prior written permission.
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
#include <arch_helpers.h>
#include <assert.h>
#include <platform.h>
#include "tspd_aarch32_private.h"

/*******************************************************************************
 * Data structure to keep track of per-cpu secure generic timer context across
 * power management operations.
 ******************************************************************************/
typedef struct timer_context {
	uint64_t cval;
	uint32_t ctl;
} timer_context_t;

static timer_context_t pcpu_timer_context[PLATFORM_CORE_COUNT];

/*******************************************************************************
 * This function initializes the secure timer to fire according to the frequency
 * defined in freq_hz parameter
 ******************************************************************************/
void tspd_service_timer_start(uint32_t freq_hz)
{
	uint64_t cval;
	uint32_t ctl = 0;

	/* Define the time value when the interrupt will be generated */
	cval = read_cntpct_el0() + (read_cntfrq_el0() / freq_hz);
	write_cntps_cval_el1(cval);

	/* Enable the secure physical timer */
	set_cntp_ctl_enable(ctl);
	write_cntps_ctl_el1(ctl);
}

/*******************************************************************************
 * This function deasserts the timer interrupt and sets it up again
 ******************************************************************************/
void tspd_service_timer_handler(uint32_t freq_hz)
{
	/*
	 * Disable the timer and reprogram it. The barriers ensure that there is
	 * no reordering of instructions around the reprogramming code.
	 */
	isb();
	write_cntps_ctl_el1(0);
	tspd_service_timer_start(freq_hz);
	isb();
}

/*******************************************************************************
 * This function deasserts the timer interrupt prior to cpu power down
 ******************************************************************************/
void tspd_service_timer_stop(void)
{
	/* Disable the timer */
	write_cntps_ctl_el1(0);
}

/*******************************************************************************
 * This function saves the timer context prior to cpu suspension
 ******************************************************************************/
void tspd_service_timer_save(void)
{
	uint32_t linear_id = platform_get_core_pos(read_mpidr());

	pcpu_timer_context[linear_id].cval = read_cntps_cval_el1();
	pcpu_timer_context[linear_id].ctl = read_cntps_ctl_el1();
	flush_dcache_range((uint64_t) &pcpu_timer_context[linear_id],
			   sizeof(pcpu_timer_context[linear_id]));
}

/*******************************************************************************
 * This function restores the timer context post cpu resummption
 ******************************************************************************/
void tspd_service_timer_restore(void)
{
	uint32_t linear_id = platform_get_core_pos(read_mpidr());

	write_cntps_cval_el1(pcpu_timer_context[linear_id].cval);
	write_cntps_ctl_el1(pcpu_timer_context[linear_id].ctl);
}