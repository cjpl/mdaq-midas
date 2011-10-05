/* $ZEL: sis1100_ioctl.c,v 1.28 2010/01/18 19:24:52 wuestner Exp $ */

/*
 * Copyright (c) 2001-2009
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

#include "sis1100_sc.h"
#include "sis5100_map.h"

static int
test_super(struct sis1100_softc* sc, struct sis1100_fdata* fd)
{
return 0;
#ifdef __NetBSD__
    if (suser(fd->p->p_ucred, &fd->p->p_acflag)) return EPERM;
#elif __linux__
    if (!capable(CAP_SYS_RAWIO)) return EPERM;
#endif
    return 0;
}

#ifdef SIS1100_NEW_CTRL
static int
ioctl_ctrl_read(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_ctrl_reg* d)
{
    int check_hw_invalid=0;

    if (fd->subdev==sis1100_subdev_ctrl) {
        if (d->offset&~0xfff)
            return EINVAL;
        mutex_lock(&sc->sem_hw);
        d->val=_readreg32(sc, d->offset);
        mutex_unlock(&sc->sem_hw);
        d->error=0;
    } else {
        if (d->offset&~0x7ff)
            return EINVAL;
        if (check_hw_invalid && sc->remote_hw==sis1100_hw_invalid)
            return ENXIO;
        d->error=sis1100_remote_reg_read(sc, d->offset, &d->val, 0);
    }
    return 0;
}

static int
ioctl_ctrl_write(struct sis1100_softc* sc, struct sis1100_fdata* fd, 
    struct sis1100_ctrl_reg* d)
{
    int check_hw_invalid=0;

    if (fd->subdev==sis1100_subdev_ctrl) {
        if (d->offset&~0xfff)
            return EINVAL;
        mutex_lock(&sc->sem_hw);
        _writereg32(sc, d->offset, d->val);
        mutex_unlock(&sc->sem_hw);
        d->error=0;
    } else {
        if (d->offset&~0x7ff)
            return EINVAL;
        if (check_hw_invalid && sc->remote_hw==sis1100_hw_invalid)
            return ENXIO;
        d->error=sis1100_remote_reg_write(sc, d->offset, d->val, 0);
    }
    return 0;
}

#else

static int
ioctl_local_ctrl_read(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_ctrl_reg* d)
{
    if (d->offset&~0xfff) return EINVAL;
    mutex_lock(&sc->sem_hw);
    d->val=_readreg32(sc, d->offset);
    mutex_unlock(&sc->sem_hw);
    d->error=0;
    return 0;
}

static int
ioctl_local_ctrl_write(struct sis1100_softc* sc, struct sis1100_fdata* fd, 
    struct sis1100_ctrl_reg* d)
{
    if (d->offset&~0xfff) return EINVAL;
    mutex_lock(&sc->sem_hw);
    _writereg32(sc, d->offset, d->val);
    mutex_unlock(&sc->sem_hw);
    d->error=0;
    return 0;
}

static int
ioctl_remote_ctrl_read(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_ctrl_reg* d)
{
    if (d->offset&~0x7ff) return EINVAL;
    if (sc->remote_hw==sis1100_hw_invalid) return ENXIO;
    d->error=sis1100_remote_reg_read(sc, d->offset, &d->val, 0);
    return 0;
}

static int
ioctl_remote_ctrl_write(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_ctrl_reg* d)
{
    if (d->offset&~0x7ff) return EINVAL;
    if (sc->remote_hw==sis1100_hw_invalid) return ENXIO;
    d->error=sis1100_remote_reg_write(sc, d->offset, d->val, 0);
    return 0;
}
#endif

static int
ioctl_ctrl_block_check(struct sis1100_ctrl_rw *d, u_int32_t max)
{
    if (d->offset&3)
        return EINVAL;
    if (d->offset&~max)
        return EINVAL;
    if (!d->fifo && (d->offset+d->count*4)&~max)
        return EINVAL;
    return 0;
}

static int
ioctl_ctrl_read_block(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_ctrl_rw *d)
{
    if (fd->subdev==sis1100_subdev_ctrl) {
        if (ioctl_ctrl_block_check(d, 0xfff))
            return EINVAL;
        d->error=0;
        sis1100_pipe_ctrl_read_local(sc, fd, d->offset, d->fifo,
                d->count, d->data);
    } else if (sc->remote_hw!=sis1100_hw_invalid) {
        u_int32_t count_read, prot_error;
        if (ioctl_ctrl_block_check(d, 0x7ff))
            return EINVAL;
        sis1100_pipe_ctrl_read_remote(sc, fd, d->offset, d->fifo,
                d->count, &count_read, d->data, &prot_error);
        d->count=count_read;
        d->error=prot_error;
    } else {
        return ENXIO;
    }
    return 0;
}

static int
ioctl_ctrl_write_block(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_ctrl_rw *d)
{
    if (fd->subdev==sis1100_subdev_ctrl) {
        if (ioctl_ctrl_block_check(d, 0xfff))
            return EINVAL;
        d->error=0;
        sis1100_pipe_ctrl_write_local(sc, fd, d->offset, d->fifo,
                d->count, d->data);
    } else if (sc->remote_hw!=sis1100_hw_invalid) {
        u_int32_t count_written, prot_error;
        if (ioctl_ctrl_block_check(d, 0x7ff))
            return EINVAL;
        sis1100_pipe_ctrl_write_remote(sc, fd,d->offset, d->fifo,
                d->count, &count_written, d->data, &prot_error);
        d->count=count_written;
        d->error=prot_error;
    } else {
        return ENXIO;
    }
    return 0;
}

static int
ioctl_plx_read(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_ctrl_reg* d)
{
    d->val=_plxreadreg32(sc, d->offset);
    d->error=0;
    return 0;
}

static int
ioctl_plx_write(struct sis1100_softc* sc, struct sis1100_fdata* fd, 
    struct sis1100_ctrl_reg* d)
{
    _plxwritereg32(sc, d->offset, d->val);
    d->error=0;
    return 0;
}

static int
ioctl_ident(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_ident* d)
{
    d->local.hw_type=sc->local_ident&0xff;
    d->local.hw_version=(sc->local_ident>>8)&0xff;
    d->local.fw_type=(sc->local_ident>>16)&0xff;
    d->local.fw_version=(sc->local_ident>>24)&0xff;

    d->remote.hw_type=sc->remote_ident&0xff;
    d->remote.hw_version=(sc->remote_ident>>8)&0xff;
    d->remote.fw_type=(sc->remote_ident>>16)&0xff;
    d->remote.fw_version=(sc->remote_ident>>24)&0xff;

    d->remote_ok=sc->remote_hw!=sis1100_hw_invalid;
    d->remote_online=(sis1100readreg(sc, sr)&sr_synch)==sr_synch;
    return 0;
}

static int
ioctl_serial_no(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    int32_t* d)
/*
 * d[0]: CMC_Type 0: 5V 1: 3.3V 2: both
 * d[1]: serial number of base board
 * d[2]: serial number of piggy back
 * d[3]: ?
 */
{
    int res;
    res=sis1100_serial_numbers(sc, d);
    return res;
}

