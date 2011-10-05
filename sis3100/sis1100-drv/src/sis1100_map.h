/* $ZEL: sis1100_map.h,v 1.11 2009/06/05 15:57:09 wuestner Exp $ */

/*
 * Copyright (c) 2001-2009
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

#ifndef _dev_pci_sis1100_map_h_
#define _dev_pci_sis1100_map_h_

struct sis1100_reg {
    u_int32_t ident;           /* 0 */
    u_int32_t sr;              /* 4 */
    u_int32_t cr;              /* 8 */
    u_int32_t semaphore;       /* c */
    u_int32_t doorbell;        /* 10 */
    u_int32_t res0[3];
    u_int32_t mailbox[8];      /* 20 */
    u_int32_t res1[16];
    u_int32_t t_hdr;           /* 80 */
    u_int32_t t_am;            /* 84 */
    u_int32_t t_adl;           /* 88 */
    u_int32_t t_adh;           /* 8c */
    u_int32_t t_dal;           /* 90 */
    u_int32_t t_dah;           /* 94 */
    u_int32_t prot_error_nw;   /* 98 */
    u_int32_t tc_hdr;          /* 9c */ 
    u_int32_t tc_dal;          /* a0 */
    u_int32_t tc_dah;          /* a4 */
    u_int32_t p_balance;       /* a8 */
    u_int32_t prot_error;      /* ac */
    u_int32_t d0_bc;           /* b0 */
    u_int32_t d0_bc_buf;       /* b4 */
    u_int32_t d0_bc_blen;      /* b8 */
    u_int32_t d_hdr;           /* bc */
    u_int32_t d_am;            /* c0 */
    u_int32_t d_adl;           /* c4 */
    u_int32_t d_adh;           /* c8 */
    u_int32_t d_bc;            /* cc */
    u_int32_t res4[2];
    u_int32_t rd_pipe_buf;     /* d8 */
    u_int32_t rd_pipe_blen;    /* dc */
    u_int32_t res5[2];
    u_int32_t tp_special;      /* e8 */
    u_int32_t tp_data;         /* ec */
    u_int32_t opt_csr;         /* f0 */
    union jtag_csr {           /* f4 */
        u_int32_t jtag_csrl;
        u_int8_t jtag_csrb[4];
    } jtag_csr;
    u_int32_t jtag_data;       /* f8 */
    u_int32_t res6[1];
    u_int32_t mailext[192];    /* 100 */

    struct {                   /* 400 */
        u_int32_t hdr;
        u_int32_t am;
        u_int32_t adl;
        u_int32_t adh;
    } sp1_descr[64];
};

/* irq bits in sr and cr */
#define irq_synch_chg     (1<< 4) /*   10 */
#define irq_inh_chg       (1<< 5) /*   20 */
#define irq_sema_chg      (1<< 6) /*   40 */
#define irq_rec_violation (1<< 7) /*   80 */
#define irq_reset_req     (1<< 8) /*  100 */
#define irq_dma_eot       (1<< 9) /*  200 */
#define irq_mbx0          (1<<10) /*  400 */
#define irq_s_xoff        (1<<11) /*  800 */
#define irq_lemo_in_0_chg (1<<12) /* 1000 */
#define irq_lemo_in_1_chg (1<<13) /* 2000 */
#define irq_lemo_in_chg   (irq_lemo_in_0_chg|irq_lemo_in_1_chg) /* 3000 */
#define irq_prot_end      (1<<14) /* 4000 */
#define irq_prot_l_err    (1<<15) /* 8000 */

#define sis1100_all_irq          0xfff0

/* bits in sr (without irqs) */
#define sr_rx_synch     (1<<0)
#define sr_tx_synch     (1<<1)
#define sr_synch        (sr_rx_synch|sr_tx_synch)
#define sr_inhibit      (1<<2)
#define sr_configured   (1<<3)
#define sr_dma0_blocked (1<<16)
#define sr_no_pread_buf (1<<17)
#define sr_prot_err     (1<<18)
#define sr_bus_tout     (1<<19)
#define sr_tp_special   (1<<20)
#define sr_tp_data      (1<<21)
#define sr_abort_dma    (1<<31)

/* bits in cr (without irqs) */
#define cr_reset        (1<<0)
#define cr_transparent  (1<<1)
#define cr_ready        (1<<2)
#define cr_bigendian    (1<<3)
#define cr_rem_reset    (1<<16)

/* bits in opt_csr  (without "internals") */
#define opt_lemo_out_0  (1<<4)
#define opt_lemo_out_1  (1<<5)
#define opt_led_0       (1<<6)
#define opt_led_1       (1<<7)
#define opt_lemo_in_0   (1<<8)
#define opt_lemo_in_1   (1<<9)
#define opt_fifo_pef    (1<<28)
#define opt_fifo_ef     (1<<29)
#define opt_s_fifo_pff  (1<<30)
#define opt_s_fifo_ff   (1<<31)

/* error codes */
#define sis1100_e_dlock     0x005
#define sis1100_le_synch    0x101
#define sis1100_le_nrdy     0x102
#define sis1100_le_xoff     0x103
#define sis1100_le_resource 0x104
#define sis1100_le_dlock    0x105
#define sis1100_le_to       0x107
#define sis1100_re_nrdy     0x202
#define sis1100_re_prot     0x206
#define sis1100_re_to       0x207
#define sis1100_re_berr     0x208
#define sis1100_re_ferr     0x209

/* bits in jtag_csr.jtag_csrl */
#define SIS1100_JT_TDI       0x001
#define SIS1100_JT_TMS       0x002
#define SIS1100_JT_TCK       0x004   /* use only if autoclock disabled */
#define SIS1100_JT_TDO       0x008
#define SIS1100_JT_ENABLE    0x100
#define SIS1100_JT_AUTOCLOCK 0x200
#define SIS1100_JT_C         0x300   /* autoclock, enable */

#endif
