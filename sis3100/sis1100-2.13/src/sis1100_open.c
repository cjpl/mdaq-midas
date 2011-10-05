/* $ZEL: sis1100_open.c,v 1.18 2009/08/31 15:30:28 wuestner Exp $ */

/*
 * Copyright (c) 2001-2008  Peter Wuestner.  All rights reserved.
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

#include "sis1100_sc.h"
#include "sis3100_map.h"

static void
init_fd(struct sis1100_fdata* fd)
{
    fd->fifo_mode=0;          /* can't be changed for sdram and dsp */
    fd->vmespace_am=9;        /* useless for sdram and dsp */
    fd->vmespace_datasize=4;  /* useless for sdram and dsp */
    fd->last_prot_err=0;
    fd->sig=0;
    fd->owned_irqs=0;         /* useless for sdram and dsp */
    fd->mindmalen_r=16;
    fd->mindmalen_w=16;
    fd->minpipelen_r=0;
    fd->minpipelen_w=0;
#if 0
    fd->mmapdma.valid=0;
#endif
    fd->old_remote_hw=fd->sc->remote_hw;
}

int
sis1100_open(struct inode *inode, struct file *file)
{
    struct sis1100_softc* sc;
    struct sis1100_fdata* fd;
    unsigned int _minor=iminor(inode);
    unsigned int card=(_minor&sis1100_MINORCARDMASK)>>sis1100_MINORCARDSHIFT;
    unsigned int subdev=(_minor&sis1100_MINORTYPEMASK)>>sis1100_MINORTYPESHIFT;
    unsigned int idx=_minor&sis1100_MINORUTMASK;
    int res=0;

    if (card >= sis1100_MAXCARDS || !sis1100_devdata[card]) {
        return -ENXIO; /*ENODEV*/
    }
    sc=sis1100_devdata[card];

    mutex_lock(&sc->sem_fdata);

    if (sc->fdata[idx]) {
        res=-EBUSY;
        goto error;
    }
    fd=kzalloc(sizeof(struct sis1100_fdata), GFP_KERNEL);
    if (!fd) {
        res=-ENOMEM;
        goto error;
    }

    fd->sc=sc;
    fd->subdev=subdev;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
    fd->pid=current->pid;
#else
    fd->pid=get_pid(task_pid(current));
#endif
    init_fd(fd);
    file->private_data = fd;

    sc->fdata[idx]=fd;

error:
    mutex_unlock(&sc->sem_fdata);
    return res;
}

static void
cleanup_fd(struct sis1100_fdata* fd)
{
    struct sis1100_softc* sc=fd->sc;
    u_int32_t mask;

    switch (sc->remote_hw) {
    case sis1100_hw_vme:
        if (fd->owned_irqs & SIS3100_IRQS) {
            sis3100writeremreg(sc, vme_irq_sc,
                    (fd->owned_irqs & SIS3100_IRQS)<<16, 0);
        }
        break;
    case sis1100_hw_camac:
        if (fd->owned_irqs & SIS5100_IRQS) {
            /*sis5100writeremreg(sc, vme_irq_sc,
                    (fd->owned_irqs & SIS5100_IRQS)<<16, 0);*/
        }
        break;
    case sis1100_hw_pci: break; /* do nothing */
    case sis1100_hw_lvd: break; /* do nothing */
    case sis1100_hw_pandapixel: break; /* do nothing */
    case sis1100_hw_invalid: break; /* do nothing */
    case sis1100_hw_psf4ad: break; /* do nothing */
    }

    if ((sc->demand_dma.status!=dma_invalid) &&
                (sc->demand_dma.owner==fd)) {
        if (sc->demand_dma.status==dma_running)
            sis1100_ddma_stop(sc, fd, 0);
        sis1100_ddma_map(sc, fd, 0);
    }

    mask=0;
    if (fd->owned_irqs & SIS1100_FRONT_IRQS) {
        mask|=(fd->owned_irqs & SIS1100_FRONT_IRQS)>>4;
    }
    if (fd->owned_irqs & SIS1100_MBX0_IRQ) {
        mask|=irq_mbx0;
    }
    if (mask)
        sis1100_disable_irq(sc, 0, mask);
}

int
sis1100_release(struct inode *inode, struct file *file)
{
    struct sis1100_softc* sc=SIS1100SC(file);
    struct sis1100_fdata* fd=SIS1100FD(file);
    unsigned int _minor=iminor(inode);
    unsigned int idx=_minor&(sis1100_MINORUTMASK);

    cleanup_fd(fd);

    mutex_lock(&sc->sem_fdata);
    sc->fdata[idx]=0;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
    put_pid(fd->pid);
#endif
    kfree(fd);
    mutex_unlock(&sc->sem_fdata);

    return 0;
}