static int
ioctl_devtype(struct sis1100_fdata* fd, int* d)
{
    *d=fd->subdev;
    return 0;
}

static int
ioctl_driverversion(int* d)
{
    *d=SIS1100_Version;
    return 0;
}

static int
ioctl_mindmalen(struct sis1100_fdata* fd, int* d)
{
/*
 *   0: never use DMA
 *   1: always use DMA
 *  >1: use DMA if transfersize (in Bytes) is >= mindmalen
 *  -1: don't change old value
 *  d[0] controls read length
 *  d[1] controls write length
 *  the old values are returned
 */
    int tmp[2];

    tmp[0]=fd->mindmalen_r;
    tmp[1]=fd->mindmalen_w;
    if (d[0]>=0) fd->mindmalen_r=d[0];
    if (d[1]>=0) fd->mindmalen_w=d[1];
    d[0]=tmp[0];
    d[1]=tmp[1];
    return 0;
}

static int
ioctl_minpipelen(struct sis1100_fdata* fd, int* d)
{
/*
 * similar to ioctl_mindmalen
 * use pipelined transfer if transfersize (in Bytes) is >= minpipelen
 * (DMA is used if transfersize is >= mindmalen)
 * (it is not yet implemented for write)
 */
    int tmp[2];

    tmp[0]=fd->minpipelen_r;
    tmp[1]=fd->minpipelen_w;
    if (d[0]>=0) fd->minpipelen_r=d[0];
    if (d[1]>=0) fd->minpipelen_w=d[1];
    d[0]=tmp[0];
    d[1]=tmp[1];
    return 0;
}

