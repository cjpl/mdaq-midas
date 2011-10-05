/* $ZEL: sis1100_write_loop.c,v 1.9 2009/02/09 22:56:03 wuestner Exp $ */

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

static int
wait_for_fifo(struct sis1100_softc* sc, int fifolen, int* cc)
{
    while (*cc>=fifolen) {
        u_int32_t sr, err;
        cpu_relax();
        sr=sis1100readreg(sc, sr);
        if (sr&0x4000) {
            //pERROR(sc, "sr=%08x", sr);
            err=sis1100readreg(sc, prot_error);
            //pERROR(sc, "error=0x%x", err);
            return err;
        }
        *cc=sis1100readreg(sc, p_balance);
    }
    return 0;
}

/*
 * sis1100_write_loop always transports data directly from userspace!
 * Access permissions have to be checked before.
 */

int
sis1100_write_loop(
    struct sis1100_softc* sc,
    struct sis1100_fdata* fd,
    u_int32_t addr,           /* VME or SDRAM address */
    int32_t am,               /* address modifier, not used if <0 */
    int size,                 /* datasize */
    int space,                /* remote space (1,2: VME; 6: SDRAM) */
    int fifo_mode,
    size_t count,             /* words (of size 'size') to be transferred */
                              /* count==0 is illegal */
    size_t* count_written,    /* words transferred */
    const u_int8_t* data,     /* source (user virtual address) */
    int* prot_error
    )
{
    u_int32_t head, error, balance;
    int idx, cc=0, fifolen, lcount;

#if 0
    pERROR(sc, "write_loop size=%d, fifo=%d num=%llu am=%02x addr=%08x",
            size, fifo_mode, (unsigned long long)count, am, addr);
#endif

    *count_written=count;
    *prot_error=0;
    head=0x00000404|(space&0x3f)<<16;

    mutex_lock(&sc->sem_hw);

    if (am>=0) {
        fifolen=sc->sendfifo_size/4-1; /* 4 words per request */
        head|=0x800;
        sis1100writereg(sc, t_am, am);
    } else {
        fifolen=sc->sendfifo_size/3-1; /* 3 words per request */
    }

    sis1100writereg(sc, sr, irq_prot_l_err);

    switch (size) {
    case 1:
        if (fifo_mode) {
            sis1100writereg(sc, t_hdr, head|(0x01000000<<(addr&3)));
            sis1100writereg(sc, t_adl, addr);
            for (idx=0; idx<count; idx++, data++) {
                u_int32_t val;
                __get_user(val, (u_int8_t*)data);
                val&=0xff;
                val|=val<<8;
                val|=val<<16;
                sis1100writereg(sc, t_dal, val);
                cc++;
                if ((*prot_error=wait_for_fifo(sc, fifolen, &cc)))
                    break;
            }
        } else {
            for (idx=0; idx<count; idx++, data++, addr++) {
                u_int32_t val;
                __get_user(val, (u_int8_t*)data);
                val&=0xff;
                val|=val<<8;
                val|=val<<16;
                sis1100writereg(sc, t_hdr, head|(0x01000000<<(addr&3)));
                sis1100writereg(sc, t_adl, addr);
                sis1100writereg(sc, t_dal, val);
                cc++;
                if ((*prot_error=wait_for_fifo(sc, fifolen, &cc)))
                    break;
            }
        }
        break;
    case 2:
        if (fifo_mode) {
            sis1100writereg(sc, t_hdr, head|(0x03000000<<(addr&3)));
            sis1100writereg(sc, t_adl, addr);
            for (idx=0; idx<count; idx++, data+=2) {
                u_int32_t val;
                __get_user(val, (u_int16_t*)data);
                val&=0xffff;
                val|=val<<16;
                sis1100writereg(sc, t_dal, val);
                cc++;
                if ((*prot_error=wait_for_fifo(sc, fifolen, &cc)))
                    break;
            }
        } else {
            for (idx=0; idx<count; idx++, data+=2, addr+=2) {
                u_int32_t val;
                __get_user(val, (u_int16_t*)data);
                val&=0xffff;
                val|=val<<16;
                sis1100writereg(sc, t_hdr, head|(0x03000000<<(addr&3)));
                sis1100writereg(sc, t_adl, addr);
                sis1100writereg(sc, t_dal, val);
                cc++;
                if ((*prot_error=wait_for_fifo(sc, fifolen, &cc)))
                    break;
            }
        }
        break;
    case 4:
        sis1100writereg(sc, t_hdr, head|0x0f000000);
        if (fifo_mode) {
            sis1100writereg(sc, t_adl, addr);
            for (idx=0; idx<count; idx++, data+=4) {
                u_int32_t val;
                __get_user(val, (u_int32_t*)data);
                sis1100writereg(sc, t_dal, val);
                cc++;
                if ((*prot_error=wait_for_fifo(sc, fifolen, &cc)))
                    break;
            }
        } else {
            for (idx=0; idx<count; idx++, data+=4, addr+=4) {
                u_int32_t val;

                __get_user(val, (u_int32_t*)data);
                sis1100writereg(sc, t_adl, addr);
                sis1100writereg(sc, t_dal, val);
                cc++;
                if ((*prot_error=wait_for_fifo(sc, fifolen, &cc)))
                    break;
            }
        }
        break;
    }

    /*
     * reading prot_error blocks until p_balance==0 OR prot_error!=0
     * ==> we have to read prot_error until p_balance is 0
     * prot_error is reset to 0 after read
     * ==> we have to store the first prot_error!=0
     */
    lcount=0;
    do {
        balance=sis1100readreg(sc, p_balance);
        do {
            error=sis1100readreg(sc, prot_error);
        } while (error==sis1100_e_dlock);
        if (error==sis1100_le_dlock) {
            head=sis1100readreg(sc, tc_hdr);
            if ((head&0x300)==0x300) {
                error=((head>>24)&0xff)|0x200;
            } else {
                error=0;
            }
        }
        lcount++;
        if (error && !*prot_error)
            *prot_error=error;
        if (error==sis1100_le_to) {
            sis1100writereg(sc, p_balance, 0);
        }
    } while (balance>0 && lcount<1000000);
{
    if (balance) {
        pERROR(sc, "write_loop: size=%d, fifo=%d, num=%llu",
                size, fifo_mode, (unsigned long long)count);
        pERROR(sc, "balance=%d, prot_error=0x%x, error=0x%x, lcount=%d",
                balance, *prot_error, error, lcount);
    }
}
    mutex_unlock(&sc->sem_hw);

    /*
     * In case of error this is not really correct, but we don't know how
     * many data have been successfully written before an error occured.
     */
    *count_written=*prot_error?0:count;

    return 0;
}
