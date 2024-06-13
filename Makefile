prefix      := /usr/local
exec_prefix := $(prefix)
libdir 	    := $(exec_prefix)/lib
includedir  := $(prefix)/include

-include config.mak

libdir     := $(DESTDIR)$(libdir)
includedir := $(DESTDIR)$(includedir)

.PHONY: install uninstall clean

objects = caen digitizer v1290 v6534

libcaen++.so: $(objects:=.o)
	$(CXX) -o $@ $^ $(LDFLAGS) -shared

%.o: %.cpp %.hpp caen.hpp
	$(CXX) -c $< -std=c++11 $(CXXFLAGS) -fPIC

install:
	install -d $(libdir)
	install libcaen++.so $(libdir)/
	install -d $(includedir)/caen++
	install caen.hpp digitizer.hpp $(includedir)/caen++

uninstall:
	-rm -v $(libdir)/libcaen++.so
	-rmdir -v --ignore-fail-on-non-empty $(libdir)/
	-rm -v $(includedir)/caen++/{caen,digitizer}.hpp
	-rmdir -vp --ignore-fail-on-non-empty $(includedir)/caen++

clean:
	rm -f config.mak $(objects:=.o) libcaen++.so
