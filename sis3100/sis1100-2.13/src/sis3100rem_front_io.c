/* $ZEL: sis3100rem_front_io.c,v 1.4 2007/11/28 21:26:40 wuestner Exp $ */

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
#include "sis3100_map.h"

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
 * 23  res vme_user_led      free
 * 22  res vme_lemo_out_3    status vme_lemo_in_3
 * 21  res vme_lemo_out_2    status vme_lemo_in_2
 * 20  res vme_lemo_out_1    status vme_lemo_in_1
 * 19  res vme_flat_out_4    status vme_flat_in_4
 * 18  res vme_flat_out_3    status vme_flat_in_3
 * 17  res vme_flat_out_2    status vme_flat_in_2
 * 16  res vme_flat_out_1    status vme_flat_in_1
 * 15
 * 14
 * 13
 * 12
 * 11  set pci_led_1         status pci_led_1
 * 10  set pci_led_0         status pci_led_0
 *  9  set pci_lemo_out_1    status pci_lemo_out_1
 *  8  set pci_lemo_out_0    status pci_lemo_out_0
 *  7  set vme_user_led      status vme_user_led
 *  6  set vme_lemo_out_3    status vme_lemo_out_3
 *  5  set vme_lemo_out_2    status vme_lemo_out_2
 *  4  set vme_lemo_out_1    status vme_lemo_out_1
 *  3  set vme_flat_out_4    status vme_flat_out_4
 *  2  set vme_flat_out_3    status vme_flat_out_3
 *  1  set vme_flat_out_2    status vme_flat_out_2
 *  0  set vme_flat_out_1    status vme_flat_out_1
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
sis3100rem_front_io(struct sis1100_softc* sc, u_int32_t* data)
{
    u_int32_t io3100, st3100, _data;

    /* XXX no error handling yet */
    io3100=_readreg32(sc, ofs(struct sis3100_reg, in_out)+0x800);
    st3100=_readreg32(sc, ofs(struct sis3100_reg, vme_master_sc)+0x800);

    _data = (io3100&0x7f007f) |    /* 3100 flat/lemo in/out */
            (st3100&0x80);         /* 3100 user led */

    io3100=*data & 0x007f007f;
    st3100=*data & 0x00800080;

    if (io3100)
        _writereg32(sc, ofs(struct sis3100_reg, in_out)+0x800, io3100);
    if (st3100)
        _writereg32(sc, ofs(struct sis3100_reg, vme_master_sc)+0x800, st3100);

    *data=_data;
    return 0;
}

int
sis3100rem_front_pulse(struct sis1100_softc* sc, u_int32_t* data)
{
    u_int32_t io3100;

    io3100=(*data<<24) & 0x7f000000;

    _writereg32(sc, ofs(struct sis3100_reg, in_out)+0x800, io3100);

    return 0;
}

int
sis3100rem_front_latch(struct sis1100_softc* sc, u_int32_t* data)
{
    u_int32_t latch, _data;

    latch=(*data<<24) & 0xff000000;
    _data=_readreg32(sc, ofs(struct sis3100_reg, in_latch_irq)+0x800);
    _writereg32(sc, ofs(struct sis3100_reg, in_latch_irq)+0x800, latch);

    *data=(_data>>24) & 0xff;
    return 0;
}
