/*
 * dstsblt.h
 *
 * Copyright 2023 Amazon Technologies, Inc. All Rights Reserved.	
 *
 * The code contained herein is licensed under the GNU General Public
 * License Version 2.
 * You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:	
 *
 * http://www.opensource.org/licenses/gpl-license.html	
 * http://www.gnu.org/copyleft/gpl.html
 */

#ifndef __DSTSBLT_H
#define __DSTSBLT_H

#include "dstsblt_interface.h"

/* Driver configuration BEGIN */
/* Maximum numbers of BLT regions supported */
#define NUM_BLT_REGIONS        1

/* Clear BLT region when locking. */
#define DSTSBLT_CLEAR_ON_LOCK

/* Allow mmap() to access BLT */
// #define DSTSBLT_ALLOW_MMAP

/* Enable debugging functionality. This is potentially unsafe for prod */
// #define DSTSBLT_DEBUG
/* Driver configuration END */

#if !defined(DSTSBLT_CLEAR_ON_LOCK) && defined(DSTSBLT_ALLOW_MMAP)
#warning "cannot restrict access via mmap()!"
#endif

#define BLT_DRV_NAME_LOGS       "DSTSBLT"
#define DRIVER_VERSION		    "2.0.0.0"
#define DRIVER_VERSION_INT      0x02000000
#define DSTSBLT_COMPATIBLE      "amazon,dstsblt"

#define BLT_MAJOR               0
#define BLT_MINOR               0

#define BLT_OF_BASENODE         "reserved-memory"
#define BLT_RESERVED_MEM_NAME   "dstsblt_reserved_memory"
#define BLT_OF_REGION_IDENT     "region-ident"
#define BLT_OF_REGION           "reg"
#define BLT_OF_NVM_PART         "nvmpart"
#define BLT_OF_NVM_OFFSET       "nvmoffset"
#define BLT_OF_NVM_ENTRYSIZE    "metricsize"

#endif
