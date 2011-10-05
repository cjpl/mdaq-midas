/* $ZEL: sis1100_sc_linux.h,v 1.28 2009/06/05 15:59:44 wuestner Exp $ */

/*
 * Copyright (c) 2001-2008
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

#ifndef _sis1100_sc_linux_h_
#define _sis1100_sc_linux_h_

#define SGL_SIZE 128
#define DMA_MAX (PAGE_SIZE*(SGL_SIZE-1))

struct sis1100_dmabuf {
    size_t size;
    void* cpu_addr;
    dma_addr_t dma_handle;
};

struct sis1100_dmapage {
    void* cpu_addr;
    dma_addr_t dma_handle;
};

#if 0
struct mmapdma {
        int valid;
};
#endif

struct handlercommand {
    volatile enum handlercomm command;
    spinlock_t lock;
};

struct sis1100_softc;

struct sis1100_fdata {
/* OS specific*/
    struct sis1100_softc* sc;
/*common*/
#if 0
    struct mmapdma mmapdma; /* init it in sis1100_open.c if reenabled !!! */
#endif
    size_t mindmalen_r, mindmalen_w;
    size_t minpipelen_r, minpipelen_w;
    enum sis1100_hw_type old_remote_hw;
    enum sis1100_subdev subdev;
    int32_t vmespace_am;
    u_int32_t vmespace_datasize;
    int fifo_mode;
    int last_prot_err;
    u_int32_t owned_irqs;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
    pid_t pid;
#else
    struct pid *pid;
#endif
    int sig;
};

/*
 * each block has to start at a page boundary (why?)
 * block length has to be a multiple of the page size (why?)
 * all blocks have to have the same size (why?)
 * at least two blocks are required
 */
struct demand_dma_block {
    char* uaddr; /* user virtual address of this block */
    size_t size; /* size of this block */

    struct sg_table table;
    int dsegs; /* number of pages for PLX descriptors */
    struct sis1100_dmapage* desc_pages;
    u_int32_t dmadpr0;
    struct timespec time;
/* LLLL1 */
    enum dmablockstatus status;
    int seq_write_start;
    int seq_write_end;
    int seq_signal;
    int seq_free;
/* LLLL0 */
};

/* demand_dma should be identical to the bsd version */
struct demand_dma {
    struct MUX mux;               /* protects this structure against concurrent
                                     initialisation from different processes */
    spinlock_t spin;              /* protects members between LLLL1 and LLLL0
                                     during runtime of DMA */

    enum dmastatus status;        /* invalid | ready | running */
    struct sis1100_fdata *owner;
    char* uaddr;                  /* user virtual address of the first block */
    size_t size;                  /* size of ONE block of mapped user pages */
    int numblocks;                /* number of blocks */
    struct demand_dma_block* block; /* array of block descriptions */
/* LLLL1 */
    int is_blocked;               /* all buffers are full, DMA is stopped */
    int writing_block;            /* block currently written by DMA or
                                     waiting until free */
    int reading_block;            /* last block given to user */
    int debug_sequence;           /* counter, incremented after each DMA-block */
/* LLLL0 */
};

struct broken {
    int d8_dma;
    int d16_dma;
    int swap_doorbell;
};

struct transparent {
    int active;
};

struct sis1100_softc {
/* OS specific*/
    u8 __iomem *plx_addr;
    u8 __iomem *reg_addr;

    struct device *device[1<<(sis1100_MINORTYPEBITS+sis1100_MINORUSERBITS)];
    struct pci_dev *pdev;
    struct cdev cdev;
    int unit;
    struct task_struct* handler;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,11)
    struct sched_param sched_param;
#endif
    struct sis1100_dmabuf dmaspace;

/* OS specific definition but common use */
    struct MUX sem_hw;              /* protects hardware */
    struct MUX sem_fdata;           /* protects fdata and fdata[*]->owned_irqs */
    struct MUX sem_irqinfo;         /* protects irq_vects and pending_irqs */
    spinlock_t lock_intcsr;         /* protects INTCSR of PLX */
    spinlock_t lock_doorbell;       /* protects sc.doorbell */
    spinlock_t lock_lemo_status;    /* protects sc.lemo_status */
    wait_queue_head_t local_wait;
    wait_queue_head_t remoteirq_wait;
    struct timer_list link_up_timer;