static int
ioctl_setvmespace(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct vmespace* d)
{
    if ((d->datasize!=1) && (d->datasize!=2) && (d->datasize!=4))
        return EINVAL;
    fd->vmespace_am=d->am;
    fd->vmespace_datasize=d->datasize;
    if (d->swap>=0) {
        sc->user_wants_swap=d->swap;
        sis1100_update_swapping(sc);
    }
    if (d->mindmalen>=0) {
        fd->mindmalen_r=d->mindmalen;
        fd->mindmalen_w=d->mindmalen;
    }
    return 0;
}

static int
ioctl_swap(struct sis1100_softc* sc, struct sis1100_fdata* fd, int* d)
{
    int old;

    old=sc->user_wants_swap;
    sc->user_wants_swap=*d;
    sis1100_update_swapping(sc);
    *d=old;
    return 0;
}

static int
ioctl_3100_timeouts(struct sis1100_softc* sc, struct sis1100_fdata* fd, int* d)
{
/*
 *  d[0]: bus error terms of 10**-9 s
 *  d[1]: arbitration timeout in terms of 10**-3 s
 */
    int tmp[2];
    if ((fd->subdev!=sis1100_subdev_remote)
        || (sc->remote_hw!=sis1100_hw_vme))
                return ENXIO;
    mutex_lock(&sc->sem_hw);
    if (sis3100_get_timeouts(sc, tmp+0, tmp+1))
        goto error;
    if (sis3100_set_timeouts(sc, d[0], d[1]))
        goto error;
    mutex_unlock(&sc->sem_hw);
    d[0]=tmp[0];
    d[1]=tmp[1];
    return 0;
error:
    mutex_unlock(&sc->sem_hw);
    return EIO;
}

static int
ioctl_front_io(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    u_int32_t* d)
{
    return sis1100_front_io(sc, d, 0);
}

static int
ioctl_front_pulse(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    u_int32_t* d)
{
    return sis1100_front_pulse(sc, d, 0);
}

static int
ioctl_front_latch(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    u_int32_t* d)
{
    return sis1100_front_latch(sc, d, 0);
}

static int
ioctl_last_error(struct sis1100_fdata* fd, u_int32_t* d)
{
    *d=fd->last_prot_err;
    return 0;
}

static int
ioctl_mapsize(struct sis1100_softc* sc, struct sis1100_fdata* fd, u_int32_t* d)
{
    switch (fd->subdev) {
        case sis1100_subdev_remote:
	    *d=pci_resource_len(sc->pdev, 3);
	    break;
	case sis1100_subdev_ram:
	    *d=sc->remote_hw==sis1100_hw_invalid?0:sc->ram_size;
	    break;
	case sis1100_subdev_ctrl:
	    *d=pci_resource_len(sc->pdev, 2);
	    break;
	case sis1100_subdev_dsp:
	    *d=0;
	    break;
	default:
	    return EINVAL;
    }
    return 0;
}

static int
ioctl_pipe(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_pipe* d)
{
    int res;
    if (sc->remote_hw==sis1100_hw_invalid) return ENXIO;
    res=sis1100_read_pipe(sc, d);
    return res;
}

static int
ioctl_write_pipe(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_writepipe* d)
{
    u_int32_t* list;
    int res;

    if (sc->remote_hw==sis1100_hw_invalid) return ENXIO;

#ifdef __NetBSD__
    list=malloc(d->num*2*sizeof(u_int32_t), M_IOCTLOPS,
            M_WAITOK/*|M_CANFAIL*/);
#elif __linux__
    list=kmalloc(d->num*2*sizeof(u_int32_t), GFP_KERNEL);
#endif
    if (!list) return ENOMEM;

    if (
#ifdef __NetBSD__
        copyin(d->data, list, d->num*2*sizeof(u_int32_t))
#elif __linux__
        copy_from_user(list, d->data, d->num*2*sizeof(u_int32_t))
#endif
        ) {
	res=EFAULT;
        goto raus;
    }

    res=0;
    d->error=sis1100_write_pipe(sc, d->am, 1/*space*/, d->num, list);

raus:
#ifdef __NetBSD__
    free(list, M_IOCTLOPS);
#elif __linux__
    kfree(list);
#endif
    return res;
}

static int
ioctl_vme_probe(struct sis1100_softc* sc, struct sis1100_fdata* fd, int* d)
{
    int dummy;
    if (sc->remote_hw!=sis1100_hw_vme) return ENXIO;
    if (sis1100_tmp_read(sc, *d, fd->vmespace_am,
	    fd->vmespace_datasize, 1/*space*/, &dummy))
	return EIO;
    return 0;
}

