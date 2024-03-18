prefix      := /usr/local
exec_prefix := $(prefix)
libdir 	    := $(exec_prefix)/lib
includedir  := $(prefix)/include

-include config.mak

libdir     := $(DESTDIR)$(libdir)
includedir := $(DESTDIR)$(includedir)

.PHONY: install uninstall clean

libcaen++.so: digitizer.o caen.o v6534.o
	$(CXX) -o $@ $^ $(LDFLAGS) -shared

digitizer.o: digitizer.cpp digitizer.hpp caen.hpp
caen.o: caen.cpp caen.hpp
v6534.o: v6534.cpp v6534.hpp caen.hpp

%.o: %.cpp
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
	rm -v config.mak {digitizer,caen,v6534}.o libcaen++.so 2> /dev/null; true