/* common */
    struct sis1100_fdata* fdata[sis1100_MINORUTMASK+1];
    u_int32_t silicon_id;
    u_int32_t local_ident, remote_ident;
    u_int32_t serial;
    volatile enum sis1100_hw_type remote_hw;
    volatile u_int32_t doorbell;
    volatile u_int32_t lemo_status;
    volatile u_int32_t mbx0;
    volatile int got_irqs;
    struct irq_vects irq_vects[8];
    struct timespec irqtimes[32];
    u_int32_t autoack_mask;
    u_int32_t pending_irqs;
    struct handlercommand handlercommand;
    loff_t ram_size;
    int dsp_present;
    int remote_endian; /* 0: little 1: big*/
    int user_wants_swap;
    volatile u_int32_t last_opt_csr; /* used by handlercomm_lemo */
    struct demand_dma demand_dma;
    void (*plxirq_dma0_hook)(struct sis1100_softc*, struct timespec);
    int using_dac;
    int consistent_using_dac;
    u_int32_t eemask;
    unsigned int eeprom_addr_len;
    int sendfifo_size; /* size of send FIFO (words) */
    int sendfifo_of;   /* words in FIFO if almost full */
    u_int32_t testflags; /* to be used for debugging */
    struct broken broken;
    struct transparent transparent;
};

extern struct sis1100_softc *sis1100_devdata[sis1100_MAXCARDS];

extern struct file_operations sis1100_fops;

#define SIS1100FD(file) ((struct sis1100_fdata*)(file)->private_data)
#define SIS1100SC(file) (((struct sis1100_fdata*)(file)->private_data)->sc)

#define _plxreadreg32(sc, offset) ioread32((sc)->plx_addr+(offset))
#define _plxwritereg32(sc, offset, val) iowrite32(val, (sc)->plx_addr+(offset))
#define _plxreadreg8(sc, offset) ioread8((sc)->plx_addr+(offset))
#define _plxwritereg8(sc, offset, val) iowrite8((val), (sc)->plx_addr+(offset))

#define _readreg32(sc, offset) ioread32((sc)->reg_addr+(offset))
#define _writereg32(sc, offset, val) iowrite32(val, (sc)->reg_addr+(offset))
#define _readreg8(sc, offset) ioread8((sc)->reg_addr+(offset))
#define _writereg8(sc, offset, val) iowrite8(val, (sc)->reg_addr+(offset))
#define _rawreadreg(sc, offset) __raw_readl((sc)->reg_addr+(offset))
#define _rawwritereg(sc, offset, val) __raw_writel(val, (sc)->reg_addr+(offset))

#define rmb_plx() rmb()
#define rmb_reg() rmb()
#define wmb_plx() wmb()
#define wmb_reg() wmb()
#define mb_plx() mb()
#define mb_reg() mb()

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18)
#   define DEV_DRIVER_STRING(x) "sis1100"
#else
#   define DEV_DRIVER_STRING(x) dev_driver_string(&_sc->pdev->dev)
#endif

#define pLOG(sc, level, fmt, arg...)                    \
    do {                                                \
        struct sis1100_softc *_sc=sc;                   \
        if (_sc)                                        \
            printk(level  "%s[%d]: " fmt "\n",          \
                    DEV_DRIVER_STRING(&_sc->pdev->dev), \
                    _sc->unit, ## arg);                 \
        else                                            \
            printk(level  fmt "\n", ## arg);            \
    } while (0)

int  sis1100_open(struct inode *inode, struct file *file);
int  sis1100_release(struct inode *inode, struct file *file);
#ifdef HAVE_UNLOCKED_IOCTL
long  sis1100_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
#else
int  sis1100_ioctl(struct inode *inode, struct file *file,
    	    	unsigned int cmd, unsigned long arg);
#endif
#ifdef HAVE_COMPAT_IOCTL
long sis1100_ioctl32(struct file * file, unsigned int cmd, unsigned long arg);
long _sis1100_ioctl(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    unsigned int cmd, void* data);
#endif
loff_t sis1100_llseek(struct file* file, loff_t offset, int orig);
int  sis1100_mmap(struct file * file, struct vm_area_struct * vma);
ssize_t sis1100_read(struct file* file, char* buf, size_t count,
    	    	loff_t* ppos);
ssize_t sis1100_write(struct file* file, const char* buf, size_t count,
    	    	loff_t* ppos);
unsigned int sis1100_poll(struct file* file,
                struct poll_table_struct* poll_table);

int sis1100_irq_thread(void* data);
void sis1100_link_up_handler(unsigned long data);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
irqreturn_t sis1100_intr(int irq, void *vsc, struct pt_regs *regs);
#else
irqreturn_t sis1100_intr(int irq, void *vsc);
#endif
int  sis1100_init(struct sis1100_softc* sc);
void sis1100_done(struct sis1100_softc* sc);

#endif
