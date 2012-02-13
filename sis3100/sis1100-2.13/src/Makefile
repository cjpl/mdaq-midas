# $ZEL: Makefile,v 1.10 2010/01/21 21:47:44 wuestner Exp $
#
# We're just setting the stage here and
# let the Kernel Makefile do the real work.
#

KERNVER := $(shell uname -r)

.PHONY: driver

driver:
	make -C /lib/modules/$(KERNVER)/build SUBDIRS=$(shell pwd) modules

install:
	make -C /lib/modules/$(KERNVER)/build SUBDIRS=$(shell pwd) modules_install

clean:
	-rm -f *.o *.ko .*.o.cmd .*.ko.cmd *.mod.c Module.symvers modules.order
	-rm -f Module.markers
	-rm -r -f .tmp_versions

MODULENAME := sis1100

ifndef src
    src:=$(shell pwd)
endif

SOURCES:=$(notdir $(wildcard $(src)/*.c))
sis1100-objs := $(patsubst %.c, %.o, $(filter-out %netbsd.c, $(SOURCES)))

ifneq (,$(findstring 2.6.,$(KERNVER)))

EXTRA_CFLAGS += -DMODULENAME=$(MODULENAME)

obj-m += sis1100.o
#obj-$(CONFIG_SIS1100) += sis1100.o

else

ifneq (,$(findstring 3., $(KERNVER)))
EXTRA_CFLAGS += -DMODULENAME=$(MODULENAME)

obj-m += sis1100.o

else

modules: sis1100.o

sis1100.o: $(sis1100-objs)
	$(LD) -r -o $@ $^

endif

endif
