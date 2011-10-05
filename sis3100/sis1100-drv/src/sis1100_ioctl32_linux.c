/* $ZEL: sis1100_ioctl32_linux.c,v 1.12 2010/01/18 19:24:52 wuestner Exp $ */

/*
 * Copyright (c) 2005-2008
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

#include <linux/version.h>
#include <linux/compat.h>

#ifdef CONFIG_COMPAT
#include <linux/ioctl.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22)
#include <linux/ioctl32.h>
#endif
#include "sis1100_sc.h"
#include "sis1100_ioctl32_linux.h"
#include "sis5100_map.h"

static int
ioctl32_setvmespace(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct vmespace_32* d)
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
ioctl32_vme_read(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_32_vme_req* d)
{
    if (sc->remote_hw==sis1100_hw_invalid)
        return ENXIO;
    d->error=sis1100_tmp_read(sc, d->addr, d->am, d->size,
	    1/*space*/, &d->data);
    return 0;
}

static int
ioctl32_vme_write(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_32_vme_req* d)
{
    if (sc->remote_hw==sis1100_hw_invalid)
        return ENXIO;
    d->error=sis1100_tmp_write(sc, d->addr, d->am, d->size,
	    1/*space*/, d->data);
    return 0;
}

static int
ioctl32_vme_write_blind(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_32_vme_req* d)
{
    if (sc->remote_hw==sis1100_hw_invalid)
        return ENXIO;
    d->error=sis1100_tmp_write_blind(sc, d->addr, d->am, d->size,
	    1/*space*/, d->data);
    return 0;
}

static int
ioctl32_vme_block_read(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_32_vme_block_req* d)
{
    int res;
    size_t num;

    if (sc->remote_hw==sis1100_hw_invalid)
        return ENXIO;

    res=sis1100_read_block(sc, fd, d->size, d->fifo, d->num, &num,
        1/*space*/, d->am, d->addr, compat_ptr(d->data), &d->error);
    d->num=num;

    return res;
}

static int
ioctl32_vme_block_write(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_32_vme_block_req* d)
{
    int res;
    size_t num;
 
    if (sc->remote_hw==sis1100_hw_invalid)
        return ENXIO;

    res=sis1100_write_block(sc, fd, d->size, d->fifo, d->num, &num,
        1/*space*/, d->am, d->addr, compat_ptr(d->data), &d->error);
    d->num=num;

    return res;
}

#ifdef SIS1100_NEW_CTRL
static int
ioctl32_ctrl_read(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_32_ctrl_reg* d)
{
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
        if (sc->remote_hw==sis1100_hw_invalid)
            return ENXIO;
        d->error=sis1100_remote_reg_read(sc, d->offset, &d->val, 0);
    }
    return 0;
}

static int
ioctl32_ctrl_write(struct sis1100_softc* sc, struct sis1100_fdata* fd, 
    struct sis1100_32_ctrl_reg* d)
{
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
        if (sc->remote_hw==sis1100_hw_invalid)
            return ENXIO;
        d->error=sis1100_remote_reg_write(sc, d->offset, d->val, 0);
    }
    return 0;
}

#else

static int
ioctl32_local_ctrl_read(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_32_ctrl_reg* d)
{
    if (d->offset&~0xfff)
        return EINVAL;
    mutex_lock(&sc->sem_hw);
    d->val=_readreg32(sc, d->offset);
    mutex_unlock(&sc->sem_hw);
    d->error=0;
    return 0;
}

static int
ioctl32_local_ctrl_write(struct sis1100_softc* sc, struct sis1100_fdata* fd, 
    struct sis1100_32_ctrl_reg* d)
{
    if (d->offset&~0xfff)
        return EINVAL;
    mutex_lock(&sc->sem_hw);
    _writereg32(sc, d->offset, d->val);
    mutex_unlock(&sc->sem_hw);
    d->error=0;
    return 0;
}

static int
ioctl32_remote_ctrl_read(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_32_ctrl_reg* d)
{
    if (d->offset&~0x7ff)
        return EINVAL;
    if (sc->remote_hw==sis1100_hw_invalid)
        return ENXIO;
    d->error=sis1100_remote_reg_read(sc, d->offset, &d->val, 0);
    return 0;
}

