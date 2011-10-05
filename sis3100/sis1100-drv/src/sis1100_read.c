/* $ZEL: sis1100_read.c,v 1.15 2009/04/26 20:28:21 wuestner Exp $ */

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

static int
sis1100_read_irqdata(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    size_t count, size_t* count_read, void __user *data, int nonblocking)
{
    struct sis1100_irq_get2 get;
    int res, version=1;

    /* count too small? */
    if (count<sizeof(struct sis1100_irq_get))
        return EINVAL;
    /* count large enough for sis1100_irq_get2? */
    if (count>=sizeof(struct sis1100_irq_get2))
        version=2;

    if (nonblocking && !irq_pending(sc, fd, fd->owned_irqs))
        return EAGAIN;

    get.irq_mask=fd->owned_irqs;
    res=sis1100_irq_wait(fd, &get);
    if (res) {
        pINFO(sc, "read_irqdata: res=%d", res);
        return res;
    }

    count=version==1?
            sizeof(struct sis1100_irq_get):sizeof(struct sis1100_irq_get2);
    if (COPY_TO_USER(data, &get, count))
        return EFAULT;

    *count_read=count;
    return 0;
}

static int
_sis1100_read(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    size_t count, size_t* count_read, u_int32_t addr, void __user *data,
    int nonblocking)
{
    int32_t am;
    int datasize;
    int space, fifo;
    int res;

    switch (fd->subdev) {
    case sis1100_subdev_ram:
        am=-1;
        datasize=4;
        space=6;
        fifo=0;
        break;
    case sis1100_subdev_remote:
        am=fd->vmespace_am;
        datasize=fd->vmespace_datasize;
        space=1;
        fifo=fd->fifo_mode;
        break;
    case sis1100_subdev_ctrl:
        return sis1100_read_irqdata(sc, fd, count, count_read, data,
                nonblocking);
    case sis1100_subdev_dsp:
        if (!sc->transparent.active)
            return ENOTTY;
        else
            return sis1100_read_transparent(sc, fd, count, count_read, data,
                nonblocking);
    default:
        return ENOTTY;
    }

    if (count%datasize)
        return EINVAL;

    res=sis1100_read_block(sc, fd,
        datasize, fifo, count/datasize, count_read, space, am,
        addr, data, &fd->last_prot_err);
    *count_read*=datasize;

    return res;
}

#ifdef __NetBSD__

int
sis1100_read(dev_t dev, struct uio* uio, int f)
{
    struct sis1100_softc* sc=SIS1100SC(dev);
    struct sis1100_fdata* fd=SIS1100FD(dev);
    size_t count, count_read;
    u_int32_t addr;
    void* data;
    int res;

    count=uio->uio_resid;
    addr=uio->uio_offset;
    data=uio->uio_iov->iov_base;

    res=_sis1100_read(sc, fd, count, &count_read, addr, data,
            0 /* XXX O_NONBLOCK !! */);
    if (!res)
        uio->uio_resid-=count_read;
    return res;
}

#elif __linux__

ssize_t sis1100_read(struct file* file, char __user *buf, size_t count,
    loff_t* ppos)
{
    struct sis1100_softc* sc=SIS1100SC(file);
    struct sis1100_fdata* fd=SIS1100FD(file);
    size_t count_read;
    int res;

    res=_sis1100_read(sc, fd, count, &count_read, *ppos, buf,
            file->f_flags&O_NONBLOCK);
    /* ppos is always zero for subdev==ctrl (irq data) */
    if (!res && fd->subdev!=sis1100_subdev_ctrl)
        *ppos+=count_read;
    return res?-res:count_read;
}

#endif
