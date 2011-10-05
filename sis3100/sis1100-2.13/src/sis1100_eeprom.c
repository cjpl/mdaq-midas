/* $ZEL: sis1100_eeprom.c,v 1.7 2008/02/06 19:57:56 wuestner Exp $ */

#include "sis1100_sc.h"
#include "linux/delay.h"

/*
 * Copyright (C) 2008
 * Peter Wuestner. Forschungszentrum Juelich GmbH
 * P.Wuestner@fz-juelich.de
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * PLX PCI905x serial EEPROM access.
 * Main parts written by Ian Abbott @ MEV Ltd. <ian.abbott@mev.co.uk>.
 * Copyright (C) 2002 MEV Limited.
 */

/*
 * Procedures in this file are used to read and write the serial EEPROM
 * which holds values for PLX-Initialisation.
 * !!Modification of the EEPROM content can render the SIS1100 unusable!!
 */

#define EE_SK	0x01000000
#define EE_CS	0x02000000
#define EE_DI	0x04000000	/* from EEPROM's point of view */
#define EE_DO	0x08000000
#define EE_DOE	0x80000000	/* for PCI9056 */

#define PLX9050_EEMASK	(EE_SK | EE_CS | EE_DI | EE_DO)
#define PLX9056_EEMASK	(EE_SK | EE_CS | EE_DI | EE_DO | EE_DOE)

