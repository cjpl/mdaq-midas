/* $ZEL: sis1100_init.c,v 1.27 2010/01/18 11:11:33 wuestner Exp $ */

/*
 * Copyright (c) 2001-2008
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
sis1100_reset(struct sis1100_softc* sc)
{
    pERROR(sc, "reset");
    sis1100writereg(sc, cr, cr_reset); /* master reset */
    /* reset remote, ignore whether it exists */
#if 1
    sis1100writereg(sc, cr, cr_rem_reset);
#endif
    sis1100_flush_fifo(sc, 1);
    sis1100writereg(sc, p_balance, 0);
    sis1100readreg(sc, prot_error);
    return 0;
}

int sis1100_serial_numbers(struct sis1100_softc* sc, int32_t* d)
/*
 * d[0]: CMC_Type 0: 5V 1: 3.3V 2: both 3,4,5: PCIe
 * d[1]: serial number of base board
 * d[2]: serial number of piggy back
 * d[3]: ?
 */
{
    int res, end;
    u_int16_t data[2];
    end=sis1100_end_of_eeprom(sc);
    if (end<0)
        return EIO;
    res=sis1100_read_eeprom(sc, 2, end, data, 0);
    if (res)
        return res;
    d[0]=((int16_t*)data)[0];
    d[1]=((int16_t*)data)[1];
    if (d[0]<0) {
        switch (sc->pdev->device) {
        case 0x0e: d[0]=3; break; /* PCIe device with piggyback */
        case 0x11: d[0]=4; break; /* PCIe device, first of four links used */
        case 0x12: d[0]=5; break; /* PCIe device with four links */
        }
    }
/* read serial number of piggy back here (not implemented) */
    d[2]=-1;
    d[3]=-1;
    return 0;
}

void
sis1100_dump_glink_status(struct sis1100_softc* sc, char* text, int locked)
{
    u_int32_t v;
    if (!locked)
        mutex_lock(&sc->sem_hw);
    pINFO(sc, "%s:", text);
    pINFO(sc, "  ident       =%08x", sis1100readreg(sc, ident));
    pINFO(sc, "  sr          =%08x", sis1100readreg(sc, sr));
    pINFO(sc, "  cr          =%08x", sis1100readreg(sc, cr));
    pINFO(sc, "  t_hdr       =%08x", sis1100readreg(sc, t_hdr));
    pINFO(sc, "  t_am        =%08x", sis1100readreg(sc, t_am));
    pINFO(sc, "  t_adl       =%08x", sis1100readreg(sc, t_adl));
    pINFO(sc, "  t_dal       =%08x", sis1100readreg(sc, t_dal));
    pINFO(sc, "  tc_hdr      =%08x", sis1100readreg(sc, tc_hdr));
    pINFO(sc, "  tc_dal      =%08x", sis1100readreg(sc, tc_dal));
    pINFO(sc, "  p_balance   =%08x", sis1100readreg(sc, p_balance));
    pINFO(sc, "  prot_error  =%08x", sis1100readreg(sc, prot_error));
    pINFO(sc, "  d0_bc       =%08x", sis1100readreg(sc, d0_bc));
    pINFO(sc, "  d0_bc_buf   =%08x", sis1100readreg(sc, d0_bc_buf));
    pINFO(sc, "  d0_bc_blen  =%08x", sis1100readreg(sc, d0_bc_blen));
    pINFO(sc, "  d_hdr       =%08x", sis1100readreg(sc, d_hdr));
    pINFO(sc, "  d_am        =%08x", sis1100readreg(sc, d_am));
    pINFO(sc, "  d_adl       =%08x", sis1100readreg(sc, d_adl));
    pINFO(sc, "  d_bc        =%08x", sis1100readreg(sc, d_bc));
    pINFO(sc, "  rd_pipe_buf =%08x", sis1100readreg(sc, rd_pipe_buf));
    pINFO(sc, "  rd_pipe_blen=%08x", sis1100readreg(sc, rd_pipe_blen));
    pINFO(sc, "");
    v=sis1100readreg(sc, opt_csr);
    pINFO(sc, "  opt_csr     =%08x", v);
    sis1100writereg(sc, opt_csr, v&0xc0f50000);
    pINFO(sc, "  opt_csr     =%08x", sis1100readreg(sc, opt_csr));
    if (!locked)
        mutex_unlock(&sc->sem_hw);
}

