/*
 * dstsblt_interface.h
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

#ifndef __DSTSBLT_INTERFACE_H
#define __DSTSBLT_INTERFACE_H

#define BLT_DRV_DEV_ENTRY       "dstsblt"

typedef struct {
    __u32 num_access;   /* Number of accesses */
    __u32 num_read;     /* Number of read() calls */
    __u32 num_write;    /* Number of write() calls */
    __u32 num_ioctl;    /* Number of ioctl() calls */
    __u32 num_mmap;     /* Number of mmap() calls */
} dstsblt_statistics_t;

typedef struct {
    __u8 partname[16];
    __u32 offset;
    __u32 entrysize;
} dstsblt_nvm_t;

typedef struct {
    __u32 id;                           /* Region ID from DTB */
    __u32 locked;                       /* Region sticky read lock status */
    __u32 version;                      /* Region interface version */
    __u32 padding;                      /* (padding) */
    __u64 base;                         /* Region physical base address */
    __u64 size;                         /* Region size */
    dstsblt_statistics_t statistics;    /* Access and call statistics */
    dstsblt_nvm_t nvm;                  /* NVM information for crashes */
} dstsblt_info_t;

#define DSTSBLT_IOCTL_CHR       'B'

/* IOCTLs */
/* Get driver version */
#define DSTSBLT_IOCTL_GETVERS   _IOR(DSTSBLT_IOCTL_CHR, 0x00, unsigned int)
/* Lock against further reads */
#define DSTSBLT_IOCTL_LOCK      _IO(DSTSBLT_IOCTL_CHR,  0x10)
/* Get dstsblt_info_t for region */
#define DSTSBLT_IOCTL_GETINFO   _IOR(DSTSBLT_IOCTL_CHR, 0x15, dstsblt_info_t)


/* Only available if DSTSBLT_DEBUG enabled */
/* Reset read lock */
#define DSTSBLT_IOCTL_UNLOCK    _IO(DSTSBLT_IOCTL_CHR,  0x90)
/* Reset statistics */
#define DSTSBLT_IOCTL_NUMRESET  _IO(DSTSBLT_IOCTL_CHR,  0xA0)

#endif
