/* $ZEL: sis1100_poll.c,v 1.7 2008/01/17 15:55:07 wuestner Exp $ */

/*
 * Copyright (c) 2004-2008
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

#ifdef __NetBSD__

int
sis1100_poll(dev_t dev, int events, struct proc *p)
{
    struct sis1100_softc* sc=SIS1100SC(dev);
    struct sis1100_fdata* fd=SIS1100FD(dev);
    int s;

    s = splbio();

    if (events & (POLLIN | POLLRDNORM)) {
        if (irq_pending(sc, fd, fd->owned_irqs)) {
            splx(s);
            return (events & (POLLIN | POLLRDNORM));
        }
        selrecord(p, &sc->sel);
    }
    splx(s);
    return 0;
}

#elif __linux__

unsigned int
sis1100_poll(struct file* file, struct poll_table_struct* poll_table)
{
    struct sis1100_softc* sc=SIS1100SC(file);
    struct sis1100_fdata* fd=SIS1100FD(file);

    poll_wait(file, &sc->remoteirq_wait, poll_table);

    if (irq_pending(sc, fd, fd->owned_irqs)) {
        return POLLIN|POLLRDNORM;
    } else {
        return 0;
    }
}

#endif
