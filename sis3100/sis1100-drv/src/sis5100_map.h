/* $ZEL: sis5100_map.h,v 1.4 2007/08/20 17:59:45 wuestner Exp $ */

/*
 * Copyright (c) 2004
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

#ifndef _dev_pci_sis5100_map_h_
#define _dev_pci_sis5100_map_h_

struct sis5100_reg {
    u_int32_t ident;              /* 0x000 */
    u_int32_t optical_sr;         /* 0x004 */
    u_int32_t optical_cr;         /* 0x008 */
    u_int32_t res0[29];
    u_int32_t in_out;             /* 0x080 */
    u_int32_t in_latch_irq;       /* 0x084 */
    u_int32_t res1[30];
    u_int32_t camac_sc;           /* 0x100 */
    u_int32_t camac_irq_sc;       /* 0x104 */
    u_int32_t lam_enable;         /* 0x108 */
    u_int32_t res2[1];
    u_int32_t multistation;       /* 0x110 */
    u_int32_t res3[60];
    u_int32_t dma_write_counter;  /* 0x204 */
    u_int32_t res4[62];
    u_int32_t dsp_sc;             /* 0x300 */
};

/* bits in in_out */
#define sis5100_io_flat_out1        (1<<0)
#define sis5100_io_flat_out2        (1<<1)
#define sis5100_io_flat_out3        (1<<2)

#define sis5100_io_lemo_out1        (1<<4)
#define sis5100_io_lemo_out2        (1<<5)
#define sis5100_io_lemo_out3        (1<<6)
/* clear is (io_*_out?)<<16 */

#define sis5100_io_flat_in1         (1<<16)
#define sis5100_io_flat_in2         (1<<17)
#define sis5100_io_flat_in3         (1<<18)

#define sis5100_io_lemo_in1         (1<<20)
#define sis5100_io_lemo_in2         (1<<21)
#define sis5100_io_lemo_in3         (1<<22)

#define sis5100_io_flat_pulse1      (1<<24)
#define sis5100_io_flat_pulse2      (1<<25)
#define sis5100_io_flat_pulse3      (1<<26)

#define sis5100_io_lemo_pulse1      (1<<28)
#define sis5100_io_lemo_pulse2      (1<<29)
#define sis5100_io_lemo_pulse3      (1<<30)

/* bits in in_latch_irq */
#define sis5100_irq_enable_flat1    (1<<0)
#define sis5100_irq_enable_flat2    (1<<1)
#define sis5100_irq_enable_flat3    (1<<2)

#define sis5100_irq_enable_lemo1    (1<<4)
#define sis5100_irq_enable_lemo2    (1<<5)
#define sis5100_irq_enable_lemo3    (1<<6)
#define sis5100_irq_dsp_irq         (1<<7)

#define sis5100_irq_stat_flat1      (1<<8)
#define sis5100_irq_stat_flat2      (1<<9)
#define sis5100_irq_stat_flat3      (1<<10)

#define sis5100_irq_stat_lemo1      (1<<12)
#define sis5100_irq_stat_lemo2      (1<<13)
#define sis5100_irq_stat_lemo3      (1<<14)
#define sis5100_irq_stat_dsp_irq    (1<<15)
/* clear is (irq_enable_*)<<16 */
#define sis5100_clear_latch_flat1   (1<<24)
#define sis5100_stat_latch_flat1    (1<<24)
#define sis5100_clear_latch_flat2   (1<<25)
#define sis5100_stat_latch_flat2    (1<<25)
#define sis5100_clear_latch_flat3   (1<<26)
#define sis5100_stat_latch_flat3    (1<<26)
#define sis5100_clear_latch_lemo1   (1<<28)
#define sis5100_stat_latch_lemo1    (1<<28)
#define sis5100_clear_latch_lemo2   (1<<29)
#define sis5100_stat_latch_lemo2    (1<<29)
#define sis5100_clear_latch_lemo3   (1<<30)
#define sis5100_stat_latch_lemo3    (1<<30)
#define sis5100_clear_dsp_irq_latch (1<<31)
#define sis5100_stat_dsp_irq_latch  (1<<31)

/* bits in camac_sc */
/* clear is (camac_*)<<16 */
#define sis5100_camac_inhibit  (1<<0)
#define sis5100_camac_user_led (1<<7)

/* bits in dsp_sc */
#define sis5100_dsp_irq_pulse  (1<<0)
#define sis5100_dsp_run        (1<<8)
#define sis5100_dsp_boot_eprom (1<<9)
#define sis5100_dsp_boot_ctrl  (1<<11)

#define sis5100_dsp_available  (1<<24)
#define sis5100_dsp_flag0      (1<<28)
#define sis5100_dsp_flag1      (1<<29)
#define sis5100_dsp_flag2      (1<<30)
#define sis5100_dsp_flag3      (1<<31)

#endif
