/* $ZEL: compat_linux.c,v 1.2 2009/02/09 22:56:02 wuestner Exp $ */

#include "compat_linux.h"


#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,25)
/**
 * sg_alloc_table - Allocate and initialize an sg table
 * @table:	The sg table header to use
 * @nents:	Number of entries in sg list
 * @gfp_mask:	GFP allocation mask
 *
 *  Description:
 *    Allocate and initialize an sg table. If @nents@ is larger than
 *    SG_MAX_SINGLE_ALLOC a chained sg table will be setup.
 *
 **/
int sg_alloc_table(struct sg_table *table, unsigned int nents, gfp_t gfp_mask)
{
	struct scatterlist *sg;

	sg=kmalloc(nents * sizeof(struct scatterlist), gfp_mask);
        if (unlikely(!sg))
                return -ENOMEM;

	memset(sg, 0, sizeof(*sg) * nents);
        table->sgl = sg;
        table->nents = table->orig_nents += nents;

	return 0;
}

/**
 * sg_free_table - Free a previously allocated sg table
 * @table:	The mapped sg table header
 *
 **/
void sg_free_table(struct sg_table *table)
{
        kfree(table->sgl);
        table->sgl = NULL;
}
#endif
