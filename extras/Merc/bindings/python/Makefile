include ../../toprules.make

module := mercsim
all: _$(module).so

INCLUDES := -I/usr/include/python2.3/ -I/usr/include/python2.2/

_$(module).so: $(module)_wrap.cxx ../libsim.so ../libcolyseus-wan.so
	g++ -Wno-deprecated -include magic.h -c $(INCLUDES) -I.. $(module)_wrap.cxx
	g++ $(module)_wrap.o -shared -o _$(module).so -L.. -lsim -lcolyseus-wan -lz -lm -lgmp 
	rm -f $(module)_wrap.o 

# this should be needed to be executed only on machines
# having the more recent version of swig. the generated
# files $(module)_wrap.cxx and $(module).py are portable

$(module)_wrap.cxx: $(module).i
ifdef DEVEL
	swig -python -c++ -shadow $(module).i
endif

objs = $(module)_wrap.o _$(module).so $(module).pyc
junk = $(wildcard MercEvents.* *.log)

include ../../botrules.make

clean:
	rm -f $(objs) $(junk)

DIST_FILES = *.i test.* *.cpp
