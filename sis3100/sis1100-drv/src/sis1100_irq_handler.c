/* $ZEL: sis1100_irq_handler.c,v 1.27 2010/01/18 18:48:03 wuestner Exp $ */

/*
 * Copyright (c) 2001-2004
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

#include "sis1100_sc.h"

/* define IRQ_DEBUG as 0 or 1 (should normally be 0) */
#define IRQ_DEBUG 0

static int
reserve_irq(struct sis1100_fdata* fd, u_int32_t mask)
{
    struct sis1100_softc* sc=fd->sc;
    int res=0, i;

    mutex_lock(&sc->sem_fdata);
#if 0
    /* XXXY muss raus */
    for (i=0; i<sis1100_MINORUTMASK+1; i++) {
        if (sc->fdata[i]) {
            struct sis1100_fdata* fdi=sc->fdata[i];
            pERROR(sc, "[%d] irqs owned by pid %u: 0x%08x%s",
                    i, pid_nr(fdi->pid), fdi->owned_irqs, sc->fdata[i]==fd?" *":"");
        }
    }
#endif
    for (i=0; i<sis1100_MINORUTMASK+1; i++) {
        if (sc->fdata[i] && sc->fdata[i]!=fd) {
            struct sis1100_fdata* fdi=sc->fdata[i];
            if (fdi->owned_irqs&mask) {
                if (fdi->pid==fd->pid) {
                    pWARNING(sc, "irq_ctl(pid=%u): "
                        "current process already ownes IRQs 0x%08x "
                        "but via different device file",
                        pid_nr(fd->pid),
                        fdi->owned_irqs&mask);
                } else {
                    pERROR(sc, "irq_ctl(pid=%u): "
                        "IRQs 0x%08x already owned by pid %u",
                        pid_nr(fd->pid),
                        fdi->owned_irqs&mask,
                        pid_nr(fdi->pid));
                    res=EBUSY;
                }
            }
        }
    }
    if (!res)
        fd->owned_irqs|=mask;

    mutex_unlock(&sc->sem_fdata);
    return res;
}

static void
release_irq(struct sis1100_fdata* fd, u_int32_t mask)
{
    struct sis1100_softc* sc=fd->sc;

    mutex_lock(&sc->sem_fdata);
    fd->owned_irqs &= ~mask;
    mutex_unlock(&sc->sem_fdata);
}

static int
_sis1100_irq_ack(struct sis1100_fdata* fd, u_int32_t irqs)
{
/*  sc->sem_irqinfo must be held by caller! */
    struct sis1100_softc* sc=fd->sc;

#if (IRQ_DEBUG)
    pERROR(sc, "_irq_ack: irqs=0x%08x", irqs);
#endif

    sc->pending_irqs&=~irqs;

    switch (sc->remote_hw) {
    case sis1100_hw_vme:
        sis3100rem_irq_ack(sc, irqs);
        break;
    case sis1100_hw_camac:
        sis5100rem_irq_ack(sc, irqs);
        break;
    case sis1100_hw_pci:
        /* do nothing */
        break;
    case sis1100_hw_lvd:
        if (irqs&0x3fffffff)
            zellvd_rem_irq_ack(sc, irqs);
        if (sc->demand_dma.status==dma_running) {
            /* set pending_irqs again if further DMA blocks are available */
            DECLARE_SPINLOCKFLAGS(flags)
            struct demand_dma* dma=&sc->demand_dma;
            int nextblock;

            SPIN_LOCK_IRQSAVE(dma->spin, flags);
            nextblock=dma->reading_block+1;
            if (nextblock>=dma->numblocks) nextblock=0;
            if (dma->block[nextblock].status==dmablock_Xfull) {
                sc->pending_irqs|=ZELLVD_DDMA_IRQ;
            }
            SPIN_UNLOCK_IRQRESTORE(dma->spin, flags);
        }
        break;
    case sis1100_hw_pandapixel:
        /* do nothing */
        break;
    case sis1100_hw_psf4ad:
        /* do nothing */
        break;
    case sis1100_hw_invalid:
        /* do nothing */
        break;
    }

    return 0;
}

