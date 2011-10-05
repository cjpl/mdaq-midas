/* $ZEL: sis1100_init_sdram.c,v 1.6 2010/01/21 21:50:17 wuestner Exp $ */

/*
 * Copyright (c) 2001-2004
 * 	Matthias Kirsch, Peter Wuestner.  All rights reserved.
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

#define SDRAM_EEPROM_CTRL_STAT  0x40000400

#define SDRAM_SCL    0x1
#define SDRAM_SDA    0x2
#define SDRAM_SDA_OE 0x4

static int
sis1100_spd_write(struct sis1100_softc* sc, u_int32_t val)
{
    u_int32_t error;
    
    sis1100writereg(sc, t_hdr, 0x0f060402);
    wmb_reg();
    sis1100writereg(sc, t_dal, val);
    wmb_reg();
    sis1100writereg(sc, t_adl, SDRAM_EEPROM_CTRL_STAT);
    mb_reg();
    do {
        error=sis1100readreg(sc, prot_error);
    } while (error==0x005);
    return error;
}

static int
sis1100_spd_read(struct sis1100_softc* sc, u_int32_t* val)
{
    u_int32_t error;
    
    sis1100writereg(sc, t_hdr, 0x0f060002);
    wmb_reg();
    sis1100writereg(sc, t_adl, SDRAM_EEPROM_CTRL_STAT);
    mb_reg();
    do {
	error=sis1100readreg(sc, prot_error);
    } while (error==0x005);
    rmb_reg();
    *val=sis1100readreg(sc, tc_dal);
    return error;
}

static int
sdram_eeprom_start(struct sis1100_softc* sc)
{
    sis1100_spd_write(sc, 0);
    sis1100_spd_write(sc, SDRAM_SDA_OE|SDRAM_SDA);
    sis1100_spd_write(sc, SDRAM_SDA_OE|SDRAM_SDA|SDRAM_SCL);
    sis1100_spd_write(sc, SDRAM_SDA_OE|SDRAM_SCL);
    sis1100_spd_write(sc, SDRAM_SDA_OE);
    sis1100_spd_write(sc, 0) ;
    return 0;
}

static int
sdram_eeprom_stop(struct sis1100_softc* sc)
{
  sis1100_spd_write(sc, 0);
  sis1100_spd_write(sc, SDRAM_SDA_OE);
  sis1100_spd_write(sc, SDRAM_SDA_OE|SDRAM_SCL);
  sis1100_spd_write(sc, SDRAM_SDA_OE|SDRAM_SDA|SDRAM_SCL);
  sis1100_spd_write(sc, SDRAM_SDA_OE|SDRAM_SDA);
  sis1100_spd_write(sc, 0);
  return 0;
}

static int
sdram_eeprom_read(struct sis1100_softc* sc, int noack, u_int8_t* val)
{
    u_int32_t d;
    u_int8_t data;
    int i;

    data=0;
    for (i=0; i<8; i++) {
        sis1100_spd_write(sc, 0);
        sis1100_spd_write(sc, SDRAM_SCL);
        sis1100_spd_write(sc, SDRAM_SCL);
        sis1100_spd_read(sc, &d);

        data<<=1;
        data|=((d & 0x100)>>8);
    }

    *val=data;

    sis1100_spd_write(sc, noack?SDRAM_SDA_OE|SDRAM_SDA:SDRAM_SDA_OE);
    sis1100_spd_write(sc, noack?SDRAM_SDA_OE|SDRAM_SDA|SDRAM_SCL:SDRAM_SDA_OE|SDRAM_SCL);
    sis1100_spd_write(sc, noack?SDRAM_SDA_OE|SDRAM_SDA|SDRAM_SCL:SDRAM_SDA_OE|SDRAM_SCL);
    sis1100_spd_write(sc, noack?SDRAM_SDA_OE|SDRAM_SDA:SDRAM_SDA_OE);
    sis1100_spd_write(sc, 0);
    return 0 ;
}

static int
sdram_eeprom_write(struct sis1100_softc* sc, u_int8_t val)
{
    u_int32_t data ;
    int i ;

    for (i=0; i<8; i++) {
        data=(val&0x80)?SDRAM_SDA_OE|SDRAM_SDA:SDRAM_SDA_OE;
        sis1100_spd_write(sc, data);
        sis1100_spd_write(sc, data);

        sis1100_spd_write(sc, data|SDRAM_SCL);

        sis1100_spd_write(sc, data);
        val<<=1;
    }

    sis1100_spd_write(sc, 0);
    sis1100_spd_write(sc, 0);
    sis1100_spd_write(sc, SDRAM_SCL);
    sis1100_spd_write(sc, SDRAM_SCL);
    sis1100_spd_write(sc, 0);
    return 0 ;
}

#if 0
static void
print_eeprominfo(struct sis1100_softc* sc, u_int8_t* eeprom_bytes,
    u_int32_t eeprom_signature)
{
    pINFO(sc, "eeprom[0..7]: %02x %02x %02x %02x %02x %02x %02x %02x",
        eeprom_bytes[0], eeprom_bytes[1], eeprom_bytes[2], eeprom_bytes[3],
        eeprom_bytes[4], eeprom_bytes[5], eeprom_bytes[6], eeprom_bytes[7]);
    pINFO(sc, "eeprom_signature=0x%04x", eeprom_signature);
}
#endif

int
sis1100_init_sdram(struct sis1100_softc* sc)
{
/* sc->sem_hw must be held by caller!*/
    u_int32_t eeprom_signature;
    u_int8_t eeprom_bytes[8];
    u_int8_t dummy;
    int i;

    sdram_eeprom_start(sc) ;
    sdram_eeprom_write(sc, 0xA0); /* device Write cmd  */
    sdram_eeprom_write(sc, 0x00); /* write address */

    sdram_eeprom_start(sc) ;

    sdram_eeprom_write(sc, 0xA1); /* device Read cmd  */

    for (i=0; i<8; i++) sdram_eeprom_read(sc, 0, eeprom_bytes+i);

    sdram_eeprom_read(sc, 1, &dummy);
    sdram_eeprom_stop(sc);

    eeprom_signature=(eeprom_bytes[3]<<16)|(eeprom_bytes[4]<<8)|(eeprom_bytes[5]);
#if 0
    print_eeprominfo(sc, eeprom_bytes, eeprom_signature);
#endif

    switch (eeprom_signature) {
    case 0x0c0901:
        sc->ram_size=64*1024*1024;
        break;
    case 0x0c0902:
        sc->ram_size=128*1024*1024;
        break;
    case 0x0d0a01:
        sc->ram_size=256*1024*1024;
        sis1100_spd_write(sc, 1<<16);
        break;
    case 0x0d0a02:
        sc->ram_size=512*1024*1024;
        sis1100_spd_write(sc, 1<<16);
        break;
    case 0xffffff:
        sc->ram_size=0;
        pINFO(sc, "no SDRAM installed");
        break;
    default:
        pERROR(sc, "SDRAM not supported: row=%d col=%d banks=%d",
                eeprom_bytes[3], eeprom_bytes[4], eeprom_bytes[5]);
        sc->ram_size=0;
    }
    return 0;
}
