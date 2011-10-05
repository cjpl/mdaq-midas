/* $ZEL: sis1100_write_dma_linux.c,v 1.13 2010/01/18 19:08:12 wuestner Exp $ */

/*
 * Copyright (c) 2001-2009
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
#include "sis3100_map.h"

int
_sis1100_write_dma(
    struct sis1100_softc* sc,
    struct sis1100_fdata* fd,
    u_int32_t addr,           /* VME or SDRAM address */
    int32_t am,               /* address modifier, not used if <0 */
    int size,                 /* datasize (1, 2 or 4) */
    int space,                /* remote space (1,2: VME; 6: SDRAM) */
    int fifo_mode,
    size_t count,             /* words to be transferred */
                              /* count==0 is illegal */
    size_t* count_written,    /* words transferred */
    const u_int8_t __user *data, /* source (user virtual address) */
    int* prot_error
    )
{
    int res, aborted=0;
    u_int32_t head, tmp, dmamode;
    sigset_t oldset;

#if 0
    pERROR(sc, "write_dma size=%d fifo=%d num=%llu addr=%08x",
        size, fifo_mode, (unsigned long long)count, addr);
#endif

    /* check whether DMA channel is idle */
    {
        u_int32_t val;
        val=plxreadreg(sc, DMACSR0_DMACSR1);
        if (!(val&0x10)) {
            int count=100000;
            pERROR(sc, "write_dma: DMACSR0=%04x, old DMA still active", val);
            /* try to kill DMA */
            plxwritereg(sc, DMACSR0_DMACSR1, 1);
            plxwritereg(sc, DMACSR0_DMACSR1, 4);
            do {
                val=plxreadreg(sc, DMACSR0_DMACSR1);
            } while (!(val&0x10) && --count);
            if (!(val&0x10)) {
                pERROR(sc, "write_dma: DMACSR0=%04x, cannot kill oldDMA", val);
                return EIO;
            }
        }
    }

    /* we need 32 bit for each word regardless of its size */
    if (count>DMA_MAX/4)
        count=DMA_MAX/4;

    /* because bit31 is used for distinction between DMA and nonDMA
       we cannot cross the 2 GiByte boundary */
    if ((addr^(addr+count*size))&0x80000000U)
        count=0x80000000U-addr;
    /* will be corrected if an error occurs */
    *count_written=count;

    switch (size) {
    case 4:
        if (copy_from_user(sc->dmaspace.cpu_addr, data, count*size))
            return EFAULT;
        break;
    case 2: {
            u_int16_t val;
            u_int32_t *caddr=sc->dmaspace.cpu_addr;
            int i;
            for (i=count; i; i--, data+=2, caddr++) {
                __get_user(val, (u_int16_t*)data);
                *caddr=val|(val<<16);
            }
        }
        break;
    case 1: {
            u_int8_t val;
            u_int32_t *caddr=sc->dmaspace.cpu_addr;
            int i;
            for (i=count; i; i--, data++, caddr++) {
                __get_user(val, (u_int8_t*)data);
                *caddr=val;
                *caddr|=*caddr<<8;
                *caddr|=*caddr<<16;
            }
        }
        break;
    }

    dmamode=0x43|(1<<7)|(1<<8)|(1<<10)|(1<<14)|(1<<17);
    plxwritereg(sc, DMAPADR0, cpu_to_le32(sc->dmaspace.dma_handle&0xffffffff));
    plxwritereg(sc, DMADAC0, cpu_to_le32((u64)sc->dmaspace.dma_handle>>32));
    plxwritereg(sc, DMALADR0, cpu_to_le32(addr&0x7ffffffcU));
    /*plxwritereg(sc, DMASIZ0, cpu_to_le32(sc->dmaspace.size));*/
    plxwritereg(sc, DMASIZ0, cpu_to_le32(count*4));
    plxwritereg(sc, DMADPR0, 0);

/* prepare PLX */
    plxwritereg(sc, DMACSR0_DMACSR1, 1<<3); /* clear irq */
    plxwritereg(sc, DMAMODE0, dmamode);

/* prepare add on logic */
    /* local space 2, BT, start with t_adl */
    head=0x00802402|(space&0x3f)<<16;
    head|=(space&0x3f)<<16;
    head|=((0x00f00000<<size)&0x0f000000)<<(addr&3);
    if (am>=0) {
        head|=0x800;
        sis1100writereg(sc, d_am, am);
    }
    if (fifo_mode)
        head|=0x4000;
    sis1100writereg(sc, d_hdr, head);
    wmb();

#if 0
    pERROR(sc, "head    =%08x count=%llu", head, (unsigned long long)count);
    pERROR(sc, "DMAPADR0=%08x", plxreadreg(sc, DMAPADR0));
    pERROR(sc, "DMADAC0 =%08x", plxreadreg(sc, DMADAC0));
    pERROR(sc, "DMALADR0=%08x", plxreadreg(sc, DMALADR0));
    pERROR(sc, "DMASIZ0 =%08x", plxreadreg(sc, DMASIZ0));
    pERROR(sc, "DMADPR0 =%08x", plxreadreg(sc, DMADPR0));
    pERROR(sc, "DMAMODE0=%08x", plxreadreg(sc, DMAMODE0));
#endif
    sis1100writereg(sc, d_adl, addr); /* only bit 31 is valid */
    sis1100writereg(sc, d_bc, count*4);
    sis1100writereg(sc, p_balance, 0);

    spin_lock_irq(&current->SIGMASK_LOCK);
    oldset = current->blocked;
    sigfillset(&current->blocked);
    sigdelset(&current->blocked, SIGKILL);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
    recalc_sigpending(current);
#else
    recalc_sigpending();
#endif
    spin_unlock_irq(&current->SIGMASK_LOCK);

/* enable irq */
    /* irq_synch_chg and irq_prot_l_err should always be enabled */
    sis1100_enable_irq(sc, 0, irq_prot_l_err|irq_synch_chg|irq_prot_end);

/* start dma */
    sc->got_irqs=0;
    mb();
    plxwritereg(sc, DMACSR0_DMACSR1, 3);

/* wait for confirmation */
    res=wait_event_interruptible(
	sc->local_wait,
	(sc->got_irqs & (got_end|got_sync|got_l_err))
	);

#if 0
    pINFO(sc, "write_dma: got IRQs 0x%x", sc->got_irqs);
    sis1100_dump_irqs(sc, "write_dma");
#endif
    sis1100_disable_irq(sc, 0, irq_prot_end);

#if 0
    if (sc->got_irqs&got_l_err) {
        pERROR(sc, "irq_prot_l_err in write_dma, irqs=0x%04x",
            sc->got_irqs);
    }
#endif
    if (res|(sc->got_irqs&(got_sync))) {
        aborted=0x300;
        if (res) {
            pINFO(sc, "write_dma: interrupted");
            aborted|=1;
        }
        if (sc->got_irqs&got_sync) {
            pWARNING(sc, "write_dma: synchronisation lost");
            aborted|=2;
        }
    }

    spin_lock_irq(&current->SIGMASK_LOCK);
    current->blocked = oldset;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
    recalc_sigpending(current);
#else
    recalc_sigpending();
#endif
    spin_unlock_irq(&current->SIGMASK_LOCK);

    *prot_error=sis1100readreg(sc, prot_error);

    if (aborted) {
        *prot_error=aborted;
        res=EIO;
    } else if (*prot_error) {
        if (*prot_error&0x200) {
            u_int32_t addr;
            switch (sc->remote_hw) {
            case sis1100_hw_vme:
                addr = ofs(struct sis3100_reg, dma_write_counter);
                tmp=sis1100_remote_reg_read(sc, addr, &head, 1);
                *count_written=head/4;
                break;
            default:
                pWARNING(sc, "dma write error in non-VME slave");
                *count_written=0;
            }
        } else {
            res=EIO;
        }
    }

    if (aborted)
        sis1100_dump_glink_status(sc, "after abort", 1);

    {
        u_int32_t val;
        val=plxreadreg(sc, DMACSR0_DMACSR1);
        if (!(val&0x10)) {
            pERROR(sc, "after write_dma: DMACSR0=%04x, new DMA still active", val);
            return EIO;
        }
    }

    return res;
}