static int
ioctl_vme_read(struct sis1100_softc* sc, struct sis1100_fdata* fd,
        struct sis1100_vme_req* d)
{
    if (sc->remote_hw==sis1100_hw_invalid) return ENXIO;
    d->error=sis1100_tmp_read(sc, d->addr, d->am, d->size,
	    1/*space*/, &d->data);
    return 0;
}

static int
ioctl_vme_write(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_vme_req* d)
{
    if (sc->remote_hw==sis1100_hw_invalid) return ENXIO;
    d->error=sis1100_tmp_write(sc, d->addr, d->am, d->size,
	    1/*space*/, d->data);
    return 0;
}

static int
ioctl_vme_write_blind(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_vme_req* d)
{
    if (sc->remote_hw==sis1100_hw_invalid)
        return ENXIO;
    d->error=sis1100_tmp_write_blind(sc, d->addr, d->am, d->size,
	    1/*space*/, d->data);
    return 0;
}

static int
ioctl_vme_block_read(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_vme_block_req* d)
{
    int res;
    if (sc->remote_hw==sis1100_hw_invalid)
        return ENXIO;
    res=sis1100_read_block(sc, fd, d->size, d->fifo, d->num, &d->num,
        1/*space*/, d->am, d->addr, d->data, &d->error);
    return res;
}

static int
ioctl_vme_super_block_read(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_vme_super_block_req* d)
{
    struct sis1100_vme_block_req* reqs;
    int res, i;

    if (sc->remote_hw==sis1100_hw_invalid)
        return ENXIO;
#ifdef __NetBSD__
    reqs=malloc(d->n*sizeof(struct sis1100_vme_block_req), M_IOCTLOPS,
            M_WAITOK/*|M_CANFAIL*/);
#elif __linux__
    reqs=kmalloc(d->n*sizeof(struct sis1100_vme_block_req), GFP_KERNEL);
#endif
    if (!reqs)
        return ENOMEM;

    if (
#ifdef __NetBSD__
        copyin(d->reqs, reqs, d->n*sizeof(struct sis1100_vme_block_req))
#elif __linux__
        copy_from_user(reqs, d->reqs,
            d->n*sizeof(struct sis1100_vme_block_req))
#endif
        ) {
	res=EFAULT;
        goto raus;
    }

    d->error=0;
    for (i=0; i<d->n; i++) {
        struct sis1100_vme_block_req* r=reqs+i;
	res=sis1100_read_block(sc, fd, r->size, r->fifo, r->num,
        &r->num, 1/*space*/, r->am, r->addr, r->data, &r->error);
	if (res) {
            d->n=i;
            d->error=res;
            break;
        }
    }
    res=0;
    if (
#ifdef __NetBSD__
        copyout(reqs, d->reqs, d->n*sizeof(struct sis1100_vme_block_req))
#elif __linux__
        copy_to_user(d->reqs, reqs,
            d->n*sizeof(struct sis1100_vme_block_req))
#endif
        ) res=EFAULT;

raus:
#ifdef __NetBSD__
    free(reqs, M_IOCTLOPS);
#elif __linux__
    kfree(reqs);
#endif
    return res;
}

static int
ioctl_vme_block_write(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_vme_block_req* d)
{
    int res;
 
    if (sc->remote_hw==sis1100_hw_invalid) return ENXIO;
    res=sis1100_write_block(sc, fd, d->size, d->fifo, d->num,
        &d->num, 1/*space*/, d->am, d->addr, d->data, &d->error);
    return res;
}

static int
ioctl_fifomode(struct sis1100_fdata* fd, int* d)
{
    int tmp;
    tmp=fd->fifo_mode;
    if (*d>=0) fd->fifo_mode=!!*d;
    *d=tmp;
    return 0;
}

static int
ioctl_irq_ctl(struct sis1100_fdata* fd, struct sis1100_irq_ctl* d)
{
    struct sis1100_irq_ctl2 d2;

    memcpy(&d2, d, sizeof(struct sis1100_irq_ctl));
    d2.auto_mask=0;
    d2.flags=0;
    return sis1100_irq_ctl(fd, &d2);
}

static int
ioctl_irq_ctl2(struct sis1100_fdata* fd, struct sis1100_irq_ctl2* d)
{
    return sis1100_irq_ctl(fd, d);
}

static int
ioctl_irq_get(struct sis1100_fdata* fd, struct sis1100_irq_get* d)
{
    int res;
    struct sis1100_irq_get2 d2;

    memcpy(&d2, d, sizeof(struct sis1100_irq_get));
    res=sis1100_irq_get(fd, &d2);
    memcpy(d, &d2, sizeof(struct sis1100_irq_get));
    return res;
}

