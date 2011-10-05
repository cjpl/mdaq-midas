/* $ZEL: sis1100_init_remote.c,v 1.19 2010/01/18 19:01:40 wuestner Exp $ */

/*
 * Copyright (c) 2001-2004
 * 	Matthias Drochner, Peter Wuestner.  All rights reserved.
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

static char* rnames[]= {
    "PCI",
    "VME",
    "CAMAC",
    "LVD",
    "PANDAPIXEL",
    "PSF4AD",
};

/*
 * sis1100_init_remote:
 *     initializes remote hardware and local data stuctures for remote part
 *     value for reset:
 *         0: dont change hardware setup (link reconnected only)
 *         1: remote reset already done by sis1100_init
 *         2: try to reset all and reinitialize (requested by user)
 */
void
sis1100_init_remote(struct sis1100_softc* sc, int reset)
{
    u_int32_t ident, typ, hv, fk, fv;
    int res;

    mutex_lock(&sc->sem_hw);

    if (reset>1) {
        sis1100writereg(sc, cr, cr_rem_reset);
        mdelay(500);
    }

    if (sis1100_flush_fifo(sc, 0)) /* clear local fifo */
        goto error;
    sis1100writereg(sc, p_balance, 0);
    sis1100readreg(sc, prot_error);

    res=sis1100_remote_reg_read(sc, 0, &ident, 1);
    if (res==0x101) { /* no hope, giving up */
        pERROR(sc, "No link.");
        goto error;
    }

    if (res) { /* error, but not hopeless */
        sis1100writereg(sc, cr, cr_rem_reset);
        mdelay(500);
        sis1100_flush_fifo(sc, 0);
        sis1100writereg(sc, p_balance, 0);
        sis1100readreg(sc, prot_error);
        res=sis1100_remote_reg_read(sc, 0, &ident, 1); /* try again */
        if (res) { /* no, hopeless */
            pERROR(sc, "error reading remote ident: err=0x%x", res);
            goto error;
        }
    }

    typ=ident&0xff;
    hv=(ident>>8)&0xff;
    fk=(ident>>16)&0xff;
    fv=(ident>>24)&0xff;
    pINFO(sc, "remote ident: 0x%08x", ident);
    if ((typ>0) && (typ<=sizeof(rnames)/sizeof(char*)))
        pINFO(sc, "remote is %s", rnames[typ-1]);
    else
        pERROR(sc, "unknown remote type %d", ident&0xff);
    pINFO(sc, "remote HW_ver %d FW_code %d FW_ver %d", hv, fk, fv);
    sc->remote_ident=ident;

    /* some defaults */
    sc->dsp_present=0;
    sc->ram_size=0;


/* swapping is undefind here; it even would depend on arbitrary user settings */
    switch (typ) {
        case sis1100_hw_pci: /* PCI */
            res=sis1100rem_init(sc, reset);
            sc->remote_endian=1; /* big endian; the remote side has to swap */
            break;
        case sis1100_hw_vme: /* VME */
            res=sis3100rem_init(sc, reset);
            sc->remote_endian=1; /* big endian */
            break;
        case sis1100_hw_camac: /* CAMAC */
            res=sis5100rem_init(sc, reset);
            sc->remote_endian=1;
            break;
        case sis1100_hw_lvd: /* (FZJ/ZEL LVDS readout system) */
            res=zellvd_rem_init(sc, reset);
            sc->remote_endian=0; /* Willi always uses little endian */
            break;
        case sis1100_hw_pandapixel: /* (Panda Pixel detector) */
            res=pandapixel_rem_init(sc, reset);
            sc->remote_endian=0;
            break;
        case sis1100_hw_psf4ad: /* (Quad-14Bit-ADC) */
            res=psf4ad_rem_init(sc, reset);
            sc->remote_endian=0;
            break;
        default:
            pINFO(sc, "remote device type not (yet) supported.");
            sc->remote_endian=1; /* just a default */
            res=-1;
            break;
    }
    if (res)
        goto error;
    sis1100_update_swapping(sc);

    sc->remote_hw=typ;

error:
    mutex_unlock(&sc->sem_hw);
}
