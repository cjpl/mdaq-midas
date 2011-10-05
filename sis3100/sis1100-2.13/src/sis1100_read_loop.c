/* $ZEL: sis1100_read_loop.c,v 1.9 2008/06/03 18:03:08 wuestner Exp $ */

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

/*
 * sis1100_read_loop always transports data directly to userspace!
 * Access permissions have to be checked before.
 */

int
sis1100_read_loop(
    struct sis1100_softc* sc,
    struct sis1100_fdata* fd,
    u_int32_t addr,           /* VME or SDRAM address */
    int32_t am,               /* address modifier, not used if <0 */
    int size,                 /* datasize (bytes/word) */
    int space,                /* remote space (1,2: VME; 6: SDRAM) */
    int fifo_mode,
    size_t count,             /* words (of size 'size') to be transferred */
                              /* count==0 is illegal */
    size_t* count_read,       /* words transferred */
    u_int8_t* data,           /* destination (user virtual address) */
    int* prot_error
    )
{
    u_int32_t head;
    int idx, res=0;

#if 0
    pERROR(sc, "read_loop size=%d count=%llu addr=%08x",
        size, (unsigned long long)count, addr);
#endif

    *count_read=count;
    head=0x00000002|(space&0x3f)<<16;
    mutex_lock(&sc->sem_hw);
    if (am>=0) {
        head|=0x800;
        sis1100writereg(sc, t_am, am);
    }
    switch (size) {
    case 1:
        for (idx=0; idx<count; idx++, data++) {
            u_int32_t val;
            sis1100writereg(sc, t_hdr, head|(0x01000000<<(addr&3)));
            sis1100writereg(sc, t_adl, addr);
            do {
                *prot_error=sis1100readreg(sc, prot_error);
            } while (*prot_error==0x005);
            if (*prot_error) {
                *count_read=idx;
                break;
            }
            val=sis1100rawreadreg(sc, tc_dal);

#if defined(__LITTLE_ENDIAN)
            __put_user((val>>((addr&3)<<3))&0xff, (u_int8_t*)(data));
#else
            __put_user((val>>((3-(addr&3))<<3))&0xff, (u_int8_t*)(data));
#endif
            if (!fifo_mode)
                addr++;
        }
        break;
    case 2:
        for (idx=0; idx<count; idx++, data+=2) {
            u_int32_t val;
            sis1100writereg(sc, t_hdr, head|(0x03000000<<(addr&3)));
            sis1100writereg(sc, t_adl, addr);
            do {
                *prot_error=sis1100readreg(sc, prot_error);
            } while (*prot_error==0x005);
            if (*prot_error) {
                *count_read=idx;
                break;
            }
            val=sis1100rawreadreg(sc, tc_dal);

#if defined(__LITTLE_ENDIAN)
            __put_user((val>>((addr&2)<<3))&0xffff, (u_int16_t*)data);
#else
            __put_user((val>>((2-(addr&2))<<3))&0xffff, (u_int16_t*)data);
#endif
            if (!fifo_mode)
                addr+=2;
        }
        break;
    case 4:
        sis1100writereg(sc, t_hdr, head|0x0f000000);
        for (idx=0; idx<count; idx++, data+=4) {
            u_int32_t val;
            sis1100writereg(sc, t_adl, addr);
            do {
                *prot_error=sis1100readreg(sc, prot_error);
            } while (*prot_error==0x005);
            if (*prot_error) {
                *count_read=idx;
                break;
            }
            val=sis1100rawreadreg(sc, tc_dal);
            __put_user(val, (u_int32_t*)data);
            if (!fifo_mode)
                    addr+=4;
        }
        break;
    }
    mutex_unlock(&sc->sem_hw);
    return res;
}
