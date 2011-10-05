/* $ZEL: sis1100_ioctl32_linux.h,v 1.5 2009/03/12 13:24:07 wuestner Exp $ */

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

#ifndef _sis1100_ioctl32_linux_h_
#define _sis1100_ioctl32_linux_h_

struct sis1100_32_vme_req {
    compat_int_t size;
    int32_t am;
    u_int32_t addr;
    u_int32_t data;
    u_int32_t error;
};

struct sis1100_32_vme_block_req {
    compat_int_t size;        /* size of dataword */
    compat_int_t fifo;
    compat_size_t num;      /* number of datawords */
    int32_t am;
    u_int32_t addr;  /* remote bus address */
    compat_caddr_t data;  /* local user space address */
    u_int32_t error;
};

struct sis1100_32_vme_super_block_req {
    compat_int_t n;
    compat_int_t error;
    struct sis1100_32_vme_block_req* reqs;
};

#if 0
struct sis1100_32_camac_scan_req {
    u_int32_t N;
    u_int32_t A;
    u_int32_t F;
    u_int32_t data;
    u_int32_t error;
};
#endif

struct sis1100_32_pipe {
    compat_int_t num;
    compat_caddr_t list; /* struct sis1100_pipelist* */
    compat_caddr_t data;
    u_int32_t error;
};

struct sis1100_32_writepipe {
    compat_int_t num;
    compat_int_t am;
    compat_caddr_t data; /* num*{addr, data} */
    u_int32_t error;
};

struct vmespace_32 {
    int32_t am;
    u_int32_t datasize;
    compat_int_t swap;          /* 1: swap words 0: don't swap -1: not changed */
    compat_int_t mapit;         /* not used */
    compat_ssize_t mindmalen; /* 0: never use DMA; 1: always use DMA; -1: not changed */
};

struct sis1100_32_ident_dev {
    compat_int_t hw_type; /*enum sis1100_hw_type hw_type;*/
    compat_int_t hw_version;
    compat_int_t fw_type;
    compat_int_t fw_version;
};

struct sis1100_32_ident {
    struct sis1100_32_ident_dev local;
    struct sis1100_32_ident_dev remote;
    compat_int_t remote_ok;
    compat_int_t remote_online;
};

struct sis1100_32_ctrl_reg {
    compat_int_t offset;
    u_int32_t val;
    u_int32_t error;
};

struct sis1100_32_dma_alloc {
    compat_size_t size;
    compat_off_t offset;
    u_int32_t dma_addr;
};

struct sis1100_32_dsp_code {
    compat_caddr_t     src;  /* pointer to code */
    u_int32_t dst;  /* load address in SHARC memory*/
    compat_int_t       size; /* code size in bytes */
};

struct sis1100_32_eeprom_req {
    u_int8_t num;    /* number of 16-bit-words */
    u_int8_t addr;   /* eeprom address */
    compat_caddr_t data; /* user space address */
};

struct sis1100_32_ddma_map {
    compat_caddr_t addr;
    compat_size_t size;
    compat_int_t num;
};

struct sis1100_32_ddma_stop {
    u_int64_t num;    /* all! transferred bytes */
    compat_int_t idx; /* index of last block written */
};

