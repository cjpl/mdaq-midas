
all: build

build:
	$(MAKE) -C sis3100_calls

clean:
	$(MAKE) -C sis3100_calls clean

install: build
	install -m 644 sis3100_calls/sis3100_vme_calls.h $(DESTDIR)/usr/lib/mdaq/include/
	install -m 644 sis3100_calls/lib_sis3100.a $(DESTDIR)/usr/lib/mdaq/lib/

