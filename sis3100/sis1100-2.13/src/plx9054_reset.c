/* $ZEL: plx9054_reset.c,v 1.3 2008/02/06 19:31:55 wuestner Exp $ */

/*
 * Copyright (c) 2001, 2003
 * 	Willi Erven, Peter Wuestner.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "sis1100_sc.h"

/* only used in initialisation; no need for semaphores */

void sis1100_reset_plx9054(struct sis1100_softc* sc)
{
    u_int32_t conf_3C=0;
    int c;

/* save value of irq-line, it will be cleared during reset */
#ifdef __NetBSD__
    conf_3C=pci_conf_read(sc->sc_pc, sc->sc_pcitag, PCI_INTERRUPT_REG);
#elif __linux__
    pci_read_config_dword(sc->pdev, 0x3c, &conf_3C);
#endif
    _plxwritereg8(sc, 0x6f, 0x40);
    wmb_plx();
    _plxwritereg8(sc, 0x6f, 0x00);
    wmb_plx();
    _plxwritereg8(sc, 0x6f, 0x20);
    mb_plx();
    c=0;
    while ((!(plxreadreg(sc, LAS0RR)) || (c<10)) && (++c<50));
    _plxwritereg8(sc, 0x6f, 0x00);

/* restore value of irq-line */
#ifdef __NetBSD__
    pci_conf_write(sc->sc_pc, sc->sc_pcitag, PCI_INTERRUPT_REG, conf_3C);
#elif __linux__
    pci_write_config_dword(sc->pdev, 0x3c, conf_3C);
#endif
}