#define SIS1100_32_SETVMESPACE     _IOW (GLINK_MAGIC,  1, struct vmespace_32)
#define SIS3100_32_VME_PROBE       _IOW (GLINK_MAGIC,  2, u_int32_t)
#define SIS3100_32_VME_READ        _IOWR(GLINK_MAGIC,  3, struct sis1100_32_vme_req)
#define SIS3100_32_VME_WRITE       _IOWR(GLINK_MAGIC,  4, struct sis1100_32_vme_req)
#define SIS3100_32_VME_BLOCK_READ  _IOWR(GLINK_MAGIC,  5, struct sis1100_32_vme_block_req)
#define SIS3100_32_VME_BLOCK_WRITE _IOWR(GLINK_MAGIC,  6, struct sis1100_32_vme_block_req)
#ifdef SIS1100_NEW_CTRL
#define SIS1100_32_CTRL_READ       _IOWR(GLINK_MAGIC,  7, struct sis1100_32_ctrl_reg)
#define SIS1100_32_CTRL_WRITE      _IOWR(GLINK_MAGIC,  8, struct sis1100_32_ctrl_reg)
#else
#define SIS1100_32_LOCAL_CTRL_READ _IOWR(GLINK_MAGIC,  7, struct sis1100_32_ctrl_reg)
#define SIS1100_32_LOCAL_CTRL_WRITE _IOWR(GLINK_MAGIC, 8, struct sis1100_32_ctrl_reg)
#define SIS1100_32_REMOTE_CTRL_READ _IOWR(GLINK_MAGIC, 9, struct sis1100_32_ctrl_reg)
#define SIS1100_32_REMOTE_CTRL_WRITE _IOWR(GLINK_MAGIC, 10, struct sis1100_32_ctrl_reg)
#endif
#define SIS1100_32_PIPE            _IOWR(GLINK_MAGIC, 11, struct sis1100_32_pipe)
#define SIS1100_32_MAPSIZE         _IOR (GLINK_MAGIC, 12, u_int32_t)
#define SIS1100_32_LAST_ERROR      _IOR (GLINK_MAGIC, 13, u_int32_t)
#define SIS1100_32_IDENT           _IOR (GLINK_MAGIC, 14, struct sis1100_32_ident)
#define SIS1100_32_FIFOMODE        _IOWR(GLINK_MAGIC, 15, compat_int_t)

#define SIS1100_32_IRQ_CTL         _IOW (GLINK_MAGIC, 17, struct sis1100_irq_ctl)
#define SIS1100_32_IRQ_CTL2        _IOW (GLINK_MAGIC, 52, struct sis1100_irq_ctl2)
#define SIS1100_32_IRQ_GET         _IOWR(GLINK_MAGIC, 18, struct sis1100_irq_get)
#define SIS1100_32_IRQ_GET2        _IOWR(GLINK_MAGIC, 53, struct sis1100_irq_get2)
#define SIS1100_32_IRQ_ACK         _IOW (GLINK_MAGIC, 19, struct sis1100_irq_ack)
#define SIS1100_32_IRQ_WAIT        _IOWR(GLINK_MAGIC, 20, struct sis1100_irq_get)
#define SIS1100_32_IRQ_WAIT2       _IOWR(GLINK_MAGIC, 54, struct sis1100_irq_get2)

#define SIS1100_32_MINDMALEN       _IOWR(GLINK_MAGIC, 21, compat_int_t[2])

#define SIS1100_32_FRONT_IO        _IOWR(GLINK_MAGIC, 22, u_int32_t)
#define SIS1100_32_FRONT_PULSE     _IOW (GLINK_MAGIC, 23, u_int32_t)
#define SIS1100_32_FRONT_LATCH     _IOWR(GLINK_MAGIC, 24, u_int32_t)

#define SIS3100_32_VME_SUPER_BLOCK_READ _IOWR(GLINK_MAGIC, 25, struct sis1100_32_vme_super_block_req)
#define SIS1100_32_WRITE_PIPE      _IOWR(GLINK_MAGIC, 26, struct sis1100_32_writepipe)

#define SIS1100_32_DMA_ALLOC       _IOWR(GLINK_MAGIC, 27, struct sis1100_32_dma_alloc)
#define SIS1100_32_DMA_FREE        _IOW (GLINK_MAGIC, 28, struct sis1100_32_dma_alloc)

