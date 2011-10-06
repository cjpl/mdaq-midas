#/usr/bin/make
SRC = $(DESTDIR)/usr/src
SHARE = $(DESTDIR)/usr/share/sis1100-dkms

all:

clean:

install:

#source tree
ifeq ("$(wildcard sis1100-$(VERSION))", "sis1100-$(VERSION)")
	install -d "$(SRC)"
	cp -a sis1100-$(VERSION) $(SRC)
	chmod 644 -R "$(SRC)/sis1100-$(VERSION)"
endif

#tarball, possibly with binaries
ifeq ("$(wildcard sis1100-$(VERSION).dkms.tar.gz)", "sis1100-$(VERSION).dkms.tar.gz")
	install -d "$(SHARE)"
	install -m 644 sis1100-$(VERSION).dkms.tar.gz "$(SHARE)"
endif

# install udev.rules
	install -m 644 sis1100-2.13/92-struck.rules $(DESTDIR)/etc/udev/rules.d/

#postinst, only if we are supporting legacy mode
ifeq ("$(wildcard common.postinst)", "common.postinst")
	install -d "$(SHARE)"
	install -m 755 $(PREFIX)/usr/lib/dkms/common.postinst $(SHARE)/postinst
endif
