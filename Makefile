# POSIX makefile

.SUFFIXES:
.SUFFIXES: .cxx .o .exe

include POSIX.inc

# GNU targets we know about
all: Franci.exe

clean:
	rm -f *.o *.exe

# dependencies
include POSIX.dep

Franci.exe : $(OBJECTS_FRANCI_LINK_PRIORITY)
	g++ $(LINK_FLAGS) -oFranci.exe $(OBJECTS_FRANCI) -lz_console -lz_csvtable -lz_langconf -lz_logging -lz_log_adapter -lz_format_util -lz_stdio_c -lz_memory
	strip --preserve-dates --strip-unneeded Franci.exe

# inference rules
# global project search paths for headers
# defines
# processing details
.cxx.o:
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(ARCH_FLAGS) $(OTHER_INCLUDEDIR) $(C_MACROS) $(CXX_MACROS) \
	 -DNDEBUG \
	 -o $*.s -S -xc++ -pipe $<
	python.exe pre_as.py $*.s
	as -o $*.o $*.s
	strip --preserve-dates --strip-unneeded $*.o
