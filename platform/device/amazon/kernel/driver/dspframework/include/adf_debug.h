/*
 * adf_debug.h
 *
 * debugfs include operate of log/cli/state
 *
 * Copyright 2020-2021 Amazon Technologies, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#ifndef _ADF_DEBUG_H_
#define _ADF_DEBUG_H_

#include <linux/workqueue.h>
#include <linux/kthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ADF_DEBUG_LOG_CLEAR_CMD     "clear\n"
#define ADF_DEBUG_LOG_CLEAR_CMD_LEN (6)

#define ADF_DEBUG_LOG_DUMP_MIN_PERIOD (20)
#ifdef CONFIG_AMZN_DSP_FRAMEWORK_CHAR_LEN
#define ADF_DEBUG_CHAR_LEN          CONFIG_AMZN_DSP_FRAMEWORK_CHAR_LEN
#else
#define ADF_DEBUG_CHAR_LEN          (1)
#endif

typedef int (*adf_mem_cpy) (uintptr_t, uintptr_t, int);
typedef int (*adf_chk_run) (int);

int adfDebug_initFs(void *debugReadFunc, void *debugWriteFunc);
void *adfDebug_query(uint8_t dspId, char **cliCmds, uint8_t cliCmdNum, void *logFliter);
int adfDebug_queryState(uint8_t dspId);
void adfDebug_printLog(uint8_t dspId, uint32_t opt);
void adfDebug_initLogDumpThread(int32_t interval);
void adfDebug_blockDbg(uint8_t dspId, uint32_t opt);

#ifdef __cplusplus
}
#endif

#endif  /* _ADF_DEBUG_H_ */
