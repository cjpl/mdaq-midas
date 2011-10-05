/* $ZEL: sis1100rem_front_io.c,v 1.3 2009/04/26 20:28:22 wuestner Exp $ */

/*
 * Copyright (c) 2003
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
#include "sis1100_map.h"

/*
 * pseudoregister front_io:
 * bit write function        read function
 * 31
 * 30
 * 29
 * 28
 * 27  res pci_led_1         free
 * 26  res pci_led_0         free
 * 25  res pci_lemo_out_1    status pci_lemo_in_1
 * 24  res pci_lemo_out_0    status pci_lemo_in_0
 * 23
 * 22
 * 21
 * 20
 * 19
 * 18
 * 17
 * 16
 * 15
 * 14
 * 13
 * 12
 * 11  set pci_led_1         status pci_led_1
 * 10  set pci_led_0         status pci_led_0
 *  9  set pci_lemo_out_1    status pci_lemo_out_1
 *  8  set pci_lemo_out_0    status pci_lemo_out_0
 *  7
 *  6
 *  5
 *  4
 *  3
 *  2
 *  1  reset lemo_changed_2    status lemo_changed_2
 *  0  reset lemo_changed_1    status lemo_changed_1
 */
/*
 * set_vme_flat_out_?=0x0000000f
 * res_vme_flat_out_?=0x000f0000
 * set_vme_lemo_out_?=0x00000070
 * res_vme_lemo_out_?=0x00700000
 * set_vme_user_led  =0x00000080
 * res_vme_user_led  =0x00800000
 * set_pci_lemo_out_?=0x00000300
 * res_pci_lemo_out_?=0x03000000
 * set_pci_led_?     =0x00000c00
 * res_pci_led_?     =0x0c000000
 */
int
sis1100rem_front_io(struct sis1100_softc* sc, u_int32_t* data)
{
    /* remote register opt_csr is not accessible */
    return 0;
}

int
sis1100rem_front_pulse(struct sis1100_softc* sc, u_int32_t* data)
{
    /* remote register opt_csr is not accessible */
    return 0;
}

int
sis1100rem_front_latch(struct sis1100_softc* sc, u_int32_t* data)
{
    /* we can see whether the status was changed but the aktual value
       of the LEMO inputs is unaccessible */

    u_int32_t latch, _data;

    latch=(*data<<12) & 0x3000;
    _data=_readreg32(sc, ofs(struct sis1100_reg, sr)+0x800);
    _writereg32(sc, ofs(struct sis1100_reg, sr)+0x800, latch);

    *data=(_data>>12) & 0x3000;
    return 0;
}
