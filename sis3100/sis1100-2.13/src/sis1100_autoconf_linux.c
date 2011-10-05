/* $ZEL: sis1100_autoconf_linux.c,v 1.37 2010/01/21 21:49:26 wuestner Exp $ */

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

struct sis1100_global {
    dev_t dev;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)
    struct class *class;
#endif
} sis1100_global;

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,18)
struct pci_device_id sis1100_table[] __devinitconst={
#else
struct pci_device_id sis1100_table[] __devinit={
#endif
    {
    PCI_VENDOR_FZJZEL, PCI_PRODUCT_FZJZEL_GIGALINK,
    PCI_ANY_ID, PCI_ANY_ID,
    0, 0,
    0
    },
    {
    PCI_VENDOR_FZJZEL, PCI_PRODUCT_FZJZEL_SIS1100_eCMC,
    PCI_ANY_ID, PCI_ANY_ID,
    0, 0,
    0
    },
    {
    PCI_VENDOR_FZJZEL, PCI_PRODUCT_FZJZEL_SIS1100_eSINGLE,
    PCI_ANY_ID, PCI_ANY_ID,
    0, 0,
    0
    },
    { 0 }
};
MODULE_DEVICE_TABLE(pci, sis1100_table);

const char* subdevnames[4]={
    "remote",
    "ram",
    "ctrl",
    "dsp"
};

MODULE_AUTHOR("Peter Wuestner <P.Wuestner@fz-juelich.de>");
MODULE_DESCRIPTION("SIS1100 PCI-VME link/interface");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_SUPPORTED_DEVICE("sis1100/sis3100/sis5100; http://www.struck.de/pcivme.htm");

#define DRIVER_VERSION "2.13"
#define DATE_STRING "2010-01-22"
#define NAME_STRING "Forschungszentrum Juelich"

struct sis1100_softc *sis1100_devdata[sis1100_MAXCARDS];

struct file_operations sis1100_fops = {
	.owner   = THIS_MODULE,
	.llseek  = sis1100_llseek,
	.read    = sis1100_read,
	.write   = sis1100_write,
#ifdef HAVE_UNLOCKED_IOCTL
	.unlocked_ioctl = sis1100_ioctl,
#else
	.ioctl   = sis1100_ioctl,
#endif
	.mmap    = sis1100_mmap,
        .poll    = sis1100_poll,
	.open    = sis1100_open,
	.release = sis1100_release,
#if defined (CONFIG_COMPAT) && defined (HAVE_COMPAT_IOCTL)
        .compat_ioctl = sis1100_ioctl32,
#endif
};

#define dev2pci(d) container_of(d, struct pci_dev, dev)
#define dev2sc(d) pci_get_drvdata(dev2pci(d))
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)
#define SHOW_ARGS(d, a, b) (d, a, b) 
#define STORE_ARGS(d, a, b, c) (d, a, b, c) 
#else
#define SHOW_ARGS(d, a, b) (d, b) 
#define STORE_ARGS(d, a, b, c) (d, b, c) 
#endif

ssize_t
show_driver_version SHOW_ARGS(struct device* d, struct device_attribute *attr, char *buf)
{
    return snprintf(buf, PAGE_SIZE, DRIVER_VERSION "\n");
}
ssize_t
show_local_ident SHOW_ARGS(struct device* d, struct device_attribute *attr, char *buf)
{
    struct sis1100_softc *sc=dev2sc(d);
    return snprintf(buf, PAGE_SIZE, "0x%08x\n", sc->local_ident);
}
ssize_t
show_remote_ident SHOW_ARGS(struct device* d, struct device_attribute *attr, char *buf)
{
    struct sis1100_softc *sc=dev2sc(d);
    if (sc->remote_hw==sis1100_hw_invalid)
        return snprintf(buf, PAGE_SIZE, "0\n");
    else
        return snprintf(buf, PAGE_SIZE, "0x%08x\n", sc->remote_ident);
}
ssize_t
show_local_serial SHOW_ARGS(struct device* d, struct device_attribute *attr, char *buf)
{
    struct sis1100_softc *sc=dev2sc(d);
    return snprintf(buf, PAGE_SIZE, "%d\n", sc->serial);
}
ssize_t
show_remote_online SHOW_ARGS(struct device* d, struct device_attribute *attr, char *buf)
{
    struct sis1100_softc *sc=dev2sc(d);
    return snprintf(buf, PAGE_SIZE, "%d\n",
            (sis1100readreg(sc, sr)&sr_synch)==sr_synch);
}
ssize_t
show_remote_size SHOW_ARGS(struct device* d, struct device_attribute *attr, char *buf)
{
    struct sis1100_softc *sc=dev2sc(d);
    return snprintf(buf, PAGE_SIZE, "0x%llx\n",
        (unsigned long long)pci_resource_len(sc->pdev, 3));
}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34)
ssize_t
show_version(struct class* c, struct class_attribute *attr, char *buf)
{
    return snprintf(buf, PAGE_SIZE, DRIVER_VERSION "\n");
}
#else
ssize_t
show_version(struct class* c, char *buf)
{
    return snprintf(buf, PAGE_SIZE, DRIVER_VERSION "\n");
}
#endif

