#  Makefile.am for MIDAS automake
#  2011-06-17: created by Exaos Lee (exaos.lee@gmail.com)
########################################################################
AUTOMAKE_OPTIONS = foreign

VPATH = @srcdir@/../midas/include:@srcdir@/../midas/src:@srcdir@/../midas/utils:@srcdir@/../mxml:@mscb_dir@
#ACLOCAL_AMFLAGS = -I m4

ROOTSYS = @root_sys@
MDAQ_CFLAGS = @mdaq_cflags@
MDAQ_LIBS =  @mdaq_libs@

AM_CFLAGS = -I@srcdir@/../midas/include -I@srcdir@/../mxml $(MDAQ_CFLAGS)
AM_CXXFLAGS = $(AM_CFLAGS) @root_cflags@
AM_LDFLAGS = $(MDAQ_LIBS)

## Core Library
MDAQ_LIBSRCS = @srcdir@/../midas/src/midas.c \
@srcdir@/../midas/src/system.c \
@srcdir@/../midas/src/mrpc.c \
@srcdir@/../midas/src/odb.c \
@srcdir@/../midas/src/ftplib.c \
@srcdir@/../midas/src/dm_eb.c \
@srcdir@/../midas/src/history_midas.cxx \
@srcdir@/../midas/src/history_sql.cxx \
@srcdir@/../midas/src/history.c \
@srcdir@/../midas/src/alarm.c \
@srcdir@/../midas/src/elog.c \
@srcdir@/../mxml/mxml.c \
@srcdir@/../mxml/strlcpy.c

lib_LIBRARIES = libmidas.a
libmidas_a_SOURCES = $(MDAQ_LIBSRCS)
libmidas_a_DEPENDENCIES = @mscb_obj@

lib_LTLIBRARIES = libmidas-shared.la
libmidas_shared_la_SOURCES =  $(MDAQ_LIBSRCS)
libmidas_shared_la_CFLAGS = $(AM_CFLAGS)
libmidas_shared_la_CXXFLAGS = $(AM_CXXFLAGS)
libmidas_shared_la_LDFLAGS = -shared -release 2.3.0
include_HEADERS =@srcdir@/../midas/include/cfortran.h \
@srcdir@/../midas/include/esone.h \
@srcdir@/../midas/include/ftplib.h \
@srcdir@/../midas/include/hardware.h \
@srcdir@/../midas/include/hbook.h \
@srcdir@/../midas/include/history.h \
@srcdir@/../midas/include/mcstd.h \
@srcdir@/../midas/include/mdsupport.h \
@srcdir@/../midas/include/mfbstd.h \
@srcdir@/../midas/include/mgd.h \
@srcdir@/../midas/include/midas.h \
@srcdir@/../midas/include/midas.inc \
@srcdir@/../midas/include/midasinc.h \
@srcdir@/../midas/include/midas_macro.h \
@srcdir@/../midas/include/mrpc.h \
@srcdir@/../midas/include/msystem.h \
@srcdir@/../midas/include/musbstd.h \
@srcdir@/../midas/include/mvmestd.h \
@srcdir@/../midas/include/pvm3.h \
@srcdir@/../midas/include/ybos.h \
@srcdir@/../mxml/mxml.h @srcdir@/../mxml/strlcpy.h
LDADD = libmidas.a

## Utilities
CORE_UTILS = mserver mhttpd mlogger odbedit mtape mhist mstat mcnaf \
    mdump lazylogger mtransition mhdump mchart webpaw odbhist melog
EXTRA_UTILS = mfe_link_test mana_link_test @mdaq_osprogs@

## Programs
bin_PROGRAMS = $(CORE_UTILS) $(EXTRA_UTILS)
# Scritps
dist_bin_SCRIPTS = @srcdir@/../midas/utils/stripchart.tcl  \
@srcdir@/../midas/utils/lazy_castor.perl \
@srcdir@/../midas/utils/lazy_copy.perl \
@srcdir@/../midas/utils/lazy_dcache.perl \
@srcdir@/../midas/utils/lazy_test.perl \
mdaq-config

mscb.o: mscb.c
	$(CXX) $(AM_CXXFLAGS) -c -o $@ $<

# per-code configure
mhttpd_SOURCES = @srcdir@/../midas/src/mhttpd.cxx @srcdir@/../midas/src/mgd.c @srcdir@/../midas/src/sequencer.cxx
mhttpd_LDADD =  @mscb_obj@ libmidas.a
mhttpd_CXXFLAGS = $(AM_CXXFLAGS) -I@mscb_dir@
mhttpd_DEPENDENCIES = @mscb_obj@
mlogger_SOURCES = mlogger.cxx
mlogger_LDFLAGS = @root_libs@ ${MDAQ_LIBS}
odbedit_SOURCES = odbedit.c cmdedit.c
mcnaf_SOURCES = mcnaf.c @srcdir@/../midas/drivers/camac/camacrpc.c
mdump_SOURCES = mdump.c mdsupport.c
mhdump_SOURCES = mhdump.cxx
mtransition_SOURCES = mtransition.cxx

lazylogger_SOURCES =  mdsupport.c lazylogger.cxx
lazylogger_LDADD = libmidas.a
lazylogger_LDFLAGS = $(AM_LIBS) @root_glibs@
lazylogger_LINK = $(CXX) $(AM_CXXFLAGS) -o $@

mfe_link_test_SOURCES = @srcdir@/../midas/src/mfe.c
mfe_link_test_CFLAGS = -DLINK_TEST $(AM_CFLAGS)
mana_link_test_SOURCES = @srcdir@/../midas/src/mana.c
mana_link_test_CFLAGS = -DLINK_TEST $(AM_CFLAGS)

