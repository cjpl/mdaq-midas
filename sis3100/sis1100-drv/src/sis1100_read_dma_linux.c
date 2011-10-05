/* $ZEL: sis1100_read_dma_linux.c,v 1.17 2010/01/18 18:50:14 wuestner Exp $ */

/*
 * Copyright (c) 2001-2008
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

int
_sis1100_read_dma(
    struct sis1100_softc* sc,
    struct sis1100_fdata* fd,
    u_int32_t addr,           /* VME or SDRAM address */
    int32_t am,               /* address modifier, not used if <0 */
    int size,                 /* datasize must be 4, 2 or 1 */
    int space,                /* remote space (1,2: VME; 6: SDRAM) */
    int fifo_mode,
    size_t count,             /* words to be transferred */
                              /* count==0 is illegal */
    size_t* count_read,       /* words transferred */
    u_int8_t __user *data,    /* destination (user virtual address) */
    int* prot_error,          /* protocol error (or 0) */
    int* may_be_more          /* more data may be available */
    )
{
    int res, aborted=0;
    u_int32_t head, dmamode;
    sigset_t oldset;

#if 0
    pERROR(sc, "read_dma size=%d, fifo=%d num=%llu am=%02x addr=%08x",
            size, fifo_mode, (unsigned long long)count, am, addr);
#endif
    /* check whether DMA channel is idle */
    {
        u_int32_t val;
        val=plxreadreg(sc, DMACSR0_DMACSR1);
        if (!(val&0x10)) {
            pERROR(sc, "read_dma: DMACSR0=%04x old DMA still active", val);
            return EIO;
        }
    }

    /* we need 32 bit for each word regardless of its size */
    if (count>DMA_MAX/4)
        count=DMA_MAX/4;
    dmamode=0x43|(1<<7)|(1<<8)|(1<<10)|(1<<11)|(1<<12)|(1<<14)|(1<<17);
    plxwritereg(sc, DMAPADR0, cpu_to_le32(sc->dmaspace.dma_handle&0xffffffff));
    plxwritereg(sc, DMADAC0, cpu_to_le32((u64)sc->dmaspace.dma_handle>>32));
    plxwritereg(sc, DMALADR0, 0);
    plxwritereg(sc, DMASIZ0, cpu_to_le32(sc->dmaspace.size));
    plxwritereg(sc, DMADPR0, cpu_to_le32(8));

/*XXX cpu_to_le32 oder implicit in plxwritereg???*/
/* prepare PLX */
    plxwritereg(sc, DMACSR0_DMACSR1, 1<<3); /* clear irq */
    plxwritereg(sc, DMAMODE0, dmamode);

/* prepare add on logic */
    /* BT, EOT, start with t_adl, DMA0 */
    head=0x0080A002;
    head|=(space&0x3f)<<16;
    head|=((0x00f00000<<size)&0x0f000000)<<(addr&3);
    if (am>=0) {
        head|=0x800;
        sis1100writereg(sc, t_am, am);
    }
    if (fifo_mode)
        head|=0x4000;
    sis1100writereg(sc, t_hdr, head);
    wmb();

    sis1100writereg(sc, t_dal, count*size);
    sis1100writereg(sc, d0_bc, 0);
    sis1100writereg(sc, d0_bc_buf, 0);

    sis1100writereg(sc, p_balance, 0);
    sis1100writereg(sc, sr, 0x200); /* clear EOT */

/* block signals */
    spin_lock_irq(&current->SIGMASK_LOCK);
    oldset = current->blocked;
    sigfillset(&current->blocked);
    sigdelset(&current->blocked, SIGKILL);
    /* dangerous, should be removed later */
    /*if (!sigismember(&oldset, SIGINT)) sigdelset(&current->blocked, SIGINT);*/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
    recalc_sigpending(current);
#else
    recalc_sigpending();
#endif
    spin_unlock_irq(&current->SIGMASK_LOCK);

/* enable dma */
    plxwritereg(sc, DMACSR0_DMACSR1, 3);

/* enable irq */
    sc->got_irqs=0;
    sis1100_enable_irq(sc, plxirq_dma0, irq_synch_chg|irq_prot_l_err);

/* start transfer */
    mb();
    sis1100writereg(sc, t_adl, addr);
    wmb();

/* wait for dma */
    res=wait_event_interruptible(
	sc->local_wait,
	(sc->got_irqs & (got_dma0|got_sync|got_l_err))
	);
    sis1100_disable_irq(sc, plxirq_dma0, irq_prot_l_err);

    if (sc->got_irqs&(got_dma0|got_l_err)) { /* transfer complete or error */
        u_int32_t bytes_read;
        /* d0_bc contains the number of bytes read (not words) */
        bytes_read=sis1100readreg(sc, d0_bc);
        *count_read=bytes_read/4;
        *may_be_more=*count_read==count;
    } else /*(res||(sc->got_irqs&(got_sync)))*/ { /* fatal */
        aborted=0x300;
        if (res) {
            pINFO(sc, "read_dma: interrupted");
            aborted|=1;
        }
        if (sc->got_irqs&got_sync) {
            pWARNING(sc, "read_dma: synchronisation lost");
            aborted|=2;
        }
        if (aborted==0x300) {
            pWARNING(sc, "read_dma: got_irqs=0x%x", sc->got_irqs);
        }
    }
    if (!(sc->got_irqs&got_dma0)) {
        u_int32_t val;
        int count=100000;
        val=plxreadreg(sc, DMACSR0_DMACSR1);
        if (!(val&0x10)) { /* DMA not stopped yet; abort it */
            sis1100writereg(sc, sr, sr_abort_dma);
            do {
                cpu_relax();
                val=plxreadreg(sc, DMACSR0_DMACSR1);
            } while (!(val&0x10) && --count);
            //pINFO(sc, "read_dma: abort DMA, count=%d", count);
        }
    }

    plxwritereg(sc, DMACSR0_DMACSR1, 0);

    spin_lock_irq(&current->SIGMASK_LOCK);
    current->blocked = oldset;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
    recalc_sigpending(current);
#else
    recalc_sigpending();
#endif
    spin_unlock_irq(&current->SIGMASK_LOCK);

    *prot_error=sis1100readreg(sc, prot_error);

    if (aborted) sis1100_flush_fifo(sc, 0);
    if (aborted) *prot_error=aborted;
    if ((*prot_error!=0) && ((*prot_error&0x200)!=0x200)) {
        res=EIO;
    }

    if (*count_read) {
        u_int32_t *caddr=sc->dmaspace.cpu_addr;
        int i;

        switch (size) {
        case 4:
            if (copy_to_user(data, caddr, *count_read*4))
                res=EFAULT;
            break;
        case 2:
            for (i=*count_read; i; i--, data+=2, caddr++)
                __put_user(*caddr&0xffff, (u_int16_t*)data);
            break;
        case 1:
            if (fifo_mode) {
                int shift=(3-(addr&3))*8;
                for (i=*count_read; i; i--, data++, caddr++) {
                    __put_user((*caddr>>shift)&0xff, (u_int8_t*)data);
                }
            } else {
                for (i=*count_read; i; i--, data++, caddr++, addr++) {
                    int shift=(3-(addr&3))*8;
                    __put_user((*caddr>>shift)&0xff, (u_int8_t*)data);
                }
            }
            break;
        }
    }

    return res;
}