int
sis1100_irq_ack(struct sis1100_fdata* fd, struct sis1100_irq_ack *data)
{
    struct sis1100_softc* sc=fd->sc;
    u_int32_t irqs;
    int res;

#if (IRQ_DEBUG)
    pERROR(sc, "_irq_ack: mask=0x%08x, owned_irqs=0x%08x, pending_irqs=0x%08x",
            data->irq_mask, fd->owned_irqs, sc->pending_irqs);
#endif
    mutex_lock(&sc->sem_irqinfo);
    irqs=fd->owned_irqs & data->irq_mask & sc->pending_irqs;
    res=_sis1100_irq_ack(fd, irqs);
    mutex_unlock(&sc->sem_irqinfo);

    return res;
}

int
sis1100_irq_ctl(struct sis1100_fdata* fd, struct sis1100_irq_ctl2* data)
{
    struct sis1100_softc* sc=fd->sc;
#if (IRQ_DEBUG)
    pINFO(sc, "irq_ctl(pid %u): signal=%d mask=0x%x",
            pid_nr(fd->pid), data->signal, data->irq_mask);
#endif

    if (data->signal) { /* reserve and enable the IRQs in data->irq_mask */
        int res;

        if ((res=reserve_irq(fd, data->irq_mask)))
            return res;

        mutex_lock(&sc->sem_irqinfo);
        res=_sis1100_irq_ack(fd, data->irq_mask);
        mutex_unlock(&sc->sem_irqinfo);
        if (res)
            return res;

        fd->sig=data->signal;
        fd->old_remote_hw=sc->remote_hw;
        sc->autoack_mask&=~data->irq_mask;
        sc->autoack_mask|=data->auto_mask&data->irq_mask;

        switch (sc->remote_hw) {
        case sis1100_hw_vme:
            sis3100rem_enable_irqs(sc, fd, data->irq_mask);
            break;
        case sis1100_hw_camac:
            sis5100rem_enable_irqs(sc, fd, data->irq_mask);
            break;
        case sis1100_hw_pci:     break; /* do nothing */
        case sis1100_hw_lvd:     break; /* nothing to be done */
        case sis1100_hw_pandapixel: break; /* nothing to be done ??? */
        case sis1100_hw_psf4ad:  break; /* nothing to be done */
        case sis1100_hw_invalid: break; /* do nothing */
        }
        /* the PCI-FRONT-IRQs are available only if remote side is VME */
        if (sc->remote_hw==sis1100_hw_vme) {
            /* enable PCI-FRONT-IRQs and MBX0_IRQ */
            u_int32_t mask=0;
            if (data->irq_mask & SIS1100_FRONT_IRQS) {
                mask|=(data->irq_mask & SIS1100_FRONT_IRQS)>>4;
            }
            if (data->irq_mask & SIS1100_MBX0_IRQ) {
                mask|=0x400;
            }
            if (mask)
                sis1100_enable_irq(sc, 0, mask);
        }
    } else { /* disable the IRQs in data->irq_mask */
        u_int32_t mask;
        int irqs;
        irqs=fd->owned_irqs & data->irq_mask;
        sc->autoack_mask&=~irqs;

        switch (sc->remote_hw) {
        case sis1100_hw_vme:
            sis3100rem_disable_irqs(sc, fd, irqs);
            break;
        case sis1100_hw_camac:
            sis5100rem_disable_irqs(sc, fd, irqs);
            break;
        case sis1100_hw_pci:     break; /* do nothing */
        case sis1100_hw_lvd:     break; /* do nothing */
        case sis1100_hw_pandapixel: break; /* do nothing ??? */
        case sis1100_hw_psf4ad:  break; /* do nothing */
        case sis1100_hw_invalid: break; /* do nothing */
        }
        if (sc->remote_hw==sis1100_hw_vme) {
            /* disable PCI-FRONT-IRQs and MBX0_IRQ */
            mask=0;
            if (irqs & SIS1100_FRONT_IRQS) {
                mask|=(irqs & SIS1100_FRONT_IRQS)>>4;
            }
            if (irqs & SIS1100_MBX0_IRQ) {
                mask|=irq_mbx0;
            }
            if (mask)
                sis1100_disable_irq(sc, 0, mask);
        }
        release_irq(fd, data->irq_mask);
    }
#if (IRQ_DEBUG)
    pINFO(sc, "irq_ctl: sig=%d owned_irqs=0x%x old_remote_hw=%d",
        fd->sig, fd->owned_irqs, fd->old_remote_hw);
#endif

    return 0;
}

