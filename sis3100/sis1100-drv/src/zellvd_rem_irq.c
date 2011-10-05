/* $ZEL: zellvd_rem_irq.c,v 1.10 2009/06/05 16:02:36 wuestner Exp $ */

/*
 * Copyright (c) 2005-2009
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

u_int32_t
zellvd_rem_irq_handler(struct sis1100_softc* sc, u_int32_t doorbell)
{
    return doorbell & ~sc->pending_irqs;
}

/*
   sis1100_ddma_handler is here because it is used only by the 
   ZEL LVD DAQsystem.
   But it could also be used by any other front end.
   Then there is the problem that we need a bit in sc->pending_irqs in order
   to let 'irq_pending' return TRUE.
   Here we use bit 30 of the IRQ word. Is it free in other cases too?
*/
u_int32_t
sis1100_ddma_handler(struct sis1100_softc* sc)
{
    return ZELLVD_DDMA_IRQ;
}

void
zellvd_rem_enable_irqs(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    u_int32_t mask)
{
}

void
zellvd_rem_disable_irqs(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    u_int32_t mask)
{
}

void
zellvd_rem_irq_ack(struct sis1100_softc* sc, u_int32_t irqs)
{
}