static int
ioctl_irq_get2(struct sis1100_fdata* fd, struct sis1100_irq_get2* d)
{
    return sis1100_irq_get(fd, d);
}

static int
ioctl_irq_ack(struct sis1100_fdata* fd, struct sis1100_irq_ack* d)
{
    return sis1100_irq_ack(fd, d);
}

static int
ioctl_irq_wait(struct sis1100_fdata* fd, struct sis1100_irq_get* d)
{
    int res;
    struct sis1100_irq_get2 d2;

    memcpy(&d2, d, sizeof(struct sis1100_irq_get));
    res=sis1100_irq_wait(fd, &d2);
    memcpy(d, &d2, sizeof(struct sis1100_irq_get));
    return res;
}

static int
ioctl_irq_wait2(struct sis1100_fdata* fd, struct sis1100_irq_get2* d)
{
    return sis1100_irq_wait(fd, d);
}

static int
ioctl_reset(struct sis1100_softc* sc, struct sis1100_fdata* fd)
{
    pERROR(sc, "reset");
    
    sis1100writereg(sc, cr, cr_reset); /* master reset */
    /* reset remote, ignore whether it exists */
#if 0
    sis1100writereg(sc, cr, cr_rem_reset);
#endif
    sis1100_flush_fifo(sc, 1);
    sis1100writereg(sc, p_balance, 0);
    sis1100readreg(sc, prot_error);
    return 0;
}

static int
ioctl_cccz(struct sis1100_softc* sc, struct sis1100_fdata* fd)
{
    int res;

    if (sc->remote_hw!=sis1100_hw_camac) return ENXIO;
    res=sis1100_tmp_camacwrite(sc, SIS5100_CAMACaddr(28, 9, 26), 0);
    return 0;
}

static int
ioctl_cccc(struct sis1100_softc* sc, struct sis1100_fdata* fd)
{
    int res;

    if (sc->remote_hw!=sis1100_hw_camac) return ENXIO;
    res=sis1100_tmp_camacwrite(sc, SIS5100_CAMACaddr(28, 8, 26), 0);
    return 0;
}

static int
ioctl_ccci(struct sis1100_softc* sc, struct sis1100_fdata* fd, int* d)
{
    int res;

    if (sc->remote_hw!=sis1100_hw_camac)
        return ENXIO;
    res=sis5100writeremreg(sc, camac_sc, *d?1:0x10000, 0);
    return 0;
}

static int
ioctl_cnaf(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_camac_req* d)
{
    u_int32_t addr;

    addr=SIS5100_CAMACaddr(d->N, d->A, d->F);

    switch (d->F&0x18) {
    case 0x00: /* read */
        d->error=sis1100_tmp_camacread(sc, addr, &d->data);
        break;
    case 0x10: /* write */
        d->error=sis1100_tmp_camacwrite(sc, addr, d->data);
        break;
    case 0x08: /* ctrl */
    case 0x18: /* ctrl */
        d->error=sis1100_tmp_camacwrite(sc, addr, 0);
    }
    return 0;
}

static int
ioctl_eeprom_size(struct sis1100_softc* sc, struct sis1100_fdata* fd,
        int* d)
{
    *d=sis1100_end_of_eeprom(sc);
    return 0;
}

static int
ioctl_read_eeprom(struct sis1100_softc* sc, struct sis1100_fdata* fd,
        struct sis1100_eeprom_req* d)
{
    return sis1100_read_eeprom(sc, d->num, d->addr, d->data, 1);
}

static int
ioctl_write_eeprom(struct sis1100_softc* sc, struct sis1100_fdata* fd, 
        struct sis1100_eeprom_req* d)
{
    if (test_super(sc, fd)) return EPERM;
    return sis1100_write_eeprom(sc, d->num, d->addr, d->data, 1);
}

static int
ioctl_testflags(struct sis1100_softc* sc, struct sis1100_fdata* fd,
        u_int32_t* d)
{
    u_int32_t old=sc->testflags;
    if (old!=*d)
        pERROR(sc, "setting testflags to 0x%08x", *d);
    sc->testflags=*d;
    *d=old;
    return 0;
}

static int
ioctl_jtag_enable(struct sis1100_softc* sc, struct sis1100_fdata* fd,
        u_int32_t* d)
{
    if (test_super(sc, fd)) return EPERM;
    *d<<=8;
    sis1100writereg(sc, jtag_csr.jtag_csrl, *d);
    return 0;
}