#define SIS5100_32_CCCZ            _IO  (GLINK_MAGIC, 29)
#define SIS5100_32_CCCC            _IO  (GLINK_MAGIC, 30)
#define SIS5100_32_CCCI            _IOW (GLINK_MAGIC, 31, compat_int_t)
#define SIS5100_32_CNAF            _IOWR(GLINK_MAGIC, 32, struct sis1100_camac_req)
#define SIS1100_32_SWAP            _IOWR(GLINK_MAGIC, 33, compat_int_t)
#define SIS3100_32_TIMEOUTS        _IOWR(GLINK_MAGIC, 34, compat_int_t[2])

#define SIS1100_32_DSP_LOAD        _IOW (GLINK_MAGIC, 35, struct sis1100_32_dsp_code)
#define SIS1100_32_DSP_RESET       _IO  (GLINK_MAGIC, 36)
#define SIS1100_32_DSP_START       _IO  (GLINK_MAGIC, 37)

#define SIS1100_32_DEMAND_DMA_MAP  _IOW (GLINK_MAGIC, 39, struct sis1100_32_ddma_map)
#define SIS1100_32_DEMAND_DMA_START _IO  (GLINK_MAGIC, 40)
#define SIS1100_32_DEMAND_DMA_STOP _IOWR(GLINK_MAGIC, 41, struct sis1100_32_ddma_stop)
#define SIS1100_32_DEMAND_DMA_MARK _IOW (GLINK_MAGIC, 42, compat_int_t)
#define SIS1100_32_DEMAND_DMA_WAIT _IOWR(GLINK_MAGIC, 43, compat_int_t)
#define SIS3100_32_VME_WRITE_BLIND _IOWR(GLINK_MAGIC, 44, struct sis1100_32_vme_req)

#define SIS1100_32_SERIAL_NO       _IOR (GLINK_MAGIC, 45, u_int32_t[4])
#define SIS1100_32_DSP_READ        _IOWR(GLINK_MAGIC, 46, struct sis1100_32_dsp_code)

#define SIS1100_32_MINPIPELEN      _IOWR(GLINK_MAGIC, 47, compat_int_t[2])

#define SIS1100_32_DSP_WR           _IOW (GLINK_MAGIC, 50, struct sis1100_dsp_code)
#define SIS1100_32_DSP_RD           _IOWR(GLINK_MAGIC, 51, struct sis1100_dsp_code)

#define SIS1100_32_RESET           _IO  (GLINK_MAGIC, 102)
#define SIS1100_32_REMOTE_RESET    _IO  (GLINK_MAGIC, 103)
#define SIS1100_32_DEVTYPE         _IOR (GLINK_MAGIC, 104, compat_int_t)
#define SIS1100_32_DRIVERVERSION   _IOR (GLINK_MAGIC, 105, compat_int_t)
#define SIS1100_32_READ_EEPROM     _IOW (GLINK_MAGIC, 106, struct sis1100_32_eeprom_req)
#define SIS1100_32_WRITE_EEPROM    _IOW (GLINK_MAGIC, 107, struct sis1100_32_eeprom_req)
#define SIS1100_32_JTAG_ENABLE     _IOW (GLINK_MAGIC, 108, u_int32_t)
#define SIS1100_32_JTAG_CTRL       _IOWR(GLINK_MAGIC, 109, u_int32_t)
#define SIS1100_32_JTAG_DATA       _IOR (GLINK_MAGIC, 110, u_int32_t)
#define SIS1100_32_JTAG_PUT        _IOW (GLINK_MAGIC, 111, u_int32_t)
#define SIS1100_32_JTAG_GET        _IOR (GLINK_MAGIC, 112, u_int32_t)

#define SIS1100_32_PLX_READ        _IOWR(GLINK_MAGIC, 113, struct sis1100_32_ctrl_reg)
#define SIS1100_32_PLX_WRITE       _IOWR(GLINK_MAGIC, 114, struct sis1100_32_ctrl_reg)
#define SIS1100_32_EEPROM_SIZE     _IOR (GLINK_MAGIC, 115, compat_int_t)

#endif