static int
ioctl32_remote_ctrl_write(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_32_ctrl_reg* d)
{
    if (d->offset&~0x7ff)
        return EINVAL;
    if (sc->remote_hw==sis1100_hw_invalid)
        return ENXIO;
    d->error=sis1100_remote_reg_write(sc, d->offset, d->val, 0);
    return 0;
}
#endif

static int
ioctl32_plx_read(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_32_ctrl_reg* d)
{
    d->val=_plxreadreg32(sc, d->offset);
    d->error=0;
    return 0;
}

static int
ioctl32_plx_write(struct sis1100_softc* sc, struct sis1100_fdata* fd, 
    struct sis1100_32_ctrl_reg* d)
{
    _plxwritereg32(sc, d->offset, d->val);
    d->error=0;
    return 0;
}

static int
ioctl32_pipe(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_32_pipe* d32)
{
    int res;
    struct sis1100_pipe d;

    if (sc->remote_hw==sis1100_hw_invalid)
        return ENXIO;

    d.num=d32->num;
    d.list=compat_ptr(d32->list);
    d.data=compat_ptr(d32->data);
    res=sis1100_read_pipe(sc, &d);
    d32->error=d.error;
    return res;
}

static int
ioctl32_ident(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_32_ident* d)
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
ioctl32_write_pipe(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_32_writepipe* d)
{
    u_int32_t* list;
    int res;

    if (sc->remote_hw==sis1100_hw_invalid)
        return ENXIO;

    list=kmalloc(d->num*2*sizeof(u_int32_t), GFP_KERNEL);
    if (!list)
        return ENOMEM;

    if (copy_from_user(list, compat_ptr((u32)d->data),
            d->num*2*sizeof(u_int32_t))) {
	res=EFAULT;
        goto raus;
    }

    res=0;
    d->error=sis1100_write_pipe(sc, d->am, 1/*space*/, d->num, list);

raus:
    kfree(list);
    return res;
}

static int
ioctl32_fifomode(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    compat_int_t* d)
{
    int tmp;
    tmp=fd->fifo_mode;
    if (*d>=0)
        fd->fifo_mode=!!*d;
    *d=tmp;
    return 0;
}

static int
ioctl32_devtype(struct sis1100_softc* sc, struct sis1100_fdata* fd,
        compat_int_t* d)
{
    *d=fd->subdev;
    return 0;
}

static int
ioctl32_driverversion(struct sis1100_softc* sc, struct sis1100_fdata* fd,
        compat_int_t* d)
{
    *d=SIS1100_Version;
    return 0;
}

