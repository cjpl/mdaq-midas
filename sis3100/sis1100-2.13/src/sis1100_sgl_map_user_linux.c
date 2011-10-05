/* $ZEL: sis1100_sgl_map_user_linux.c,v 1.6 2008/06/03 18:03:08 wuestner Exp $ */

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
#include <linux/pagemap.h>

/* The following functions are stolen from drivers/scsi/st.c. */

int
sgl_map_user_pages(struct sg_table *table, const char* xuaddr, size_t count,
        int rw)
{
	int res, i, j;
	unsigned int nr_pages;
	struct page **pages;
        unsigned long uaddr=(unsigned long)xuaddr;
        struct scatterlist *sg;

	nr_pages = ((uaddr & ~PAGE_MASK) + count + ~PAGE_MASK) >> PAGE_SHIFT;

	/* User attempted Overflow! */
	if ((uaddr + count) < uaddr)
		return -EINVAL;

	/* Too big */
        if (nr_pages > table->nents) {
                printk(KERN_ERR
                    "sgl_map_user_pages: nr_pages=%d but max_pages=%d\n",
                        nr_pages, table->nents);
		return -ENOMEM;
        }
	/* Hmm? */
	if (count == 0)
		return 0;

	if ((pages = kzalloc(nr_pages * sizeof(*pages), GFP_KERNEL)) == NULL) {
                printk(KERN_ERR
                    "sgl_map_user_pages: kmalloc(%d*sizeof(struct page*)) failed\n",
                        nr_pages);
		return -ENOMEM;
        }

        /* Try to fault in all of the necessary pages */
	down_read(&current->mm->mmap_sem);
        /* rw==READ means read from drive, write into memory area */
	res = get_user_pages(
		current,
		current->mm,
		uaddr,
		nr_pages,
		rw == READ,
		0, /* don't force */
		pages,
		NULL);
	up_read(&current->mm->mmap_sem);

	/* Errors and no page mapped should return here */
	if (res < nr_pages) {
                printk(KERN_ERR "sgl_map_user_pages: nr_pages=%d; res=%d\n",
                    nr_pages, res);
		goto out_unmap;
        }

        for (i=0; i < nr_pages; i++) {
                /* FIXME: flush superflous for rw==READ,
                 * probably wrong function for rw==WRITE
                 */
		flush_dcache_page(pages[i]);
        }

	/* Populate the scatter/gather list */
	for_each_sg(table->sgl, sg, table->nents, i) {
                if (i==0) {
                        unsigned int offset, len;
                        offset = uaddr & ~PAGE_MASK;
                        if (offset+count>PAGE_SIZE)
                            len=PAGE_SIZE-offset;
                        else
                            len=count;
                        sg_set_page(sg, pages[0], len, offset);
                        count-=len;
                } else {
                        sg_set_page(sg, pages[i],
                            count < PAGE_SIZE ? count : PAGE_SIZE, 0);
                        count -= PAGE_SIZE;
                }
        }

	kfree(pages);
	return nr_pages;

out_unmap:
	if (res > 0) {
		for (j=0; j < res; j++)
			page_cache_release(pages[j]);
                res=-EIO;
	}
	kfree(pages);
	return res;
}


/* And unmap them... */
int
sgl_unmap_user_pages(struct sg_table *table, int dirtied)
{
        struct scatterlist *sg;
        unsigned int i;

        for_each_sg(table->sgl, sg, table->nents, i) {
		struct page *page;
		page = sg_page(sg);

		if (dirtied)
			SetPageDirty(page);
		/* FIXME: cache flush missing for rw==READ
		 * FIXME: call the correct reference counting function
		 */
		page_cache_release(page);
	}

	return 0;
}

#ifdef CONFIG_SPARC64
void
dump_sgl(struct scatterlist *sgl, int nr_pages)
{
	int i;

	for (i=0; i<nr_pages; i++) {
            printk(KERN_ERR "sgl[%d]: page=%p offs=%d len=%d dma_len=%d\n",
                i, sgl[i].page, sgl[i].offset, sgl[i].length, sgl[i].dma_length);
        }
}
#endif
