/* $ZEL: zellvd_map.h,v 1.3 2009/12/08 15:41:21 wuestner Exp $ */

/*
 * Copyright (c) 2004
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

#ifndef _dev_pci_zellvd_map_h_
#define _dev_pci_zellvd_map_h_

struct lvd_reg {
        u_int32_t           ident;
        u_int32_t           sr;             /* Status Register */
#define LVD_SR_SPEED_0      0x80000000
#define LVD_SR_SPEED_1      0x40000000
#define LVD_SR_TX_SYNCH_0   0x20000000
#define LVD_SR_TX_SYNCH_1   0x10000000
#define LVD_SR_RX_SYNCH_0   0x08000000
#define LVD_SR_RX_SYNCH_1   0x04000000
#define LVD_SR_LINK_B       0x02000000
#define LVD_DBELL_LVD_ERR   0x01000000
#define LVD_DBELL_EVENTPF   0x003f0000
#define LVD_DBELL_EVENT     0x00003f00
#define LVD_DBELL_INT       0x00000001

        u_int32_t           cr;             /* Control Register, 4 bit */
#define LVD_CR_DDTX        0x01    /* direct data transfer */
#define LVD_CR_SCAN        0x02    /* scan LVD front bus */
#define LVD_CR_SINGLE      0x04    /* single event dorbell */
#define LVD_CR_LED0        0x08    /* SIS1100opt front LED (upper) */
#define LVD_CR_LED1        0x10    /* SIS1100opt front LED (lower) */
#define LVD_CR_LEDs        (LVD_CR_LED0|LVD_CR_LED1)

        u_int32_t           res0C;
        u_int32_t           ddt_counter;
        u_int32_t           ddt_blocksz;
        u_int32_t           jtag_csr;       /* JTAG control/status */
        u_int32_t           jtag_data;      /* JTAG data */
#define LVDMC_JT_TDI           0x001
#define LVDMC_JT_TMS           0x002
#define LVDMC_JT_TDO           0x008
#define LVDMC_JT_ENABLE        0x100
#define LVDMC_JT_AUTO_CLOCK    0x200
#define LVDMC_JT_SLOW_CLOCK    0x400
#define LVDMC_JT_C             (LVDMC_JT_ENABLE|LVDMC_JT_AUTO_CLOCK)
        u_int32_t           timer;          /* 0.1 ms */
};

/* bits in in_out */
#define lvd_io_lemo_out1 (1<<4)
#define lvd_io_lemo_out2 (1<<5)
/* clear is (io_*_out?)<<16 */
#define lvd_io_lemo_in1 (1<<20)
#define lvd_io_lemo_in2 (1<<21)

/* error codes */
#define lvd_re_berr        0x211 /* Bus Error */
#define lvd_re_retr        0x212 /* Retry */
#define lvd_re_atimeout    0x214 /* Arbitration timeout */

#endif
