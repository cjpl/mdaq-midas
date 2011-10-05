/* $ZEL: sis1100_read_block.c,v 1.13 2009/04/26 20:28:22 wuestner Exp $ */

/*
 * Copyright (c) 2001-2009
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
 * size    : number of bytes per dataword (1, 2 or 4)
 * num     : number of words to be read
 * num_read: number of words read
 */
int
sis1100_read_block(struct sis1100_softc* sc, struct sis1100_fdata* fd,
        int size, int fifo, size_t num, size_t* num_read, int space,
        int32_t am, u_int32_t addr, u_int8_t* __user data, u_int32_t* error)
{
    int res=0;

#if 0
    pERROR(sc, "read_block data=%p, size=%d, fifo=%d num=%llu am=%02x addr=%08x",
            data, size, fifo, (unsigned long long)num, am, addr);
    pERROR(sc, "mindmalen=%lu", fd->mindmalen_r);
#endif

    if (!num) {
        *error=0;
        *num_read=0;
        return 0;
    }

    if (sc->remote_hw==sis1100_hw_invalid)
        return ENXIO;
#if 0
    if ((sc->remote_hw==sis1100_hw_lvd) && (num*size<4096)) {
/*
 *         firmware bug in LVD controller
 *         there is always the whole FIFO content read regardless of the
 *         requested length
 */
        pERROR(sc, "lvd block read needs at least 4096 bytes (not %lld)",
                (unsigned long long)(num*size));
        return EINVAL;
    }
#endif

    if (!ACCESS_OK(data, num*size, 0))
        return EFAULT;

    if (fd->mindmalen_r && num>=fd->mindmalen_r) {
        res=sis1100_read_dma(sc, fd, addr, am, size,
                space, fifo, num, num_read, data, error);
    } else if (fd->minpipelen_r && (num>=fd->minpipelen_r)) {
        res=sis1100_read_pipe_seq(sc, fd, addr, am, size,
                space, fifo, num, num_read, data, error);
    } else {
        if (num==1) {
            u_int32_t val;
            *error=sis1100_tmp_read(sc, addr, am, size, space, &val);

            switch (size) {
            case 4: __put_user(val, (u_int32_t*)data); break;
            case 2: __put_user(val, (u_int16_t*)data); break;
            case 1: __put_user(val, (u_int8_t*)data); break;
            }
            *num_read=!*error;
        } else {
            res=sis1100_read_loop(sc, fd, addr, am, size,
                    space, fifo, num, num_read, data, error);
        }
    }
    return res;
}
