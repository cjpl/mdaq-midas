/* $ZEL: sis1100_sc.h,v 1.37 2010/01/18 19:02:35 wuestner Exp $ */

/*
 * Copyright (c) 2001-2008
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

#ifndef _sis1100_sc_h_
#define _sis1100_sc_h_

#ifdef __NetBSD__
#   include "dev/pci/compat_netbsd.h"
#elif __linux__
#   include "compat_linux.h"
#else
#   error INVALID or UNKNOWN SYSTEM
#endif

#include "plx9054reg.h"
#include "sis1100_map.h"
#include "sis1100_var.h"

#ifndef PCI_VENDOR_FZJZEL
#   define PCI_VENDOR_FZJZEL 0x1796
#endif
/* classical PCI device */
#ifndef PCI_PRODUCT_FZJZEL_GIGALINK
#   define PCI_PRODUCT_FZJZEL_GIGALINK 0x0001
#endif
/* PCIe device with piggyback */
#ifndef PCI_PRODUCT_FZJZEL_SIS1100_eCMC
#   define PCI_PRODUCT_FZJZEL_SIS1100_eCMC 0x000e
#endif
/* PCIe device with four links, only first link used */
#ifndef PCI_PRODUCT_FZJZEL_SIS1100_eSINGLE
#   define PCI_PRODUCT_FZJZEL_SIS1100_eSINGLE 0x0011
#endif
/* PCIe device with four links */
#ifndef PCI_PRODUCT_FZJZEL_SIS1100_eQUAD
#   define PCI_PRODUCT_FZJZEL_SIS1100_eQUAD 0x0012
#endif

struct irq_vects {
    struct timespec time;
    int32_t vector;
    int valid;
};

enum irqs_got {
    got_dma0=1,
    got_dma1=2,
    got_end=4,
    got_eot=8,
    got_xoff=16,
    got_sync=32,
    got_l_err=64
};

enum handlercomm {
    handlercomm_doorbell=1,
    /*handlercomm_synch=2,*/
    handlercomm_lemo=4,
    handlercomm_mbx0=8,
    handlercomm_up=16,
    handlercomm_down=32,
    handlercomm_ddma=64,
#ifndef __linux__
    handlercomm_die=128
#endif
};

enum dmastatus {
    dma_invalid,
    dma_ready,
    dma_running
};

enum dmablockstatus {
    dmablock_Xfree=0, /* free */
    dmablock_Xdma=1,  /* currently in use for DMA*/
    dmablock_Xfull=2, /* full */
    dmablock_Xuser=4  /* full and given to user */
};

#define SIS5100_CAMACaddr(N, A, F) \
        (((F)&0x1f)<<11 | ((N)&0x1f)<<6 | ((A)&0xf)<<2)

/*
 * dev/pci/sis1100_sc_netbsd.h resp. sis1100_sc_linux.h are included here
 * and not earlyer because they need the definitions above
 */

#ifdef __NetBSD__
#   include "dev/pci/sis1100_sc_netbsd.h"
#elif __linux__
#   include "sis1100_sc_linux.h"
#else
#   error INVALID or UNKNOWN SYSTEM
#endif

#define SWAP32(a) ( (((u_int32_t)(a) << 24) & 0xff000000) | \
                    (((u_int32_t)(a) << 8) & 0x00ff0000) | \
                    (((u_int32_t)(a) >> 8) & 0x0000ff00) | \
                    (((u_int32_t)(a) >>24) & 0x00000ff0) )

#define irq_pending(sc, fd, mask) \
    ((sc->pending_irqs & (fd->owned_irqs & mask)) || \
        (fd->old_remote_hw!=sc->remote_hw))

#define plxreadreg(sc, reg) \
    _plxreadreg32(sc, ofs(struct plx9054reg, reg))
#define plxwritereg(sc, reg, val) \
    _plxwritereg32(sc, ofs(struct plx9054reg, reg), val)

#define sis1100readreg(sc, reg) \
    _readreg32(sc, ofs(struct sis1100_reg, reg))
