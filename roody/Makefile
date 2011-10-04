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
CXXFLAGS = $(ARCH_FLAGS) -Wall -Wuninitialized -fPIC $(MDAQ_CFLAGS) $(ROOT_CFLAGS)
INCLUDE := -I. -Iroody/include -Iroody/src -Iaddons -IrootanalibNetDirectory
CXXFLAGS += $(INCLUDE)
LDFLAGS += -Llib -lNetDirectory $(ROOT_GLIBS) $(MDAQ_LIBS)

################################################################

ROODY_OBJS = obj/DataSourceTDirectory.o obj/Roody.o obj/RoodyDict.o \
       obj/TGTextDialog.o obj/TGTextDialogDict.o \
       obj/TPeakFindPanel.o obj/TPeakFindPanelDict.o \
       obj/MTGListTree.o obj/MTGListTreeDict.o \
       obj/TAxisLimitsDialog.o obj/TAxisLimitsDialogDict.o \
       obj/RoodyXML.o

libNetDirectory_OBJS = objnet/TNetDirectory.o \
	objnet/TNetDirectoryDict.o \
	objnet/netDirectoryServer.o \
	objnet/RootLock.o

#-----------------------------------------------------------------------
ALL = pre_dir lib/libNetDirectory.a bin/roody_el bin/roody lib/roody.so

.SUFFIXES: .cxx .o .so .a

all: $(ALL)

pre_dir:
	@mkdir -p bin lib obj objnet

bin/roody_el: obj/main_el.o $(ROODY_OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

bin/roody: obj/main.o $(ROODY_OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

lib/roody.so: $(ROODY_OBJS) lib/libNetDirectory.a
	$(CXX) $^ -o $@ -shared $(LDFLAGS)

lib/libNetDirectory.a: $(libNetDirectory_OBJS)
	-rm -f $@
	ar -rv $@ $^

depend:
ifdef HAVE_DEPEND
	-g++ $(CXXFLAGS) $(INCLUDE) -c -MM roody/src/*.cxx > Makefile.depends1
	sed 's#^#obj/#' Makefile.depends1 > Makefile.depends2
	sed 's#^obj/ #   #' Makefile.depends2 > Makefile.depends
	-rm -f Makefile.depends1 Makefile.depends2
endif

obj/main_el.o: addons/main_el.cxx addons/rootsys.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

obj/main.o: roody/src/main.cxx addons/rootsys.h

addons/rootsys.h:
	@echo "char const *rootsys = \"$(ROOTSYS)\";" > $@

clean:
	rm -v -f bin/roody_el bin/roody obj/* core roody/src/*Dict.* lib/*.so \
		objnet/* rootana/libNetDirectory/%Dict.* lib/*.a

obj/%Dict.o: roody/include/%Dict.h

roody/include/%Dict.h: roody/src/%Dict.cxx

roody/src/%Dict.cxx: roody/include/%.h
	LD_LIBRARY_PATH=$(ROOT_LIBDIR) $(CINT) -f $@ -c $^

obj/%.o: roody/src/%.cxx
	$(CXX) $(CXXFLAGS) -c -o $@ $<

objnet/%Dict.o: objnet/%Dict.cxx
	$(CXX) $(CXXFLAGS) -c -o $@ $<

objnet/%Dict.cxx: rootana/libNetDirectory/%.h
	LD_LIBRARY_PATH=$(ROOT_LIBDIR) $(CINT) -f $@ -c $^

objnet/%.o: rootana/libNetDirectory/%.cxx
	$(CXX) $(CXXFLAGS) -Irootana/libNetDirectory -c -o $@ $<

#obj/%.o: @srcdir@roody/src/%.cpp
#	$(CXX) $(CXXFLAGS) -c -o $@ $<

-include Makefile.depends

