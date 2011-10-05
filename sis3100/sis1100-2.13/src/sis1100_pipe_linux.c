/* $ZEL: sis1100_pipe_linux.c,v 1.8 2010/01/24 21:05:28 wuestner Exp $ */

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

int
sis1100_read_pipe(struct sis1100_softc* sc, struct sis1100_pipe* control)
{
    struct sis1100_pipelist* list;
    struct sis1100_dmabuf dmabuf;
    int i, error, balance=0, res=0, num;

    list=kmalloc(control->num*sizeof(struct sis1100_pipelist), GFP_USER);
    if (!list) return ENOMEM;

    if (copy_from_user(list, control->list,
    	    control->num*sizeof(struct sis1100_pipelist))) {
    	kfree(list);
    	return EFAULT;
    }

    /* how many bytes do we have to read? */
    dmabuf.size=0;
    for (i=0; i<control->num; i++) {
        /* XXX und wenn die Worte unterschiedlich lang sind? */
        if (!(list[i].head&0x400))
            dmabuf.size+=sizeof(u_int32_t);
    }

    if (dmabuf.size) {
        dmabuf.cpu_addr=pci_alloc_consistent(sc->pdev,
    	    dmabuf.size, &dmabuf.dma_handle);
        if (!dmabuf.cpu_addr) {
    	    kfree(list);
    	    return ENOMEM;
        }
    } else {
        dmabuf.size=0;
        dmabuf.dma_handle=0;
        dmabuf.cpu_addr=0;
    }

    mutex_lock(&sc->sem_hw);
    sis1100writereg(sc, rd_pipe_buf, dmabuf.dma_handle);
    sis1100writereg(sc, rd_pipe_blen, dmabuf.size);

    sis1100_disable_irq(sc, 0, irq_prot_end);

    sis1100writereg(sc, t_hdr, 0); /* avoid premature start */
    for (i=0; i<control->num; i++) {
    	u_int32_t head;

    	sis1100writereg(sc, t_am, list[i].am);
    	sis1100writereg(sc, t_adl, list[i].addr);

    	head=(list[i].head&0x0f3f0400) /* be, remote space and w/r */
                          |0x00400001; /* local space 1, am, start */
 
    	if (list[i].head&0x400) { /* write request */
    	    sis1100writereg(sc, t_dal, list[i].data);
	    head&=~0x00400000; /* no pipeline mode */
    	}
    	sis1100writereg(sc, t_hdr, head);
    }

    sc->got_irqs=0;
    sis1100_enable_irq(sc, 0, irq_prot_end);

    res=wait_event_interruptible(
            sc->local_wait,
            ((balance=sis1100readreg(sc, p_balance))==0)
        );

    sis1100_disable_irq(sc, 0, irq_prot_end);

    error=sis1100readreg(sc, prot_error);
    if (!balance) sis1100writereg(sc, p_balance, 0);

    if (error||res) {
    	/*printk(KERN_INFO
    	    "sis1100_read_pipe: error=0x%0x, res=%d\n", error, res);
	dump_glink_status(sc, "after pipe", 1);*/
    	sis1100_flush_fifo(sc, 1);
    }
    num=dmabuf.size-sis1100readreg(sc, rd_pipe_blen);
    mutex_unlock(&sc->sem_hw);

    control->error=error;
    control->num=num;

    kfree(list);
    if (dmabuf.size) {
        if (copy_to_user(control->data, dmabuf.cpu_addr, dmabuf.size)) {
    	    /*mem_map_unreserve(virt_to_page(dmabuf.cpu_addr));*/
    	    pci_free_consistent(sc->pdev, dmabuf.size,
    	        dmabuf.cpu_addr, dmabuf.dma_handle);
    	    res=EFAULT;
        }
        pci_free_consistent(sc->pdev, dmabuf.size, dmabuf.cpu_addr,
            dmabuf.dma_handle);
    }

    return res;
}

