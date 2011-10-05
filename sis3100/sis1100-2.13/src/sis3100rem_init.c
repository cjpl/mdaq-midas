/* $ZEL: sis3100rem_init.c,v 1.14 2010/01/21 21:50:53 wuestner Exp $ */

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

static int
sis3100_dsp_present(struct sis1100_softc* sc)
{
    u_int32_t dsp_sc;
    int res;

    res=sis3100readremreg(sc, dsp_sc, &dsp_sc, 1);
    if (res) {
        pINFO(sc, "3100: read dsp_sc: res=%d", res);
        return 0;
    }
    return !!(dsp_sc&sis3100_dsp_available);
}

int
sis3100_get_timeouts(struct sis1100_softc* sc, int* berr, int* arb)
{
    u_int32_t stat, error;
    int berr_timer, long_timer, berr_time, long_time;

    error=sis3100readremreg(sc, vme_master_sc, &stat, 1);
    if (error)
        return error;

    berr_timer=(stat>>14)&3;
    long_timer=(stat>>12)&3;
    berr_time=0;
    long_time=0;
    switch (berr_timer) {
        case 0: berr_time=1250; break;
        case 1: berr_time=6250; break;
        case 2: berr_time=12500; break;
        case 3: berr_time=100000; break;
    }
    switch (long_timer) {
        case 0: long_time=  1; break;
        case 1: long_time= 10; break;
        case 2: long_time= 50; break;
        case 3: long_time=100; break;
    }
    *berr=berr_time;
    *arb=long_time;
    return 0;
}

static void
sis3100_dump_timeouts(struct sis1100_softc* sc)
{
    int berr_time, long_time;

    sis3100_get_timeouts(sc, &berr_time, &long_time);
    pINFO(sc, "3100: berr_time=%d ns", berr_time);
    pINFO(sc, "3100: long_time=%d ms", long_time);
}

int
sis3100_set_timeouts(struct sis1100_softc* sc, int berr, int arb)
/* berr in terms of 10**-9 s */
/* arb  in terms of 10**-3 s */
{
    u_int32_t bits=0;
    int res;

    if ((res=sis3100readremreg(sc, vme_master_sc, &bits, 1))) {
        pINFO(sc, "3100: error reading timeout values");
        return res;
    }

    if (berr>=0) {
        int timer;
        if (berr>12500)
            timer=3;
        else if (berr>6250)
            timer=2;
        else if (berr>1250)
            timer=1;
        else 
            timer=0;
        bits|=(timer|(~timer&3)<<16)<<14;
    }

    if (arb>=0) {
        int timer;
        if (arb>50)
            timer=3;
        else if (arb>10)
            timer=2;
        else if (arb>1)
            timer=1;
        else 
            timer=0;
        bits|=(timer|(~timer&3)<<16)<<12;
    }

    if (bits) {
        res=sis3100writeremreg(sc, vme_master_sc, bits, 1);
        if (res)
            pINFO(sc, "3100: error writing timeout values");
        sis3100_dump_timeouts(sc);
    }
    return res;
}

int
sis3100rem_init(struct sis1100_softc* sc, int reset)
{
/* sc->sem_hw must be held by caller!*/
#define MIN_FV_0 3
#define MAX_FV_0 7
#define MIN_FV_4 1
#define MAX_FV_4 2
    int min_fv=0, max_fv=0;
    u_int32_t hv, fk, fv, stat;

    hv=(sc->remote_ident>>8)&0xff;
    fk=(sc->remote_ident>>16)&0xff;
    fv=(sc->remote_ident>>24)&0xff;

    switch (sc->remote_ident&0x00ffff00) {
    case 0x00010100: /* sis3100 */
        min_fv=MIN_FV_0;
        max_fv=MAX_FV_0;
        break;
    case 0x00010200: /* sis3104 */
        min_fv=MIN_FV_4;
        max_fv=MAX_FV_4;
        break;
    case 0x00210200: /* 2G 3104 */
        min_fv=MIN_FV_4;
        max_fv=MAX_FV_4;
        break;
    default:
        pERROR(sc, "310x: remote hw/fw type not supported");
        return -1;
    }
    if (fv<min_fv) {
        pERROR(sc, "310x: remote firmware version too old;"
                " at least version %d is required.",
                min_fv);
        return -1;
    }
    if (fv>max_fv) {
        pINFO(sc, "310x: Driver not tested with"
                " remote firmware versions higher than %d.",
                max_fv);
    }

    if (reset>0) {
        sis3100writeremreg(sc, vme_irq_sc, 0x00fe0001, 1); /*disable VME IRQs*/
        sis3100writeremreg(sc, vme_master_sc, 8, 1);
        sis3100_set_timeouts(sc, 5000, 10);
    }

    switch (sc->remote_ident&0x00ffff00) {
    case 0x00010100:
        if (sis1100_init_sdram(sc)<0)
            return -1;
        pINFO(sc, "3100: size of SDRAM: 0x%llx (%lld MByte)",
            sc->ram_size, sc->ram_size>>20);
        sc->dsp_present=sis3100_dsp_present(sc);
        pINFO(sc, "3100: DSP is %spresent", sc->dsp_present?"":"not ");
        break;
    case 0x00010200:
        pINFO(sc, "remote device is SIS3104");
        sc->ram_size=0;
        sc->dsp_present=0;
        break;
    case 0x00210200:
        pINFO(sc, "remote device is SIS3104 with 2G link");
        sc->ram_size=0;
        sc->dsp_present=0;
        break;
    }

    sis3100readremreg(sc, vme_master_sc, &stat, 1);
    pINFO(sc, "3100: remote stat=0x%08x", stat);
    if (!(stat&vme_system_controller)) {
        pINFO(sc, "3100: System Controller NOT enabled!");
    }

    return 0;
}
