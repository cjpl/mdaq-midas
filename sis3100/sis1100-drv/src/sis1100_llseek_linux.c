/* $ZEL: sis1100_llseek_linux.c,v 1.4 2008/11/24 02:00:13 wuestner Exp $ */

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

/* llseek is only needed for LINUX */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

loff_t sis1100_llseek(struct file* file, loff_t offset, int orig)
{
    struct sis1100_softc* sc=SIS1100SC(file);
    struct sis1100_fdata* fd=SIS1100FD(file);
    loff_t old=file->f_pos;

    switch (orig) {
        case SEEK_SET: file->f_pos=offset; break;
        case SEEK_CUR: file->f_pos+=offset; break;
        case SEEK_END:
            if (fd->subdev==sis1100_subdev_ram) {
                if (sc->remote_hw==sis1100_hw_invalid)
                    file->f_pos=offset;
                else
                    file->f_pos=sc->ram_size+offset;
            } else
                return -EINVAL;
            break;
    }
    if ((file->f_pos<0) ||
        (file->f_pos>
            ((fd->subdev==sis1100_subdev_ram)?sc->ram_size:0xffffffffU))) {
        file->f_pos=old;
        return -EINVAL;
    }
    return file->f_pos;
}
