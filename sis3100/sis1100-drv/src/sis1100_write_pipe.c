/* $ZEL: sis1100_write_pipe.c,v 1.3 2007/09/11 21:33:35 wuestner Exp $ */

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

int
sis1100_write_pipe(struct sis1100_softc* sc, int32_t am,
    int space, int num, u_int32_t* data)
{
    u_int32_t error;
    u_int32_t head;
    int i;

    head=0x0f000404|(space&0x3f)<<16;

    mutex_lock(&sc->sem_hw);
    if (am>=0) {
        head|=0x800;
        sis1100writereg(sc, t_am, am);
    }
    sis1100writereg(sc, t_hdr, head);
    for (i=0; i<num; i++) {
        sis1100writereg(sc, t_adl, *data++);
        sis1100writereg(sc, t_dal, *data++);
    }

    do {
        error=sis1100readreg(sc, prot_error);
    } while (error==0x005);
    mutex_unlock(&sc->sem_hw);

    return error;
}
