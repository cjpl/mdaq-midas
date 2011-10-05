/* $ZEL: sis5100rem_irq.c,v 1.5 2009/04/26 20:28:22 wuestner Exp $ */

/*
 * Copyright (c) 2004-2009
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
#include "sis5100_map.h"

u_int32_t
sis5100rem_irq_handler(struct sis1100_softc* sc, u_int32_t doorbell)
{
/*
 * sc->sem_irqinfo must be held by caller!
 */
    u_int32_t irqmask, _doorbell;

    /* reorder bits in doorbell to match camac_irq_sc, lam_enable and
       human logic */
    /* bit 0..22: LAMs, bit 24..32: IO and DSP */
    _doorbell  = doorbell       & 0x000000ff; /* LAM 1..8 */
    _doorbell |= (doorbell>>8)  & 0x007fff00; /* LAM 9..23 */
    _doorbell |= (doorbell<<16) & 0xf7000000; /* FLAT, LEMO and DSP */

    mutex_lock(&sc->sem_hw);

    /* block IRQs in CAMAC controller */
    sis5100readremreg(sc, lam_enable, &irqmask, 1);
    irqmask &= ~(_doorbell & SIS5100_LAM_IRQS);
    if (irqmask) {
        sis5100writeremreg(sc, lam_enable, irqmask, 1);
    }

    /* block and clear FRONT-IRQs in CAMAC controller*/
    irqmask = (_doorbell & SIS5100_EXT_IRQS);
    if (irqmask) {
        sis5100writeremreg(sc, in_latch_irq, irqmask | (irqmask >> 8), 1);
    }

    /* update irqs */
    sis5100writeremreg(sc, in_latch_irq, 1<<15, 1);

    mutex_unlock(&sc->sem_hw);

    return _doorbell&~sc->pending_irqs;;
}

void
sis5100rem_enable_irqs(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    u_int32_t mask)
{
    u_int32_t irqmask;

    mutex_lock(&sc->sem_hw);

    /* enable LAM IRQs */
    sis5100readremreg(sc, lam_enable, &irqmask, 1);
    irqmask |= mask & SIS5100_LAM_IRQS;
    if (irqmask) {
        sis5100writeremreg(sc, lam_enable, irqmask, 1);
    }

    /* enable FRONT-IRQs and DSP_IRQ */
    irqmask = mask & SIS5100_EXT_IRQS;
    if (irqmask) {
        /* clear latched bit */
        sis5100writeremreg(sc, in_latch_irq, irqmask, 1);
        /* enable irq */
        sis5100writeremreg(sc, in_latch_irq, irqmask>>24, 1);
    }
    /* update irqs */
    sis5100writeremreg(sc, in_latch_irq, 1<<15, 1);

    mutex_unlock(&sc->sem_hw);
}

void
sis5100rem_disable_irqs(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    u_int32_t mask)
{
    u_int32_t irqmask;

    mutex_lock(&sc->sem_hw);

    /* block IRQs in CAMAC controller */
    sis5100readremreg(sc, lam_enable, &irqmask, 1);
    irqmask &= ~(mask & SIS5100_LAM_IRQS);
    if (irqmask) {
        sis5100writeremreg(sc, lam_enable, irqmask, 1);
    }

    /* block and clear FRONT-IRQs in CAMAC controller*/
    irqmask = (mask & SIS5100_EXT_IRQS);
    if (irqmask) {
        sis5100writeremreg(sc, in_latch_irq, irqmask | (irqmask >> 8), 1);
    }

    mutex_unlock(&sc->sem_hw);
}

void
sis5100rem_irq_ack(struct sis1100_softc* sc, u_int32_t irqs)
{
    u_int32_t irqmask;

    mutex_lock(&sc->sem_hw);

    sis5100readremreg(sc, lam_enable, &irqmask, 1);
    irqmask |= irqs & SIS5100_LAM_IRQS;
    if (irqmask)
        sis5100writeremreg(sc, lam_enable, irqmask, 1);

    irqmask=irqs & SIS5100_EXT_IRQS;
    if (irqmask)
        sis5100writeremreg(sc, in_latch_irq, irqmask >> 24, 1);

    /* update irqs */
    sis5100writeremreg(sc, in_latch_irq, 1<<15, 1);

    mutex_unlock(&sc->sem_hw);
}