static int
ioctl32_mindmalen(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    compat_int_t* d)
{
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
ioctl32_dma_alloc(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_32_dma_alloc* d)
{
    return ENOTTY;
}

static int
ioctl32_dma_free(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_32_dma_alloc* d)
{
    /*return sis1100_dma_free(sc, fd, d);*/
    return ENOTTY;
}

static int
ioctl32_ccci(struct sis1100_softc* sc, struct sis1100_fdata* fd,
        compat_int_t* d)
{
    int res;

    if (sc->remote_hw!=sis1100_hw_camac)
        return ENXIO;
    res=sis5100writeremreg(sc, camac_sc, *d?1:0x10000, 0);
    return 0;
}

static int
ioctl32_swap(struct sis1100_softc* sc, struct sis1100_fdata* fd,
        compat_int_t* d)
{
    int old;

    old=sc->user_wants_swap;
    sc->user_wants_swap=*d;
    sis1100_update_swapping(sc);
    *d=old;
    return 0;
}

static int
ioctl32_3100_timeouts(struct sis1100_softc* sc, struct sis1100_fdata* fd,
        compat_int_t* d)
{
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
ioctl32_sis1100_ddma_stop(struct sis1100_softc* sc, struct sis1100_fdata* fd,
        struct sis1100_32_ddma_stop *d)
{
    struct sis1100_ddma_stop stop;
    int res;

    stop.num=d->num;
    stop.idx=d->idx;

    res=sis1100_ddma_stop(sc, fd, &stop);

    d->num=stop.num;
    d->idx=stop.idx;

    return res; 
}

static int
ioctl32_sis1100_ddma_mark(struct sis1100_softc* sc, struct sis1100_fdata* fd,
        compat_int_t *d)
{
    int data=*d;
    int res=sis1100_ddma_mark(sc, fd, &data);
    *d=data;
    return res; 
}

static int
ioctl32_sis1100_ddma_wait(struct sis1100_softc* sc, struct sis1100_fdata* fd,
        compat_int_t *d)
{
    int data=*d;
    int res=sis1100_ddma_wait(sc, fd, &data);
    *d=data;
    return res; 
}

static int
ioctl32_dsp_load(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_32_dsp_code* d32)
{
    struct sis1100_dsp_code d;
    int res;

    d.src=compat_ptr(d32->src);
    d.dst=d32->dst;
    d.size=d32->size;
    res=sis1100_dsp_load(sc, fd, &d);
    return res;
}

static int
ioctl32_dsp_read(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_32_dsp_code* d32)
{
    struct sis1100_dsp_code d;
    int res;

    d.src=compat_ptr(d32->src);
    d.dst=d32->dst;
    d.size=d32->size;
    res=sis1100_dsp_load(sc, fd, &d);
    return res;
}

static int
ioctl32_dsp_wr(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_32_dsp_code* d32)
{
    struct sis1100_dsp_code d;
    int res;

    d.src=compat_ptr(d32->src);
    d.dst=d32->dst;
    d.size=d32->size;
    res=sis1100_dsp_wr(sc, fd, &d);
    return res;
}

static int
ioctl32_dsp_rd(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_32_dsp_code* d32)
{
    struct sis1100_dsp_code d;
    int res;

    d.src=compat_ptr(d32->src);
    d.dst=d32->dst;
    d.size=d32->size;
    res=sis1100_dsp_rd(sc, fd, &d);
    return res;
}

static int
ioctl32_ddma_map(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    struct sis1100_32_ddma_map* d32)
{
    struct sis1100_ddma_map d;
    d.num=d32->num;
    d.addr=compat_ptr(d32->addr);
    d.size=d32->size;
    return sis1100_ddma_map(sc, fd, &d);
}

static int
ioctl32_eeprom_size(struct sis1100_softc* sc, struct sis1100_fdata* fd,
        compat_int_t* d)
{
    *d=sis1100_end_of_eeprom(sc);
    return 0;
}

static int
ioctl32_read_eeprom(struct sis1100_softc* sc, struct sis1100_fdata* fd,
        struct sis1100_32_eeprom_req* d)
{
    return sis1100_read_eeprom(sc, d->num, d->addr, compat_ptr(d->data), 1);
}

static int
ioctl32_write_eeprom(struct sis1100_softc* sc, struct sis1100_fdata* fd, 
        struct sis1100_32_eeprom_req* d)
{
    /*if (test_super(sc, fd)) return EPERM;*/
    return sis1100_write_eeprom(sc, d->num, d->addr, compat_ptr(d->data), 1);
}

static int
_sis1100_ioctl32(struct sis1100_softc* sc, struct sis1100_fdata* fd,
    unsigned int cmd, void* data)
{
    int res=0;

    switch (cmd) {
    case SIS1100_32_SETVMESPACE:
        res=ioctl32_setvmespace(sc, fd, (struct vmespace_32*)data);
        break;
    case SIS3100_32_VME_READ:
        res=ioctl32_vme_read(sc, fd, (struct sis1100_32_vme_req*)data);
        break;
    case SIS3100_32_VME_WRITE:
        res=ioctl32_vme_write(sc, fd, (struct sis1100_32_vme_req*)data);
        break;
    case SIS3100_32_VME_BLOCK_READ:
        res=ioctl32_vme_block_read(sc, fd,
                (struct sis1100_32_vme_block_req*)data);
        break;
    case SIS3100_32_VME_BLOCK_WRITE:
        res=ioctl32_vme_block_write(sc, fd,
                (struct sis1100_32_vme_block_req*)data);
        break;
#ifdef SIS1100_NEW_CTRL
    case SIS1100_32_CTRL_READ:
        res=ioctl32_ctrl_read(sc, fd, (struct sis1100_32_ctrl_reg*)data);
        break;
    case SIS1100_32_CTRL_WRITE:
        res=ioctl32_ctrl_write(sc, fd, (struct sis1100_32_ctrl_reg*)data);
        break;
#else
    case SIS1100_32_LOCAL_CTRL_READ:
        res=ioctl32_local_ctrl_read(sc, fd, (struct sis1100_32_ctrl_reg*)data);
        break;
    case SIS1100_32_LOCAL_CTRL_WRITE:
        res=ioctl32_local_ctrl_write(sc, fd, (struct sis1100_32_ctrl_reg*)data);
        break;
    case SIS1100_32_REMOTE_CTRL_READ:
        res=ioctl32_remote_ctrl_read(sc, fd, (struct sis1100_32_ctrl_reg*)data);
        break;
    case SIS1100_32_REMOTE_CTRL_WRITE:
        res=ioctl32_remote_ctrl_write(sc, fd,
                (struct sis1100_32_ctrl_reg*)data);
        break;
#endif
    case SIS1100_32_PIPE:
        res=ioctl32_pipe(sc, fd, (struct sis1100_32_pipe*)data);
        break;
    case SIS1100_32_IDENT:
        res=ioctl32_ident(sc, fd, (struct sis1100_32_ident*)data);
        break;
    case SIS1100_32_FIFOMODE:
        res=ioctl32_fifomode(sc, fd, (compat_int_t*)data);
        break;
    case SIS1100_32_MINDMALEN:
        res=ioctl32_mindmalen(sc, fd, (compat_int_t*)data);
        break;
#if 0
    case SIS3100_VME_SUPER_BLOCK_READ:
        res=ioctl32_vme_super_block_read(sc, fd,
                (struct sis1100_32_vme_super_block_req*)data);
        break;
#endif
    case SIS1100_32_WRITE_PIPE:
        res=ioctl32_write_pipe(sc, fd, (struct sis1100_32_writepipe*)data);
        break;
    case SIS1100_32_DMA_ALLOC:
        res=ioctl32_dma_alloc(sc, fd, (struct sis1100_32_dma_alloc*)data);
        break;
    case SIS1100_32_DMA_FREE:
        res=ioctl32_dma_free(sc, fd, (struct sis1100_32_dma_alloc*)data);
        break;
    case SIS5100_32_CCCI:
        res=ioctl32_ccci(sc, fd, (compat_int_t*)data);
        break;
    case SIS1100_32_SWAP:
        res=ioctl32_swap(sc, fd, (compat_int_t*)data);
        break;
    case SIS3100_32_TIMEOUTS:
        res=ioctl32_3100_timeouts(sc, fd, (compat_int_t*)data);
        break;
    case SIS1100_32_DSP_LOAD:
        res=ioctl32_dsp_load(sc, fd, (struct sis1100_32_dsp_code*)data);
        break;
    case SIS1100_32_DSP_READ:
        res=ioctl32_dsp_read(sc, fd, (struct sis1100_32_dsp_code*)data);
        break;
    case SIS1100_32_DSP_WR:
        res=ioctl32_dsp_wr(sc, fd, (struct sis1100_32_dsp_code*)data);
        break;
    case SIS1100_32_DSP_RD:
        res=ioctl32_dsp_rd(sc, fd, (struct sis1100_32_dsp_code*)data);
        break;
    case SIS1100_32_DEMAND_DMA_MAP:
        res=ioctl32_ddma_map(sc, fd, (struct sis1100_32_ddma_map*)data);
        break;
    case SIS1100_32_DEMAND_DMA_STOP:
        res=ioctl32_sis1100_ddma_stop(sc, fd,
                (struct sis1100_32_ddma_stop*)data);
        break;
    case SIS1100_32_DEMAND_DMA_MARK:
        res=ioctl32_sis1100_ddma_mark(sc, fd, (compat_int_t*)data);
        break;
    case SIS1100_32_DEMAND_DMA_WAIT:
        res=ioctl32_sis1100_ddma_wait(sc, fd, (compat_int_t*)data);
        break;
    case SIS3100_32_VME_WRITE_BLIND:
        res=ioctl32_vme_write_blind(sc, fd, (struct sis1100_32_vme_req*)data);
        break;
    case SIS1100_32_DEVTYPE:
        res=ioctl32_devtype(sc, fd, (compat_int_t*)data);
        break;
    case SIS1100_32_DRIVERVERSION:
        res=ioctl32_driverversion(sc, fd, (compat_int_t*)data);
        break;
    case SIS1100_32_READ_EEPROM:
        res=ioctl32_read_eeprom(sc, fd, (struct sis1100_32_eeprom_req*)data);
        break;
    case SIS1100_32_WRITE_EEPROM:
        res=ioctl32_write_eeprom(sc, fd, (struct sis1100_32_eeprom_req*)data);
        break;
    case SIS1100_32_PLX_READ:
        res=ioctl32_plx_read(sc, fd, (struct sis1100_32_ctrl_reg*)data);
        break;
    case SIS1100_32_PLX_WRITE:
        res=ioctl32_plx_write(sc, fd, (struct sis1100_32_ctrl_reg*)data);
        break;
    case SIS1100_32_EEPROM_SIZE:
        res=ioctl32_eeprom_size(sc, fd, (compat_int_t*)data);
        break;

    default:
#ifdef HAVE_COMPAT_IOCTL
        return _sis1100_ioctl(sc, fd, cmd, data);
#else
        pINFO(sc, "ioctl32: cmd=0x%x DIR=%d TYPE=%d NR=%d SIZE=%d", cmd,
            _IOC_DIR(cmd), _IOC_TYPE(cmd), _IOC_NR(cmd), _IOC_SIZE(cmd));
        res=ENOTTY; break;
#endif
    }
    return res;
}

union alles {
    struct vmespace_32                               vmespace;
    struct sis1100_32_vme_req                         vme_req;
    struct sis1100_32_vme_block_req             vme_block_req;
    struct sis1100_32_ctrl_reg                       ctrl_reg;
    struct sis1100_32_pipe                               pipe;
    struct sis1100_32_ident                             ident;
    struct sis1100_32_vme_super_block_req vme_super_block_req;
    struct sis1100_32_writepipe                     writepipe;
    struct sis1100_32_dma_alloc                     dma_alloc;
    struct sis1100_32_dsp_code                       dsp_code;
    struct sis1100_32_eeprom_req                   eeprom_req;
};

#define MAX_DATA (sizeof(union alles))

#ifdef HAVE_COMPAT_IOCTL
long
sis1100_ioctl32(struct file * file, unsigned int cmd, unsigned long arg)
#else
static int
sis1100_ioctl32(unsigned int fd_, unsigned int cmd,
    unsigned long arg, struct file * file)
#endif
{
    struct sis1100_softc* sc=SIS1100SC(file);
    struct sis1100_fdata* fd=SIS1100FD(file);
    u_int8_t data[MAX_DATA];
    int res;

    if ((cmd&IOC_INOUT) && (_IOC_SIZE(cmd)>MAX_DATA)) {
        pINFO(sc, "sis1100_ioctl32: cmd=0x%08x _IOC_SIZE(cmd)=%d",
            cmd, _IOC_SIZE(cmd));
        return -EINVAL;
    }

    if (cmd&IOC_IN) {
        if (copy_from_user(&data, (void *)arg, _IOC_SIZE(cmd)))
                return -EFAULT;
    }
    if ((res=_sis1100_ioctl32(sc, fd, cmd, &data)))
            return -res;

    if (cmd&IOC_OUT) {
        if (copy_to_user((void *)arg, &data, _IOC_SIZE(cmd)))
                return -EFAULT;
    }
    return 0;
}

#ifndef HAVE_COMPAT_IOCTL
static struct ioctl_trans sis1100_ioctl32_trans[] = {
    {SIS1100_32_SETVMESPACE, sis1100_ioctl32, 0},
    {SIS3100_32_VME_PROBE, 0, 0},
    {SIS3100_32_VME_READ, sis1100_ioctl32, 0},
    {SIS3100_32_VME_WRITE, sis1100_ioctl32, 0},
    {SIS3100_32_VME_BLOCK_READ, sis1100_ioctl32, 0},
    {SIS3100_32_VME_BLOCK_WRITE, sis1100_ioctl32, 0},
#ifdef SIS1100_NEW_CTRL
    {SIS1100_32_CTRL_READ, sis1100_ioctl32, 0},
    {SIS1100_32_CTRL_WRITE, sis1100_ioctl32, 0},
#else
    {SIS1100_32_LOCAL_CTRL_READ, sis1100_ioctl32, 0},
    {SIS1100_32_LOCAL_CTRL_WRITE, sis1100_ioctl32, 0},
    {SIS1100_32_REMOTE_CTRL_READ, sis1100_ioctl32, 0},
    {SIS1100_32_REMOTE_CTRL_WRITE, sis1100_ioctl32, 0},
#endif
    {SIS1100_32_PIPE, sis1100_ioctl32, 0},
    {SIS1100_32_MAPSIZE, 0, 0},
    {SIS1100_32_LAST_ERROR, 0, 0},
    {SIS1100_32_IDENT, sis1100_ioctl32, 0},
    {SIS1100_32_FIFOMODE, sis1100_ioctl32, 0},
    {SIS1100_32_IRQ_CTL, sis1100_ioctl32, 0},
    {SIS1100_32_IRQ_GET, sis1100_ioctl32, 0},
    {SIS1100_32_IRQ_EXTGET, sis1100_ioctl32, 0},
    {SIS1100_32_IRQ_ACK, 0, 0},
    {SIS1100_32_IRQ_WAIT, sis1100_ioctl32, 0},
    {SIS1100_32_IRQ_EXTWAIT, sis1100_ioctl32, 0},
    {SIS1100_32_MINDMALEN, sis1100_ioctl32, 0},
    {SIS1100_32_FRONT_IO, 0, 0},
    {SIS1100_32_FRONT_PULSE, 0, 0},
    {SIS1100_32_FRONT_LATCH, 0, 0},
    {SIS3100_32_VME_SUPER_BLOCK_READ, sis1100_ioctl32, 0},
    {SIS1100_32_WRITE_PIPE, sis1100_ioctl32, 0},
    {SIS1100_32_DMA_ALLOC, sis1100_ioctl32, 0},
    {SIS1100_32_DMA_FREE, sis1100_ioctl32, 0},
    {SIS5100_32_CCCZ, 0, 0},
    {SIS5100_32_CCCC, 0, 0},
    {SIS5100_32_CCCI, sis1100_ioctl32, 0},
    {SIS5100_32_CNAF, 0, 0},
    {SIS1100_32_SWAP, sis1100_ioctl32, 0},
    {SIS3100_32_TIMEOUTS, sis1100_ioctl32, 0},
    {SIS1100_32_DSP_LOAD, sis1100_ioctl32, 0},
    {SIS1100_32_DSP_READ, sis1100_ioctl32, 0},
    {SIS1100_32_DSP_RESET, 0, 0},
    {SIS1100_32_DSP_START, 0, 0},
    {SIS1100_32_DEMAND_DMA_MAP, sis1100_ioctl32, 0},
    {SIS1100_32_DEMAND_DMA_START, 0, 0},
    {SIS1100_32_DEMAND_DMA_STOP, sis1100_ioctl32, 0},
    {SIS1100_32_DEMAND_DMA_MARK, sis1100_ioctl32, 0},
    {SIS1100_32_DEMAND_DMA_WAIT, sis1100_ioctl32, 0},
    {SIS3100_32_VME_WRITE_BLIND, sis1100_ioctl32, 0},
    {SIS1100_32_SERIAL_NO, 0, 0},
    {SIS1100_32_DSP_WR, sis1100_ioctl32, 0},
    {SIS1100_32_DSP_RD, sis1100_ioctl32, 0},
    {SIS1100_32_RESET, 0, 0},
    {SIS1100_32_REMOTE_RESET, 0, 0},
    {SIS1100_32_DEVTYPE, sis1100_ioctl32, 0},
    {SIS1100_32_DRIVERVERSION, sis1100_ioctl32, 0},
    {SIS1100_32_READ_EEPROM, sis1100_ioctl32, 0},
    {SIS1100_32_WRITE_EEPROM, sis1100_ioctl32, 0},
    {SIS1100_32_JTAG_ENABLE, 0, 0},
    {SIS1100_32_JTAG_CTRL, 0, 0},
    {SIS1100_32_JTAG_DATA, 0, 0},
    {SIS1100_32_JTAG_PUT, 0, 0},
    {SIS1100_32_JTAG_GET, 0, 0},
    {SIS1100_PLX_READ, sis1100_ioctl32, 0},
    {SIS1100_PLX_WRITE, sis1100_ioctl32, 0},
    {SIS1100_EEPROM_SIZE, sis1100_ioctl32, 0},
    {SIS1100_TESTFLAGS, 0, 0},
    {SIS1100_IRQ_ACK, 0, 0},
    {0, 0, },
};

void __init
sis1100_ioctl32_init(void)
{
    int i;

    for (i=0; sis1100_ioctl32_trans[i].cmd!=0; i++) {
        register_ioctl32_conversion(sis1100_ioctl32_trans[i].cmd,
                sis1100_ioctl32_trans[i].handler);
    }
}

void
sis1100_ioctl32_exit(void)
{
    int i;

    for (i=0; sis1100_ioctl32_trans[i].cmd!=0; i++)
        unregister_ioctl32_conversion(sis1100_ioctl32_trans[i].cmd);
}
#endif /* HAVE_COMPAT_IOCTL */

#endif /* CONFIG_COMPAT */