static DEVICE_ATTR(driver_version, 0444, show_driver_version, 0);
static DEVICE_ATTR(local_ident, 0444, show_local_ident, 0);
static DEVICE_ATTR(remote_ident, 0444, show_remote_ident, 0);
static DEVICE_ATTR(local_serial, 0444, show_local_serial, 0);
static DEVICE_ATTR(remote_online, 0444, show_remote_online, 0);
static DEVICE_ATTR(remote_size, 0444, show_remote_size, 0);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)
static CLASS_ATTR(version, 0444, show_version, NULL);
#endif

const char *subnames[4]={"remote", "ram", "ctrl", "dsp"};

/*
 * sis1100_linux_init is called whenever a sis1100 PCI card is found
 */
static int __devinit
init_sis1100_device(struct pci_dev *pdev, const struct pci_device_id *ent)
{
        struct sis1100_softc *sc;
        const char *cardname;
	int unit, res, i;

        switch (pdev->device) {
        case 0x1: cardname="SIS1100 PCI"; break;
        case 0xe: cardname="SIS1100 PCIe+Mezzanine"; break;
        case 0x11: cardname="SIS1100 PCIe single link"; break;
        default: cardname="unknown SIS1100";
        };
        pINFO(0, "sis1100: found %s at %s", cardname, pci_name(pdev));

        for (unit = 0; unit < sis1100_MAXCARDS; unit++) {
                if (!sis1100_devdata[unit]) break;
        }
        if (unit == sis1100_MAXCARDS) {
            pERROR(0, "%s: driver compiled for only %d cards",
                    DEV_DRIVER_STRING(&pdev->dev), sis1100_MAXCARDS);
            res=-EIO;
            goto error;
        }

        sc = kzalloc(sizeof(struct sis1100_softc), GFP_KERNEL);
        if (!sc) {
            res=-ENOMEM;
            goto error;
        }
        sis1100_devdata[unit] = sc;
        sc->unit = unit;
	sc->pdev = pdev;

        for (i=0; i<=sis1100_MINORUTMASK; i++)
            sc->fdata[i]=0;

        sc->testflags=0;
        init_waitqueue_head(&sc->local_wait);
        init_waitqueue_head(&sc->remoteirq_wait);
        init_timer(&sc->link_up_timer);
        sc->link_up_timer.function=sis1100_link_up_handler;
        sc->link_up_timer.data=(unsigned long)sc;

        mutex_init(&sc->sem_hw);
        mutex_init(&sc->sem_fdata);
        mutex_init(&sc->sem_irqinfo);
        mutex_init(&sc->demand_dma.mux);
        spin_lock_init(&sc->lock_intcsr);
        spin_lock_init(&sc->handlercommand.lock);
        spin_lock_init(&sc->lock_doorbell);
        spin_lock_init(&sc->lock_lemo_status);
        spin_lock_init(&sc->demand_dma.spin);
        sc->handlercommand.command=0;
        sc->transparent.active=0;

	cdev_init(&sc->cdev, &sis1100_fops);
	sc->cdev.owner = THIS_MODULE;

        sc->demand_dma.status=dma_invalid;
        sc->plxirq_dma0_hook=0;

        sc->handler=kthread_create(sis1100_irq_thread, sc, "sis1100_%d",
                sc->unit);
        if (IS_ERR(sc->handler)) {
            res=PTR_ERR(sc->handler);
            pERROR(sc, "kthread_create: %d", res);
            goto error_alloc_sc;
        }
        wake_up_process(sc->handler);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,11)
        sc->sched_param.sched_priority=20;
        res=sched_setscheduler(sc->handler, SCHED_FIFO, &sc->sched_param);
        if (res!=0) {
            pINFO(sc, "sched_setscheduler: %d", res);
        }
#endif

#if 1
	res=pci_enable_device(pdev);
#else
        res=pci_enable_device_bars(pdev, 0x5);
