/* $ZEL: plx9054reg.h,v 1.4 2008/02/06 19:32:41 wuestner Exp $ */

/*
 * Copyright (c) 2001-2004
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

#ifndef _plx9054reg_h_
#define _plx9054reg_h_

struct plx9054reg {
	u_int32_t LAS0RR, LAS0BA, MARBR, BIGEND_LMISC_PROT_AREA;
	u_int32_t EROMRR, EROMBA, LBRD0, DMRR;
	u_int32_t DMLBAM, DMLDAI, DMPBAM, DMCFGA;
/* 30 */
	u_int32_t OPQIS, OPQIM, _dummy1, _dummy2;
/* 40 */
	u_int32_t IQP, OQP, MBOX2, MBOX3;
	u_int32_t MBOX4, MBOX5, MBOX6, MBOX7;
	u_int32_t P2LDBELL, L2PDBELL, INTCSR, CNTRL;
	u_int32_t PCIHIDR, PCIHREV, MBOX0, MBOX1;
/* 80 */
	u_int32_t DMAMODE0, DMAPADR0, DMALADR0, DMASIZ0;
	u_int32_t DMADPR0, DMAMODE1, DMAPADR1, DMALADR1;
	u_int32_t DMASIZ1, DMADPR1, DMACSR0_DMACSR1, DMAARB;
	u_int32_t DMATHR, DMADAC0, DMADAC1, _dummy3;
/* c0 */
	u_int32_t MQCR, QBAR, IFHPR, IFTPR;
	u_int32_t IPHPR, IPTPR, OFHPR, OFTPR;
	u_int32_t OPHPR, OPTPR, QSR, _dummy4;
/* f0 */
	u_int32_t LAS1RR, LAS1BA, LBRD1, DMDAC;
};

struct plx9054_dmadesc {
	volatile u_int32_t pcistart;
        volatile u_int32_t localstart;
        volatile u_int32_t size;
        volatile u_int32_t next;
};
struct plx9054_dmadesc_dac {
	volatile u_int32_t pcistart;
        volatile u_int32_t localstart;
        volatile u_int32_t size;
        volatile u_int32_t next;
        volatile u_int32_t pcihigh;
        u_int32_t dummy[3];
};

                                /* to clear the interrupt you must ... */
#define plxirq_mbox     (1<< 3) /* read the mailbox */
#define plxirq_pci      (1<< 8)
#define plxirq_doorbell (1<< 9) /* clear the doorbell bits */
#define plxirq_local    (1<<11) /* clear the local irq source */
#define plxirq_dma0     (1<<18) /* clear the dma status bits */
#define plxirq_dma1     (1<<19) /* clear the dma status bits */

#define plxirq_doorbell_active (1<<13)
#define plxirq_local_active    (1<<15)
#define plxirq_dma0_active     (1<<21)
#define plxirq_dma1_active     (1<<22)

#endif
