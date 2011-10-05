/* $ZEL: sis1100_irq.c,v 1.23 2010/01/18 11:15:31 wuestner Exp $ */

/*
 * Copyright (c) 2001-2007
 * 	Matthias Drochner, Peter Wuestner.  All rights reserved.
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

/* define PLXIRQ_DEBUG as 0 or 1 (should normally be 0) */
#define PLXIRQ_DEBUG 0

#if (PLXIRQ_DEBUG)
#   define PLXIRQ_BRAKE 100
#   define pPLXIRQ(sc, fmt, arg...) pERROR(sc, fmt, ## arg)
#else
#   undef PLXIRQ_BRAKE
#   define pPLXIRQ(sc, fmt, arg...) do {} while(0)
#endif

void
sis1100_dump_irqs(struct sis1100_softc* sc, const char* text)
{
    char s[128];
    int pos=0, len;

    if (sc->got_irqs&got_dma0) {
        len=sprintf(s+pos, " dma0");
        pos+=len;
    }
    if (sc->got_irqs&got_dma1) {
        len=sprintf(s+pos, " dma1");
        pos+=len;
    }
    if (sc->got_irqs&got_end) {
        len=sprintf(s+pos, " end");
        pos+=len;
    }
    if (sc->got_irqs&got_eot) {
        len=sprintf(s+pos, " eot");
        pos+=len;
    }
    if (sc->got_irqs&got_xoff) {
        len=sprintf(s+pos, " xoff");
        pos+=len;
    }
    if (sc->got_irqs&got_sync) {
        len=sprintf(s+pos, " sync");
        pos+=len;
    }
    if (sc->got_irqs&got_l_err) {
        len=sprintf(s+pos, " l_err");
        pos+=len;
    }
    pERROR(sc, "%s got IRQs:%s", text, s);
}

int
sis1100_enable_irq(struct sis1100_softc* sc,
    u_int32_t plx_mask, u_int32_t sis_mask)
{
    DECLARE_SPINLOCKFLAGS(flags)

    pPLXIRQ(sc, "enable_irq plx: %08x sis: %08x", plx_mask, sis_mask);
    if (plx_mask) {
        SPIN_LOCK_IRQSAVE(sc->lock_intcsr, flags);
        plxwritereg(sc, INTCSR, plxreadreg(sc, INTCSR)|plx_mask);
        SPIN_UNLOCK_IRQRESTORE(sc->lock_intcsr, flags);
    }

    if (sis_mask) {
        sis_mask&=sis1100_all_irq;
        sis1100writereg(sc, sr, sis_mask); /* clear pending irqs */
        sis1100writereg(sc, cr, sis_mask); /* enable irqs */
    }
    return 0;
}

int
sis1100_disable_irq(struct sis1100_softc* sc,
    u_int32_t plx_mask, u_int32_t sis_mask)
{
    DECLARE_SPINLOCKFLAGS(flags)

    pPLXIRQ(sc, "disable_irq plx: %08x sis: %08x", plx_mask, sis_mask);
    if (plx_mask) {
        SPIN_LOCK_IRQSAVE(sc->lock_intcsr, flags);
        plxwritereg(sc, INTCSR, plxreadreg(sc, INTCSR)&~plx_mask);
        SPIN_UNLOCK_IRQRESTORE(sc->lock_intcsr, flags);
    }

    if (sis_mask)
        sis1100writereg(sc, cr, (sis_mask&sis1100_all_irq)<<16);
    return 0;
}

/* Doorbell | Local | DMA0 | DMA1 */
#define HANDLED_IRQS (plxirq_doorbell_active|plxirq_local_active|\
                      plxirq_dma0_active|plxirq_dma1_active)

