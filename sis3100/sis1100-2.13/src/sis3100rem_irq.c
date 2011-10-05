/* $ZEL: sis3100rem_irq.c,v 1.15 2009/06/05 16:02:08 wuestner Exp $ */

/*
 * Copyright (c) 2001-2009
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
#include "sis3100_map.h"

static int
_sis3100_irq_acknowledge(struct sis1100_softc* sc, int level, int32_t *vector)
{
/*
 * sc->sem_hw must be held by caller!
 */
    u_int32_t error;

    if (level&1)
        sis1100writereg(sc, t_hdr, 0x0c010802);
    else
        sis1100writereg(sc, t_hdr, 0x03010802);
    sis1100writereg(sc, t_am, (1<<14)|0x3f);
    sis1100writereg(sc, t_adl, level<<1);
    do {
        error=sis1100readreg(sc, prot_error);
    } while (error==sis1100_e_dlock);
#if 0
    pERROR(sc, "3100: Iack(%d), error=0x%x, irqcount=%d, jiffies=%ld",
        level, error, sc->irqcount, jiffies);
#endif
    if (error) {
        pERROR(sc, "3100: error in Iack level %d: 0x%x", level, error);
        *vector=-1;
        return -1;
    } else {
        *vector=sis1100readreg(sc, tc_dal)&0xff;
        return 0;
    }
}

#if 0
/* for internal tests only */
int
sis3100_irq_acknowledge(struct sis1100_softc* sc, int level, int32_t *vector)
{
    int res;
    mutex_lock(&sc->sem_hw);
    res=_sis3100_irq_acknowledge(sc, level, vector);
    mutex_unlock(&sc->sem_hw);
    if (!res) {
        mutex_lock(&sc->sem_irqinfo);
        sc->irq_vects[level].vector=*vector;
        sc->irq_vects[level].time=sc->irqtimes[level];
        sc->irq_vects[level].Xvalid=1;
        mutex_unlock(&sc->sem_irqinfo);
    }
    return res;
}
#endif

u_int32_t
sis3100rem_irq_handler(struct sis1100_softc* sc, u_int32_t doorbell)
{
/*
 * sc->sem_irqinfo must be held by caller!
 */
    u_int32_t irqmask, new_irqs;
    int level;

    mutex_lock(&sc->sem_hw);

    new_irqs=doorbell&~sc->pending_irqs;

    /* block IRQs in VME controller*/
    irqmask=doorbell & SIS3100_VME_IRQS;
    if (irqmask) {
        sis3100writeremreg(sc, vme_irq_sc, irqmask<<16, 1);
    }

    /* obtain irq vectors from VME */
    for (level=7; level>0; level--) {
        if (new_irqs & (1<<level)) {
            int res;
            res=_sis3100_irq_acknowledge(sc, level,
                    &sc->irq_vects[level].vector);
            sc->irq_vects[level].time=sc->irqtimes[level];
            sc->irq_vects[level].valid=1;
        }
    }

    /* block and clear FRONT-IRQs in VME controller*/
    irqmask=doorbell & SIS3100_EXT_IRQS;
    if (irqmask) {
        sis3100writeremreg(sc, in_latch_irq, (irqmask<<8) | (irqmask<<16), 1);
    }

    /* update irqs */
    sis3100writeremreg(sc, vme_irq_sc, 1<<15, 1);

    mutex_unlock(&sc->sem_hw);

    return new_irqs;
}

void
sis3100rem_enable_irqs(struct sis1100_softc* sc,
        struct sis1100_fdata* fd, u_int32_t mask)
{
    u_int32_t irqmask;

    mutex_lock(&sc->sem_hw);

    /* enable VME IRQs */
    irqmask=mask&SIS3100_VME_IRQS;
    if (irqmask) {
        sis3100writeremreg(sc, vme_irq_sc, irqmask, 1);
    }
    /* enable VME-FRONT-IRQs and SIS3100_DSP_IRQ */
    irqmask=mask&SIS3100_EXT_IRQS;
    if (irqmask) {
        /* clear latched bit */
        sis3100writeremreg(sc, in_latch_irq, irqmask<<16, 1);
        /* enable irq */
        sis3100writeremreg(sc, in_latch_irq, irqmask>>8, 1);
    }
    /* update irqs */
    sis3100writeremreg(sc, vme_irq_sc, 1<<15, 1);

    mutex_unlock(&sc->sem_hw);
}

void
sis3100rem_disable_irqs(struct sis1100_softc* sc,
        struct sis1100_fdata* fd, u_int32_t mask)
{
    u_int32_t irqmask;

    mutex_lock(&sc->sem_hw);

    irqmask=mask&SIS3100_VME_IRQS;
    if (irqmask) {
        sis3100writeremreg(sc, vme_irq_sc, irqmask<<16, 1);
    }

    irqmask=mask&SIS3100_EXT_IRQS;
    if (irqmask) {
        sis3100writeremreg(sc, in_latch_irq, irqmask<<8, 1);
    }

    mutex_unlock(&sc->sem_hw);
}

void
sis3100rem_get_vector(struct sis1100_softc* sc, u_int32_t irqs,
                        struct sis1100_irq_get2* data)
{
/*
 * sc->sem_irqinfo must be held by caller!
 */
#if 0
    pERROR(sc, "get_vector: irqs=0x%08x", irqs);
#endif
    if (irqs & SIS3100_VME_IRQS) {
        int level;
        /* find highest level set */
        for (level=7; level>0; level--) {
            if ((1<<level) & irqs) {
                if (!sc->irq_vects[level].valid) {
                    pERROR(sc, "get_vector: irqs=0x%08x but level %d not valid",
                        irqs, level);
                    continue;
                }
                data->level=level;
                data->vector=sc->irq_vects[level].vector;
                /* overwrite time because we know it better */
                if (!(irqs&~0xff)) {
                    data->irq_sec=sc->irq_vects[level].time.tv_sec;
                    data->irq_nsec=sc->irq_vects[level].time.tv_nsec;
                }
#if 0
                pERROR(sc, "get_vector: vector=0x%08x, level=%d",
                        data->vector, level);
#endif
                /* is it correct to invalidate vector here or
                   should it be done in irq_ack? */
                sc->irq_vects[level].valid=0;
                break;
            }
        }
    }
}

void
sis3100rem_irq_ack(struct sis1100_softc* sc, u_int32_t irqs)
{
    u_int32_t irqmask;

    mutex_lock(&sc->sem_hw);

    irqmask=irqs & SIS3100_VME_IRQS;
    if (irqmask)
        sis3100writeremreg(sc, vme_irq_sc, irqmask, 1);

    irqmask=irqs & SIS3100_EXT_IRQS;
    if (irqmask)
        sis3100writeremreg(sc, in_latch_irq, irqmask>>8, 1);

    /* update irqs */
    sis3100writeremreg(sc, vme_irq_sc, 1<<15, 1);

    mutex_unlock(&sc->sem_hw);
}