int
sis1100_read_pipe_seq(
    struct sis1100_softc* sc,
    struct sis1100_fdata* fd,
    u_int32_t addr,           /* VME or SDRAM address */
    int32_t am,               /* address modifier, not used if <0 */
    int size,                 /* wordsize (1, 2 or 4) */
    int space,                /* remote space (1,2: VME; 6: SDRAM) */
    int fifo_mode,
    size_t count,             /* words to be transferred */
                              /* count==0 is illegal */
    size_t* count_read,       /* words transferred */
    u_int8_t* __user data,    /* destination (user virtual address) */
    int* prot_err
    )
{
    struct sis1100_dmabuf dmabuf;
    int i, error, balance=0, res=0;
    u_int32_t head;

    /* how many bytes do we have to read? */
    dmabuf.size=count*sizeof(u_int32_t);

    dmabuf.cpu_addr=pci_alloc_consistent(sc->pdev, dmabuf.size,
        &dmabuf.dma_handle);
    if (!dmabuf.cpu_addr)
        return ENOMEM;

    mutex_lock(&sc->sem_hw);
    sis1100writereg(sc, rd_pipe_buf, dmabuf.dma_handle);
    sis1100writereg(sc, rd_pipe_blen, dmabuf.size);

    sis1100_disable_irq(sc, 0, irq_prot_end);

    head=(0xff>>(8-size))<<24; /* byte enable */
    head|=(space&0x3f)<<16;    /* remote space */
    head|=0x00400000;          /* local space */
    if (am>=0) {
        head|=0x00000800;
        sis1100writereg(sc, t_am, am);
    }
    head|=0x0000002;           /* send request with addr */
    sis1100writereg(sc, t_hdr, head);
    if (fifo_mode) {
        for (i=0; i<count; i++) {
    	    sis1100writereg(sc, t_adl, addr);
        }
    } else {
        for (i=0; i<count; i++) {
    	    sis1100writereg(sc, t_adl, addr);
            addr+=size;
        }
    }

    sc->got_irqs=0;
    sis1100_enable_irq(sc, 0, irq_prot_end);

    res=wait_event_interruptible(
            sc->local_wait,
            ((balance=sis1100readreg(sc, p_balance))==0)
        );

    sis1100_disable_irq(sc, 0, irq_prot_end);

    error=sis1100readreg(sc, prot_error);
    if (!balance)
        sis1100writereg(sc, p_balance, 0);

    if (error||res) {
    	/*printk(KERN_INFO
    	    "sis1100_read_pipe_seq: error=0x%0x, res=%d\n", error, res);
	dump_glink_status(sc, "after pipe", 1);*/
    	sis1100_flush_fifo(sc, 1);
    }
    *count_read=count;
    mutex_unlock(&sc->sem_hw);

    switch (size) {
    case 4:
        if (copy_to_user(data, dmabuf.cpu_addr, dmabuf.size))
            res=EFAULT;
        break;
    case 2: {
            u_int32_t *caddr=dmabuf.cpu_addr;
            for (i=dmabuf.size/2; i; i--, data+=2, caddr++)
                __put_user(*caddr&0xffff, (u_int16_t*)data);
        }
        break;
    case 1:
        if (fifo_mode) {
            u_int32_t *caddr=dmabuf.cpu_addr;
            int shift=(3-(addr&3))*8;
            for (i=dmabuf.size; i; i--, data++, caddr++) {
                __put_user((*caddr>>shift)&0xff, (u_int8_t*)data);
            }
        } else {
            u_int32_t *caddr=dmabuf.cpu_addr;
            for (i=dmabuf.size; i; i--, data++, caddr++, addr++) {
                int shift=(3-(addr&3))*8;
                __put_user((*caddr>>shift)&0xff, (u_int8_t*)data);
            }
        }
        break;
    }

    pci_free_consistent(sc->pdev, dmabuf.size, dmabuf.cpu_addr,
        dmabuf.dma_handle);

    return res;
}
