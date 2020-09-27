/*
 *Copyright (c)  2018  Guangdong OPPO Mobile Comm Corp., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <soc/oppo/oppo_healthinfo.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>
#include "internal.h"
#include  <linux/memory_monitor.h>

struct alloc_wait_para allocwait_para = {0,0,0,0,0,0,0,0};
struct ion_wait_para ionwait_para = {0,0,0,0,0};

#ifdef CONFIG_OPPO_HEALTHINFO
extern bool ohm_memmon_ctrl;
extern bool ohm_memmon_logon;
extern bool ohm_memmon_trig;
extern void ohm_action_trig(int type);

extern bool ohm_ionmon_ctrl;
extern bool ohm_ionmon_logon;
extern bool ohm_ionmon_trig;

#else
static bool ohm_memmon_ctrl = false;
static bool ohm_memmon_logon = false;
static bool ohm_memmon_trig = false;

static bool ohm_ionmon_ctrl = false;
static bool ohm_ionmon_logon = false;
static bool ohm_ionmon_trig = false;

void ohm_action_trig(int type)
{
        return;
}
#endif

extern void warn_alloc_failed(gfp_t gfp_mask, unsigned int order, const char *fmt, ...);

static int alloc_wait_h_ms = 500;
static int alloc_wait_l_ms = 100;
static int alloc_wait_log_ms = 1000;
static int alloc_wait_trig_ms = 10000;

static int ion_wait_h_ms = 500;
static int ion_wait_l_ms = 100;
static int ion_wait_log_ms = 1000;
static int ion_wait_trig_ms = 10000;

void memory_alloc_monitor(gfp_t gfp_mask, unsigned int order, u64 wait_ms)
{
        int fg = 0;
        if (!ohm_memmon_ctrl)
                return;

        fg = current_is_fg();
        if (fg) {
                if (wait_ms >= alloc_wait_h_ms) {
                        allocwait_para.fg_alloc_wait_h_cnt++;
		} else if (wait_ms >= alloc_wait_l_ms) {
			allocwait_para.fg_alloc_wait_l_cnt++;
                }

		if (allocwait_para.fg_alloc_wait_max_ms < wait_ms) {
			allocwait_para.fg_alloc_wait_max_ms = wait_ms;
			allocwait_para.fg_alloc_wait_max_order = order;
		}
	}

	if (wait_ms >= alloc_wait_h_ms) {
		allocwait_para.total_alloc_wait_h_cnt++;
                if (ohm_memmon_logon && (wait_ms >= alloc_wait_log_ms)) {
                        ohm_debug("[alloc_wait / %s] long, order %d, wait %lld ms!\n", (fg ? "fg":"bg"), order, wait_ms);
                        warn_alloc(gfp_mask, "page allocation stalls for %lld ms, order: %d", wait_ms, order);
                }
                if (ohm_memmon_trig && wait_ms >= alloc_wait_trig_ms) {
                        /* Trig Uevent */
                        ohm_action_trig(OHM_MEM_MON);
                }
	} else if (wait_ms >= alloc_wait_l_ms) {
		allocwait_para.total_alloc_wait_l_cnt++;
        }

	if (allocwait_para.total_alloc_wait_max_ms < wait_ms) {
		allocwait_para.total_alloc_wait_max_ms = wait_ms;
		allocwait_para.total_alloc_wait_max_order = order;
	}
}

void oppo_ionwait_monitor(u64 wait_ms)
{
    int fg = 0;
    if (!ohm_ionmon_ctrl)
    	return;

	fg = current_is_fg();
	if (fg) {
       if (wait_ms >= ion_wait_h_ms) {
			ionwait_para.fg_ion_wait_h_cnt++;
       } else if (wait_ms >= ion_wait_l_ms) {
       		ionwait_para.fg_ion_wait_l_cnt++;
       }
	}

	if (wait_ms >= ion_wait_h_ms) {
		ionwait_para.total_ion_wait_h_cnt++;
        if (ohm_ionmon_logon && (wait_ms >= ion_wait_log_ms)) {
        	ohm_debug("[ion_wait / %s] long, wait %lld ms!\n", (fg ? "fg":"bg"), wait_ms);
        }
        if (ohm_ionmon_trig && wait_ms >= ion_wait_trig_ms) {
            /* Trig Uevent */
        	ohm_action_trig(OHM_ION_MON);
        }
	} else if (wait_ms >= ion_wait_l_ms) {
    		ionwait_para.total_ion_wait_l_cnt++;
		}

   if (ionwait_para.total_ion_wait_max_ms < wait_ms) {
		ionwait_para.total_ion_wait_max_ms = wait_ms;
   }
}

module_param_named(alloc_wait_h_ms, alloc_wait_h_ms, int, S_IRUGO | S_IWUSR);
module_param_named(alloc_wait_l_ms, alloc_wait_l_ms, int, S_IRUGO | S_IWUSR);
module_param_named(alloc_wait_log_ms, alloc_wait_log_ms, int, S_IRUGO | S_IWUSR);
module_param_named(alloc_wait_trig_ms, alloc_wait_trig_ms, int, S_IRUGO | S_IWUSR);

module_param_named(ion_wait_h_ms, ion_wait_h_ms, int, S_IRUGO | S_IWUSR);
module_param_named(ion_wait_l_ms, ion_wait_l_ms, int, S_IRUGO | S_IWUSR);
module_param_named(ion_wait_log_ms, ion_wait_log_ms, int, S_IRUGO | S_IWUSR);

