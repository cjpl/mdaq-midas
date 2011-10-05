/* $ZEL: sis1100_dsp.c,v 1.7 2008/06/19 19:19:53 wuestner Exp $ */

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
#include "sis3100_map.h"
#include "sis5100_map.h"

int
sis1100_dsp_reset(struct sis1100_softc* sc, struct sis1100_fdata* fd)
{
    int error;

    if (!sc->dsp_present)
        return ENXIO;
    switch (sc->remote_hw) {
        case sis1100_hw_vme:
            mutex_lock(&sc->sem_hw);
            sis3100writeremreg(sc, dsp_sc, sis3100_dsp_boot_ctrl, 1);
            error=sis3100writeremreg(sc, dsp_sc, sis3100_dsp_run<<16, 1);
            mutex_unlock(&sc->sem_hw);
            break;
        case sis1100_hw_camac:
            mutex_lock(&sc->sem_hw);
            sis5100writeremreg(sc, dsp_sc, sis5100_dsp_boot_ctrl, 1);
            error=sis5100writeremreg(sc, dsp_sc, sis5100_dsp_run<<16, 1);
            mutex_unlock(&sc->sem_hw);
            break;
        default:
            return ENOTTY;
    }
#if 0
    if (error)
        pINFO(sc, "dsp_reset: error=0x%x", error);
#endif
    return error?EIO:0;
}

int
sis1100_dsp_start(struct sis1100_softc* sc, struct sis1100_fdata* fd)
{
    int error;

    if (!sc->dsp_present) return ENXIO;
    switch (sc->remote_hw) {
        case sis1100_hw_vme:
            mutex_lock(&sc->sem_hw);
            sis3100writeremreg(sc, dsp_sc, sis3100_dsp_boot_ctrl, 1);
            error=sis3100writeremreg(sc, dsp_sc, sis3100_dsp_run, 1);
            mutex_unlock(&sc->sem_hw);
            break;
        case sis1100_hw_camac:
            mutex_lock(&sc->sem_hw);
            sis5100writeremreg(sc, dsp_sc, sis5100_dsp_boot_ctrl, 1);
            error=sis5100writeremreg(sc, dsp_sc, sis5100_dsp_run, 1);
            mutex_unlock(&sc->sem_hw);
            break;
        default:
            return ENOTTY;
    }
#if 0
    if (error)
        pINFO(sc, "dsp_start: error=0x%x", error);
#endif
    return error?EIO:0;
}

static int
write_to_dsp(struct sis1100_softc* sc, u_int32_t addr, u_int32_t data)
{
    u_int32_t error, head=0x0f060402;

    sis1100writereg(sc, t_hdr, head);
    wmb_reg();
    sis1100writereg(sc, t_dal, data);
    wmb_reg();
    sis1100writereg(sc, t_adl, addr);
    mb_reg();
    do {
        error=sis1100readreg(sc, prot_error);
    } while (error==sis1100_e_dlock);
    return error;
}

static int
read_from_dsp(struct sis1100_softc* sc, u_int32_t addr, u_int32_t* data)
{
    u_int32_t error, head=0x0f060002;

    sis1100writereg(sc, t_hdr, head);
    wmb_reg();
    sis1100writereg(sc, t_adl, addr);
    mb_reg();
    do {
	error=sis1100readreg(sc, prot_error);
    } while (error==sis1100_e_dlock);
    rmb_reg();
    *data=sis1100rawreadreg(sc, tc_dal);
    return error;
}

