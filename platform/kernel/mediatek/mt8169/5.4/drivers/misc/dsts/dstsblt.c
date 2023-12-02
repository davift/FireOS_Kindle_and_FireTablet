/*
 * dstsblt.c
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

#include <linux/miscdevice.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_reserved_mem.h>
#include <linux/libfdt_env.h>
#include <linux/mm.h>
#include <linux/cdev.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <limits.h>
#include <linux/ioctl.h>

#include "dstsblt.h"


unsigned int blt_major = BLT_MAJOR;
unsigned int blt_minor = BLT_MINOR;

struct dstsblt_reserved_mem {
    phys_addr_t base;
    phys_addr_t size;
};

typedef struct {
    struct cdev cdev;
    struct dstsblt_reserved_mem rmem;
    dev_t dev;
    void *mapped_va;
    int id;
    int valid;
    int used;

    /* read protection */
    int locked;

    /* Access statistics */
    dstsblt_statistics_t statistics;

    /* NVM info */
    dstsblt_nvm_t nvm;
} dstsblt_priv_data_t;

static dstsblt_priv_data_t dstsblt_priv_data[NUM_BLT_REGIONS] = {0u};

static struct dstsblt_reserved_mem rmem_drv = {0};
static unsigned int blt_num_regions = 0;
static struct class *cl;
static dev_t chrdev;


static int dstsblt_dev_open(struct inode *iptr, struct file *fptr)
{
    dstsblt_priv_data_t *bltpriv;

    pr_debug("[%s] open", BLT_DRV_NAME_LOGS);

    bltpriv = (dstsblt_priv_data_t*) container_of(iptr->i_cdev, dstsblt_priv_data_t, cdev);
    fptr->private_data = (void*) bltpriv;

    if (bltpriv->statistics.num_access < UINT_MAX)
        ++(bltpriv->statistics.num_access);

    if (!bltpriv->valid)
        return -EIO;

    return 0;
}

static ssize_t dstsblt_dev_read(struct file *fp, char *buff, size_t len, loff_t *off)
{
    dstsblt_priv_data_t *devdata = (dstsblt_priv_data_t *) fp->private_data;
    size_t rlen = 0;

    pr_debug("[%s] read: %px, len %i, ofs %i", BLT_DRV_NAME_LOGS, devdata->rmem.base, len, *off);

    if (devdata->statistics.num_read < UINT_MAX)
        ++(devdata->statistics.num_read);

    if (devdata->locked)
        return -EPERM;

    if (*off < 0)
        return -EINVAL;
    else if (*off >= devdata->rmem.size)
        return 0;
    else {
        rlen = devdata->rmem.size - *off;
        if (len < rlen)
            rlen = len;
    }

    copy_to_user(buff, (void *) devdata->mapped_va + *off, rlen);
    *off += rlen;

	return rlen;
}

#ifdef DSTSBLT_DEBUG
static ssize_t dstsblt_dev_write(struct file *fp, const char *buff,
                    size_t len, loff_t *off)
{
    dstsblt_priv_data_t *devdata = (dstsblt_priv_data_t *) fp->private_data;

    pr_debug("[%s] write: %px, len %i, ofs %i", BLT_DRV_NAME_LOGS, devdata->rmem.base, len, *off);

    if (devdata->statistics.num_write < UINT_MAX)
        ++(devdata->statistics.num_write);

    if (devdata->locked)
        return -EPERM;

    if (*off < 0)
        return -EINVAL;
    else if (*off >= devdata->rmem.size)
        return -ENOSPC;
    else {
        size_t wlen = devdata->rmem.size - *off;

        if (len < wlen)
            wlen = len;

        copy_from_user((void *) devdata->mapped_va + *off, buff, wlen);
        *off += wlen;

        return wlen;
    }

    return 0;
}
#else /* ifndef DSTSBLT_DEBUG */
static ssize_t dstsblt_dev_write(struct file *fp, const char *buff,
                    size_t len, loff_t *off)
{
    dstsblt_priv_data_t *devdata = (dstsblt_priv_data_t *) fp->private_data;

    if (devdata->statistics.num_write < UINT_MAX)
        ++(devdata->statistics.num_write);

    return -EACCES;
}
#endif

static int dstsblt_dev_release(struct inode *iptr, struct file *fp)
{
    pr_debug("[%s] release", BLT_DRV_NAME_LOGS);
    return 0;
}

#ifdef DSTSBLT_DEBUG
static void dstsblt_reset_statistics(dstsblt_priv_data_t * const devdata)
{
    memset(&devdata->statistics, 0, sizeof(devdata->statistics));
}
#endif

