/* $ZEL: sis1100rem_init.c,v 1.8 2010/01/18 19:09:13 wuestner Exp $ */

/*
 * Copyright (c) 2003-2008
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
sis1100rem_init(struct sis1100_softc* sc, int reset)
{
/* sc->sem_hw must be held by caller!*/
    u_int32_t hv, fk, fv;

    hv=(sc->remote_ident>>8)&0xff;
    fk=(sc->remote_ident>>16)&0xff;
    fv=(sc->remote_ident>>24)&0xff;

    switch (sc->remote_ident&0x00ffff00) {
    case 0x00010100: /* PCI card */
#define MIN_FV 4
#define MAX_FV 8
        if (fv<MIN_FV) {
            pERROR(sc, "1100: remote firmware version too old;"
                    " at least version %d is required.",
                    MIN_FV);
            return -1;
        }
        if (fv>MAX_FV) {
            pINFO(sc, "1100: Driver not tested with"
                    " remote firmware versions higher than %d.",
                    MAX_FV);
        }
#undef MIN_FV
#undef MAX_FV
        break;
    case 0x00010200: /* PCIe card with piggyback */
    case 0x00020200: /* PCIe card without piggyback, single link */
#define MIN_FV 1
#define MAX_FV 2
        if (fv<MIN_FV) {
            pERROR(sc, "1100e: remote firmware version too old;"
                    " at least version %d is required.",
                    MIN_FV);
            return -1;
        }
        if (fv>MAX_FV) {
            pINFO(sc, "1100e: Driver not tested with"
                    " remote firmware versions higher than %d.",
                    MAX_FV);
        }
#undef MIN_FV
#undef MAX_FV
        break;
    default:
        pINFO(sc, "1100: remote hw/fw type not supported");
        return -1;
    }

    return 0;
}
