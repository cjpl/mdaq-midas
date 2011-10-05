/* $ZEL: sis5100rem_init.c,v 1.9 2010/01/21 21:51:24 wuestner Exp $ */

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

#include "sis1100_sc.h"
#include "sis5100_map.h"

static int
sis5100_dsp_present(struct sis1100_softc* sc)
{
    u_int32_t dsp_sc;
    int res;

    res=sis5100readremreg(sc, dsp_sc, &dsp_sc, 1);
    if (res) {
        pINFO(sc, "5100: read dsp_sc: error=0x%x", res);
        return 0;
    }
    return !!(dsp_sc&sis5100_dsp_available);
}

int
sis5100rem_init(struct sis1100_softc* sc, int reset)
{
/* sc->sem_hw must be held by caller!*/
#define MIN_FV_1 1
#define MAX_FV_1 1
#define MIN_FV_2 1
#define MAX_FV_2 1
    u_int32_t hv, fk, fv;

    hv=(sc->remote_ident>>8)&0xff;
    fk=(sc->remote_ident>>16)&0xff;
    fv=(sc->remote_ident>>24)&0xff;

    switch (sc->remote_ident&0x00ffff00) {
    case 0x00010100:
        if (fv<MIN_FV_1) {
            pERROR(sc, "5100: remote firmware version too old;"
                    " at least version %d is required.",
                    MIN_FV_1);
            return -1;
        }
        if (fv>MAX_FV_1) {
            pINFO(sc, "5100: Driver not tested with"
                    " remote firmware versions higher than %d.",
                    MAX_FV_1);
        }
        break;
    case 0x00020100:
        if (fv<MIN_FV_2) {
            pERROR(sc, "5100: remote firmware version too old;"
                    " at least version %d is required.",
                    MIN_FV_2);
            return -1;
        }
        if (fv>MAX_FV_2) {
            pINFO(sc, "5100: Driver not tested with"
                    " remote firmware versions higher than %d.",
                    MAX_FV_2);
        }
        break;
    default:
        pERROR(sc, "5100: remote hw/fw type not supported");
        return -1;
    }

#if 0
    if (sis1100_init_sdram(sc)<0)
        return -1;
    pINFO(sc, "5100: size of SDRAM: 0x%llx (%lld MByte)",
        sc->ram_size, sc->ram_size>>20);
#endif

    sc->dsp_present=sis5100_dsp_present(sc);
    pINFO(sc, "5100: DSP is %spresent", sc->dsp_present?"":"not ");

    return 0;
}
