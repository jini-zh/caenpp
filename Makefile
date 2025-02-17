prefix      := /usr/local
exec_prefix := $(prefix)
bindir	    := $(exec_prefix)/bin
libdir 	    := $(exec_prefix)/lib
includedir  := $(prefix)/include

-include config.mak

libdir     := $(DESTDIR)$(libdir)
includedir := $(DESTDIR)$(includedir)

CXXFLAGS ?= -O2 -pipe -march=native

version = 0.0.0

objects = caen vme $(digitizer) v792 v812 v1290 v1495 v6534

.PHONY: all distclean clean install uninstall

all: libcaen++.so caen-rw

libcaen++.so: $(objects:=.o)
	$(CXX) -o $@ $^ $(LDFLAGS) -shared

caen-rw: caen-rw.o libcaen++.so
	$(CXX) -o $@ $< -L . -lcaen++ -lCAENComm $(and $(digitizer),-lCAENDigitizer)

%.o: %.cpp %.hpp caen.hpp
	$(CXX) -c $< -std=c++11 $(CXXFLAGS) -fPIC

install: libcaen++.so caen-rw
	install -d $(libdir)
	install -m 755 libcaen++.so $(libdir)/libcaen++.so.$(version)
	ln -sf libcaen++.so.$(version) $(libdir)/libcaen++.so
	-ldconfig $(libdir)
	install -d $(includedir)/caen++
	install -m 644 $(objects:=.hpp) $(includedir)/caen++
	install -d $(bindir)
	install -m 755 caen-rw $(bindir)/caen-rw

uninstall:
	-rm -v $(libdir)/libcaen++.so $(libdir)/libcaen++.so.$(version)
	-rmdir -v --ignore-fail-on-non-empty $(libdir)/
	-rm -v $(addprefix $(includedir)/caen++/,$(objects:=.hpp))
	-rmdir -v --ignore-fail-on-non-empty $(includedir)/caen++
	-rmdir -v --ignore-fail-on-non-empty $(includedir)
	-rm -v $(bindir)/caen-rw
	-rmdir -v --ignore-fail-on-non-empty $(bindir)/

clean:
	rm -f $(objects:=.o) libcaen++.so
	rm -f caen-rw.o caen-rw

distclean: clean
	rm -f config.mak
