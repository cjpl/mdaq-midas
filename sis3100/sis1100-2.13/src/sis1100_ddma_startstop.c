/* $ZEL: sis1100_ddma_startstop.c,v 1.11 2009/04/26 20:28:21 wuestner Exp $ */

/*
 * Copyright (c) 2005-2008
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

/* start_dma may be called from interrupt! */
static void
start_dma(struct sis1100_softc *sc, int block)
{
    DECLARE_SPINLOCKFLAGS(flags)
    struct demand_dma* dma = &sc->demand_dma;
    u_int32_t intcsr;

    SPIN_LOCK_IRQSAVE(dma->spin, flags);
    dma->block[block].seq_write_start=dma->debug_sequence;
    dma->block[block].status=dmablock_Xdma;
    SPIN_UNLOCK_IRQRESTORE(dma->spin, flags);

    /* clear IRQ */
    plxwritereg(sc, DMACSR0_DMACSR1, 1<<3);

    /* enable IRQ */
    SPIN_LOCK_IRQSAVE(sc->lock_intcsr, flags);
    intcsr=plxreadreg(sc, INTCSR);
    plxwritereg(sc, INTCSR, intcsr|plxirq_dma0);
    SPIN_UNLOCK_IRQRESTORE(sc->lock_intcsr, flags);

    /* restart DMA */
#if 0
    pERROR(sc, "start_dma: dmadpr0=%08x", dma->block[block].dmadpr0|9);
#endif
    plxwritereg(sc, DMADPR0, dma->block[block].dmadpr0|9);
    plxwritereg(sc, DMACSR0_DMACSR1, 3);
}


/* plxirq_dma0_hook is called from interrupt! */
static void
plxirq_dma0_hook(struct sis1100_softc *sc, struct timespec time)
{
    DECLARE_SPINLOCKFLAGS(flags)
    struct demand_dma* dma = &sc->demand_dma;
    int writing_block, next_block, used;

#if 0
    pERROR(sc, "plxirq_dma0_hook, writing_block=%d", dma->writing_block);
#endif
    SPIN_LOCK_IRQSAVE(dma->spin, flags);
    writing_block=dma->writing_block;
    next_block=writing_block+1;
    if (next_block>=dma->numblocks)
        next_block=0;
    dma->block[writing_block].status=dmablock_Xfull;
    dma->block[writing_block].time=time;
    dma->block[writing_block].seq_write_end=dma->debug_sequence;
    dma->debug_sequence++;
    used=dma->block[next_block].status!=dmablock_Xfree;
    dma->writing_block=next_block;
    SPIN_UNLOCK_IRQRESTORE(dma->spin, flags);

    if (used) {
#if 0
        pERROR(sc, "dma0_hook: next block (%d) is used (%d)", next_block,
            dma->block[next_block].status);
#endif
        SPIN_LOCK_IRQSAVE(dma->spin, flags);
        dma->is_blocked=1;
        SPIN_UNLOCK_IRQRESTORE(dma->spin, flags);
    } else {
        start_dma(sc, next_block);
    }

    /* let irq_thread inform the user process */
    /* dma_sync is done in sis1100_irq_get() */
    SPIN_LOCK_IRQSAVE(sc->handlercommand.lock, flags);
    sc->handlercommand.command|=handlercomm_ddma;
    SPIN_UNLOCK_IRQRESTORE(sc->handlercommand.lock, flags);
    wake_up_process(sc->handler);
}

int
sis1100_ddma_start(struct sis1100_softc *sc, struct sis1100_fdata* fd)
{
    struct demand_dma* dma = &sc->demand_dma;
    u_int32_t dmamode, dmadpr0;
    int i;

pERROR(sc, "ddma_start");

    mutex_lock(&dma->mux);
    /* is an other process using dma? */
    if ((dma->status!=dma_invalid) && (dma->owner!=fd)) {
        mutex_unlock(&dma->mux);
        return EPERM;
    }
    if (dma->status==dma_running) {
        mutex_unlock(&dma->mux);
        return EALREADY;
    }
    if (dma->status!=dma_ready) {
        mutex_unlock(&dma->mux);
        return EINVAL;
    }

    /* declare all blocks as free */
    for (i=0; i<dma->numblocks; i++) {
        dma->block[i].status=dmablock_Xfree;
        dma->block[i].seq_write_start=-1;
        dma->block[i].seq_write_end=-1;
        dma->block[i].seq_signal=-1;
        dma->block[i].seq_free=-1;
    }
    dma->debug_sequence=0;
    dma->reading_block=-1;

    dma->is_blocked=0;
    dma->writing_block=0;
    dma->block[0].status=dmablock_Xdma;

    /* prepare PLX */
    dmamode=3     /* bus width 32 bit */
        |(1<<6)   /* enabe TA#/READY# input */
        |(1<<7)   /* enable BTERM# input */
        |(1<<8)   /* enable loacal burst */
        |(1<<9)   /* scatter/gather mode */
        |(1<<10)  /* enabe "done interrupt" */
        |(1<<11)  /* local address is constant */
        |(1<<12)  /* demand mode */
        |(1<<17); /* routing DMA interrupt to PCI bus */
        /*|(1<<14)*/  /* enable DMA EOT# */
        /* EOT stops DMA after the first event! */
        if (sc->using_dac)
            dmamode|=(1<<18);

    plxwritereg(sc, DMACSR0_DMACSR1, 1<<3); /* clear irq */
    plxwritereg(sc, DMAMODE0, dmamode);
    dmadpr0=dma->block[0].dmadpr0|9;
pERROR(sc, "dma_start: DMADPR0 <-- 0x%08x", dmadpr0);
    plxwritereg(sc, DMADPR0, dmadpr0);

    sc->plxirq_dma0_hook=plxirq_dma0_hook;

    /* enable dma irq in plx chip */
    sis1100_enable_irq(sc, plxirq_dma0, 0);

    /* clear byte counter */
    sis1100writereg(sc, d0_bc, 0);
#if 0
    /* unnecessary */
    lvd_writeremreg(sc, ddt_counter, 0, 0);
#endif

    /* enable remote access */
    sis1100writereg(sc, cr, cr_ready);
    mb_plx();

    /* enable and start dma transfer */
    pDEBUG(sc, "dma_start: Start DMA transfer");
    plxwritereg(sc, DMACSR0_DMACSR1, 3);

    dma->status=dma_running;
    mutex_unlock(&dma->mux);

    return 0;
}

