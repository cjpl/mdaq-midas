/* $ZEL: sis1100_tmp_read.c,v 1.11 2009/02/09 22:56:03 wuestner Exp $ */

/*
 * Copyright (c) 2001-2008
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
 * sis1100_tmp_read transports data only to kernel space
 */
int
sis1100_tmp_read(struct sis1100_softc* sc,
    	u_int32_t addr, int32_t am, int size, int space, u_int32_t* data)
{
    u_int32_t be, _data;
    u_int32_t error;
    u_int32_t head;

    be=((0x00f00000<<size)&0x0f000000)<<(addr&3);

    head=0x00000002|(space&0x3f)<<16|be;

    mutex_lock(&sc->sem_hw);

    if (am>=0) {
        head|=0x800;
        sis1100writereg(sc, t_am, am);
    }
    sis1100writereg(sc, t_hdr, head);
    wmb_reg();
    sis1100writereg(sc, t_adl, addr);
    mb_reg();
    do {
	error=sis1100readreg(sc, prot_error);
    } while (error==sis1100_e_dlock);
    rmb_reg();
    if (error==sis1100_le_dlock) {
        head=sis1100rawreadreg(sc, t_hdr);
        if ((head&0x300)==0x300)
            error=0x200|(head>>24);
        else
            error=0;
    }
    _data=sis1100rawreadreg(sc, tc_dal);

    mutex_unlock(&sc->sem_hw);

#if 0
    pERROR(sc, "tmp_read(addr=%08x size=%d): %08x", addr, size, _data);
#endif
#if defined(__LITTLE_ENDIAN)
        switch (size) {
            case 4: *data=_data; break;
            case 2: *data=(_data>>((addr&2)<<3))&0xffff; break;
            case 1: *data=(_data>>((addr&3)<<3))&0xff; break;
        }
#else
        switch (size) {
            case 4: *data=_data; break;
            case 2: *data=(_data>>((2-(addr&2))<<3))&0xffff; break;
            case 1: *data=(_data>>((3-(addr&3))<<3))&0xff; break;
        }
#endif
    return error;
}

int
sis1100_tmp_camacread(struct sis1100_softc* sc, u_int32_t addr, u_int32_t* data)
{
    u_int32_t error;
    u_int32_t head;

    head=0x0f010002; /* be=0xf space=1 write=0 start_with_addr */

    mutex_lock(&sc->sem_hw);
    sis1100writereg(sc, t_hdr, head);
    wmb_reg();
    sis1100writereg(sc, t_adl, addr);
    mb_reg();
    do {
            error=sis1100readreg(sc, prot_error);
    } while (error==sis1100_e_dlock);
    rmb_reg();
    if (error==sis1100_le_dlock) {
        head=sis1100rawreadreg(sc, t_hdr);
        if ((head&0x300)==0x300)
            error=0x200|(head>>24);
        else
            error=0;
    }
    *data=sis1100readreg(sc, tc_dal);
    mutex_unlock(&sc->sem_hw);

    return error;
}

int
sis1100_remote_reg_read(struct sis1100_softc* sc, u_int32_t offs,
    u_int32_t* data, int locked)
{
    u_int32_t error, head;

    if (!locked) mutex_lock(&sc->sem_hw);
    *data=_readreg32(sc, offs+0x800);
    rmb_reg();

    do {
	error=sis1100readreg(sc, prot_error);
    } while (error==sis1100_e_dlock);

    if (error==sis1100_le_dlock) {
        head=sis1100readreg(sc, tc_hdr);
        if ((head&0x300)==0x300) { /* error confirmation */
            error=((head>>24)&0xff)|0x200;
        } else {
            error=0;
            *data=sis1100readreg(sc, tc_dal);
        }
    }

    if (!locked) mutex_unlock(&sc->sem_hw);

    return error;
}
