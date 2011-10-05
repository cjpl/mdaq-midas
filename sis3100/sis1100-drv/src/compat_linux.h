/* $ZEL: compat_linux.h,v 1.24 2009/08/31 15:13:37 wuestner Exp $ */

/*
 * Copyright (c) 2003-2008
 * 	Peter Wuestner.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _compat_linux_h_
#define _compat_linux_h_

#include <linux/version.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/mm.h>
#include <linux/poll.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <asm/poll.h>
#include <asm/uaccess.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10)
#include <asm-generic/iomap.h>
#endif

#define ofs(what, elem) ((size_t)&(((what *)0)->elem))

#ifndef NBPG
#define NBPG PAGE_SIZE
#endif

#define DECLARE_SPINLOCKFLAGS(flags) unsigned long flags;
#define SPIN_LOCK_IRQSAVE(lock, flags) spin_lock_irqsave(&(lock), flags)
#define SPIN_UNLOCK_IRQRESTORE(lock, flags) spin_unlock_irqrestore(&(lock), flags)

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16)
#   define MUX semaphore
#   define mutex_init(sem) init_MUTEX(sem)
#   define mutex_lock(sem) down(sem)
#   define mutex_unlock(sem) up(sem)
#else
#   define MUX mutex
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,14)
#   define kzalloc(a, b) kmalloc((a), (b))
#endif

#define KFREE(p) kfree(p)
#define KMALLOC(s) kmalloc(s, GFP_KERNEL)

#define ACCESS_OK(buf, count, write) \
    access_ok(write?VERIFY_READ:VERIFY_WRITE, buf, count)

#define COPY_TO_USER(dest, src, size) \
    copy_to_user(dest, src, size)

#define wakeup(x) wake_up(x)

#define RMB(t, h, o, l) rmb()
#define WMB(t, h, o, l) wmb()
#define MB(t, h, o, l) mb()

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,23)
typedef void irqreturn_t;
#define IRQ_NONE
#define IRQ_HANDLED
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#define SIGMASK_LOCK sigmask_lock
#else
#define SIGMASK_LOCK sighand->siglock
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
#define pid_nr(pid) (pid)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
static inline unsigned iminor(struct inode *inode)
{
	return MINOR(inode->i_rdev);
}
#endif

#define LINUX_RETURN(x) return -(x)

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18)
#define IRQF_SHARED SA_SHIRQ
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,25)
struct sg_table {
	struct scatterlist *sgl;	/* the list */
	unsigned int nents;		/* number of mapped entries */
	unsigned int orig_nents;	/* original size of list */
};
void sg_free_table(struct sg_table *);
int sg_alloc_table(struct sg_table *, unsigned int, gfp_t);
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
#define for_each_sg(sglist, sg, nr, __i)	\
	for (__i = 0, sg = (sglist); __i < (nr); __i++, sg++)
#define sg_page(sg) (sg)->page
#define sg_set_page(sg, _page, _len, _offset) \
    do {                                      \
        (sg)->page = _page;                   \
	(sg)->offset = _offset;               \
	(sg)->length = _len;                  \
    } while (0)

#endif

#ifndef DMA_BIT_MASK
/* available since 2.6.25 */
#define DMA_BIT_MASK(n)	(((n) == 64) ? ~0ULL : ((1ULL<<(n))-1))
#endif

#if 0
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22)
#ifndef MAX_UDELAY_MS
#define MAX_UDELAY_MS	5
#endif
#ifndef mdelay
#define mdelay(n) (\
	(__builtin_constant_p(n) && (n)<=MAX_UDELAY_MS) ? udelay((n)*1000) : \
	({unsigned long __ms=(n); while (__ms--) udelay(1000);}))
#endif
#endif
#endif

int sgl_map_user_pages(struct sg_table *table, const char* uaddr,
        size_t count, int rw);
int sgl_unmap_user_pages(struct sg_table *table, int dirtied);
void dump_sgl(struct sg_table *table);

#endif