#define sis1100readregb(sc, reg) \
    _readreg8(sc, ofs(struct sis1100_reg, reg))
#define sis1100writereg(sc, reg, val) \
    _writereg32(sc, ofs(struct sis1100_reg, reg), val)
#define sis1100writeb(sc, reg, val) \
    _writereg8(sc, ofs(struct sis1100_reg, reg), val)

#define sis1100rawreadreg(sc, reg) \
    _rawreadreg(sc, ofs(struct sis1100_reg, reg))
#define sis1100rawwritereg(sc, reg, val) \
    _rawwritereg(sc, ofs(struct sis1100_reg, reg), val)

#define sis1100readremreg(sc, reg, val, locked) \
 sis1100_remote_reg_read(sc, ofs(struct sis1100_reg, reg), val, locked)
#define sis1100writeremreg(sc, reg, val, locked) \
 sis1100_remote_reg_write(sc, ofs(struct sis1100_reg, reg), val, locked)

#define sis3100readremreg(sc, reg, val, locked) \
 sis1100_remote_reg_read(sc, ofs(struct sis3100_reg, reg), val, locked)
#define sis3100writeremreg(sc, reg, val, locked) \
 sis1100_remote_reg_write(sc, ofs(struct sis3100_reg, reg), val, locked)

#define sis5100readremreg(sc, reg, val, locked) \
 sis1100_remote_reg_read(sc, ofs(struct sis5100_reg, reg), val, locked)
#define sis5100writeremreg(sc, reg, val, locked) \
 sis1100_remote_reg_write(sc, ofs(struct sis5100_reg, reg), val, locked)

#define lvd_readremreg(sc, reg, val, locked) \
 sis1100_remote_reg_read(sc, ofs(struct lvd_reg, reg), val, locked)
#define lvd_writeremreg(sc, reg, val, locked) \
 sis1100_remote_reg_write(sc, ofs(struct lvd_reg, reg), val, locked)

#define pandapixel_readremreg(sc, reg, val, locked) \
 sis1100_remote_reg_read(sc, ofs(struct pandapixel_reg, reg), val, locked)
#define pandapixel_writeremreg(sc, reg, val, locked) \
 sis1100_remote_reg_write(sc, ofs(struct pandapixel_reg, reg), val, locked)