#ifdef DSTSBLT_CLEAR_ON_LOCK
static void dstsblt_clear_region(dstsblt_priv_data_t * const devdata)
{
    pr_debug("[%s] Clearing region %px len %i", BLT_DRV_NAME_LOGS, devdata->mapped_va,
                devdata->rmem.size);

    if (!devdata->valid)
        return;

    memset_io((void *) devdata->mapped_va, 0, devdata->rmem.size);
}
#else /* ifndef DSTSBLT_CLEAR_ON_LOCK */
static void dstsblt_clear_region(dstsblt_priv_data_t * const devdata)
{
    return;
}
#endif

static long dstsblt_dev_ioctl(struct file *fp, unsigned int cmd,
                            unsigned long data)
{
    dstsblt_priv_data_t *devdata = (dstsblt_priv_data_t *) fp->private_data;

    pr_debug("[%s] got ioctl cmd %u", BLT_DRV_NAME_LOGS, cmd);

    if (devdata->statistics.num_ioctl < UINT_MAX)
        ++(devdata->statistics.num_ioctl);

    switch (cmd) {
    case DSTSBLT_IOCTL_LOCK:
        devdata->locked = 1;
        dstsblt_clear_region(devdata);
        break;

    case DSTSBLT_IOCTL_GETINFO: {
        dstsblt_info_t info = { .id = devdata->id, .base = devdata->rmem.base,
                                .version = DRIVER_VERSION_INT, .size = devdata->rmem.size,
                                .locked = devdata->locked};
        memcpy(&info.statistics, &devdata->statistics, sizeof(dstsblt_statistics_t));
        memcpy(&info.nvm, &devdata->nvm, sizeof(dstsblt_nvm_t));
        copy_to_user((void*) data, (void*) &info, sizeof(dstsblt_info_t));
    }
    break;

    case DSTSBLT_IOCTL_GETVERS: {
        int version = DRIVER_VERSION_INT;
        copy_to_user((int*) data, &version, sizeof(int));
    }
    break;

#ifdef DSTSBLT_DEBUG
    case DSTSBLT_IOCTL_UNLOCK:
        devdata->locked = 0;
        break;

    case DSTSBLT_IOCTL_NUMRESET:
        dstsblt_reset_statistics(devdata);
        break;
#endif

    default:
        return -EINVAL;
        break;
    }

    return 0;
}

#ifdef DSTSBLT_ALLOW_MMAP
static int dstsblt_dev_mmap(struct file * fp, struct vm_area_struct * vma)
{
    dstsblt_priv_data_t *devdata = (dstsblt_priv_data_t *) fp->private_data;
    size_t len = vma->vm_end - vma->vm_start;
    int ret = -1;

    pr_debug("[%s] got mmap %px %px (len %llu) prot: %i, off: %i", BLT_DRV_NAME_LOGS, vma->vm_start,
            vma->vm_end, len, vma->vm_page_prot, vma->vm_pgoff);

    if (devdata->statistics.num_mmap < UINT_MAX)
        ++(devdata->statistics.num_mmap);

    if (devdata->locked)
        return -EPERM;

    /* Do not allow mmap offsets. PAGESIZE alignment makes this mostly useless and complex */
    if (vma->vm_pgoff) {
        pr_err("[%s] mmap failed, offsets not permitted", BLT_DRV_NAME_LOGS);
        return -EINVAL;
    }

#ifndef DSTSBLT_DEBUG
    if (vma->vm_page_prot & (PROT_WRITE | PROT_EXEC)) {
        pr_err("[%s] mmap failed, page protection error", BLT_DRV_NAME_LOGS);
        return -EPERM;
    }
#endif

    /* Only entire region shall be mapped. Enforce complexity reduction */
    if (len != devdata->rmem.size) {
        pr_err("[%s] mmap failed, size requested: %llu; size avail: %llu", BLT_DRV_NAME_LOGS,
                len, devdata->rmem.size);
        return -EIO;
    }

    ret = remap_pfn_range(vma, vma->vm_start, devdata->rmem.base >> PAGE_SHIFT,
                            len, vma->vm_page_prot);

    if (ret) {
        pr_err("[%s] mmap remap failed", BLT_DRV_NAME_LOGS);
        return ret;
    }

    return 0;
}
#else /* ifndef DSTSBLT_ALLOW_MMAP: No mmap support */
static int dstsblt_dev_mmap(struct file * fp, struct vm_area_struct * vma)
{
    return -EIO;
}
#endif