#define MAXFLUSHWORDS_DUMP 100
#define MAXFLUSHWORDS 16000000
int
sis1100_flush_fifo(struct sis1100_softc* sc, int silent)
{
/*  sc->sem_hw must be held by caller! */
    u_int32_t sr, special, data;
    int count=0, res=0;

    sis1100writereg(sc, cr, cr_transparent);
    mb_reg();
    sr=sis1100readreg(sc, sr);
    if (sr&(sr_tp_special|sr_tp_data))
        pINFO(sc, "flush_fifo: sr=0x%08x", sr);
    while (sr&(sr_tp_special|sr_tp_data)) {
        while (sr&sr_tp_data) {
            data=sis1100readreg(sc, tp_data);
            if (!silent)
                pINFO(sc, "data   =          0x%08x", data);
            sr=sis1100readreg(sc, sr);
            count++;
            if (count>MAXFLUSHWORDS_DUMP)
                silent=1;
            if (count>MAXFLUSHWORDS) {
                res=-1;
                goto error;
            }
        }
        while ((sr&(sr_tp_special|sr_tp_data))==sr_tp_special) {
            special=sis1100readreg(sc, tp_special);
            if (!silent)
                pINFO(sc, "special=0x%08x", special);
            sr=sis1100readreg(sc, sr);
            count++;
            if (count>MAXFLUSHWORDS_DUMP)
                silent=1;
            if (count>MAXFLUSHWORDS) {
                res=-1;
                goto error;
            }
        }
    }
error:
    sis1100writereg(sc, cr, cr_transparent<<16);

    if (res) {
        pINFO(sc, "too many data in fifo; giving up");
    }

    if (count)
        pINFO(sc, "flushed %d words from fifo", count);
    return 0;
}

static void
sis1100_set_swapping(struct sis1100_softc* sc, int swap)
{
    u_int32_t tmp;

#if 0
    pINFO(sc, "set swap to %d", swap);
#endif
    tmp=plxreadreg(sc, BIGEND_LMISC_PROT_AREA);
    if (swap) {
        sis1100writereg(sc, cr, 0x8);
        plxwritereg(sc, BIGEND_LMISC_PROT_AREA, tmp|(3<<6));
    } else {
        sis1100writereg(sc, cr, 0x80000);
        plxwritereg(sc, BIGEND_LMISC_PROT_AREA, tmp&~(3<<6));
    }
}

void
sis1100_update_swapping(struct sis1100_softc* sc)
{
    int swap;
#if defined(__LITTLE_ENDIAN)
    int local_endian=0;
#elif defined(__BIG_ENDIAN)
    int local_endian=1;
#else
#   error UNKNOWN ENDIAN
#endif

#if 0
    pINFO(sc, "local endian is %s; remote endian is %s; user swap is %d ",
        local_endian?"big":"little",
        sc->remote_endian?"big":"little",
        sc->user_wants_swap);
#endif

    swap=0;
    if (local_endian) swap=!swap;
    if (sc->remote_endian) swap=!swap;
    if (sc->user_wants_swap) swap=!swap;
    sis1100_set_swapping(sc, swap);
}

