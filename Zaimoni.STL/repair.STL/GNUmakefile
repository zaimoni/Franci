# Makefile for repair.STL directory
# assumes GNU make

.SUFFIXES:
.SUFFIXES: .cpp .a .o

include ../POSIX.inc
include ../POSIX.dep

CFLAGS := -fargument-alias -finline -Os -Wall -Wno-long-long --pedantic
CXXFLAGS := $(CFLAGS)

# GNU targets we know about
all: algorithm
clean:
	rm -f algorithm stl_diagnose.hpp


# rules
stl_diagnose.hpp: stl_diagnose.in compile.test/has_std_copy_n.cpp compile.test/has_std_auto_ptr.cpp compile.test/has_std_unique_ptr.cpp
	cp stl_diagnose.in stl_diagnose.hpp
	if $(CXX) -ohas_std_copy_n.exe $(CXXFLAGS) -mconsole $(ARCH_FLAGS) compile.test/has_std_copy_n.cpp; then echo "#define ZAIMONI_HAS_STD_COPY_N 1" >> stl_diagnose.hpp; fi
	-rm has_std_copy_n.exe
	if $(CXX) -ohas_std_auto_ptr.exe $(CXXFLAGS) -mconsole $(ARCH_FLAGS) compile.test/has_std_auto_ptr.cpp; then echo "#define ZAIMONI_HAS_STD_AUTO_PTR 1" >> stl_diagnose.hpp; fi
	-rm has_std_auto_ptr.exe
	if $(CXX) -ohas_std_unique_ptr.exe $(CXXFLAGS) -mconsole $(ARCH_FLAGS) compile.test/has_std_unique_ptr.cpp; then echo "#define ZAIMONI_HAS_STD_UNIQUE_PTR 1" >> stl_diagnose.hpp; fi
	-rm has_std_unique_ptr.exe

algorithm: stl_diagnose.hpp algorithm.in1 algorithm.in2 algorithm.in3
	cp algorithm.in1 algorithm
	cpp -xc++ algorithm.in2 | grep -vF \# >> algorithm
	cat algorithm.in3 >> algorithm

.cpp.o:
	$(CXX) -c $< $(CXXFLAGS) $(ARCH_FLAGS) -o $*.o