static int
ioctl_jtag_ctrl(struct sis1100_softc* sc, struct sis1100_fdata* fd,
        u_int32_t* d)
{
    if (test_super(sc, fd)) return EPERM;
    sis1100writereg(sc, jtag_csr.jtag_csrb[1], *d);
    *d=sis1100readreg(sc, jtag_csr.jtag_csrl);
    return 0;
}

static int
ioctl_jtag_data(struct sis1100_softc* sc, struct sis1100_fdata* fd,
        u_int32_t* d)
{
    *d=sis1100readreg(sc, jtag_data);
    return 0;
}

static int
ioctl_jtag_put(struct sis1100_softc* sc, struct sis1100_fdata* fd,
        u_int32_t* d)
{
    if (test_super(sc, fd)) return EPERM;
    sis1100writereg(sc, jtag_csr.jtag_csrl, *d);
    return 0;
}

static int
ioctl_jtag_get(struct sis1100_softc* sc, struct sis1100_fdata* fd,
        u_int32_t* d)
{
    *d=sis1100readreg(sc, jtag_csr.jtag_csrl);
    return 0;
}

#if !(defined(__linux__) && defined(HAVE_COMPAT_IOCTL))
static
#endif
long _sis1100_ioctl(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    unsigned int cmd, void* data)
{
    int res=0;

    switch (cmd) {
    case SIS1100_SETVMESPACE:
        res=ioctl_setvmespace(sc, fd, (struct vmespace*)data); break;
    case SIS3100_VME_PROBE:
        res=ioctl_vme_probe(sc, fd, (int*)data); break;
    case SIS3100_VME_READ:
        res=ioctl_vme_read(sc, fd, (struct sis1100_vme_req*)data); break;
    case SIS3100_VME_WRITE:
        res=ioctl_vme_write(sc, fd, (struct sis1100_vme_req*)data); break;
    case SIS3100_VME_WRITE_BLIND:
        res=ioctl_vme_write_blind(sc, fd, (struct sis1100_vme_req*)data); break;
    case SIS3100_VME_BLOCK_READ:
        res=ioctl_vme_block_read(sc, fd, (struct sis1100_vme_block_req*)data);
        break;
    case SIS3100_VME_BLOCK_WRITE:
        res=ioctl_vme_block_write(sc, fd, (struct sis1100_vme_block_req*)data);
        break;
#ifdef SIS1100_NEW_CTRL
    case SIS1100_CTRL_READ:
        res=ioctl_ctrl_read(sc, fd, (struct sis1100_ctrl_reg*)data);
        break;
    case SIS1100_CTRL_WRITE:
        res=ioctl_ctrl_write(sc, fd, (struct sis1100_ctrl_reg*)data);
        break;
#else
    case SIS1100_LOCAL_CTRL_READ:
        res=ioctl_local_ctrl_read(sc, fd, (struct sis1100_ctrl_reg*)data);
        break;
    case SIS1100_LOCAL_CTRL_WRITE:
        res=ioctl_local_ctrl_write(sc, fd, (struct sis1100_ctrl_reg*)data);
        break;
    case SIS1100_REMOTE_CTRL_READ:
        res=ioctl_remote_ctrl_read(sc, fd, (struct sis1100_ctrl_reg*)data);
        break;
    case SIS1100_REMOTE_CTRL_WRITE:
        res=ioctl_remote_ctrl_write(sc, fd, (struct sis1100_ctrl_reg*)data);
        break;
#endif
    case SIS1100_CTRL_READ_BLOCK:
        res=ioctl_ctrl_read_block(sc, fd, (struct sis1100_ctrl_rw*)data);
        break;
    case SIS1100_CTRL_WRITE_BLOCK:
        res=ioctl_ctrl_write_block(sc, fd, (struct sis1100_ctrl_rw*)data);
        break;

    case SIS1100_PIPE:
        res=ioctl_pipe(sc, fd, (struct sis1100_pipe*)data); break;
    case SIS1100_MAPSIZE:
        res=ioctl_mapsize(sc, fd, (u_int32_t*)data); break;
    case SIS1100_LAST_ERROR:
        res=ioctl_last_error(fd, (u_int32_t*)data); break;
    case SIS1100_IDENT:
        res=ioctl_ident(sc, fd, (struct sis1100_ident*)data); break;
    case SIS1100_SERIAL_NO:
        res=ioctl_serial_no(sc, fd, (int*)data); break;
    case SIS1100_FIFOMODE:
        res=ioctl_fifomode(fd, (int*)data); break;

    case SIS1100_IRQ_CTL:
        res=ioctl_irq_ctl(fd, (struct sis1100_irq_ctl*)data); break;
    case SIS1100_IRQ_CTL2:
        res=ioctl_irq_ctl2(fd, (struct sis1100_irq_ctl2*)data); break;
    case SIS1100_IRQ_GET:
        res=ioctl_irq_get(fd, (struct sis1100_irq_get*)data); break;
    case SIS1100_IRQ_GET2:
        res=ioctl_irq_get2(fd, (struct sis1100_irq_get2*)data); break;
    case SIS1100_IRQ_ACK:
        res=ioctl_irq_ack(fd, (struct sis1100_irq_ack*)data); break;
    case SIS1100_IRQ_WAIT:
        res=ioctl_irq_wait(fd, (struct sis1100_irq_get*)data); break;
    case SIS1100_IRQ_WAIT2:
        res=ioctl_irq_wait2(fd, (struct sis1100_irq_get2*)data); break;

    case SIS1100_MINDMALEN:
        res=ioctl_mindmalen(fd, (int*)data); break;
    case SIS1100_MINPIPELEN:
        res=ioctl_minpipelen(fd, (int*)data); break;

    case SIS1100_FRONT_IO:
        res=ioctl_front_io(sc, fd, (u_int32_t*)data); break;
    case SIS1100_FRONT_PULSE:
        res=ioctl_front_pulse(sc, fd, (u_int32_t*)data); break;
    case SIS1100_FRONT_LATCH:
        res=ioctl_front_latch(sc, fd, (u_int32_t*)data); break;

    case SIS3100_VME_SUPER_BLOCK_READ:
        res=ioctl_vme_super_block_read(sc, fd,
                (struct sis1100_vme_super_block_req*)data);
        break;
    case SIS1100_WRITE_PIPE:
        res=ioctl_write_pipe(sc, fd, (struct sis1100_writepipe*)data); break;

    case SIS5100_CCCZ:
        res=ioctl_cccz(sc, fd); break;
    case SIS5100_CCCC:
        res=ioctl_cccc(sc, fd); break;
    case SIS5100_CCCI:
        res=ioctl_ccci(sc, fd, (int*)data); break;
    case SIS5100_CNAF:
        res=ioctl_cnaf(sc, fd, (struct sis1100_camac_req*)data); break;
    case SIS1100_SWAP:
        res=ioctl_swap(sc, fd, (int*)data); break;
    case SIS3100_TIMEOUTS:
        res=ioctl_3100_timeouts(sc, fd, (int*)data); break;

    case SIS1100_DSP_LOAD:
        res=sis1100_dsp_load(sc, fd, (struct sis1100_dsp_code*)data); break;
    case SIS1100_DSP_READ:
        res=sis1100_dsp_read(sc, fd, (struct sis1100_dsp_code*)data); break;
    case SIS1100_DSP_RESET:
        res=sis1100_dsp_reset(sc, fd); break;
    case SIS1100_DSP_START:
        res=sis1100_dsp_start(sc, fd); break;
    case SIS1100_DSP_WR:
        res=sis1100_dsp_wr(sc, fd, (struct sis1100_dsp_code*)data); break;
    case SIS1100_DSP_RD:
        res=sis1100_dsp_rd (sc, fd, (struct sis1100_dsp_code*)data); break;

    case SIS1100_DEMAND_DMA_MAP:
        res=sis1100_ddma_map(sc, fd, (struct sis1100_ddma_map*)data); break;
    case SIS1100_DEMAND_DMA_START:
        res=sis1100_ddma_start(sc, fd); break;
    case SIS1100_DEMAND_DMA_STOP:
        res=sis1100_ddma_stop(sc, fd, (struct sis1100_ddma_stop*)data); break;
    case SIS1100_DEMAND_DMA_MARK:
        res=sis1100_ddma_mark(sc, fd, (int*)data); break;
    case SIS1100_DEMAND_DMA_WAIT:
        res=sis1100_ddma_wait(sc, fd, (int*)data); break;

    case SIS1100_RESET:
        res=ioctl_reset(sc, fd); break;
    case SIS1100_REMOTE_RESET:
        sis1100_init_remote(sc, 2); break;
    case SIS1100_DEVTYPE:
        res=ioctl_devtype(fd, (int*)data); break;
    case SIS1100_DRIVERVERSION:
        res=ioctl_driverversion((int*)data); break;
    case SIS1100_READ_EEPROM:
        res=ioctl_read_eeprom(sc, fd, (struct sis1100_eeprom_req*)data); break;
    case SIS1100_WRITE_EEPROM:
        res=ioctl_write_eeprom(sc, fd, (struct sis1100_eeprom_req*)data); break;
    case SIS1100_EEPROM_SIZE:
        res=ioctl_eeprom_size(sc, fd, (int*)data); break;
    case SIS1100_TESTFLAGS:
        res=ioctl_testflags(sc, fd, (u_int32_t*)data); break;
    case SIS1100_TRANSPARENT:
        res=sis1100_transparent(sc, fd, (int32_t*)data); break;

    case SIS1100_JTAG_ENABLE:
        res=ioctl_jtag_enable(sc, fd, (u_int32_t*)data); break;
    case SIS1100_JTAG_CTRL:
        res=ioctl_jtag_ctrl(sc, fd, (u_int32_t*)data); break;
    case SIS1100_JTAG_DATA:
        res=ioctl_jtag_data(sc, fd, (u_int32_t*)data); break;
    case SIS1100_JTAG_PUT:
        res=ioctl_jtag_put(sc, fd, (u_int32_t*)data); break;
    case SIS1100_JTAG_GET:
        res=ioctl_jtag_get(sc, fd, (u_int32_t*)data); break;
    case SIS1100_PLX_READ:
        res=ioctl_plx_read(sc, fd, (struct sis1100_ctrl_reg*)data); break;
    case SIS1100_PLX_WRITE:
        res=ioctl_plx_write(sc, fd, (struct sis1100_ctrl_reg*)data); break;

    default:
/*
        pINFO(sc, "cmd=0x%x DIR=%d TYPE=%d NR=%d SIZE=%d", cmd,
            _IOC_DIR(cmd), _IOC_TYPE(cmd), _IOC_NR(cmd), _IOC_SIZE(cmd));
*/
        res=ENOTTY; break;
    }
    return res;
}