/*****************************************************************************/
/* Send a few bits.  Assumes *cntrl is valid. */
static void
eeprom_put_bits(struct sis1100_softc* sc, u_int32_t *cntrl, u_int32_t bits,
		unsigned int nbits)
{
	u_int32_t cn = *cntrl;

	while (nbits--) {
		if (bits & (1 << nbits)) {
			cn |= ((EE_DI | EE_DOE) & sc->eemask);
							/* DI=1 */
		} else {
			cn &= ~((EE_DI | EE_DOE) & sc->eemask);
							/* DI=0 */
		}
		cn &= ~EE_SK;			/* SK=0 */
		plxwritereg(sc, CNTRL, cn);
		udelay(2);
		cn |= EE_SK;			/* SK=1 */
		plxwritereg(sc, CNTRL, cn);
		udelay(2);
	}
	*cntrl = cn;
}
/*****************************************************************************/
/* Assert CS and send start bit. */
static void
eeprom_start_cmd(struct sis1100_softc* sc, u_int32_t *cntrl)
{
	u_int32_t cn;

	cn = plxreadreg(sc, CNTRL);
	cn = (cn & ~EE_SK) | ((EE_CS | EE_DI | EE_DOE) & sc->eemask);
						/* SK=0, CS=1, DI=1, DOE1=1 */
	plxwritereg(sc, CNTRL, cn);
	udelay(2);
	cn |= EE_SK;				/* SK=1 */
	plxwritereg(sc, CNTRL, cn);
	udelay(2);
	*cntrl = cn;
}
/*****************************************************************************/
/* Deassert CS.  Assumes *cntrl is valid. */
static void
eeprom_end_cmd(struct sis1100_softc* sc, u_int32_t *cntrl)
{
	u_int32_t cn = *cntrl;

	cn &= ~((EE_CS | EE_SK | EE_DI | EE_DOE) & sc->eemask);
						/* CS=0, SK=0, DI=0, DOE=0 */
	plxwritereg(sc, CNTRL, cn);
	udelay(2);
	*cntrl = cn;
}
/*****************************************************************************/
static void
eeprom_cmd_write_enable(struct sis1100_softc* sc)
{
	u_int32_t cntrl;

	eeprom_start_cmd(sc, &cntrl);
	eeprom_put_bits(sc, &cntrl, 0x3, 4);
	eeprom_put_bits(sc, &cntrl, 0, sc->eeprom_addr_len - 2);
	eeprom_end_cmd(sc, &cntrl);
}
/*****************************************************************************/
static void
eeprom_cmd_write_disable(struct sis1100_softc* sc)
{
	u_int32_t cntrl;

	eeprom_start_cmd(sc, &cntrl);
	eeprom_put_bits(sc, &cntrl, 0, sc->eeprom_addr_len + 2);
	eeprom_end_cmd(sc, &cntrl);
}
/*****************************************************************************/
/* Wait for programming cycle to complete. */
static int
eeprom_wait_prog(struct sis1100_softc* sc)
{
	unsigned long timeout;
	u32 cn;
	int retval;

	timeout = 1 + (((50 * HZ) + 999) / 1000);	/* ~50ms */
	cn = plxreadreg(sc, CNTRL);
	cn = (cn & ~EE_SK) | ((EE_CS | EE_DI | EE_DOE) & sc->eemask);
						/* SK=0, CS=1, DI=1, DOE=1 */
	plxwritereg(sc, CNTRL, cn);
	timeout+=jiffies;
	retval = EIO;
	udelay(2);
	do {
		schedule();
		cn = plxreadreg(sc, CNTRL);
		if ((cn & EE_DO) != 0) {
			/* Cycle complete.  Clear ready status (optional). */
			cn |= EE_SK;		/* SK=1 */
			plxwritereg(sc, CNTRL, cn);
			udelay(2);
			retval = 0;
			break;
		}
	} while (time_before(jiffies, timeout));
	cn &= ~((EE_CS | EE_SK | EE_DI | EE_DOE) & sc->eemask);
						/* CS=0, SK=0, DI=0, DOE=0 */
	plxwritereg(sc, CNTRL, cn);
	udelay(2);
	return retval;
}
/*****************************************************************************/
static int
eeprom_cmd_write_word(struct sis1100_softc* sc, unsigned int offset,
        u_int16_t data)
{
	u_int32_t cntrl;

	eeprom_start_cmd(sc, &cntrl);
	eeprom_put_bits(sc, &cntrl, 0x1, 2);
	eeprom_put_bits(sc, &cntrl, offset, sc->eeprom_addr_len);
	eeprom_put_bits(sc, &cntrl, data, 16);
	eeprom_end_cmd(sc, &cntrl);
	return eeprom_wait_prog(sc);
}
/*****************************************************************************/
static int
eeprom_cmd_read_word(struct sis1100_softc* sc, unsigned int offset,
        u_int32_t *data)
{
	u_int32_t cntrl;
	u_int32_t d;
	int i;
	int retval = 0;

	eeprom_start_cmd(sc, &cntrl);
	eeprom_put_bits(sc, &cntrl, 0x2, 2);
	eeprom_put_bits(sc, &cntrl, offset, sc->eeprom_addr_len);
	udelay(1);
	/* Check dummy bit DO==0. */
	cntrl = plxreadreg(sc, CNTRL);
	if ((cntrl & EE_DO) != 0) {
		retval = EIO;
		goto out;
	}
	cntrl |= ((EE_DI | EE_DOE) & sc->eemask);    /* DI=1, DOE=1 */
	/* Read 32 data bits m.s.b. first. */
	d = 0;
	for (i = 0; i < 32; i++) {
		d <<= 1;
		cntrl &= ~EE_SK;		/* SK=0 */
		plxwritereg(sc, CNTRL, cntrl);
		udelay(2);
		cntrl |= EE_SK;			/* SK=1 */
		plxwritereg(sc, CNTRL, cntrl);
		udelay(3);
		cntrl = plxreadreg(sc, CNTRL);
		if ((cntrl & EE_DO) != 0) {
			d |= 1;
		}
	}
	*data = d;
out:
	eeprom_end_cmd(sc, &cntrl);

	return retval;
}
/*****************************************************************************/
int
sis1100_write_eeprom_(struct sis1100_softc* sc,
        u_int8_t num, u_int8_t addr, u_int16_t* data, int userspace)
{
    int res=0, i;

    if (userspace) {
        if (!access_ok(VERIFY_READ, data, num*2))
            return EFAULT;
    }

    mutex_lock(&sc->sem_hw);

    eeprom_cmd_write_enable(sc);

    for (i=0; i<num; i++, addr++, data++) {
        u_int16_t data_;
        if (userspace)
            __get_user(data_, data);
        else
            data_=*data;

        res = eeprom_cmd_write_word(sc, addr, data_);
        if (res)
            goto error;
    }

    eeprom_cmd_write_disable(sc);

error:
    mutex_unlock(&sc->sem_hw);
    return res;
}
/*****************************************************************************/
int
sis1100_read_eeprom_(struct sis1100_softc* sc,
        u_int8_t num, u_int8_t addr, u_int16_t* data, int userspace)
{
    u_int32_t cache;
    int cached_addr=-1, i;

    if (userspace) {
        if (!access_ok(VERIFY_WRITE, data, num*2))
            return EFAULT;
    }

    mutex_lock(&sc->sem_hw);

    for (i=0; i<num; i++, addr++, data++) {
        u_int16_t data_;
        if (addr&1) {
            if (cached_addr+1!=addr) {
                eeprom_cmd_read_word(sc, addr-1, &cache);
                cached_addr=addr-1;
            }
            data_=cache&0xffff;
        } else {
            eeprom_cmd_read_word(sc, addr, &cache);
            cached_addr=addr;
            data_=(cache>>16)&0xffff;
        }

        if (userspace)
            __put_user(data_, data);
        else
            *data=data_;
    }

    mutex_unlock(&sc->sem_hw);
    return 0;
}
/*****************************************************************************/
int
sis1100_read_eeprom(struct sis1100_softc* sc,
        u_int8_t num, u_int8_t addr, u_int16_t* data, int userspace)
{
        int res=EIO;

        switch (sc->silicon_id) {
        case 0x905410b5: /* classical PCI device with plx9054 */
            sc->eemask=PLX9050_EEMASK;
            sc->eeprom_addr_len=8;
            res=sis1100_read_eeprom_(sc, num, addr, data, userspace);
            break;
        case 0x905610b5: /* PCI Express device with _plx8311 */
            sc->eemask=PLX9056_EEMASK;
            sc->eeprom_addr_len=8;
            res=sis1100_read_eeprom_(sc, num, addr, data, userspace);
            break;
        default:
            pERROR(sc, "unknown device ID 0x%04x", sc->silicon_id);
        }
        return res;
}
/*****************************************************************************/
int
sis1100_write_eeprom(struct sis1100_softc* sc,
        u_int8_t num, u_int8_t addr, u_int16_t* data, int userspace)
{
        int res=EIO;

        switch (sc->silicon_id) {
        case 0x905410b5: /* classical PCI device with plx9054 */
            sc->eemask=PLX9050_EEMASK;
            sc->eeprom_addr_len=8;
            res=sis1100_write_eeprom_(sc, num, addr, data, userspace);
            break;
        case 0x905610b5: /* PCI Express device with _plx8311 */
            sc->eemask=PLX9056_EEMASK;
            sc->eeprom_addr_len=8;
            res=sis1100_write_eeprom_(sc, num, addr, data, userspace);
            break;
        default:
            pERROR(sc, "unknown device ID 0x%04x", sc->silicon_id);
        }
        return res;
}
/*****************************************************************************/
int
sis1100_end_of_eeprom(struct sis1100_softc* sc)
{
        switch (sc->silicon_id) {
        case 0x905410b5: /* classical PCI device with plx9054 */
            return 0x2C;
        case 0x905610b5: /* PCI Express device with plx8311 (contains 9056) */
            return 0x32;
        default:
            pERROR(sc, "eeprom: unknown device ID 0x%04x", sc->silicon_id);
            return -1;
        }
}
/*****************************************************************************/
/*****************************************************************************/