int
sis1100_irq_get(struct sis1100_fdata* fd, struct sis1100_irq_get2* data)
{
    struct sis1100_softc* sc=fd->sc;
    u_int32_t mask;
    int res=0, i;

    mutex_lock(&sc->sem_irqinfo);

    data->irqs=sc->pending_irqs & fd->owned_irqs;
    if (fd->old_remote_hw!=sc->remote_hw) {
        if (sc->remote_hw!=sis1100_hw_invalid)
            data->remote_status=1;
        else
            data->remote_status=-1;
        fd->old_remote_hw=sc->remote_hw;
    } else {
        data->remote_status=0;
    }

    data->opt_status=sc->last_opt_csr;
    data->mbx0=sc->mbx0;

    data->level=0;
    data->vector=0;

    if (!data->irqs)
        goto exit;

    /* copy time of IRQ with highest bit in data->irqs */
    for (mask=data->irqs, i=31; !(mask&0x80000000); mask<<=1, i--);
    data->irq_sec=sc->irqtimes[i].tv_sec;
    data->irq_nsec=sc->irqtimes[i].tv_nsec;

    switch (sc->remote_hw) {
    case sis1100_hw_vme:
        if (data->irqs & SIS3100_VME_IRQS)
            sis3100rem_get_vector(sc, data->irqs & data->irq_mask, data);
        break;
    case sis1100_hw_lvd:
        if (sc->demand_dma.status==dma_running) {
            struct demand_dma* dma=&sc->demand_dma;
            int nextblock=dma->reading_block+1;
            if (nextblock>=dma->numblocks) nextblock=0;
            if (dma->block[nextblock].status==dmablock_Xfull) {
                /* synchronize dma buffer before user can access the data */
                pci_dma_sync_sg_for_cpu(sc->pdev,
                        dma->block[nextblock].table.sgl,
		        dma->block[nextblock].table.nents, DMA_FROM_DEVICE);
                data->vector=nextblock;
                data->irq_sec=dma->block[nextblock].time.tv_sec;
                data->irq_nsec=dma->block[nextblock].time.tv_nsec;
                dma->block[nextblock].status=dmablock_Xuser;
                dma->block[nextblock].seq_signal=dma->debug_sequence;
                dma->reading_block=nextblock;
            } else {
                pERROR(sc, "sis1100_irq_get: block %d is not ready for read",
                        nextblock);
                data->vector=-1;
            }
#if (IRQ_DEBUG)
            pERROR(sc, "sis1100_irq_get: vector=%d", data->vector);
#endif
        }
        break;
    default: /* do nothing */
        {}
    }

    mask=sc->autoack_mask&data->irqs;
    if (mask)
        res=_sis1100_irq_ack(fd, mask);

exit:
    mutex_unlock(&sc->sem_irqinfo);
    return res;
}

int
sis1100_irq_wait(struct sis1100_fdata* fd, struct sis1100_irq_get2* data)
{
    struct sis1100_softc* sc=fd->sc;
    int res=0;
#if (IRQ_DEBUG)
    pINFO(sc, "irq_wait: pending=0x%x mask=0x%x",
            sc->pending_irqs, data->irq_mask);
#endif
    res=wait_event_interruptible(sc->remoteirq_wait,
            irq_pending(sc, fd, data->irq_mask));

    if (res)
        return EINTR;

    res=sis1100_irq_get(fd, data);

    return res;
}