int
sis1100_init(struct sis1100_softc* sc)
{
    u_int32_t typ, hv, fk, fv;
    size_t maplen;
    int MIN_FV, MAX_FV, expected_pci_id;
    int res, i;

    sc->local_ident=sis1100readreg(sc, ident);
    /* size of the local send FIFO in words (33 bit) */
    sc->sendfifo_size=1024;
    sc->sendfifo_of=sc->sendfifo_size-128;
    sc->broken.d16_dma=0;
    sc->broken.d8_dma=0;
    sc->broken.swap_doorbell=0;

    pINFO(sc, "local ident: 0x%08x", sc->local_ident);
    typ=sc->local_ident&0xff;
    hv=(sc->local_ident>>8)&0xff;
    fk=(sc->local_ident>>16)&0xff;
    fv=(sc->local_ident>>24)&0xff;

    if (typ!=1) {
        int i;
    	pERROR(sc, "ident=0x%08x; claims not to be a PCI Device", typ);
        for (i=0; i<=0x10; i+=4) {
            u_int32_t v;
            v=_readreg32(sc, i);
            pERROR(sc, "0x%02x: 0x%08x", i, v);
        }
    	res=ENXIO;
    	goto raus;
    }
    pINFO(sc, "HW version %d; FW code %d; FW version %d", hv, fk, fv);

    switch (sc->local_ident&0x00ffff00) { /* HW version and FW code */
    case 0x00010100: /* classical PCI device */
        MIN_FV=4;
        MAX_FV=7;
        expected_pci_id=0x0001;
#if 1
        sc->broken.d16_dma=1; /* firmware bug */
        sc->broken.d8_dma=1;  /* firmware bug */
#endif
        break;
    case 0x00010200: /* PCIe device with piggyback */
        MIN_FV=1;
        MAX_FV=1;
        expected_pci_id=0x000e;
        break;
    case 0x00020200: /* PCIe device with four links, only first link used */
        MIN_FV=1;
        MAX_FV=2;
        expected_pci_id=0x0011;
        sc->sendfifo_size=4096;
        sc->sendfifo_of=sc->sendfifo_size-128;
        /*sc->broken.swap_doorbell=1;*/
        break;
    case 0x00220200: /* 2G 1100ecmc */
        MIN_FV=1;
        MAX_FV=2;
        expected_pci_id=0x0011;
        sc->sendfifo_size=4096;
        sc->sendfifo_of=sc->sendfifo_size-128;
        pINFO(sc, "2GBit link!");  
        break;
    default:
        pERROR(sc, "Hard- or Firmware not known");
        res=ENXIO;
        goto raus;
    }
    if (fv<MIN_FV) {
        pERROR(sc, "Firmware version too old;"
                " at least version %d is required.",
                MIN_FV);
        res=ENXIO;
    	goto raus;
    }
    if (fv>MAX_FV)
        pINFO(sc, "Driver not tested with"
                " firmware versions higher than %d.",
                MAX_FV);
    if (sc->pdev->device!=expected_pci_id)
        pWARNING(sc, "unexpected PCI ID 0x%04x, should be 0x%04x",
            sc->pdev->device, expected_pci_id);

    if (sc->broken.d16_dma)
        pWARNING(sc, "D16 DMA write not possible");
    if (sc->broken.d8_dma)
        pWARNING(sc, "D8 DMA write not possible");
    if (sc->broken.swap_doorbell)
        pWARNING(sc, "doorbell swapped");


    maplen=pci_resource_len(sc->pdev, 2);
    if (maplen!=0x1000) {
    	pERROR(sc, "wrong size of space 0: 0x%lx instead of 0x1000",
            (unsigned long)maplen);
    	res=ENXIO;
    	goto raus;
    }

    maplen=pci_resource_len(sc->pdev, 3);
    pINFO(sc, "size of remote space: 0x%llx (%lld MByte)",
	(unsigned long long)maplen, (unsigned long long)(maplen>>20));

    sc->silicon_id=_plxreadreg32(sc, 0x70);
    pDEBUG(sc, "hardwired device id: 0x%04x", (sc->silicon_id>>16)&0xffff);

    /* print serial number if available */
    {
        int32_t data[4];
        char *types[]={
                "5V", "3.3V", "universal",
                "PCIe+Opt", "PCIe_single_link", "PCIe_quad_link"
        };
        res=sis1100_serial_numbers(sc, data);
        if (res)
            pINFO(sc, "sis1100_serial_numbers returns %d", res);

        if (data[1]<0) {
            pINFO(sc, "serial number not set");
        } else {
            if (data[0]<=5)
                pINFO(sc, "board type: %s, serial=%d",
                    types[data[0]], data[1]);
            else
                pINFO(sc, "board type: %d, serial=%d",
                    data[0], data[1]);
        }
    }

/* common initialisation code */
    /* reset all we can */
    sis1100writereg(sc, cr, cr_reset); /* master reset */
    sis1100writereg(sc, cr, cr_rem_reset); /* reset remote, ignore wether it exists */
    mdelay(500);
    if (sis1100_flush_fifo(sc, 0)) { /* clear local fifo */
        res=EIO;
        goto raus;
    }
    sis1100writereg(sc, cr, cr_reset); /* master reset again */
    switch ((sc->silicon_id>>16)&0xffff) {
    case 0x9054:
        sis1100_reset_plx9054(sc);
        break;
    case 0x9056:
        sis1100_reset_plx9056(sc);
        break;
    default:
        pERROR(sc, "unexpected hardwired device id 0x%04x; "
                "skipping reset sequence", sc->silicon_id);
    }
    sis1100writereg(sc, p_balance, 0);
    sis1100readreg(sc, prot_error);

    /* sis1100_dump_glink_status(sc, "INITIAL DUMP"); */

    /* enable PCI Initiator-to-PCI Memory */
    plxwritereg(sc, DMRR, 0);
    plxwritereg(sc, DMLBAM, 0);
    plxwritereg(sc, DMPBAM, 1);

    sis1100writereg(sc, cr, 8); /* big endian */

    sc->got_irqs=0;
    for (i=0; i<=7; i++)
        sc->irq_vects[i].valid=0;
    sc->autoack_mask=0;
    sc->pending_irqs=0;
    sc->doorbell=0;
    sc->lemo_status=0;

    /* enable IRQs */
    sis1100_disable_irq(sc, 0xffffffff, 0xffffffff);
    sis1100_enable_irq(sc, plxirq_pci|plxirq_mbox|plxirq_doorbell|plxirq_local,
	    irq_synch_chg|irq_inh_chg|irq_sema_chg|
	    irq_rec_violation|irq_reset_req);

    sc->user_wants_swap=0;
    sc->remote_ident=0;
    sc->remote_hw=sis1100_hw_invalid;
    sc->remote_endian=1;

    sis1100_init_remote(sc, 1);
    res=0;

raus:
    return res;
}

void
sis1100_done(struct sis1100_softc* sc)
{
    /* DMA Ch. 0/1: not enabled */
    plxwritereg(sc, DMACSR0_DMACSR1, 0);
    /* disable interrupts */
    plxwritereg(sc, INTCSR, 0);
}
