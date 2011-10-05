/* $ZEL: sis1100_write.c,v 1.7 2009/04/26 20:28:22 wuestner Exp $ */

/*
 * Copyright (c) 2003-2004
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
sis1100_write_irq(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    size_t count, size_t* count_written, const void __user *data)
{
    int res;

    /* count too small? */
    if (count<sizeof(struct sis1100_irq_ack))
        return EINVAL;

    if (count==sizeof(struct sis1100_irq_ack)) {
        struct sis1100_irq_ack ack;
        if (copy_from_user(&ack, data, count))
            return -EFAULT;
        res=sis1100_irq_ack(fd, &ack);
    } else if (count>=sizeof(struct sis1100_irq_ctl)) {
        struct sis1100_irq_ctl2 ctl;
        size_t xcount;
        ctl.auto_mask=ctl.flags=0;
        xcount=count;
        if (xcount>sizeof(struct sis1100_irq_ctl2))
            xcount=sizeof(struct sis1100_irq_ctl2);
        if (copy_from_user(&ctl, data, xcount))
            return -EFAULT;
        res=sis1100_irq_ctl(fd, &ctl);
    } else {
        res=EINVAL;
    }

    *count_written=count;
    return res;
}

static int
_sis1100_write(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    size_t count, size_t* count_written, u_int32_t addr,
    const char __user *data)
{
    int32_t am;
    int datasize;
    int space, fifo;
    int res;

#if 0
    pERROR(sc, "sis1100_write data=%p addr=%08x count=%llu size=%d",
        data, addr, (unsigned long long)count, fd->vmespace_datasize);
#endif
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
    case sis1100_subdev_dsp:
        am=-1;
        datasize=4;
        space=6;
        fifo=0;
        break;
    case sis1100_subdev_ctrl:
        return sis1100_write_irq(sc, fd, count, count_written, data);
    default:
        return ENOTTY;
    }

    if (count%datasize)
        return EINVAL;

    res=sis1100_write_block(sc, fd,
        datasize, fifo, count/datasize, count_written, space, am,
        addr, data, &fd->last_prot_err);
    *count_written*=datasize;

    return res;
}

#ifdef __NetBSD__
int
sis1100_write(dev_t dev, struct uio* uio, int f)
{
    struct sis1100_softc* sc=SIS1100SC(dev);
    struct sis1100_fdata* fd=SIS1100FD(dev);
    size_t count, count_written;
    u_int32_t addr;
    char* data;
    int res;

    count=uio->uio_resid;
    addr=uio->uio_offset;
    data=uio->uio_iov->iov_base;

    res=_sis1100_write(sc, fd, count, &count_written, addr, data);

    if (!res) {
        uio->uio_resid-=count_written;
    }
    return res;
}

#elif __linux__
ssize_t
sis1100_write(struct file* file, const char __user *buf, size_t count,
    loff_t* ppos)
{
    struct sis1100_softc* sc=SIS1100SC(file);
    struct sis1100_fdata* fd=SIS1100FD(file);
    size_t count_written;
    int res;

    res=_sis1100_write(sc, fd, count, &count_written, *ppos, buf);
    if (!res)
        *ppos+=count_written;

    return res?-res:count_written;
}

#endif