int
sis1100_dsp_load(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_dsp_code* d)
{
    u_int8_t* __user code=d->src;
    u_int8_t v[6];
    u_int32_t addr, w0, w12;
    int error=0, i, j;

    if (!sc->dsp_present)
        return ENXIO;

    if (d->dst%4)
        return EINVAL;
    if (d->size%6)
        return EINVAL;

    if (!access_ok(VERIFY_READ, code, d->size))
        return EFAULT;

    addr=d->dst+sis3100_sharc_sram;
    for (i=d->size/6; i && !error; i--) {
        for (j=0; j<6; j++) {
#ifdef __NetBSD__
            v[j]=fubyte(code);
#elif __linux__
            __get_user(v[j], code);
#endif
            code++;
        }
#if 0
        pINFO(sc, "%08x %02x%02x%02x%02x%02x%02x",
                (addr-sis3100_sharc_sram)>>2,
                v[0], v[1], v[2], v[3], v[4], v[5]);
#endif
        w0=v[5] | (v[4]<<8);
        w12=v[3] | (v[2]<<8) | (v[1]<<16) | (v[0]<<24);
        mutex_lock(&sc->sem_hw);
        error=write_to_dsp(sc, sis3100_sharc_d48, w0);
        error|=write_to_dsp(sc, addr, w12);
        mutex_unlock(&sc->sem_hw);
        addr+=4;
    }
    return error?EIO:0;
}

int
sis1100_dsp_read(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_dsp_code* d)
{
    u_int8_t* __user code=d->src;
    u_int8_t v[6];
    u_int32_t addr, w0, w12;
    int error=0, i, j;

    if (!sc->dsp_present)
        return ENXIO;

    if (d->dst%4)
        return EINVAL;
    if (d->size%6)
        return EINVAL;

    if (!access_ok(VERIFY_WRITE, code, d->size)) {
        pERROR(sc, "dsp_read: FAULT");
        return EFAULT;
    }

    addr=d->dst+sis3100_sharc_sram;
    for (i=d->size/6; i && !error; i--) {
        mutex_lock(&sc->sem_hw);
        error=read_from_dsp(sc, addr, &w12);
        error|=read_from_dsp(sc, sis3100_sharc_d48, &w0);
        mutex_unlock(&sc->sem_hw);
        v[5]=w0&0xff;
        v[4]=(w0>>8)&0xff;
        v[3]=w12&0xff;
        v[2]=(w12>>8)&0xff;
        v[1]=(w12>>16)&0xff;
        v[0]=(w12>>24)&0xff;
#if 0
        pINFO(sc, "%08x %02x%02x%02x%02x%02x%02x",
                (addr-sis3100_sharc_sram)>>2,
                v[0], v[1], v[2], v[3], v[4], v[5]);
#endif
        for (j=0; j<6; j++) {
#ifdef __NetBSD__
            subyte(v[j], code);
#elif __linux__
            __put_user(v[j], code);
#endif
            code++;
        }
        addr+=4;
    }
    return error?EIO:0;
}

int
sis1100_dsp_wr(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_dsp_code* d)
{
    u_int8_t * __user code=d->src;
    u_int32_t addr;
    u_int32_t dat;
    int ct;

    if (!sc->dsp_present)
        return ENXIO;

    if (d->dst%4)
        return EINVAL;
    if (d->size%4)
        return EINVAL;
    if (!access_ok(VERIFY_READ, code, d->size))
        return EFAULT;

    addr = d->dst + sis3100_sharc_sram;
    for (ct=0; ct<d->size; ct+=4) {
        __get_user(dat, (u_int32_t*)code);
        if (sis1100_tmp_write(sc, addr, -1/*am*/, 4/*datasize*/,
                6/*space*/, dat)!=0) {
            return EIO;
        }

        code+=4;
        addr+=4;
    }
    return 0;
}

int
sis1100_dsp_rd(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_dsp_code* d)
{
    u_int8_t * __user code=d->src;
    u_int32_t addr;
    u_int32_t dat;
    int ct;

    if (d->dst%4)
        return EINVAL;
    if (d->size%4)
        return EINVAL;
    if (!access_ok(VERIFY_WRITE, code, d->size))
        return EFAULT;

    addr = d->dst + sis3100_sharc_sram;
    for (ct=0; ct<d->size; ct+=4) {
        if (sis1100_tmp_read(sc, addr, -1/*am*/, 4/*datasize*/,
                6/*space*/, &dat)!=0) {
            return EIO;
        }
        __put_user(dat, (u_int32_t*)code);

        code+=4;
        addr+=4;
    }

    return 0;
}
