/* $ZEL: sis1100_ddma_map_linux.c,v 1.6 2010/01/18 09:54:30 wuestner Exp $ */

/*
 * Copyright (c) 2005-2008
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

void
sis1100_ddma_zero(struct demand_dma_block* block)
{
    block->uaddr=0;
    block->desc_pages=0;
    block->table.sgl=0;
    block->table.nents=0;
    block->table.orig_nents=0;
}

void
sis1100_ddma_unmap_block(struct sis1100_softc *sc,
        struct demand_dma_block* block)
{
    int i;

    if (block->desc_pages) {
        for (i=0; i<block->dsegs; i++) {
            if (block->desc_pages[i].cpu_addr)
                pci_free_consistent(sc->pdev, PAGE_SIZE,
                    block->desc_pages[i].cpu_addr,
                    block->desc_pages[i].dma_handle);
        }
        vfree(block->desc_pages);
    }

    if (block->table.nents)
        pci_unmap_sg(sc->pdev, block->table.sgl,
            block->table.nents, PCI_DMA_FROMDEVICE);

    if (block->table.orig_nents)
        sgl_unmap_user_pages(&block->table, 0);

    sg_free_table(&block->table);
}

int
sis1100_ddma_map_block(struct sis1100_softc *sc,
    struct demand_dma_block* block)
{
    unsigned int nr_pages; /* number of user pages to be mapped */
    unsigned long uaddr=(unsigned long)block->uaddr;
    int dpseg, i;
    int descrsize;
    int res;

#if 0
    pINFO(sc, "dma_map_block: size=%lld addr=%p",
            (unsigned long long)block->size, block->uaddr);
#endif

    descrsize=sc->using_dac?
            sizeof(struct plx9054_dmadesc_dac):sizeof(struct plx9054_dmadesc);

    /* number of user pages to be mapped */
    nr_pages=((uaddr&~PAGE_MASK)+block->size+~PAGE_MASK)>>PAGE_SHIFT;

#if 0
    pINFO(sc, "dma_map_block: pages to be mapped: %d", nr_pages);
#endif

    /* alloc memory for the scatterlist */
    res=sg_alloc_table(&block->table, nr_pages, GFP_KERNEL);
    if (res) {
        pERROR(sc, "dma_map_block: sg_alloc_table(%u pages) failed; res=%d",
            nr_pages, res);
        return res;
    }

    /* wire the user pages and fill */
    res=sgl_map_user_pages(&block->table, block->uaddr, block->size, READ);
    if (res<0) {
/* XXX block->table not freed */
        pERROR(sc, "dma_map: sgl_map_user_pages for %d pages failed", nr_pages);
        return -res;
    }
    /* sanity check */
    if (res!=nr_pages) {
        pERROR(sc, "dma_map: sgl_map_user_pages returns %d instead of %d\n",
                res, nr_pages);
    }
    pDEBUG(sc, "dma_map: mapped %d pages", nr_pages);

    /* map the user pages for DMA */
    block->table.nents=pci_map_sg(sc->pdev, block->table.sgl,
            nr_pages, PCI_DMA_FROMDEVICE);
    if (!block->table.nents) {
/* XXX sgl_map_user_pages not called */
/* XXX block->table not freed */
        pERROR(sc, "dma_map: pci_map_sg failed");
        return EIO;
    }
    pDEBUG(sc, "dma_map: mapped %d pieces for DMA", block->table.nents);

    /* pages needed for desriptors */
    dpseg = NBPG / descrsize; /* desriptors per page */
    block->dsegs = block->table.nents / dpseg +1;

    /* alloc memory to store the addresses of desriptor pages */
    block->desc_pages=vmalloc(block->dsegs* sizeof(struct sis1100_dmapage));
    if (!block->desc_pages) {
/* XXX pci_unmap_sg not called */
/* XXX sgl_map_user_pages not called */
/* XXX block->table not freed */
        pERROR(sc, "dma_map: vmalloc desc_pages (%d pages) failed",
                block->dsegs);
        return ENOMEM;
    }
    memset(block->desc_pages, 0, block->dsegs* sizeof(struct sis1100_dmapage));
    pDEBUG(sc, "dma_map: demand_dma.desc_pages=%p", block->desc_pages);

    /* allocate the desriptor pages */
    for (i=0; i<block->dsegs; i++) {
        struct sis1100_dmapage* buf=block->desc_pages+i;
        pDEBUG(sc, "dma_map: i=%d buf=%p", i, buf);
        buf->cpu_addr=pci_alloc_consistent(sc->pdev, PAGE_SIZE,
                &buf->dma_handle);
        pDEBUG(sc, "dma_map: buf->cpu_addr=%p buf->dma_handle=%08llx",
                buf->cpu_addr, (unsigned long long)(buf->dma_handle));
        if (!buf->cpu_addr) {
/* XXX block->desc_pages not freed */
/* XXX pci_free_consistent not called */
/* XXX pci_unmap_sg not called */
/* XXX sgl_map_user_pages not called */
/* XXX block->table not freed */
            pERROR(sc, "dma_map: pci_alloc_consistent page[%d] failed", i);
            return ENOMEM;
        }
    }

    /* fill descriptor buffer for PLX */
    {
        /* int id; index of descriptor in s/g list */
        int ip; /* index of page for descriptors */
        int ii; /* index of descriptor in page */
        int i;
        struct scatterlist* sg;

        block->dmadpr0=block->desc_pages[0].dma_handle;

        ip=ii=0;
        for_each_sg(block->table.sgl, sg, block->table.nents, i) {
            u_int32_t next;

            /* we always use plx9054_dmadesc_dac because the first four
               elements of plx9054_dmadesc_dac and plx9054_dmadesc
               are identical */
            struct plx9054_dmadesc_dac* plx_desc =
                (struct plx9054_dmadesc_dac*)
                    (block->desc_pages[ip].cpu_addr+ii*descrsize);
            ii++;
            if (ii>=dpseg) {
                ip++;
                ii=0;
            }
            if (i<block->table.nents-1)
                next=(block->desc_pages[ip].dma_handle+ii*descrsize)|0x9;
            else
                next=0xb;

            plx_desc->pcihigh    = cpu_to_le32((u64)sg_dma_address(sg)>>32);
            plx_desc->pcistart   = cpu_to_le32(sg_dma_address(sg)&0xffffffff);
            plx_desc->size       = cpu_to_le32(sg_dma_len(sg));
            plx_desc->localstart = cpu_to_le32(0);
            plx_desc->next       = cpu_to_le32(next);

#ifdef DEBUG            
            if (id<3) {
                pERROR(sc, "id=%d", id);
                pERROR(sc, "desc      =%p",     plx_desc);
                pERROR(sc, "pcihigh   =0x%08x", plx_desc->pcihigh);
                pERROR(sc, "pcistart  =0x%08x", plx_desc->pcistart);
                pERROR(sc, "size      =0x%08x", plx_desc->size);
                pERROR(sc, "localstart=0x%08x", plx_desc->localstart);
                pERROR(sc, "next      =0x%08x", plx_desc->next);
            }
#endif
        }
    }

    return 0;
}