#endif
	if (res)
            goto error_thread_create;

	res = pci_request_regions(pdev, "sis1100");
	if (res)
		goto error_enable_dev;

        sc->plx_addr=0;
        sc->reg_addr=0;

        res=-ENOMEM;
        sc->plx_addr = pci_iomap(pdev, 0, 0);
        if (!sc->plx_addr) {
                pERROR(sc, "can't map plx space");
                goto error_pci_iomap;
        }
        pDEBUG(sc, "plx mapped at %p", sc->plx_addr);

        sc->reg_addr = pci_iomap(pdev, 2, 0);
        if (!sc->reg_addr) {
                pERROR(sc, "can't map register space");
                goto error_pci_iomap;
        }
        pDEBUG(sc, "regs mapped at %p", sc->reg_addr);

	res = request_irq(pdev->irq, sis1100_intr, IRQF_SHARED, "sis1100", sc);
	if (res) {
            pERROR(sc, "error installing irq");
            goto error_pci_iomap;
        }

        /* determine the PLX chip used (9054 or 9056(==8311)) */
        sc->silicon_id=_plxreadreg32(sc, 0x70);
        pWARNING(sc, "hardwired device id: 0x%04x", (sc->silicon_id>>16)&0xffff);

        /* select a usable DMA mask */
#if 1
        if ((sc->silicon_id==0x9054) && !pci_set_dma_mask(pdev, DMA_BIT_MASK(64))) {
                sc->using_dac = 1;
                sc->consistent_using_dac = 1;
                pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(64));
        } else
#endif
        if (!pci_set_dma_mask(pdev, DMA_BIT_MASK(32))) {
                pWARNING(sc, "64 Bit DMA not available");
                sc->using_dac = 0;
                sc->consistent_using_dac = 0;
                pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(32));
        } else {
                pWARNING(sc, "No suitable DMA available");
                goto error_request_irq;
        }

        /* allocate memory for DMA (block read/write) */
        sc->dmaspace.size=DMA_MAX;
        sc->dmaspace.cpu_addr=pci_alloc_consistent(sc->pdev,
                sc->dmaspace.size, &sc->dmaspace.dma_handle);
        if (!sc->dmaspace.cpu_addr) {
                pERROR(sc, "pci_alloc_consistent for r/w failed");
                res=-ENOMEM;
                goto error_request_irq;
        }
#if 0
        pINFO(sc, "dma_handle for block r/w: 0x%016llx",
                (u64)sc->dmaspace.dma_handle);
#endif

        /*
         * for dma read/write we use the DMA_64BIT_MASK if possible
         * but for further coherent DMA allocations we only want
         * DMA_32BIT_MASK (PLX can only use 32 bit for descriptor blocks)
         */
        pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(32));

    	res=-sis1100_init(sc);
	if (res) {
	    goto error_alloc_dmaspace;
	}
	pci_set_master(pdev);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
        {
            const int types=1<<sis1100_MINORTYPEBITS; /* ==4 */
            const int maxusers=1<<sis1100_MINORUSERBITS;
            const int devices=types*maxusers;
            const int minor=sc->unit*devices;
            int u, t, idx;

            res=cdev_add(&sc->cdev, MKDEV(MAJOR(sis1100_global.dev), minor),
                    devices);
	    if (res) {
                pERROR(sc, "cdev_add failed: %d", res);
                goto error_init;
            }

            for (idx=0; idx<(maxusers*types); idx++)
                sc->device[idx]=0;

            idx=0;
            for (t=0; t<types; t++) {
                for (u=0; u<maxusers; u++) {
                    sc->device[idx]=device_create(sis1100_global.class,
                            &pdev->dev,
                            MKDEV(MAJOR(sis1100_global.dev), minor+idx),
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
                            sc,
#endif
                            "sis1100_%x%x%s", unit, u, subnames[t]);
                            if (IS_ERR(sc->device[idx])) {
                                res=PTR_ERR(sc->device[idx]);
                                pERROR(sc, "create device: %d", res);
                                goto error_cdev_add;
                            }
                    idx++;
                }
            }
        }
#endif

        /* create files for sysfs */
        res|=device_create_file(&pdev->dev, &dev_attr_driver_version);
        res|=device_create_file(&pdev->dev, &dev_attr_local_ident);
        res|=device_create_file(&pdev->dev, &dev_attr_remote_ident);
        res|=device_create_file(&pdev->dev, &dev_attr_local_serial);
        res|=device_create_file(&pdev->dev, &dev_attr_remote_online);
        res|=device_create_file(&pdev->dev, &dev_attr_remote_size);
        /* res is ignored */

	pci_set_drvdata(pdev, sc);
    	return 0;


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
error_cdev_add:
        cdev_del(&sc->cdev);

error_init:
#endif
        sis1100_done(sc);

error_alloc_dmaspace:
        pci_free_consistent(sc->pdev, sc->dmaspace.size,
                sc->dmaspace.cpu_addr, sc->dmaspace.dma_handle);

error_request_irq:
        free_irq(pdev->irq, sc);

error_pci_iomap:
        if (sc->plx_addr)
            pci_iounmap(pdev, sc->plx_addr);
        if (sc->reg_addr)
            pci_iounmap(pdev, sc->reg_addr);
	pci_release_regions(pdev);