#ifdef __NetBSD__
int
sis1100_ioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
    struct sis1100_softc* sc=SIS1100SC(dev);
    struct sis1100_fdata* fd=SIS1100FD(dev);
    fd->p=p;
    return _sis1100_ioctl(sc, fd, cmd, data);
}
#elif __linux__

/*
 * This union is only used to determine the size of the longest
 * ioctl argument. If you add an ioctl with an argument which is
 * not obviously shorter then union all add it here.
 */
union all {
    struct vmespace                               vmespace;
    struct sis1100_vme_req                         vme_req;
    struct sis1100_vme_block_req             vme_block_req;
    struct sis1100_ctrl_reg                       ctrl_reg;
    struct sis1100_pipe                               pipe;
    struct sis1100_ident                             ident;
    struct sis1100_irq_ctl                         irq_ctl;
    struct sis1100_irq_get                         irq_get;
    struct sis1100_irq_ack                  irq_ackirq_ack;
    struct sis1100_vme_super_block_req vme_super_block_req;
    struct sis1100_writepipe                     writepipe;
    struct sis1100_dma_alloc                     dma_alloc;
    struct sis1100_camac_req                     camac_req;
    struct sis1100_dsp_code                       dsp_code;
    struct sis1100_eeprom_req                   eeprom_req;
};