static const struct file_operations dstsblt_fops = {
    .owner = THIS_MODULE,
    .open = dstsblt_dev_open,
    .read = dstsblt_dev_read,
    .write = dstsblt_dev_write,
    .release = dstsblt_dev_release,
    .unlocked_ioctl = dstsblt_dev_ioctl,
    .mmap = dstsblt_dev_mmap,
};

static int dstsblt_setup_device(dstsblt_priv_data_t *bltpriv, int index)
{
    int err = 1;
    struct device* ret = NULL;

    bltpriv->dev = MKDEV(blt_major, blt_minor + index);

    cdev_init(&bltpriv->cdev, &dstsblt_fops);
    bltpriv->cdev.owner = THIS_MODULE;
    bltpriv->cdev.ops = &dstsblt_fops;
    err = cdev_add(&bltpriv->cdev, bltpriv->dev, 1);

    if (err) {
        pr_err("[%s] Error %i adding cdev %i", BLT_DRV_NAME_LOGS, err, MINOR(bltpriv->dev));
        return err;
    }

    if (IS_ERR(ret = device_create(cl, NULL, bltpriv->dev, NULL, "%s%i",
                BLT_DRV_DEV_ENTRY, index))) {
        pr_err("[%s] Error creating /dev/%s%i", BLT_DRV_NAME_LOGS, BLT_DRV_DEV_ENTRY, index);
        cdev_del(&bltpriv->cdev);
        err = PTR_ERR(ret);
    }
    else
        pr_info("[%s] BLT device /dev/%s%i created for id 0x%X", BLT_DRV_NAME_LOGS,
                BLT_DRV_DEV_ENTRY, index, bltpriv->id);

    return err;
}

static int dstsblt_register_region(const unsigned int region)
{
    dstsblt_priv_data_t *priv_data_elem = &dstsblt_priv_data[region];
    struct dstsblt_reserved_mem *rmem_drv = &priv_data_elem->rmem;
    void *vamap = NULL;
    int err;

    pr_debug("[%s] Processing region %i", BLT_DRV_NAME_LOGS, region);

    /* Sanity checks */
    if (region >= NUM_BLT_REGIONS)
        return -1;

    if (((rmem_drv->base & (PAGE_SIZE-1)) || (rmem_drv->size & (PAGE_SIZE-1)))) {
        pr_err("[%s] Misaligned memory error (base: %px, size: %px)", BLT_DRV_NAME_LOGS,
                rmem_drv->base, rmem_drv->size);
        return -1;
    }

    if ((!rmem_drv->base) || (!rmem_drv->size)) {
        pr_err("[%s] BLT memory area invalid", BLT_DRV_NAME_LOGS);
        return -1;
    }

    vamap = ioremap((resource_size_t) priv_data_elem->rmem.base, priv_data_elem->rmem.size);
    if (vamap)
        priv_data_elem->mapped_va = vamap;
    else {
        pr_err("[%s] cannot map %px", BLT_DRV_NAME_LOGS, priv_data_elem->rmem.base);
        return -1;
    }

    err = dstsblt_setup_device(priv_data_elem, priv_data_elem->id);
    if (err) {
        pr_err("[%s] failed to register region %i (err %i)", BLT_DRV_NAME_LOGS, region, err);
        iounmap(vamap);
        return -1;
    }

    priv_data_elem->valid = 1;

    return 0;
}

