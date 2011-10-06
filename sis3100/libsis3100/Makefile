#-----------------------------------------------------------
MVME_SRC    = $(shell mdaq-config --drvdir)/vme/sis3100/sis3100.c
MVME_LIB    = libmvme_sis3100.a
MDAQ_CFLAGS = $(shell mdaq-config --cflags)

#-----------------------------------------------------------
all: sis3100 $(MVME_LIB)

clean:
	$(MAKE) -C sis3100_calls clean
	-rm *.o *~ \#* $(MVME_LIB)

#-----------------------------------------------------------
sis3100.o: $(MVME_SRC)
	$(CC) -c -Isis3100_calls $(MDAQ_CFLAGS) $<

$(MVME_LIB): sis3100.o
	ar cr $@ $^

sis3100:
	$(MAKE) -C sis3100_calls
#-----------------------------------------------------------

install-sis3100: sis3100
	install -m 644 sis3100_calls/sis3100_vme_calls.h $(DESTDIR)/usr/lib/mdaq/include/
	install -m 644 sis3100_calls/lib_sis3100.a $(DESTDIR)/usr/lib/mdaq/lib/

install-mvme: $(MVME_LIB)
	install -m 644 $^ $(DESTDIR)/usr/lib/mdaq/lib/

