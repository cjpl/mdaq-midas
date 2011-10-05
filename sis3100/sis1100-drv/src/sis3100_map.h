/* $ZEL: sis3100_map.h,v 1.6 2009/02/09 22:56:03 wuestner Exp $ */

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

#ifndef _dev_pci_sis3100_map_h_
#define _dev_pci_sis3100_map_h_

struct sis3100_reg {
    u_int32_t ident;              /* 0x000 */
    u_int32_t optical_sr;         /* 0x004 */
    u_int32_t optical_cr;         /* 0x008 */
    u_int32_t res0[29];
    u_int32_t in_out;             /* 0x080 */
    u_int32_t in_latch_irq;       /* 0x084 */
    u_int32_t lemo_io_level;      /* 0x088 SIS3104 only */
    u_int32_t res1[29];
    u_int32_t vme_master_sc;      /* 0x100 */
    u_int32_t vme_irq_sc;         /* 0x104 */
    u_int32_t res2[62];
    u_int32_t vme_slave_sc;       /* 0x200 SIS3100 only */
    u_int32_t dma_write_counter;  /* 0x204 */
    u_int32_t res3[62];
    u_int32_t dsp_sc;             /* 0x300 SIS3100 only */
    u_int32_t res4[63];
    u_int32_t vme_addr_map[256];  /* 0x400 SIS3100 only */
};

/* bits in in_out for SIS3100 */
#define sis3100_io_flat_out1 (1<<0)
#define sis3100_io_flat_out2 (1<<1)
#define sis3100_io_flat_out3 (1<<2)
#define sis3100_io_flat_out4 (1<<3)
#define sis3100_io_lemo_out1 (1<<4)
#define sis3100_io_lemo_out2 (1<<5)
#define sis3100_io_lemo_out3 (1<<6)
/* clear is (io_*_out?)<<16 */
#define sis3100_io_flat_in1 (1<<16)
#define sis3100_io_flat_in2 (1<<17)
#define sis3100_io_flat_in3 (1<<18)
#define sis3100_io_flat_in4 (1<<19)
#define sis3100_io_lemo_in1 (1<<20)
#define sis3100_io_lemo_in2 (1<<21)
#define sis3100_io_lemo_in3 (1<<22)
#define sis3100_io_flat_pulse1 (1<<24)
#define sis3100_io_flat_pulse2 (1<<25)
#define sis3100_io_flat_pulse3 (1<<26)
#define sis3100_io_flat_pulse4 (1<<27)
#define sis3100_io_lemo_pulse1 (1<<28)
#define sis3100_io_lemo_pulse2 (1<<29)
#define sis3100_io_lemo_pulse3 (1<<30)

/* bits in in_out for SIS3104 */
#define sis3104_io_nim_out1 (1<<0)
#define sis3104_io_nim_out2 (1<<1)
#define sis3104_io_ttl_out1 (1<<2)
#define sis3104_io_ttl_out2 (1<<3)
/* clear is (io_*_out?)<<16 */
#define sis3100_io_nim_in1 (1<<16)
#define sis3100_io_nim_in2 (1<<17)
#define sis3100_io_ttl_in1 (1<<18)
#define sis3100_io_ttl_in2 (1<<19)
#define sis3104_io_nim_pulse1 (1<<24)
#define sis3104_io_nim_pulse2 (1<<25)
#define sis3104_io_ttl_pulse1 (1<<26)
#define sis3104_io_ttl_pulse2 (1<<27)

/* bits in lemo_io_level */
#define sis3100_io_level_ttl_out1 (1<<0)
#define sis3100_io_level_ttl_out2 (1<<1)
#define sis3100_io_level_ttl_in1  (1<<2)
#define sis3100_io_level_ttl_in2  (1<<3)
#define sis3100_io_level_enable   (1<<15)
#define sis3100_io_level_nim_out1 (1<<16)
#define sis3100_io_level_nim_out2 (1<<17)
#define sis3100_io_level_nim_in1  (1<<18)
#define sis3100_io_level_nim_in2  (1<<19)
#define sis3100_io_level_disable  (1<<31)

/* bits in vme_master_sc */
#define vme_system_controller_enable (1<<0)
#define vme_sys_reset         (1<<1)
#define vme_lemo_out_reset    (1<<2)    /* SIS3100 only (?) */
#define vme_power_on_reset    (1<<3)
#define vme_request_level     (3<<4)
#define vme_requester_type    (1<<6)
#define vme_user_led          (1<<7)    /* SIS3100 only */
#define vme_enable_retry      (1<<8)    /* SIS3100 only */
#define vme_disable_retry     (1<<9)    /* SIS3104 only */
#define vme_force_dearbit     (1<<10)   /* SIS3104 only */
#define vme_long_timer        (3<<12)
#define vme_berr_timer        (3<<14)
#define vme_system_controller (1<<16)

/* bits in dsp_sc */
#define sis3100_dsp_run        (1<<8)
#define sis3100_dsp_boot_eprom (1<<9)
#define sis3100_dsp_boot_ctrl  (1<<11)

#define sis3100_dsp_available  (1<<24)
#define sis3100_dsp_flag0      (1<<28)
#define sis3100_dsp_flag1      (1<<29)
#define sis3100_dsp_flag2      (1<<30)
#define sis3100_dsp_flag3      (1<<31)

/* error codes */
#define sis3100_re_berr        0x211 /* Bus Error */
#define sis3100_re_retr        0x212 /* Retry */
#define sis3100_re_atimeout    0x214 /* Arbitration timeout */

/* addresses in space 6 (DSP/MEM) (SIS3100 only) */
#define sis3100_sharc_dpram     0x40000000 /* SHARC dual ported RAM */
#define sis3100_sharc_dpram_end 0x400003fc
#define sis3100_sdram_spd       0x40000400 /* OPT SDRAM SPD EEPROM */
#define sis3100_sharc_boot      0x81000000 /* SHARC boot flash PROM */
#define sis3100_sharc_boot_end  0x811ffffc
#define sis3100_sharc_sram      0x81200000 /* SHARC SRAM */
#define sis3100_sharc_sram_end  0x812ffffc
#define sis3100_sharc_d48       0x81300000 /* SHARC d48 register */

#endif
