/* $ZEL*/

/*
 * Copyright (c) 2009
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

#include "sis1100_sc.h"

static int
wait_for_fifo(struct sis1100_softc* sc, int fifolen, int* cc)
{
    while (*cc>=fifolen) {
        u_int32_t sr, err;
        cpu_relax();
        sr=sis1100readreg(sc, sr);
        if (sr&0x4000) {
            //pERROR(sc, "sr=%08x", sr);
            err=sis1100readreg(sc, prot_error);
            //pERROR(sc, "error=0x%x", err);
            return err;
        }
        *cc=sis1100readreg(sc, p_balance);
    }
    return 0;
}

int
sis1100_pipe_ctrl_read_local(
	struct sis1100_softc* sc,
	struct sis1100_fdata* fd,
	u_int32_t addr,
	int fifo,
	u_int32_t count,	/* words to be transferred */
				/* count==0 is illegal */
	u_int32_t __user *data	/* destination (user virtual address) */
)
{
	int idx;

#if 0
	pERROR(sc, "pipe_ctrl_read_local fifo=%d count=%u addr=%08x",
		fifo, count, addr);
#endif

	mutex_lock(&sc->sem_hw);
	for (idx=0; idx<count; idx++, data++) {
		u_int32_t val;
                val=_readreg32(sc, addr);
		__put_user(val, data);
		if (!fifo)
			addr+=4;
	}
	mutex_unlock(&sc->sem_hw);
	return 0;
}

/*
XXX das ist 'loop', nicht 'pipe'!
*/
int
sis1100_pipe_ctrl_read_remote(
	struct sis1100_softc* sc,
	struct sis1100_fdata* fd,
	u_int32_t addr,
	int fifo,
	u_int32_t count,	/* words to be transferred */
				/* count==0 is illegal */
	u_int32_t* count_read,	/* words transferred */
	u_int32_t __user *data,	/* destination (user virtual address) */
	u_int32_t* prot_error
	)
{
	u_int32_t head, prot_error_;
	int idx;

#if 0
	pERROR(sc, "pipe_ctrl_read_remote fifo=%d count=%u addr=%08x",
		fifo, count, addr);
#endif

	*count_read=count;
	*prot_error=0;
	head=0x0f000002;
	mutex_lock(&sc->sem_hw);
	sis1100writereg(sc, t_hdr, head);
	for (idx=0; idx<count; idx++, data++) {
		u_int32_t val;
		sis1100writereg(sc, t_adl, addr);
		do {
			prot_error_=sis1100readreg(sc, prot_error);
		} while (prot_error_==0x005);
		if (prot_error_) {
			*prot_error=prot_error_;
			*count_read=idx;
			break;
		}
		val=sis1100rawreadreg(sc, tc_dal);
		__put_user(val, data);
		if (!fifo)
			addr+=4;
	}
	mutex_unlock(&sc->sem_hw);
	return 0;
}

int
sis1100_pipe_ctrl_write_local(
	struct sis1100_softc* sc,
	struct sis1100_fdata* fd,
	u_int32_t addr,
	int fifo,
	u_int32_t count,	/* words to be transferred */
				/* count==0 is illegal */
	u_int32_t __user *data	/* source (user virtual address) */
)
{
	int idx;

#if 0
	pERROR(sc, "pipe_ctrl_write_local fifo=%d count=%u addr=%08x",
		fifo, count, addr);
#endif

	mutex_lock(&sc->sem_hw);
	for (idx=0; idx<count; idx++, data++) {
		u_int32_t val;
		__get_user(val, data);
		_writereg32(sc, addr, val);
		if (!fifo)
			addr+=4;
	}
	mutex_unlock(&sc->sem_hw);
	return 0;
}

int
sis1100_pipe_ctrl_write_remote(
	struct sis1100_softc* sc,
	struct sis1100_fdata* fd,
	u_int32_t addr,
	int fifo,
	u_int32_t count,		/* words to be transferred */
					/* count==0 is illegal */
	u_int32_t* count_written,	/* words transferred */
	u_int32_t __user *data,		/* source (user virtual address) */
	u_int32_t* prot_error
)
{
    u_int32_t head, error, balance;
    int idx, cc=0, fifolen, lcount;

#if 0
    pERROR(sc, "pipe_ctrl_write_remote fifo=%d count=%u addr=%08x",
            fifo, count, addr);
#endif

    *count_written=count;
    *prot_error=0;
    head=0x0f000404;

    mutex_lock(&sc->sem_hw);

    fifolen=sc->sendfifo_size/3-1; /* 3 words per request */

    sis1100writereg(sc, t_hdr, head);
    if (fifo) {
        sis1100writereg(sc, t_adl, addr);
        for (idx=0; idx<count; idx++, data++) {
            u_int32_t val;
            __get_user(val, (u_int32_t*)data);
            sis1100writereg(sc, t_dal, val);
            cc++;
            if ((*prot_error=wait_for_fifo(sc, fifolen, &cc)))
                break;
        }
    } else {
        for (idx=0; idx<count; idx++, data++, addr+=4) {
            u_int32_t val;

            __get_user(val, (u_int32_t*)data);
            sis1100writereg(sc, t_adl, addr);
            sis1100writereg(sc, t_dal, val);
            cc++;
            if ((*prot_error=wait_for_fifo(sc, fifolen, &cc)))
                break;
        }
    }

    /*
     * reading prot_error blocks until p_balance==0 OR prot_error!=0
     * ==> we have to read prot_error until p_balance is 0
     * prot_error is reset to 0 after read
     * ==> we have to store the first prot_error!=0
     */
    lcount=0;
    do {
        balance=sis1100readreg(sc, p_balance);
        do {
            error=sis1100readreg(sc, prot_error);
        } while (error==sis1100_e_dlock);
        if (error==sis1100_le_dlock) {
            head=sis1100readreg(sc, tc_hdr);
            if ((head&0x300)==0x300) {
                error=((head>>24)&0xff)|0x200;
            } else {
                error=0;
            }
        }
        lcount++;
        if (error && !*prot_error)
            *prot_error=error;
        if (error==sis1100_le_to) {
            sis1100writereg(sc, p_balance, 0);
        }
    } while (balance>0 && lcount<1000000);
{
    if (balance) {
        pERROR(sc, "pipe_ctrl_write: fifo=%d, num=%u",
                fifo, count);
        pERROR(sc, "balance=%d, prot_error=0x%x, error=0x%x, lcount=%d",
                balance, *prot_error, error, lcount);
    }
}
    mutex_unlock(&sc->sem_hw);

    /*
     * In case of error this is not really correct, but we don't know how
     * many data have been successfully written before an error occured.
     */
    *count_written=*prot_error?0:count;

    return 0;
}
