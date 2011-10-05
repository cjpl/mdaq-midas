/* $ZEL: pandapixel_map.h,v 1.3 2008/01/17 15:55:06 wuestner Exp $ */

/*
 * Copyright (c) 2006-2008
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

#ifndef _dev_pci_pandapixel_map_h_
#define _dev_pci_pandapixel_map_h_

struct pandapixel_reg {
        u_int32_t            ident;
        u_int32_t            sr;     /* dorbell shadow */
        u_int32_t            cr;     /* Control Register, 4 bit */
#define PANDAPIXEL_CR_nRESET 0x03    /* !reset */
#define PANDAPIXEL_CR_LED0   0x08    /* SIS1100opt front LED (upper) */
#define PANDAPIXEL_CR_LED1   0x10    /* SIS1100opt front LED (lower) */
#define PANDAPIXEL_CR_START  0x20    /* start of input state machine
                                       bit is automatically reset after start */
        u_int32_t            res0C;
        u_int32_t            res10;
        u_int32_t            res14;
        u_int32_t            strb_len;
        u_int32_t            lvl1_dly;  /* lvl1 delay */
        u_int32_t            timer;     /* write: reset; read: 0.1 ms counter */
        u_int32_t            input;
        u_int32_t            bitcount;
        u_int32_t            lvl1_len;
        u_int32_t            synch_len;
};

#endif
