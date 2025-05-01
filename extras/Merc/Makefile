include toprules.make

#include default config
include Makefile.conf-default
#if exists, include local config which overrides defaults
ifneq ($(shell ls Makefile.conf 2>/dev/null),)
include Makefile.conf
endif

versrcs = Version.cpp
verhdrs = Mercury.h
verobjs = $(shell ls $(versrcs) | \
		perl -pne 's|(.*)/([^/]*)\.cpp|$$1/.$$2.o|')

coresrcs = $(wildcard $(patsubst %,%/*.cpp,$(coredirs)))
corehdrs = $(wildcard $(patsubst %,%/*.h,$(coredirs)))
coreobjs = $(shell ls $(coresrcs) | \
		perl -pne 's|(.*)/([^/]*)\.cpp|$$1/.$$2.o|') \
		$(verobjs)

simsrcs = $(wildcard $(patsubst %,%/*.cpp,$(simdirs)))
simhdrs = $(wildcard $(patsubst %,%/*.h,$(simdirs)))
simobjs = $(shell ls $(simsrcs) | \
		perl -pne 's|(.*)/([^/]*)\.cpp|$$1/.$$2.o|')

wansrcs = $(wildcard $(patsubst %,%/*.cpp,$(wandirs)))
wanhdrs = $(wildcard $(patsubst %,%/*.h,$(wandirs)))
wanobjs = $(shell ls $(wansrcs) | \
		perl -pne 's|(.*)/([^/]*)\.cpp|$$1/.$$2.o|')

rpcdirs = rpc
rpcsrcs = $(wildcard $(patsubst %,%/*.cpp,$(rpcdirs)))
rpchdrs = $(wildcard $(patsubst %,%/*.h,$(rpcdirs)))
rpcobjs = $(shell ls $(rpcsrcs) | \
		perl -pne 's|(.*)/([^/]*)\.cpp|$$1/.$$2.o|')

ifeq ($(make_onlysim),YES)
subdirs += $(simdirs)
TARGETS += onlysim
endif
ifeq ($(make_sim),YES)
subdirs += $(coredirs) $(simdirs)
TARGETS += sim
endif
ifeq ($(make_wan),YES)
subdirs += $(coredirs) $(wandirs)
TARGETS += wan
endif
ifeq ($(make_rpc),YES)
subdirs += $(rpcdirs)
TARGETS += rpclib
endif
ifeq ($(make_wanarchive),YES)
subdirs += $(coredirs) $(wandirs)
TARGETS += wan-archive
endif
ifeq ($(make_simarchive), YES)
subdirs += $(coredirs) $(simdirs)
TARGETS += sim-archive
endif

SUBDIRS = $(shell echo $(subdirs) | tr '\\ ' '\n' | sort | uniq | tr '\n' ' ')
APPDIRS += $(appdirs)

###############################################################################

all: $(TARGETS) $(APPDIRS)

simtarg     = libmerc-sim$(LIBEXT)
onlysimtarg = libsim$(LIBEXT)
wantarg     = libmerc-wan$(LIBEXT)
rpctarg     = libmerc-rpc$(LIBEXT)
wanarchivetarg = libmerc-wan.a
simarchivetarg = libmerc-sim.a

onlysim: $(onlysimtarg)
sim: $(simtarg)
wan: $(wantarg)
rpclib: $(rpctarg)
wan-archive: $(wanarchivetarg)
sim-archive: $(simarchivetarg)

precompile:
ifeq ($(RELEASE),release)
	@-rm -f $(verobjs)
endif

version: precompile $(verobjs)

###############################################################################

$(simtarg): $(SUBDIRS)
ifneq ($(MAKECMDGOALS),clean)
	@echo "+ Linking $@"
 ifeq  ($(OS),Linux)
	@$(CPP) -shared $(OPTFLAGS) -Wl,-soname,$(simtarg) -o $@ $(coreobjs) $(simobjs)
 else
 ifeq  ($(OS),Darwin)
	@$(CPP) -dynamiclib $(OPTFLAGS) -o $(simtarg) $(coreobjs) $(simobjs) -lz
 else
 ifeq  ($(OS),Win32)
	ERROR # XXX: Dunno what to do on windows
 else
	ERROR
 endif
 endif
 endif
endif

$(onlysimtarg): $(SUBDIRS)
ifneq ($(MAKECMDGOALS),clean)
	@echo "+ Linking $@"
 ifeq  ($(OS),Linux)
	@$(CPP) -shared $(OPTFLAGS) -Wl,-soname,$(onlysimtarg) -o $@ $(simobjs)
 else
 ifeq  ($(OS),Darwin)
	@$(CPP) -dynamiclib $(OPTFLAGS) -o $(onlysimtarg) $(simobjs)
 else
 ifeq  ($(OS),Win32)
	ERROR # XXX: Dunno what to do on windows
 else
	ERROR
 endif
 endif
 endif
endif

$(rpctarg): $(SUBDIRS)
ifneq ($(MAKECMDGOALS),clean)
	@echo "+ Linking $@"
 ifeq  ($(OS),Linux)
	@$(CPP) -shared $(OPTFLAGS) -Wl,-soname,$(rpctarg) -o $@ $(rpcobjs)
 else
 ifeq  ($(OS),Darwin)
	@$(CPP) -dynamiclib $(OPTFLAGS) -o $(rpctarg) $(rpcobjs) -lz
 else
 ifeq  ($(OS),Win32)
	ERROR # XXX: Dunno what to do on windows
 else
	ERROR
 endif
 endif
 endif
endif

$(wantarg): $(version) $(SUBDIRS)
ifneq ($(MAKECMDGOALS),clean)
	@echo "+ Linking $@"
 ifeq  ($(OS),Linux)
	@$(CPP) -shared $(OPTFLAGS) -Wl,-soname,$(wantarg) -o $@ $(coreobjs) $(wanobjs)
 else
 ifeq  ($(OS),Darwin)
	@$(CPP) -dynamiclib $(OPTFLAGS) -o $(wantarg) $(coreobjs) $(wanobjs)
 else
 ifeq  ($(OS),Win32)
	ERROR # XXX: Dunno what to do on windows
 else
	ERROR
 endif
 endif
 endif
endif

$(wanarchivetarg): $(version) $(SUBDIRS)
ifneq ($(MAKECMDGOALS),clean)
	@echo "+ Archiving $@"
	@ar rc $@ $(coreobjs) $(wanobjs)
	@ranlib $@
endif

$(simarchivetarg): $(version) $(SUBDIRS)
ifneq ($(MAKECMDGOALS),clean)
	@echo "+ Archiving $@"
	@ar rc $@ $(coreobjs) $(simobjs) $(wanobjs)
	@ranlib $@
endif

$(APPDIRS): $(TARGETS)

###############################################################################

clean: $(SUBDIRS) $(APPDIRS) 

doc: doxygen/doxygen.conf doxygen/doxygen.css $(coresrcs)
	doxygen doxygen/doxygen.conf

DIST_FILES = mercury wan-env sim-env \
      util configs/* tests apps doxygen/* \
      rpc bindings/python *.cpp *.h Mercury.* Makefile* distmake.sh *.make tools/*.sh \
      tools/*.pl tools/*.el AUTHORS BUGS COPYING README

include botrules.make