#define pINFO(sc, fmt, arg...) \
        pLOG(sc, KERN_INFO, fmt, ## arg)

#define pERROR(sc, fmt, arg...) \
        pLOG(sc, KERN_ERR, fmt, ## arg)

#define pWARNING(sc, fmt, arg...) \
        pLOG(sc, KERN_WARNING, fmt, ## arg)

#ifdef DEBUG
#define pDEBUG(sc, fmt, arg...) \
        pLOG(sc, KERN_ERR, fmt, ## arg)
#else
#define pDEBUG(sc, fmt, arg...) \
        do {} while(0)
#endif

int  sis1100_irq_handler(void* data);
u_int32_t sis3100rem_irq_handler(struct sis1100_softc* sc, u_int32_t);
u_int32_t sis5100rem_irq_handler(struct sis1100_softc* sc, u_int32_t);
u_int32_t zellvd_rem_irq_handler(struct sis1100_softc* sc, u_int32_t);
u_int32_t pandapixel_rem_irq_handler(struct sis1100_softc* sc, u_int32_t);
u_int32_t sis1100_synch_handler(struct sis1100_softc* sc);
u_int32_t sis1100_lemo_handler(struct sis1100_softc* sc);
u_int32_t sis1100_mbox0_handler(struct sis1100_softc* sc);
u_int32_t sis1100_ddma_handler(struct sis1100_softc* sc);

void sis3100rem_enable_irqs(struct sis1100_softc*, struct sis1100_fdata*,
        u_int32_t mask);
void sis5100rem_enable_irqs(struct sis1100_softc*, struct sis1100_fdata*,
        u_int32_t mask);
void zellvd_rem_enable_irqs(struct sis1100_softc*, struct sis1100_fdata*,
        u_int32_t mask);
void pandapixel_rem_enable_irqs(struct sis1100_softc* sc, struct sis1100_fdata*,
        u_int32_t mask);
void sis3100rem_disable_irqs(struct sis1100_softc*, struct sis1100_fdata*,
        u_int32_t mask);
void sis5100rem_disable_irqs(struct sis1100_softc*, struct sis1100_fdata*,
        u_int32_t mask);
void zellvd_rem_disable_irqs(struct sis1100_softc*, struct sis1100_fdata*,
        u_int32_t mask);
void pandapixel_rem_disable_irqs(struct sis1100_softc* sc, struct sis1100_fdata*,
        u_int32_t mask);
void sis3100rem_irq_ack(struct sis1100_softc* sc, u_int32_t irqs);
void sis5100rem_irq_ack(struct sis1100_softc* sc, u_int32_t irqs);
void zellvd_rem_irq_ack(struct sis1100_softc* sc, u_int32_t irqs);
#if 0
void pandapixel_rem_irq_ack(struct sis1100_softc* sc, u_int32_t irqs);
#endif
void sis3100rem_get_vector(struct sis1100_softc* sc, u_int32_t irqs,
        struct sis1100_irq_get2* data);

int  sis1100_init(struct sis1100_softc* sc);

void sis1100_init_remote(struct sis1100_softc* sc, int reset);
int  sis1100rem_init(struct sis1100_softc* sc, int reset);
int  sis3100rem_init(struct sis1100_softc* sc, int reset);
int  sis5100rem_init(struct sis1100_softc* sc, int reset);
int  zellvd_rem_init(struct sis1100_softc* sc, int reset);
int  pandapixel_rem_init(struct sis1100_softc* sc, int reset);
int  psf4ad_rem_init(struct sis1100_softc* sc, int reset);
int  sis1100_init_sdram(struct sis1100_softc* sc);
int  sis1100_serial_numbers(struct sis1100_softc* sc, int32_t* d);

void sis1100_dump_glink_status(struct sis1100_softc* sc, char* text, int locked);
int sis1100_flush_fifo(struct sis1100_softc* sc, int silent);

int sis1100_transparent(struct sis1100_softc*, struct sis1100_fdata*, int32_t*);
int sis1100_read_transparent(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    size_t count, size_t* count_read, void __user *data, int nonblocking);
int sis1100_write_transparent(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    size_t count, size_t* count_written, void __user *data, int nonblocking);

int sis1100_disable_irq(struct sis1100_softc* sc,
        u_int32_t plx_mask, u_int32_t sis_mask);
int sis1100_enable_irq(struct sis1100_softc* sc,
        u_int32_t plx_mask, u_int32_t sis_mask);
int sis1100_tmp_read(struct sis1100_softc *sc,
        u_int32_t addr, int32_t am, int size, int space, u_int32_t* data);
int sis1100_tmp_write(struct sis1100_softc *sc,
        u_int32_t addr, int32_t am, int size, int space, u_int32_t data);
int sis1100_tmp_write_blind(struct sis1100_softc *sc,
        u_int32_t addr, int32_t am, int size, int space, u_int32_t data);
int sis1100_tmp_camacread(struct sis1100_softc *sc,
        u_int32_t addr, u_int32_t* data);
int sis1100_tmp_camacwrite(struct sis1100_softc *sc,
        u_int32_t addr, u_int32_t data);
int sis1100_read_pipe_seq(struct sis1100_softc *sc, struct sis1100_fdata* fd,
        u_int32_t addr, int32_t am, int size, int space, int fifo,
        size_t count, size_t* count_read, u_int8_t* data, int* prot_err);
int sis1100_read_dma(struct sis1100_softc *sc, struct sis1100_fdata* fd,
        u_int32_t addr, int32_t am, int size, int space, int fifo,
        size_t count, size_t* count_read, u_int8_t* data, int* prot_err);
int _sis1100_read_dma(struct sis1100_softc *sc, struct sis1100_fdata* fd,
        u_int32_t addr, int32_t am, int size, int space, int fifo,
        size_t count, size_t* count_read, u_int8_t* data, int* prot_err, 
        int* may_be_more);
int sis1100_write_dma(struct sis1100_softc *sc, struct sis1100_fdata* fd,
        u_int32_t addr, int32_t am, int size, int space, int fifo,
        size_t count, size_t* count_written, const u_int8_t* data, int* prot_err);
int _sis1100_write_dma(struct sis1100_softc *sc, struct sis1100_fdata* fd,
        u_int32_t addr, int32_t am, int size, int space, int fifo,
        size_t count, size_t* count_written, const u_int8_t* data, int* prot_err);
int sis1100_read_loop(struct sis1100_softc *sc, struct sis1100_fdata* fd,
        u_int32_t addr, int32_t am, int size, int space, int fifo,
        size_t count, size_t* count_read, u_int8_t* data, int* prot_err);
int sis1100_write_loop(struct sis1100_softc *sc, struct sis1100_fdata* fd,
        u_int32_t addr, int32_t am, int size, int space, int fifo,
        size_t count, size_t* count_written, const u_int8_t* data, int* prot_err);
int  sis1100_read_pipe(struct sis1100_softc* sc, struct sis1100_pipe* control);
int  sis1100_write_pipe(struct sis1100_softc*, int32_t am, int space,
        int num, u_int32_t* data);
void sis1100_reset_plx9054(struct sis1100_softc* sc);
void sis1100_reset_plx9056(struct sis1100_softc* sc);
int  sis1100_reset(struct sis1100_softc* sc);

int  sis1100_read_block(struct sis1100_softc* sc, struct sis1100_fdata* fd,
        int size, int fifo, size_t num, size_t* num_read, int space,
        int32_t am, u_int32_t addr, u_int8_t* data, u_int32_t* error);
int  sis1100_write_block(struct sis1100_softc* sc, struct sis1100_fdata* fd,
        int size, int fifo, size_t num, size_t* num_written, int space,
        int32_t am, u_int32_t addr, const u_int8_t* data, u_int32_t* error);

int  sis1100_irq_ctl(struct sis1100_fdata* fd, struct sis1100_irq_ctl2* data);
int  sis1100_irq_get(struct sis1100_fdata* fd, struct sis1100_irq_get2* data);
int  sis1100_irq_ack(struct sis1100_fdata* fd, struct sis1100_irq_ack* data);
int  sis1100_irq_wait(struct sis1100_fdata* fd, struct sis1100_irq_get2* data);
int  sis1100_remote_reg_read(struct sis1100_softc* sc, u_int32_t offs,
        u_int32_t* data, int locked);
int  sis1100_remote_reg_write(struct sis1100_softc* sc, u_int32_t offs,
        u_int32_t data, int locked);

int  sis1100_front_io(struct sis1100_softc* sc, u_int32_t* data, int locked);
int  sis1100_front_pulse(struct sis1100_softc* sc, u_int32_t* data, int locked);
int  sis1100_front_latch(struct sis1100_softc* sc, u_int32_t* data, int locked);
int  sis1100rem_front_io(struct sis1100_softc* sc, u_int32_t* data);
int  sis1100rem_front_pulse(struct sis1100_softc* sc, u_int32_t* data);
int  sis1100rem_front_latch(struct sis1100_softc* sc, u_int32_t* data);
int  sis3100rem_front_io(struct sis1100_softc* sc, u_int32_t* data);
int  sis3100rem_front_pulse(struct sis1100_softc* sc, u_int32_t* data);
int  sis3100rem_front_latch(struct sis1100_softc* sc, u_int32_t* data);
int  sis5100rem_front_io(struct sis1100_softc* sc, u_int32_t* data);
int  sis5100rem_front_pulse(struct sis1100_softc* sc, u_int32_t* data);
int  sis5100rem_front_latch(struct sis1100_softc* sc, u_int32_t* data);

void sis1100_update_swapping(struct sis1100_softc* sc);
int sis3100_set_timeouts(struct sis1100_softc* sc, int berr, int arb);
int sis3100_get_timeouts(struct sis1100_softc* sc, int* berr, int* arb);

int sis1100_dma_alloc(struct sis1100_softc *sc, struct sis1100_fdata* fd,
    struct sis1100_dma_alloc* d);
int sis1100_dma_free(struct sis1100_softc *sc, struct sis1100_fdata* fd,
    struct sis1100_dma_alloc* d);

int sis1100_check_rw_access(struct sis1100_softc*,
    off_t, off_t, size_t, int, const char*, int);

int sis1100_remote_reset(struct sis1100_softc* sc, int locked);

int sis1100_dsp_reset(struct sis1100_softc* sc, struct sis1100_fdata* fd);
int sis1100_dsp_start(struct sis1100_softc* sc, struct sis1100_fdata* fd);
int sis1100_dsp_load(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_dsp_code* d);
int sis1100_dsp_read(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_dsp_code* d);
int sis1100_dsp_wr(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_dsp_code* d);
int sis1100_dsp_rd(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_dsp_code* d);

int sis1100_ddma_map(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_ddma_map* map);
int sis1100_ddma_start(struct sis1100_softc* sc, struct sis1100_fdata* fd);
int sis1100_ddma_stop(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_ddma_stop*);
int sis1100_ddma_mark(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    unsigned int* block);
int sis1100_ddma_wait(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    unsigned int* block);
void sis1100_ddma_zero(struct demand_dma_block* block);
void sis1100_ddma_unmap_block(struct sis1100_softc *sc,
    struct demand_dma_block* block);
int sis1100_ddma_map_block(struct sis1100_softc *sc,
    struct demand_dma_block* block);

int sis1100_read_eeprom(struct sis1100_softc* sc,
         u_int8_t num, u_int8_t addr, u_int16_t* data, int userspace);
int sis1100_read_eeprom_plx9054(struct sis1100_softc* sc,
         u_int8_t num, u_int8_t addr, u_int16_t* data, int userspace);
int sis1100_read_eeprom_plx8311(struct sis1100_softc* sc,
         u_int8_t num, u_int8_t addr, u_int16_t* data, int userspace);
int sis1100_write_eeprom(struct sis1100_softc* sc,
         u_int8_t num, u_int8_t addr, u_int16_t* data, int userspace);
int sis1100_write_eeprom_plx9054(struct sis1100_softc* sc,
         u_int8_t num, u_int8_t addr, u_int16_t* data, int userspace);
int sis1100_write_eeprom_plx8311(struct sis1100_softc* sc,
         u_int8_t num, u_int8_t addr, u_int16_t* data, int userspace);
int sis1100_end_of_eeprom(struct sis1100_softc* sc);
void sis1100_dump_irqs(struct sis1100_softc* sc, const char* text);

int sis1100_pipe_ctrl_read_local(struct sis1100_softc* sc,
	struct sis1100_fdata* fd, u_int32_t addr, int fifo_mode,
	u_int32_t count, u_int32_t __user *data);
int sis1100_pipe_ctrl_read_remote(struct sis1100_softc* sc,
	struct sis1100_fdata* fd, u_int32_t addr, int fifo_mode,
	u_int32_t count, u_int32_t* count_read,
	u_int32_t __user *data, u_int32_t* prot_error);
int sis1100_pipe_ctrl_write_local(struct sis1100_softc* sc,
	struct sis1100_fdata* fd, u_int32_t addr, int fifo_mode,
	u_int32_t count, u_int32_t __user *data);
int sis1100_pipe_ctrl_write_remote( struct sis1100_softc* sc,
	struct sis1100_fdata* fd, u_int32_t addr, int fifo_mode,
	u_int32_t count, u_int32_t* count_written,
	u_int32_t __user *data, u_int32_t* prot_error);

#endif
