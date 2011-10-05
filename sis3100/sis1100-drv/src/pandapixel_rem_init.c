/* $ZEL: pandapixel_rem_init.c,v 1.7 2010/01/21 21:48:55 wuestner Exp $ */

/*
 * Copyright (c) 2006-2008
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
#include "pandapixel_map.h"

int
pandapixel_rem_init(struct sis1100_softc* sc, int reset)
{
/* sc->sem_hw must be held by caller!*/
#define MIN_FV 16
#define MAX_FV 255

    u_int32_t hv, fk, fv;

    hv=(sc->remote_ident>>8)&0xff;
    fk=(sc->remote_ident>>16)&0xff;
    fv=(sc->remote_ident>>24)&0xff;

    switch (hv) {
    case 1:
        if (fv<MIN_FV) {
            pERROR(sc, "pandapixel: remote firmware version too old;"
                    " at least version %d is required.",
                    MIN_FV);
            return -1;
        }
        if (fv>MAX_FV) {
            pINFO(sc, "pandapixel: Driver not tested with"
                    " remote firmware versions higher than %d.",
                    MAX_FV);
        }
        break;
    default:
        pERROR(sc, "pandapixel: remote hw/fw 0x%08x not supported",
            sc->remote_ident);
    }

    sc->dsp_present=0;

    if (reset>0)
        pandapixel_writeremreg(sc, cr, 0, 1); /* reset cr */

    return 0;
}
