# Makefile.am for roody
# 2011-06-27 created by Exaos Lee <Exaos.Lee(at)gmail.com>
################################################################
## MIDAS config
MDAQ_CFG := $(shell which mdaq-config)
ifndef MDAQ_CFG
$(error "mdaq-config" not found!)
endif
MDAQ_CFLAGS = $(shell $(MDAQ_CFG) --cflags)
MDAQ_LIBS   = $(shell $(MDAQ_CFG) --libs)

## ROOT config
ROOT_CFG := $(shell which root-config)
ifndef ROOT_CFG
$(error "root-config" not found!)
endif
ROOTSYS = $(shell $(ROOT_CFG) --prefix)
ARCH := $(shell $(ROOT_CFG) --arch)
# ROOT flags
CINT = $(shell $(ROOT_CFG) --bindir)/rootcint
ROOT_LIBDIR = $(shell $(ROOT_CFG) --libdir)
ROOT_CFLAGS = $(shell $(ROOT_CFG) --cflags)
ROOT_GLIBS  = $(shell $(ROOT_CFG) --glibs) -lXMLParser -lThread

HAVE_HBOOK    := $(shell cat $(ROOT_LIBDIR)/libHbook.so 2> /dev/null)
HAVE_MINICERN := $(shell cat $(ROOT_LIBDIR)/libminicern.so 2> /dev/null)
HAVE_SPECTRUM := $(shell cat $(ROOT_LIBDIR)/libSpectrum.so 2> /dev/null)

ifdef HAVE_HBOOK
ROOT_CFLAGS += -DHAVE_HBOOK
ROOT_GLIBS  += -lHbook
ifdef HAVE_MINICERN
ROOT_GLIBS  += -lminicern
endif
endif

ifdef HAVE_SPECTRUM
ROOT_GLIBS  += -lSpectrum
endif

## Arch flags
ifeq ($(ARCH), linux)
ARCH_FLAGS = -m32 -O2 -g
HAVE_DEPEND = 1
HAVE_ROODYSO = 1
endif

ifeq ($(ARCH), linuxx8664gcc)
ARCH_FLAGS = -O2 -g
HAVE_DEPEND = 1
HAVE_ROODYSO = 1
endif

#-----------------------------------------------------------------------
CXXFLAGS = $(ARCH_FLAGS) -Wall -Wuninitialized -fPIC $(MDAQ_CFLAGS) $(ROOT_CFLAGS)\
 -DHAVE_LIBNETDIRECTORY
INCLUDE := -I. -Iroody/include -Iroody/src -Ibuild -Irootana/libNetDirectory
CXXFLAGS += $(INCLUDE)
LDFLAGS += -Lbuild -lNetDirectory $(ROOT_GLIBS) $(MDAQ_LIBS)

################################################################

ROODY_OBJS = build/obj/DataSourceTDirectory.o build/obj/Roody.o \
       build/obj/RoodyDict.o \
       build/obj/TGTextDialog.o build/obj/TGTextDialogDict.o \
       build/obj/TPeakFindPanel.o build/obj/TPeakFindPanelDict.o \
       build/obj/MTGListTree.o build/obj/MTGListTreeDict.o \
       build/obj/TAxisLimitsDialog.o build/obj/TAxisLimitsDialogDict.o \
       build/obj/RoodyXML.o

libNetDirectory_OBJS = build/objnet/TNetDirectory.o \
	build/objnet/TNetDirectoryDict.o \
	build/objnet/netDirectoryServer.o \
	build/objnet/RootLock.o

ROOTANA_OBJS = build/objana/TMidasFile.o  build/objana/TMidasEvent.o \
	build/objana/XmlOdb.o  build/objana/HttpOdb.o \
	build/objana/midasServer.o \
	build/objana/TMidasOnline.o \
	build/objnet/RootLock.o \
	build/objnet/netDirectoryServer.o

ROOTANA_EXAMPLES = build/event_dump.exe  build/analyzer.exe \
	build/test_midasServer.exe build/testODB.exe

#-----------------------------------------------------------------------
ALL = pre_dir build/libNetDirectory.a \
      build/roody_el build/roody build/roody.so \
      build/librootana.a  $(ROOTANA_EXAMPLES)

.SUFFIXES: .cxx .o .so .a

all: $(ALL)

pre_dir:
	@mkdir -p build/obj build/objnet build/objana

build/roody_el: build/obj/main_el.o $(ROODY_OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

build/roody: build/obj/main.o $(ROODY_OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

build/roody.so: $(ROODY_OBJS) build/libNetDirectory.a
	$(CXX) $^ -o $@ -shared $(LDFLAGS)

build/librootana.a: $(ROOTANA_OBJS)
	-rm -f $@
	ar -rv $@ $^

build/libNetDirectory.a: $(libNetDirectory_OBJS)
	-rm -f $@
	ar -rv $@ $^

build/%.exe: rootana/%.cxx build/librootana.a
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

depend:
ifdef HAVE_DEPEND
	-g++ $(CXXFLAGS) $(INCLUDE) -c -MM roody/src/*.cxx > Makefile.depends1
	sed 's#^#build/obj/#' Makefile.depends1 > Makefile.depends2
	sed 's#^build/obj/ #   #' Makefile.depends2 > Makefile.depends
	-rm -f Makefile.depends1 Makefile.depends2
endif

build/obj/main_el.o: addons/main_el.cxx build/rootsys.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

build/obj/main.o: roody/src/main.cxx build/rootsys.h

build/rootsys.h:
	@echo "char const *rootsys = \"$(ROOTSYS)\";" > $@

clean-dicts:
	rm -v -f roody/src/*Dict.* rootana/libNetDirectory/*Dict.*

clean: clean-dicts
	rm -v -rf build/

build/obj/%Dict.o: roody/include/%Dict.h

roody/include/%Dict.h: roody/src/%Dict.cxx

roody/src/%Dict.cxx: roody/include/%.h
	LD_LIBRARY_PATH=$(ROOT_LIBDIR) $(CINT) -f $@ -c $^

build/obj/%.o: roody/src/%.cxx
	$(CXX) $(CXXFLAGS) -c -o $@ $<

build/objnet/%Dict.o: build/objnet/%Dict.cxx
	$(CXX) $(CXXFLAGS) -c -o $@ $<

build/objnet/%Dict.cxx: rootana/libNetDirectory/%.h
	LD_LIBRARY_PATH=$(ROOT_LIBDIR) $(CINT) -f $@ -c $^

build/objnet/%.o: rootana/libNetDirectory/%.cxx
	$(CXX) $(CXXFLAGS) -c -o $@ $<

build/objana/%.o: rootana/%.cxx
	$(CXX) $(CXXFLAGS) -c -o $@ $<