int
sis1100_ddma_stop(struct sis1100_softc *sc, struct sis1100_fdata* fd,
        struct sis1100_ddma_stop* d)
{
    struct demand_dma* dma=&sc->demand_dma;

pERROR(sc, "ddma_stop");

    mutex_lock(&dma->mux);
    /* is an other process using dma? */
    if ((dma->status!=dma_invalid)&&(dma->owner!=fd)){
        mutex_unlock(&dma->mux);        
        return EPERM;
    }
    if (dma->status!=dma_running) {
        mutex_unlock(&dma->mux);        
        return EINVAL;
    }

    sis1100_disable_irq(sc, plxirq_dma0, 0);
    sc->plxirq_dma0_hook=0;

    /* enable DMA EOT# */
    plxwritereg(sc, DMAMODE0, plxreadreg(sc, DMAMODE0)|(1<<14));
    if (!(plxreadreg(sc, DMACSR0_DMACSR1)&0x10)) {
        int c=0;
        sis1100writereg(sc, sr, sr_abort_dma);
        while (!(plxreadreg(sc, DMACSR0_DMACSR1)&0x10) && (c++<1000000)) {}
        if (!(plxreadreg(sc, DMACSR0_DMACSR1)&0x10)) {
	    pERROR(sc, "dma_stop: DMA NOT STOPED");
            mutex_unlock(&dma->mux);        
	    return EIO;
        }
    }
    /* disable remote access */
    sis1100writereg(sc, cr, cr_ready<<16);
    /*
    {
        size_t _bytes;
        _bytes=sis1100readreg(sc, byte_counter);
        pINFO(sc, "dma_stop: byte_count=%u", _bytes);
        if (bytes)
            *bytes=_bytes;
    }
    */

    sis1100_flush_fifo(sc, 1);

    dma->status=dma_ready;
    mutex_unlock(&dma->mux);

    return 0;
}

int
sis1100_ddma_wait(struct sis1100_softc *sc, struct sis1100_fdata* fd,
        unsigned int* block)
{
    struct demand_dma* dma=&sc->demand_dma;
    int res;

#if 0
    pERROR(sc, "ddma_wait");
#endif

    mutex_lock(&dma->mux);

    /* are we the owner and DMA is running? */
    if ((dma->status!=dma_running) || (dma->owner!=fd)) {
        mutex_unlock(&dma->mux);
        return EINVAL;
    }

    sc->got_irqs=0;

    if ((plxreadreg(sc, DMACSR0_DMACSR1)&0x10)) {
        goto finished;
    }

#ifdef __NetBSD__
    while (!(res||(sc->got_irqs&(got_dma0)))) {
        res = tsleep(&sc->local_wait, PCATCH, "dma_wait", 10*hz);
    }
#else
    res=wait_event_interruptible (
        sc->local_wait,
        (sc->got_irqs & got_dma0)
    );
#endif
    if (res) {
        mutex_unlock(&dma->mux);
        return EINTR;
    }

finished:

    sis1100_disable_irq(sc, plxirq_dma0, 0);

    /*
    {
        size_t _bytes;
        _bytes=sis1100readreg(sc, byte_counter);
        pDEBUG(sc, "dma_wait: byte_count=%u", _bytes);
        if (bytes)
            *bytes=_bytes;
    }
    */

    dma->status=dma_ready;
    mutex_unlock(&dma->mux);

    return 0;
}

int
sis1100_ddma_mark(struct sis1100_softc *sc, struct sis1100_fdata* fd,
        unsigned int* block)
{
    DECLARE_SPINLOCKFLAGS(flags)
    struct demand_dma* dma=&sc->demand_dma;
    int restart_block=-1;

    /* are we the owner and DMA is running? */
    if (dma->owner!=fd) {
        pERROR(sc, "ddma_mark: not owner");
        return EPERM;
    }
    if (dma->status!=dma_running) {
        pERROR(sc, "ddma_mark: not running");
        return ESRCH;
    }

    if (*block>=dma->numblocks) {
        pERROR(sc, "ddma_mark: block %d is invalid", *block);
        return EINVAL;
    }

#if 0
    pERROR(sc, "ddma_mark: mark %d", *block);
#endif
    SPIN_LOCK_IRQSAVE(dma->spin, flags);
    if (dma->block[*block].status!=dmablock_Xuser) {
        SPIN_UNLOCK_IRQRESTORE(dma->spin, flags);
        pERROR(sc, "ddma_mark: block %d not used!", *block);
        return EINVAL;
    }
    dma->block[*block].status=dmablock_Xfree;
    if ((dma->is_blocked) && (*block==dma->writing_block)) {
        restart_block=*block;
        dma->is_blocked=0;
    }
    SPIN_UNLOCK_IRQRESTORE(dma->spin, flags);

    if (restart_block>=0) {
        start_dma(sc, restart_block);
    }

    return 0;
}
