/* $ZEL: sis1100_irq_thread.c,v 1.25 2010/01/24 21:04:26 wuestner Exp $ */

/*
 * Copyright (c) 2002-2009
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
 * This is a kernel thread which sleeps until it is waked up by 
 * doorbell irq, irq_synch_chg, irq_lemo_in_chg or link_up_timer;
 * It is responsible to inform the user process about VME interrupts,
 * CAMAC LAMs, interrupts caused by front panel IO and insertion or removal
 * of the optical link
 */

static void
_sis1100_irq_thread(struct sis1100_softc* sc, enum handlercomm command)
{
    u_int32_t new_irqs=0;
    int i;

    mutex_lock(&sc->sem_irqinfo);

    if (command&handlercomm_doorbell) {
        DECLARE_SPINLOCKFLAGS(flags)
        u_int32_t doorbell;

        SPIN_LOCK_IRQSAVE(sc->lock_doorbell, flags);
        doorbell=sc->doorbell;
        sc->doorbell=0;
        SPIN_UNLOCK_IRQRESTORE(sc->lock_doorbell, flags);

        switch (sc->remote_hw) {
        case sis1100_hw_vme:
            new_irqs|=sis3100rem_irq_handler(sc, doorbell);
            break;
        case sis1100_hw_camac:
            new_irqs|=sis5100rem_irq_handler(sc, doorbell);
            break;
        case sis1100_hw_pci:
            /* do nothing */
            break;
        case sis1100_hw_lvd:
            new_irqs|=zellvd_rem_irq_handler(sc, doorbell);
            break;
        case sis1100_hw_pandapixel:
            new_irqs|=pandapixel_rem_irq_handler(sc, doorbell);
            break;
        case sis1100_hw_psf4ad:
            /* do nothing */
        case sis1100_hw_invalid:
            /* do nothing */
            break;
        }
    }

    if (command&handlercomm_lemo) {
        new_irqs|=sis1100_lemo_handler(sc);
    }

    if (command&handlercomm_mbx0) {
        new_irqs|=sis1100_mbox0_handler(sc);
    }

    /* this is called from sis1100_link_up_handler for both
       'UP' and 'DOWN' of link one second after status change */
    if (command&handlercomm_up) {
        new_irqs|=sis1100_synch_handler(sc);
    }

    if (command&handlercomm_ddma) {
        new_irqs|=sis1100_ddma_handler(sc);
    }

    sc->pending_irqs|=new_irqs;

    mutex_unlock(&sc->sem_irqinfo);

    /* inform processes via signal if requested */
    mutex_lock(&sc->sem_fdata);
    for (i=0; i<sis1100_MINORUTMASK+1; i++) {
        if (sc->fdata[i]) {
            struct sis1100_fdata* fd=sc->fdata[i];
            if (fd->sig>0 && ((new_irqs & fd->owned_irqs)||
                              (fd->old_remote_hw!=sc->remote_hw))) {
                int res;
                /* XXXY muss raus */
                pERROR(sc, "irq_pending=%d pending_irqs=0x%x",
                        irq_pending(sc, fd, fd->owned_irqs),
                        sc->pending_irqs);
                pERROR(sc, "sig=%d new_irqs=0x%x owned_irqs=0x%x",
                        fd->sig, new_irqs, fd->owned_irqs);
                pERROR(sc, "old_remote_hw=%d remote_hw=%d",
                        fd->old_remote_hw, sc->remote_hw);
                /* XXXY muss raus */
                pERROR(sc, "send sig to %d", pid_nr(fd->pid));
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
                res=kill_proc(fd->pid, fd->sig, 1);
#else
                res=kill_pid(fd->pid, fd->sig, 1);
#endif
                if (res)
                    pINFO(sc, "send sig %d to %u: res=%d",
                            fd->sig, pid_nr(fd->pid), res);
            }
        }
    }
    mutex_unlock(&sc->sem_fdata);

    /* wake up processes waiting in sis1100_irq_wait or doing select */
#ifdef __NetBSD__
    wakeup(&sc->remoteirq_wait);
    selwakeup(&sc->sel);
#elif __linux__
    wake_up_interruptible(&sc->remoteirq_wait);
#endif
}

#ifdef __linux__
int
#else
void
#endif
sis1100_irq_thread(void* data)
{
    struct sis1100_softc* sc=(struct sis1100_softc*)data;
    enum handlercomm command;
    DECLARE_SPINLOCKFLAGS(flags);

#ifdef __linux__
#if LINUX_VERSION_CODE < 0x20600
    daemonize();
    snprintf(current->comm, sizeof(current->comm), "sis1100_%02d", sc->unit);
    SPIN_LOCK_IRQSAVE(current->sigmask_lock, flags);
    sigemptyset(&current->blocked);
    recalc_sigpending(current);
    SPIN_UNLOCK_IRQRESTORE(current->sigmask_lock, flags);
#endif
#endif /*__linux__*/

    while (1) {
#ifdef __NetBSD__
        tsleep(&sc->handler_wait, PCATCH, "thread_vmeirq", 0);
#elif __linux__
        /* prepare to sleep */
	__set_current_state(TASK_INTERRUPTIBLE);
        /* don't sleep if command!=0 */
        if (sc->handlercommand.command)
	    __set_current_state(TASK_RUNNING);
        else
            schedule();
#endif

        if (kthread_should_stop())
            return 0;

        SPIN_LOCK_IRQSAVE(sc->handlercommand.lock, flags);
        command=sc->handlercommand.command;
        sc->handlercommand.command=0;
        SPIN_UNLOCK_IRQRESTORE(sc->handlercommand.lock, flags);

#if 0
        pERROR(sc, "irq_thread: command=0x%x", command);
#endif

        _sis1100_irq_thread(sc, command);

#ifdef __linux__
	if (signal_pending (current)) {
	    SPIN_LOCK_IRQSAVE(current->SIGMASK_LOCK, flags);
	    flush_signals(current);
	    SPIN_UNLOCK_IRQRESTORE(current->SIGMASK_LOCK, flags);
	}
#endif /*__linux__*/

    }
#ifdef __linux__
    return 0;
#endif
}
