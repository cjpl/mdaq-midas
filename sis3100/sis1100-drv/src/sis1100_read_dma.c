/* $ZEL: sis1100_read_dma.c,v 1.14 2009/04/26 20:28:22 wuestner Exp $ */

/*
 * Copyright (c) 2003-2004
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
sis1100_read_dma(
    struct sis1100_softc* sc,
    struct sis1100_fdata* fd,
    u_int32_t addr,           /* VME or SDRAM address */
    int32_t am,               /* address modifier, not used if <0 */
    int size,                 /* wordsize (1, 2 or 4) */
    int space,                /* remote space (1,2: VME; 6: SDRAM) */
    int fifo_mode,
    size_t count,             /* words to be transferred */
                              /* count==0 is illegal */
    size_t* count_read,       /* words transferred */
    u_int8_t* __user data,    /* destination (user virtual address) */
    int* prot_err
    )
{
    int res, may_be_more;
    size_t _count_read, orig_count=count;

    mutex_lock(&sc->sem_hw);
    do {
        size_t counter; /* bytes(!) transferred so far */
        res=_sis1100_read_dma(sc, fd, addr, am, size, space, fifo_mode,
                count, &_count_read, data, prot_err, &may_be_more);

        if (res)
            break;

        counter=_count_read*size;
        if (!fifo_mode)
            addr+=counter;
        data+=counter;
        count-=_count_read;
    } while (count && may_be_more && !res && !*prot_err);

    mutex_unlock(&sc->sem_hw);

    *count_read=orig_count-count;

    return res;
}
