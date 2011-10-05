/* $ZEL: sis1100_synch_handler.c,v 1.11 2010/01/18 19:03:14 wuestner Exp $ */

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

#include "sis1100_sc.h"

/*
 * This function ist started by a timer (sc->link_up_timer) 1 second after
 * the optical link state has changed.
 * It is executed in irq context!
 * The awakened kernel thread (sis1100_irq_thread) calls
 * sis1100_synch_handler below
 */
void
#ifdef __NetBSD__
sis1100_link_up_handler(void* data)
#elif __linux__
sis1100_link_up_handler(unsigned long data)
#endif
{
    struct sis1100_softc* sc=(struct sis1100_softc*)data;
    DECLARE_SPINLOCKFLAGS(flags)

    SPIN_LOCK_IRQSAVE(sc->handlercommand.lock, flags);
    sc->handlercommand.command|=handlercomm_up;
    SPIN_UNLOCK_IRQRESTORE(sc->handlercommand.lock, flags);
#ifdef __NetBSD__
    wakeup(&sc->handler_wait);
#elif __linux__
    wake_up_process(sc->handler);
#endif
}

/*
 * this is called from _sis1100_irq_thread
 */
u_int32_t
sis1100_synch_handler(struct sis1100_softc* sc)
{
    u_int32_t status;

    sis1100_enable_irq(sc, 0, irq_synch_chg|irq_reset_req|irq_prot_l_err);
    status=sis1100readreg(sc, sr);

    if ((status&sr_synch)==sr_synch) {
        if (sc->remote_hw!=sis1100_hw_invalid) {
            pINFO(sc, "synch_handler: remote_hw=%d (not hw_invalid)",
                    sc->remote_hw);
        }
        sis1100_init_remote(sc, 0);
    }
    return 0;
}