error_enable_dev:
        pci_disable_device(pdev);

error_thread_create:
        kthread_stop(sc->handler);

error_alloc_sc:
        sis1100_devdata[sc->unit] = 0;
        kfree(sc);

error:
    	return res;
}

/*
 * sis1100_linux_done is called whenever a sis1100 PCI card disappears
 * or when the module is unloaded
 * it is unclear what happens if the device is still in use (DMA active)
 * during unload
 * (unload is forced e.g. by shutdown)
 */
static void __devexit
done_sis1100_device(struct pci_dev *pdev)
{
	struct sis1100_softc *sc;

	sc = pci_get_drvdata(pdev);

        /* delete files from sysfs */
        device_remove_file(&pdev->dev, &dev_attr_remote_size);
        device_remove_file(&pdev->dev, &dev_attr_remote_online);
        device_remove_file(&pdev->dev, &dev_attr_local_serial);
        device_remove_file(&pdev->dev, &dev_attr_remote_ident);
        device_remove_file(&pdev->dev, &dev_attr_local_ident);
        device_remove_file(&pdev->dev, &dev_attr_driver_version);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
        {
            const int types=1<<sis1100_MINORTYPEBITS; /* ==4 */
            const int maxusers=1<<sis1100_MINORUSERBITS;
            const int devices=types*maxusers;
            const int minor=sc->unit*devices;
            int idx;

            idx=0;
            for (idx=0; idx<maxusers*types; idx++) {
                device_destroy(sis1100_global.class,
                            MKDEV(MAJOR(sis1100_global.dev), minor+idx));
            }
        }
#endif

        cdev_del(&sc->cdev);
        sis1100_done(sc);
        pci_free_consistent(sc->pdev, sc->dmaspace.size,
                sc->dmaspace.cpu_addr, sc->dmaspace.dma_handle);
        free_irq(pdev->irq, sc);
	del_timer_sync(&sc->link_up_timer);
        if (sc->plx_addr)
            pci_iounmap(pdev, sc->plx_addr);
        if (sc->reg_addr)
            pci_iounmap(pdev, sc->reg_addr);
	pci_release_regions(pdev);
        pci_disable_device(pdev);
        kthread_stop(sc->handler);
        sis1100_devdata[sc->unit] = 0;
        kfree(sc);
	pci_set_drvdata(pdev, 0);
}

static struct pci_driver sis1100_driver = {
	.name     = "sis1100",
	.id_table = sis1100_table,
	.probe    = init_sis1100_device,
	.remove   = done_sis1100_device,
#if 0
        .shutdown = shutdown_sis1100,
#endif
};

/*
 * init_pcidrv_module is called when the module is loaded
 */
static int __init
init_sis1100_module(void)
{
    	int res;

        printk(KERN_INFO "SIS1100 driver V" DRIVER_VERSION
            " (c) " DATE_STRING " " NAME_STRING "\n");

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)
        sis1100_global.class=class_create(THIS_MODULE, "sis1100");
        if (IS_ERR(sis1100_global.class)) {
                res=PTR_ERR(sis1100_global.class);
                printk(KERN_ERR "sis1100: can't register class: %d\n", res);
                goto err;
        }
	res=class_create_file(sis1100_global.class, &class_attr_version);
	if (res) {
		printk(KERN_ERR "sis1100: can't create sysfs version file: %d\n", res);
		goto err_class;
	}
#endif

        res=alloc_chrdev_region(&sis1100_global.dev, 0, 256, "sis1100");
        if (res) {
                printk(KERN_WARNING "sis1100: cannot alloc chrdev region: %d\n",
                        res);
                goto err_attr;
        }

	res = pci_register_driver(&sis1100_driver);
	if (res) {
                printk(KERN_ERR "sis1100: can't register pci driver: %d\n", res);
	        goto err_alloc_chrdev;
        }

        return 0;

err_alloc_chrdev:
	unregister_chrdev_region(sis1100_global.dev, 256);
err_attr:
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)
	class_remove_file(sis1100_global.class, &class_attr_version);
err_class:
	class_destroy(sis1100_global.class);
err:
#endif
	return res;
}

/*
 * cleanup_pcidrv_module is called when the module is unloaded
 */
static void __exit
cleanup_sis1100_module(void)
{
    	printk(KERN_INFO "sis1100 driver exit\n");
	pci_unregister_driver(&sis1100_driver);
        unregister_chrdev_region(sis1100_global.dev, 256);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)
	class_remove_file(sis1100_global.class, &class_attr_version);
        class_destroy(sis1100_global.class);
#endif
}

module_init(init_sis1100_module);
module_exit(cleanup_sis1100_module);
