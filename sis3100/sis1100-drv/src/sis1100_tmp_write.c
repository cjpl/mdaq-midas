/* $ZEL: sis1100_tmp_write.c,v 1.11 2009/04/26 20:28:22 wuestner Exp $ */

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
 * sis1100_tmp_write transports data only from kernel space
 */
int
sis1100_tmp_write(struct sis1100_softc* sc,
    	u_int32_t addr, int32_t am, int size, int space, u_int32_t data)
{
    u_int32_t be;
    u_int32_t error;
    u_int32_t head;

    /* the following switch cold be replaced by one line if firmware of
       sis3104 would be correct:
       data=(data&(0xffffffffU>>((4-size)<<3)))<<((addr&3)<<3);
    */
    switch (size) {
    case 4: /* do nothing */
        break;
    case 2:
        data = data & 0xffff ;
        data = data + (data<<16);
        break;
    case 1:
        data = data & 0xff ;
        data = data + (data<<8) + (data<<16) + (data<<24);
        break;
    }

    be=((0x00f00000<<size)&0x0f000000)<<(addr&3);

    head=0x00000402|(space&0x3f)<<16|be;

    mutex_lock(&sc->sem_hw);

    if (am>=0) {
        head|=0x800;
        sis1100writereg(sc, t_am, am);
    }
    sis1100writereg(sc, t_hdr, head);
    wmb_reg();
    sis1100writereg(sc, t_dal, data);
    wmb_reg();
    sis1100writereg(sc, t_adl, addr);
    mb_reg();
    do {
        error=sis1100readreg(sc, prot_error);
    } while (error==sis1100_e_dlock);
    if (error==sis1100_le_dlock) {
        head=sis1100rawreadreg(sc, t_hdr);
        if ((head&0x300)==0x300)
            error=0x200|(head>>24);
        else
            error=0;
    }

    mutex_unlock(&sc->sem_hw);

    return error;
}

/*
 * sis1100_tmp_write_blind is similar to sis1100_tmp_write but does not
 * wait for any confirmation (this is usefull if the receive FIFO is filled
 * up with other data 
 */
int
sis1100_tmp_write_blind(struct sis1100_softc* sc,
    	u_int32_t addr, int32_t am, int size, int space, u_int32_t data)
{
    u_int32_t be;
    u_int32_t head;

    data=(data&(0xffffffffU>>((4-size)<<3)))<<((addr&3)<<3);

    be=((0x00f00000<<size)&0x0f000000)<<(addr&3);

    head=0x00000402|(space&0x3f)<<16|be;

    mutex_lock(&sc->sem_hw);

    if (am>=0) {
        head|=0x800;
        sis1100writereg(sc, t_am, am);
    }
    sis1100writereg(sc, t_hdr, head);
    wmb_reg();
    sis1100writereg(sc, t_dal, data);
    wmb_reg();
    sis1100writereg(sc, t_adl, addr);
    mb_reg();
    sis1100readreg(sc, ident); /* dummy read (to flush PCI writes) */

    mutex_unlock(&sc->sem_hw);

    return 0;
}

int
sis1100_tmp_camacwrite(struct sis1100_softc* sc, u_int32_t addr, u_int32_t data)
{
    u_int32_t error;
    u_int32_t head;

    head=0x0f010402; /* be=0xf space=1 write=1 start_with_addr */

    mutex_lock(&sc->sem_hw);
    sis1100writereg(sc, t_hdr, head);
    wmb_reg();
    sis1100writereg(sc, t_dal, data);
    wmb_reg();
    sis1100writereg(sc, t_adl, addr);
    mb_reg();
    do {
        error=sis1100readreg(sc, prot_error);
    } while (error==sis1100_e_dlock);
    if (error==sis1100_le_dlock) {
        head=sis1100rawreadreg(sc, t_hdr);
        if ((head&0x300)==0x300)
            error=0x200|(head>>24);
        else
            error=0;
    }
    mutex_unlock(&sc->sem_hw);
    return error;
}

int
sis1100_remote_reg_write(struct sis1100_softc* sc, u_int32_t offs,
    u_int32_t data, int locked)
{
    u_int32_t head, error;

    if (!locked) mutex_lock(&sc->sem_hw);
    _writereg32(sc, offs+0x800, data);
    mb_reg();
    do {
        error=sis1100readreg(sc, prot_error);
    } while (error==sis1100_e_dlock);
    if (error==sis1100_le_dlock) {
        head=sis1100rawreadreg(sc, t_hdr);
        if ((head&0x300)==0x300)
            error=0x200|(head>>24);
        else
            error=0;
    }
    if (!locked) mutex_unlock(&sc->sem_hw);

    return error;
}
