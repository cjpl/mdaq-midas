/* $ZEL: sis1100_mmap.c,v 1.11 2009/04/26 20:28:21 wuestner Exp $ */

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

/*
 * this is a macro because of different types used by linux and BSD
 */
#define _sis1100_mmap(sc, fd, offset, base, size)          \
    switch (fd->subdev) {                                  \
        case sis1100_subdev_remote:                        \
            if (!sc->rem_size) {                           \
                pINFO(sc, "mmap: remote space not mapped.");\
                LINUX_RETURN(ENOTTY);                      \
            }                                              \
            base=sc->rem_addr;                             \
            size=sc->rem_size;                             \
            break;                                         \
        case sis1100_subdev_ctrl:                          \
            base=sc->reg_addr;                             \
            size=sc->reg_size;                             \
            break;                                         \
        case sis1100_subdev_ram: /* nobreak */             \
        case sis1100_subdev_dsp:                           \
            size=0;                                        \
    }

#ifdef __NetBSD__

paddr_t
sis1100_mmap(dev_t handle, off_t off, int prot)
{
    struct sis1100_softc* sc = SIS1100SC(handle);
    struct sis1100_fdata* fd = SIS1100FD(handle);

    u_int32_t size=0;
    bus_addr_t base=0;
    if ((fd->subdev!=sis1100_subdev_ctrl) || (off<sc->reg_size)) {
        _sis1100_mmap(sc, fd, off, base, size)
        if (size==0)
            return -1;
        return i386_btop(base+off);
        /*return bus_space_mmap(sc->reg_t, base, off, BUS_SPACE_MAP_LINEAR, prot);*/
    } else { /* dma space for pipeline read */
        if (fd->mmapdma.valid && (off>=fd->mmapdma.off) &&
                        (off-fd->mmapdma.off<fd->mmapdma.size)) {
            paddr_t addr=bus_dmamem_mmap(fd->mmapdma.dmat,
                    &fd->mmapdma.segs, 1,
                    off-fd->mmapdma.off,
                    VM_PROT_READ|VM_PROT_WRITE,
                    BUS_DMA_WAITOK|BUS_DMA_COHERENT);
            return addr;
        } else {
            return -1;
        }
    }
}

#elif __linux__

int
sis1100_mmap(struct file * file, struct vm_area_struct * vma)
{
    struct sis1100_softc *sc = SIS1100SC(file);
    struct sis1100_fdata *fd = SIS1100FD(file);

    u_int32_t size=0;
    int error=0;
    unsigned long base=0;

    /*_sis1100_mmap(sc, fd, vma->vm_pgoff, base, size)*/
    switch (fd->subdev) {
    case sis1100_subdev_remote:
        base=pci_resource_start(sc->pdev, 3);
        size=pci_resource_len(sc->pdev, 3);
        break;
    case sis1100_subdev_ctrl:
        base=pci_resource_start(sc->pdev, 2);
        size=pci_resource_len(sc->pdev, 2);
        break;
    case sis1100_subdev_ram: /* nobreak */
    case sis1100_subdev_dsp:
        size=0;
    }

    /*   offset in bytes           + size of mapping          */
    if ((vma->vm_pgoff<<PAGE_SHIFT)+(vma->vm_end-vma->vm_start)>
            PAGE_ALIGN(size))
    	return -EINVAL;
    vma->vm_flags |= VM_RESERVED;
    vma->vm_flags |= VM_IO;
    vma->vm_flags |= VM_DONTEXPAND;
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,12)
    error=io_remap_page_range(
            vma,
            vma->vm_start, base+(vma->vm_pgoff<<PAGE_SHIFT),
    	    vma->vm_end-vma->vm_start, vma->vm_page_prot);
#else
/**
 * remap_pfn_range - remap kernel memory to userspace
 * @vma: user vma to map to
 * @addr: target user address to start at
 * @pfn: physical address of kernel memory
 * @size: size of map area
 * @prot: page protection flags for this mapping
 *
 *  Note: this is only safe if the mm semaphore is held when called.
 *
int remap_pfn_range(struct vm_area_struct *vma, unsigned long addr,
		    unsigned long pfn, unsigned long size, pgprot_t prot)
*/
    error=io_remap_pfn_range(
            vma,
            vma->vm_start, (base+(vma->vm_pgoff<<PAGE_SHIFT)) >> PAGE_SHIFT,
    	    vma->vm_end-vma->vm_start, vma->vm_page_prot);
#endif
    return error;
}
#endif