#define MAX_DATA (sizeof(union all))
    
#ifdef HAVE_UNLOCKED_IOCTL
long
sis1100_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
#else
int
sis1100_ioctl(struct inode *inode, struct file *file,
	      unsigned int cmd, unsigned long arg)
#endif
{
    struct sis1100_softc* sc=SIS1100SC(file);
    struct sis1100_fdata* fd=SIS1100FD(file);
    u_int8_t data[MAX_DATA];
    int res;

    if ((cmd&IOC_INOUT) && (_IOC_SIZE(cmd)>MAX_DATA)) {
        pINFO(sc, "sis1100_ioctl: cmd=0x%08x _IOC_SIZE(cmd)=%d",
            cmd, _IOC_SIZE(cmd));
        return -EINVAL;
    }

    if ((cmd&IOC_IN) && (_IOC_SIZE(cmd) <= MAX_DATA)) {
        if (copy_from_user(&data, (void *)arg, _IOC_SIZE(cmd)))
                return -EFAULT;
    }
    if ((res=_sis1100_ioctl(sc, fd, cmd, &data)))
            return -res;

    if (cmd&IOC_OUT) {
        if (copy_to_user((void *)arg, &data, _IOC_SIZE(cmd)))
                return -EFAULT;
    }
    return 0;
}

#endif
