/* $ZEL: sis1100_front_io.c,v 1.8 2009/04/26 20:28:21 wuestner Exp $ */

/*
 * Copyright (c) 2001-2006
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
 * pseudoregister front_io:
 * bit write function        read function
 * 27  res pci_led_1         free
 * 26  res pci_led_0         free
 * 25  res pci_lemo_out_1    status pci_lemo_in_1
 * 24  res pci_lemo_out_0    status pci_lemo_in_0
 * 11  set pci_led_1         status pci_led_1
 * 10  set pci_led_0         status pci_led_0
 *  9  set pci_lemo_out_1    status pci_lemo_out_1
 *  8  set pci_lemo_out_0    status pci_lemo_out_0
 */
/*
 * set_pci_lemo_out_?=0x00000300
 * res_pci_lemo_out_?=0x03000000
 * set_pci_led_?     =0x00000c00
 * res_pci_led_?     =0x0c000000
 */

/*
 * sis1100_front_io
 *   sis1100rem_front_io
 *   sis3100rem_front_io
 *   sis5100rem_front_io
 * 
 * sis1100_front_pulse
 *   sis1100rem_front_pulse
 *   sis3100rem_front_pulse
 *   sis5100rem_front_pulse
 * 
 * sis1100_front_latch
 *   sis1100rem_front_latch
 *   sis3100rem_front_latch
 *   sis5100rem_front_latch
 */

int
sis1100_front_io(struct sis1100_softc* sc, u_int32_t* data, int locked)
{
    u_int32_t opt1100, _data;

    if (!locked)
        mutex_lock(&sc->sem_hw);

    opt1100=sis1100readreg(sc, opt_csr);

    _data = ((opt1100&0xf0)<<4) |  /* 1100 lemo out and led */
            ((opt1100&0x300)<<16); /* 1100 lemo in */

    opt1100&=0xff;
    opt1100&=~((*data>>20) & 0xf0);
    opt1100|=(*data>>4) & 0xf0;
    sis1100writereg(sc, opt_csr, opt1100);

    switch (sc->remote_hw) {
    case sis1100_hw_invalid: break;
    case sis1100_hw_pci:
        sis1100rem_front_io(sc, data);
        break;
    case sis1100_hw_vme:
        sis3100rem_front_io(sc, data);
        break;
    case sis1100_hw_camac:
        sis5100rem_front_io(sc, data);
        break;
    case sis1100_hw_lvd: break;
    case sis1100_hw_pandapixel: break;
    default:
        pINFO(sc, "front_io: remote_hw %d not known", sc->remote_hw);
    }

    if (!locked)
        mutex_unlock(&sc->sem_hw);
    *data|=_data;
    return 0;
}

int
sis1100_front_pulse(struct sis1100_softc* sc, u_int32_t* data, int locked)
{
    if (!locked)
        mutex_lock(&sc->sem_hw);

    switch (sc->remote_hw) {
    case sis1100_hw_invalid: break;
    case sis1100_hw_pci: break;
    case sis1100_hw_vme:
        sis3100rem_front_pulse(sc, data);
        break;
    case sis1100_hw_camac: break;
    case sis1100_hw_lvd: break;
    case sis1100_hw_pandapixel: break;
    default:
        pINFO(sc, "front_pulse: remote_hw %d not known", sc->remote_hw);
    }

    if (!locked)
        mutex_unlock(&sc->sem_hw);

    return 0;
}

int
sis1100_front_latch(struct sis1100_softc* sc, u_int32_t* data, int locked)
{
    if (!locked)
        mutex_lock(&sc->sem_hw);

    switch (sc->remote_hw) {
    case sis1100_hw_invalid: break;
    case sis1100_hw_pci: break;
    case sis1100_hw_vme:
        sis3100rem_front_latch(sc, data);
        break;
    case sis1100_hw_camac: break;
    case sis1100_hw_lvd: break;
    case sis1100_hw_pandapixel: break;
    default:
        pINFO(sc, "front_latch: remote_hw %d not known", sc->remote_hw);
    }

    if (!locked)
        mutex_unlock(&sc->sem_hw);

    return 0;
}

#if 0
/* to be used for debugging with oscilloscope */
void
_front_pulse(struct sis1100_softc* sc, u_int32_t data)
{
    u_int32_t opt1100;
    opt1100=sis1100readreg(sc, opt_csr)&0xf;
    opt1100|=(data<<4)&0xf0;
    sis1100writereg(sc, opt_csr, opt1100);
}
#endif
