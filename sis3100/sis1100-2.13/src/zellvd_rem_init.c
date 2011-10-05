/* $ZEL: zellvd_rem_init.c,v 1.8 2010/01/21 21:52:01 wuestner Exp $ */

/*
 * Copyright (c) 2004-2008
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
#include "zellvd_map.h"

int
zellvd_rem_init(struct sis1100_softc* sc, int reset)
{
/* sc->sem_hw must be held by caller!*/
#define MIN_FV_1 1
#define MAX_FV_1 17
#define MIN_FV_2 17
#define MAX_FV_2 17
#define MIN_FV_3 1
#define MAX_FV_3 1
    int min_fv=0, max_fv=0;
    u_int32_t hv, fk, fv;

    hv=(sc->remote_ident>>8)&0xff;
    fk=(sc->remote_ident>>16)&0xff;
    fv=(sc->remote_ident>>24)&0xff;

    switch (sc->remote_ident&0x00ffff00) {
    case 0x00010100: /* LVD controller with F1 */
        min_fv=MIN_FV_1;
        max_fv=MAX_FV_1;
        break;
    case 0x00010200: /* LVD controller with GPX */
        min_fv=MIN_FV_2;
        max_fv=MAX_FV_2;
        break;
    case 0x00010300: /* LVD controller with both F1 and GPX */
        min_fv=MIN_FV_3;
        max_fv=MAX_FV_3;
        break;
    default:
        pERROR(sc, "zellvd: remote hw/fw type not supported");
        return -1;
    }
    if (fv<min_fv) {
        pERROR(sc, "zellvd: remote firmware version too old;"
                " at least version %d is required.",
                min_fv);
        return -1;
    }
    if (fv>max_fv) {
        pINFO(sc, "zellvd: Driver not tested with"
                " remote firmware versions higher than %d.",
                max_fv);
    }

    if (reset>0)
        lvd_writeremreg(sc, cr, 0, 1); /* reset cr */

    return 0;
}
