// SPDX-License-Identifier: GPL-2.0-only
/*
 * Generic show_mem() implementation
 *
 * Copyright (C) 2008 Johannes Weiner <hannes@saeurebad.de>
 */

#include <linux/mm.h>
#include <linux/cma.h>
#if IS_ENABLED(CONFIG_AMAZON_METRICS_MEMORY_LOG)
#include <linux/vmstat.h>
#include <linux/sched/loadavg.h>
#include <linux/metricslog.h>

#define MEMORY_METRICS_BUFF_SIZE 512
static char g_m_buf[MEMORY_METRICS_BUFF_SIZE];

void amz_metrics_memory_status(void)
{
	int i;
	char buf[MEMORY_METRICS_BUFF_SIZE] = {0};
	unsigned long psi_some_avg[3];
	unsigned long free_pages;
	unsigned long mpages;
	int dma_fragment[MAX_ORDER];
	long avg_fragment = 0;
	const struct amazon_logger_ops *amazon_logger;

	amz_metrics_memory_psi(psi_some_avg);
	sprintf(buf, "psiInt=%lu;IN,psiFrac=%lu;IN",
		LOAD_INT(psi_some_avg[0]), LOAD_FRAC(psi_some_avg[0]));
	amz_metrics_memory_extfrag(dma_fragment, &free_pages, &mpages);
	for (i = 0; i < MAX_ORDER; i++) {
		avg_fragment += dma_fragment[i];
		sprintf(buf, "%s,fragOrder%d=%lu;IN", buf, i, dma_fragment[i]);
	}
	avg_fragment = avg_fragment / MAX_ORDER;
	sprintf(buf, "%s,fragOrderAvg=%lu;IN,freePages=%lu;IN,totalPages=%lu;IN",
		buf, avg_fragment, free_pages, mpages);

	amazon_logger = amazon_logger_ops_get();
	if (amazon_logger && amazon_logger->minerva_metrics_log) {
		amazon_logger->minerva_metrics_log(g_m_buf, MEMORY_METRICS_BUFF_SIZE,
			"%s:%s:100:%s,%s,%s,%s:us-east-1", METRICS_MEMORY_GROUP_ID,
			METRICS_MEMORY_SCHEMA_ID, PREDEFINED_ESSENTIAL_KEY,
			PREDEFINED_DEVICE_ID_KEY, PREDEFINED_OS_KEY, buf);
	}
}
#endif

void show_mem(unsigned int filter, nodemask_t *nodemask)
{
	pg_data_t *pgdat;
	unsigned long total = 0, reserved = 0, highmem = 0;

	printk("Mem-Info:\n");
	show_free_areas(filter, nodemask);

	for_each_online_pgdat(pgdat) {
		int zoneid;

		for (zoneid = 0; zoneid < MAX_NR_ZONES; zoneid++) {
			struct zone *zone = &pgdat->node_zones[zoneid];
			if (!populated_zone(zone))
				continue;

			total += zone->present_pages;
			reserved += zone->present_pages - zone_managed_pages(zone);

			if (is_highmem_idx(zoneid))
				highmem += zone->present_pages;
		}
	}

	printk("%lu pages RAM\n", total);
	printk("%lu pages HighMem/MovableOnly\n", highmem);
	printk("%lu pages reserved\n", reserved);
#ifdef CONFIG_CMA
	printk("%lu pages cma reserved\n", totalcma_pages);
#endif
#ifdef CONFIG_MEMORY_FAILURE
	printk("%lu pages hwpoisoned\n", atomic_long_read(&num_poisoned_pages));
#endif
#if IS_ENABLED(CONFIG_AMAZON_METRICS_MEMORY_LOG)
	amz_metrics_memory_status();
#endif
}