static int __init dstsblt_init(void)
{
    int err = -1;
    unsigned int region = 0;
    int regions_failed = 0;
    struct device_node *np_blt = NULL;

    do {
        struct property* prop;
        phys_addr_t base = 0;
        phys_addr_t size = 0;
        u32 region_id = 0;
        int len = 0;
        u32 data = 0;

        np_blt = of_find_compatible_node(np_blt, NULL, DSTSBLT_COMPATIBLE);
        if (!np_blt)
            break;

        if (!of_device_is_available(np_blt))
            continue;

        if (((!np_blt->parent) || (!np_blt->parent->name)) ||
            strcmp(np_blt->parent->name, BLT_OF_BASENODE)) {
            pr_err("[%s] node is not in '%s'\n", BLT_DRV_NAME_LOGS, BLT_OF_BASENODE);
            continue;
        }

        pr_debug("[%s] Compatible active node: %s/%s\n", \
            BLT_DRV_NAME_LOGS, BLT_OF_BASENODE, np_blt->name);

        prop = of_find_property(np_blt, BLT_OF_REGION, &len);
        if ((!prop) || (len != 2*sizeof(base))) {
            pr_err("[%s] Invalid '%s' property\n", BLT_DRV_NAME_LOGS, BLT_OF_REGION_IDENT);
            continue;
        }

        memcpy(&base, prop->value, sizeof(phys_addr_t));
        base = fdt64_to_cpu(base);

        memcpy(&size, ((phys_addr_t*) prop->value) + 1, sizeof(phys_addr_t));
        size = fdt64_to_cpu(size);

        prop = of_find_property(np_blt, BLT_OF_REGION_IDENT, &len);
        if ((!prop) || (len != sizeof(region_id))) {
            pr_err("[%s] Invalid '%s' property\n", BLT_DRV_NAME_LOGS, BLT_OF_REGION_IDENT);
            continue;
        }

        memcpy(&region_id, prop->value, sizeof(region_id));
        region_id = fdt32_to_cpu(region_id);

        if ((region_id >= NUM_BLT_REGIONS) || (dstsblt_priv_data[region_id].used)) {
            pr_err("[%s] Invalid region id %i\n", BLT_DRV_NAME_LOGS, region_id);
            continue;
        }

        dstsblt_priv_data[region_id].rmem.base = base;
        dstsblt_priv_data[region_id].rmem.size = size;
        dstsblt_priv_data[region_id].id = region_id;
        dstsblt_priv_data[region_id].used = 1;

        prop = of_find_property(np_blt, BLT_OF_NVM_PART, &len);
        if ((prop) && (len < sizeof(dstsblt_priv_data[region_id].nvm.partname)))
            memcpy(&dstsblt_priv_data[region_id].nvm.partname, prop->value, len);

        prop = of_find_property(np_blt, BLT_OF_NVM_OFFSET, &len);
        if ((prop) && (len == sizeof(data)))
        {
            memcpy(&data, prop->value, sizeof(data));
            dstsblt_priv_data[region_id].nvm.offset = fdt32_to_cpu(data);
        }

        prop = of_find_property(np_blt, BLT_OF_NVM_ENTRYSIZE, &len);
        if ((prop) && (len == sizeof(data)))
        {
            memcpy(&data, prop->value, sizeof(data));
            dstsblt_priv_data[region_id].nvm.entrysize = fdt32_to_cpu(data);
        }

        ++blt_num_regions;
    } while (np_blt);

    if (!blt_num_regions) {
        pr_err("[%s] No valid regions, init failed", BLT_DRV_NAME_LOGS);
        return -EIO;
    }

    err = alloc_chrdev_region(&chrdev, blt_minor, blt_num_regions, BLT_DRV_DEV_ENTRY);
    if (err < 0) {
        pr_err("[%s] Cannot get chrdev region", BLT_DRV_NAME_LOGS);
        goto init_fail;
    }
    blt_major = MAJOR(chrdev);

    cl = class_create(THIS_MODULE, BLT_DRV_DEV_ENTRY);
    if (!cl) {
        pr_err("[%s] Cannot create class %s", BLT_DRV_NAME_LOGS, BLT_DRV_DEV_ENTRY);
        err = -EIO;
        goto init_fail_class;
    }

    for (region = 0; region < NUM_BLT_REGIONS; ++region) {
        if (dstsblt_priv_data[region].used) {
            err = dstsblt_register_region(region);
            if (err)
                ++regions_failed;
        }
    }

    if (regions_failed == blt_num_regions) {
        pr_err("[%s] No valid regions, init failed", BLT_DRV_NAME_LOGS);
        err = -EIO;
        goto init_allregions_failed;
    }

    return 0;

    init_allregions_failed:
    class_destroy(cl);

    init_fail_class:
    unregister_chrdev_region(chrdev, blt_num_regions);

    init_fail:
    memset(&rmem_drv, 0, sizeof(rmem_drv));
    pr_err("[%s] initialization failed", BLT_DRV_NAME_LOGS);

    return err;
}

static void __exit dstsblt_exit(void)
{
    int region;

    for (region = 0; region < blt_num_regions; ++region) {
        dstsblt_priv_data_t *priv_data_elem = &dstsblt_priv_data[region];
        if (priv_data_elem->valid) {
            dstsblt_clear_region(priv_data_elem);
            priv_data_elem->valid = 0;
            device_destroy(cl, priv_data_elem->dev);
            cdev_del(&priv_data_elem->cdev);
            iounmap(priv_data_elem->mapped_va);
        }
    }
    class_destroy(cl);
    unregister_chrdev_region(chrdev, blt_num_regions);
}

module_init(dstsblt_init);
module_exit(dstsblt_exit);

MODULE_AUTHOR("Johannes Obermaier");
MODULE_DESCRIPTION("DSTS BLT driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRIVER_VERSION);