#ifdef __NetBSD__
int
sis1100_intr(void* vsc)
#elif __linux__
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
irqreturn_t
sis1100_intr(int irq, void *vsc, struct pt_regs *regs)
#else
irqreturn_t
sis1100_intr(int irq, void *vsc)
#endif
#endif
{
    DECLARE_SPINLOCKFLAGS(flags)
    struct sis1100_softc* sc=(struct sis1100_softc*)vsc;
    u_int32_t intcsr;
    struct timespec now={0, 0};
    int local, handler_command, wakeup_local;
#ifdef PLXIRQ_BRAKE
    static int emergency_count=0;
#endif

    intcsr=plxreadreg(sc, INTCSR);
    if (!(intcsr & HANDLED_IRQS)) {
        /*pPLXIRQ(sc, "NIX IRQ! intcsr=0x%x", intcsr);*/
        return IRQ_NONE;
    } else {
        pPLXIRQ(sc, "HAB IRQ! intcsr=0x%x", intcsr);
    }

#ifdef PLXIRQ_BRAKE
    if (++emergency_count>PLXIRQ_BRAKE) {
        sis1100_disable_irq(sc, plxirq_pci, 0);
        pERROR(sc, "irqs disabled after %d", PLXIRQ_BRAKE);
    }
#endif

    getnstimeofday(&now);

    local=0;
    handler_command=0;
    wakeup_local=0;

    if (intcsr&plxirq_doorbell_active) { /* == VME/CAMAC IRQ */
        u_int32_t help;
        int i;

        help=plxreadreg(sc, L2PDBELL);
        plxwritereg(sc, L2PDBELL, help);

        pPLXIRQ(sc, "plxirq: doorbell=0x%x", help);
#if 0
        if (sc->broken.swap_doorbell) /* currently always false */
            help=SWAP32(help);
#endif
        SPIN_LOCK_IRQSAVE(sc->lock_doorbell, flags);
        sc->doorbell|=help;
        SPIN_UNLOCK_IRQRESTORE(sc->lock_doorbell, flags);
        for (i=0; help; i++) {
            if (help&1)
                sc->irqtimes[i]=now;
            help>>=1;
        }
        handler_command|=handlercomm_doorbell;
    }

    if (intcsr&plxirq_local_active) { /* local Interrupt */
        local=1;
    }
    if (intcsr&plxirq_dma0_active) { /* DMA0 Interrupt */
        pPLXIRQ(sc, "plxirq: dma0");
        SPIN_LOCK_IRQSAVE(sc->lock_intcsr, flags);
        plxwritereg(sc, INTCSR, intcsr&~plxirq_dma0);
        SPIN_UNLOCK_IRQRESTORE(sc->lock_intcsr, flags);
        if (sc->plxirq_dma0_hook!=0) {
            pPLXIRQ(sc, "will call hook %p", sc->plxirq_dma0_hook);
            sc->plxirq_dma0_hook(sc, now);
        } else {
            sc->got_irqs|=got_dma0;
            wakeup_local=1;
        }
    }
    if (intcsr&plxirq_dma1_active) { /* DMA1 Interrupt */
        pPLXIRQ(sc, "plxirq: dma1");
        SPIN_LOCK_IRQSAVE(sc->lock_intcsr, flags);
        plxwritereg(sc, INTCSR, intcsr&~plxirq_dma1);
        SPIN_UNLOCK_IRQRESTORE(sc->lock_intcsr, flags);
        sc->got_irqs|=got_dma1;
        wakeup_local=1;
    }
    if (local) {
        u_int32_t status;

        status=sis1100readreg(sc, sr);
        pPLXIRQ(sc, "irq: status=0x%08x", status);
        if (status&irq_synch_chg) {
            sis1100_disable_irq(sc, 0, irq_synch_chg|
                irq_reset_req|irq_prot_end|irq_prot_l_err);

            sc->got_irqs|=got_sync;
            wakeup_local=1;

            if ((status&sr_synch)==sr_synch) {
                pINFO(sc, "link is UP   status =0x%08x", status);
            } else {
                pINFO(sc, "link is DOWN status =0x%08x", status);
                sc->remote_hw=sis1100_hw_invalid;
                /* There is no special code for 'handlercomm_down' in
                   sis1100_irq_thread.
                   But the link status change is always checked und
                   signals are sent out */
                handler_command|=handlercomm_down;
            }
#ifdef __NetBSD__
            callout_reset(&sc->link_up_timer, hz, sis1100_link_up_handler, sc);
#elif __linux__
            mod_timer(&sc->link_up_timer, jiffies+HZ);
#endif
        }
        if (status&irq_inh_chg) {
            pERROR(sc, "INH_CHG");
        }
        if (status&irq_sema_chg) {
            pERROR(sc, "SEMA_CHG");
        }
        if (status&irq_rec_violation) {
            pERROR(sc, "REC_VIOLATION");
        }
        if (status&irq_reset_req) {
            pERROR(sc, "RESET_REQ");
        }
        if (status&irq_dma_eot) {
            pPLXIRQ(sc, "localirq: dma_eot");
            sc->got_irqs|=got_eot;
            wakeup_local=1;
        }
        if (status&irq_mbx0) {
            sc->mbx0=sis1100readreg(sc, mailext[0]);
            pPLXIRQ(sc, "localirq: mailext[0]=0x%x", sc->mbx0);
            handler_command|=handlercomm_mbx0;
        }
#ifdef NEVER
        /* xoff and xon are handled by the FPGA code itself
         * the driver should do nothing. Only counting for statistic purposes
         * is of any interest
         */
        if (status&irq_s_xoff) {
            pINFO(sc, "S_XOFF");
            pINFO(sc, "status=0x%08x", status);
            /* count it */
        }
#endif
        if (status&irq_lemo_in_chg) {
            pPLXIRQ(sc, "localirq: lemo_in_chg, status=0x%08x", status);
            SPIN_LOCK_IRQSAVE(sc->lock_lemo_status, flags);
            sc->lemo_status|=status&irq_lemo_in_chg;
            SPIN_UNLOCK_IRQRESTORE(sc->lock_lemo_status, flags);
            sc->last_opt_csr=sis1100readreg(sc, opt_csr);
            handler_command|=handlercomm_lemo;
        }
        if (status&irq_prot_end) {
            pPLXIRQ(sc, "localirq: got_end");
            sc->got_irqs|=got_end;
            wakeup_local=1;
        }
        if (status&irq_prot_l_err) {
            pPLXIRQ(sc, "localirq: prot_l_err");
            /*sis1100writeremreg(sc, sr, irq_prot_l_err, 0);*/
            sc->got_irqs|=got_l_err;
            wakeup_local=1;
        }
        sis1100writereg(sc, sr, status);
    }

    if (wakeup_local)
        wakeup(&sc->local_wait);

    if (handler_command) {
        SPIN_LOCK_IRQSAVE(sc->handlercommand.lock, flags);
        sc->handlercommand.command|=handler_command;
        SPIN_UNLOCK_IRQRESTORE(sc->handlercommand.lock, flags);
        wake_up_process(sc->handler);
    }

    return IRQ_HANDLED;
}
