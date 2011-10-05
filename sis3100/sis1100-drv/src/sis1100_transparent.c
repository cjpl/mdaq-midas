/* $ZEL: sis1100_transparent.c,v 1.1 2009/04/26 20:33:45 wuestner Exp $ */

/*
 * Copyright (c) 2009
 * 	Peter Wuestner.  All rights reserved.
 */

#include "sis1100_sc.h"

static void
wait_transparent(struct sis1100_softc* sc)
{
    u_int32_t sr;
    do {
        /* relax ... */
        sr=sis1100readreg(sc, sr);
    } while (!(sr&(sr_tp_special|sr_tp_data)));
}

int
sis1100_read_transparent(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    size_t count, size_t* count_read, void __user *data, int nonblocking)
{
    u_int32_t sr, val;

    sr=sis1100readreg(sc, sr);
    if (!(sr&(sr_tp_special|sr_tp_data)) && !nonblocking)
        wait_transparent(sc);
    while (count && sr&(sr_tp_special|sr_tp_data)) {
        while (sr&sr_tp_data) {
            val=sis1100readreg(sc, tp_data);
             __put_user(0x00000001, (u_int32_t*)data);
             __put_user(val, (u_int32_t*)data);
            (*count_read)+=2;
            sr=sis1100readreg(sc, sr);
        }

        while ((sr&(sr_tp_special|sr_tp_data))==sr_tp_special) {
            val=sis1100readreg(sc, tp_special);
             __put_user(0x80000001, (u_int32_t*)data);
             __put_user(val, (u_int32_t*)data);
            (*count_read)+=2;
            sr=sis1100readreg(sc, sr);
        }
    }
    return 0;
}

int
sis1100_write_transparent(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    size_t count, size_t* count_written, void __user *data, int nonblocking)
{
    return 0;
}

static int
start_transparent(struct sis1100_softc* sc, struct sis1100_fdata* fd)
{
    sis1100writereg(sc, cr, cr_transparent|cr_ready);
    mb_reg();
    sis1100readreg(sc, sr);
    sc->transparent.active=1;
    return 0;
}

static int
stop_transparent(struct sis1100_softc* sc, struct sis1100_fdata* fd)
{
    sc->transparent.active=0;
    sis1100writereg(sc, cr, cr_ready<<16);
    /* transparent bit is reset in sis1100_flush_fifo */
    return sis1100_flush_fifo(sc, 0);
}

int
sis1100_transparent(struct sis1100_softc* sc, struct sis1100_fdata* fd,
        int32_t* d)
{
    int res=0, old_mode=sc->transparent.active;

    if (*d>0)
        res=start_transparent(sc, fd);
    else if (*d==0)
        res=stop_transparent(sc, fd);

    *d=old_mode;
    return res;
}
