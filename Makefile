# POSIX makefile

.SUFFIXES:
.SUFFIXES: .cxx .o .exe

include POSIX.inc

# GNU targets we know about
all: Franci.exe Franci_console.exe

clean:
	rm -f *.o *.exe lib/host.zcc/*.a
	cd Zaimoni.STL; make clean

# dependencies
include POSIX.dep

make_Zaimoni_STL:
	cd Zaimoni.STL; make host_install

Franci.exe : make_Zaimoni_STL $(OBJECTS_FRANCI_LINK_PRIORITY)
	g++ -oFranci.exe $(LINK_FLAGS) $(ARCH_FLAGS) $(OBJECTS_FRANCI) -lz_console -lz_csvtable -lz_langconf -lz_logging -lz_log_adapter -lz_format_util -lz_stdio_c -lz_memory
	strip --preserve-dates --strip-unneeded Franci.exe

Franci_console.exe : make_Zaimoni_STL $(OBJECTS_FRANCI_CONSOLE_LINK_PRIORITY)
	g++ -oFranci_console.exe $(LINK_FLAGS) $(ARCH2_FLAGS) $(OBJECTS_FRANCI_CONSOLE) -lz_cstdio_console -lz_csvtable -lz_langconf -lz_logging -lz_log_adapter -lz_format_util -lz_stdio_c -lz_memory
	strip --preserve-dates --strip-unneeded Franci_console.exe

# inference rules
# defines
# processing details
.cxx.o:
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(ARCH_FLAGS) $(OTHER_INCLUDEDIR) $(C_MACROS) $(CXX_MACROS) \
	 -DNDEBUG \
	 -o $*.o -c -xc++ -pipe $<
	strip --preserve-dates --strip-unneeded $*.o

include POSIX2.inc
