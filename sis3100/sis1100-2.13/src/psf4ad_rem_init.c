/* $ZEL: psf4ad_rem_init.c,v 1.3 2010/01/18 19:01:14 wuestner Exp $ */

#include "sis1100_sc.h"

int
psf4ad_rem_init(struct sis1100_softc* sc, int reset)
{
/* sc->sem_hw must be held by caller!*/
#define MIN_FV 0
#define MAX_FV 255

    u_int32_t hv, fk, fv;

    hv=(sc->remote_ident>>8)&0xff;
    fk=(sc->remote_ident>>16)&0xff;
    fv=(sc->remote_ident>>24)&0xff;

    switch (hv) {
    case 1:
        if (fv<MIN_FV) {
            pERROR(sc, "psf4ad: remote firmware version too old;"
                    " at least version %d is required.",
                    MIN_FV);
            return -1;
        }
        if (fv>MAX_FV) {
            pINFO(sc, "psf4ad: Driver not tested with"
                    " remote firmware versions higher than %d.",
                    MAX_FV);
        }
        break;
    default:
        pERROR(sc, "psf4ad: remote hw/fw 0x%08x not supported",
            sc->remote_ident);
    }

    sc->dsp_present=0;

    return 0;
}