## listing all optionally built programs in EXTRA_PROGRAMS
EXTRA_PROGRAMS = mh2sql dio mlxspeaker
mh2sql_SOURCES = mh2sql.cxx
dio_SOURCES = dio.c
mlxspeaker_SOURCES = mlxspeaker.c

### Special objects: frontend backend framework
FE_OBJS = mfe.o fal.o mana.o @anaobj_cern@ @anaobj_root@

fe_backend_objs: $(FE_OBJS)

mfe.o: mfe.c msystem.h midas.h midasinc.h mrpc.h

fal.o: fal.c msystem.h midas.h midasinc.h mrpc.h
	$(CXX) -Dextname -DMANA_LITE -c $(AM_CXXFLAGS) -o $@ $<

mana.o: mana.c msystem.h midas.h midasinc.h mrpc.h

hmana.o: mana.c msystem.h midas.h midasinc.h mrpc.h
	$(CC) -Dextname -DHAVE_HBOOK -c $(AM_CFLAGS) -o $@ $<

rmana.o: mana.c msystem.h midas.h midasinc.h mrpc.h
	$(CXX) -DUSE_ROOT $(AM_CXXFLAGS) -c -o $@ $<

### Documentation
dox-html: $(mdaq_imgs)
	@$(MAKE) -C @srcdir@/../midas/doc html

######################################################################
## Install

install-exec-local: fe_backend_objs mdaq-config
	$(INSTALL) $(FE_OBJS) $(DESTDIR)/@libdir@
	@mkdir -p $(DESTDIR)/@prefix@/bin
	$(INSTALL) mdaq-config $(DESTDIR)/@prefix@/bin/mdaq-config
	$(INSTALL) @srcdir@/../addons/mdaq.py $(DESTDIR)/@prefix@/bin/mdaq

install-data-local:
# src
	@cd @srcdir@/../midas/src && find ./ -type f -exec \
	   $(INSTALL) -v -D -m 644 "{}" "$(DESTDIR)/@datadir@/src/{}" \;
# drivers
	@cd @srcdir@/../midas/drivers && find ./ -type f -exec \
	   $(INSTALL) -v -D -m 644 "{}" "$(DESTDIR)/@datadir@/drivers/{}" \;
# examples
	@cd @srcdir@/../midas/examples && find ./ -type f -exec \
	   $(INSTALL) -v -D -m 644 "{}" "$(DESTDIR)/@docdir@/examples/{}" \;
# install config file
	@mkdir -p $(DESTDIR)/@docdir@/etc
	$(INSTALL) -v -D -m 644 @srcdir@/../addons/exptab $(DESTDIR)/@docdir@/etc/exptab
# Install mdaq.yaml example to docdir
	$(INSTALL) -v -D -m 644 @srcdir@/../addons/mdaq.yaml $(DESTDIR)/@docdir@/mdaq.yaml

install-html-local: dox-html
	@cd @srcdir@/../midas/doc && mv images/* html/ && find ./html/ \
	   -type f -exec $(INSTALL) -vDm644 "{}" "$(DESTDIR)/@htmldir@/{}" \;

install-man-local: dox
	@cd @srcdir@/../midas/doxfiles && find ./man/ -type f -exec \
	   $(INSTALL) -v -D -m 644 "{}" "@mandir@/{}" \;

uninstall-local:
	@echo "Uninstall analysis backend objects"
	@for i in $(fe_backend_objs) ; do rm -f $(DESTDIR)/@libdir@/$i ; done
# Remove mdaq-config
	@echo "Remove mdaq-config"
	@rm -f $(DESTDIR)/@prefix@/bin/mdaq-config
# remove system config file
	@rm -f $(DESTDIR)/etc/exptab
# remove html docs
	@rm -rf $(DESTDIR)/@htmldir@/html
# remove: src, examples, drivers
	@rm -rf $(DESTDIR)/@datadir@/src
	@rm -rf $(DESTDIR)/@docdir@/examples
	@rm -rf $(DESTDIR)/@datadir@/drivers

####################################################################
mdaq-env:
#	@echo "coresrcdir = @coresrcdir@"
#	@echo "examplesdir = @examplesdir@"
#	@echo "drvdir = @drvdir@"
	@echo "ROOTSYS = @root_sys@"
	@echo "ROOTCFG = @root_cfg@"
	@echo "MYSQLCFG = @MYSQLCFG@"
#	@echo "host_cpu = @host_cpu@"
#	@echo "host_os = @host_os@"
#	@echo "host_vendor = @host_vendor@"
#	@echo "target_os = @target_os@"
#	@echo "target_cpu = @target_cpu@"
#	@echo "inc_files = @inc_files@"
#	@echo "drv_files = @drv_files@"
#	@echo "exp_files = @exp_files@"
	@echo "target = @target@"
	@echo "MDAQ_CFLAGS = $(MDAQ_CFLAGS)"
	@echo "MDAQ_LIBS = $(MDAQ_LIBS)"
	@echo "LIBS = $(LIBS)"
	@echo "OSFLAGS = @mdaq_osflags@"
	@echo "OSPROGS = @mdaq_osprogs@"
	@echo "AM_CFLAGS = ${AM_CFLAGS}"
	@echo "AM_CXXFLAGS = ${AM_CXXFLAGS}"
	@echo "htmldir = @htmldir@"
	@echo "mandir = @mandir@"

