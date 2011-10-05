/* $ZEL: sis1100_dma_alloc_linux.c,v 1.3 2008/11/19 21:45:45 wuestner Exp $ */

/*
 * Copyright (c) 2005
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

#if !defined(__linux__)
#error Invalid Operating System
#endif

#if 0
int
sis1100_dma_alloc(struct sis1100_softc *sc, struct sis1100_fdata* fd,
    struct sis1100_dma_alloc* d)
{
pINFO(sc, "sis1100_dma_alloc!");
    if (fd->mmapdma.valid) {
        pINFO(sc, "mmapdma already valid");
        return EINVAL;
    }
    return ENOTTY;
}

int
sis1100_dma_free(struct sis1100_softc *sc, struct sis1100_fdata* fd,
    struct sis1100_dma_alloc* d)
{
pINFO(sc, "sis1100_dma_free!");
    if (!fd->mmapdma.valid)
        return 0;
    return ENOTTY;
}
#endif
