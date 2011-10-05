/* $ZEL: sis1100_write_dma.c,v 1.7 2008/06/03 18:03:08 wuestner Exp $ */

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

int
sis1100_write_dma(
    struct sis1100_softc* sc,
    struct sis1100_fdata* fd,
    u_int32_t addr,           /* VME or SDRAM address */
    int32_t am,               /* address modifier, not used if <0 */
    int size,                 /* datasize must be 4 for DMA but is not checked*/
    int space,                /* remote space (1,2: VME; 6: SDRAM) */
    int fifo_mode,
    size_t count,             /* words to be transferred */
                              /* count==0 is illegal */
    size_t* count_written,    /* words transferred */
    const u_int8_t* data,     /* source (user virtual address) */
    int* prot_err
    )
{
    int res;
    size_t _count_written, orig_count=count;

    mutex_lock(&sc->sem_hw);
    do {
        size_t counter;
        res=_sis1100_write_dma(sc, fd, addr, am, size, space, fifo_mode,
                count, &_count_written, data, prot_err);

        if (res)
            break;

        counter=_count_written*size;
        if (!fifo_mode)
            addr+=counter;
        data+=counter;
        count-=_count_written;
    } while (count && !res && !*prot_err);

    mutex_unlock(&sc->sem_hw);

    *count_written=orig_count-count;
    return res;
}
