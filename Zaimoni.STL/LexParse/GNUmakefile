# Makefile for LexParse directory
# assumes GNU make

.SUFFIXES:
.SUFFIXES: .cpp .a .o

include ../POSIX.inc
include ../POSIX.dep

CXXFLAGS := $(CFLAGS_CORE) $(CFLAGS_MICROOPT)

#LIBS := libz_langconf.a libz_langconf_iso.a libz_csvtable.a libz_csvtable_iso.a
LIBS := libz_langconf.a libz_langconf_iso.a libz_csvtable.a

# GNU targets we know about
all: $(LIBS)

clean:
	rm -f *.a *.o

host_install: $(addprefix ../../$(host_libdir)/, $(LIBS))

# rules
LangConf.o: LangConf.cpp $(LangConf_hpp) $(addprefix ../, $(POD_hpp))
LangConf_iso.o: LangConf.cpp $(LangConf_hpp) $(addprefix ../, $(POD_hpp))
CSVTable.o: CSVTable.cpp $(CSVTable_hpp) $(addprefix ../, $(cstdio)) ../fstream
CSVTable_iso.o: CSVTable.cpp $(CSVTable_hpp) $(addprefix ../, $(cstdio)) ../fstream

.cpp.o:
	$(CXX) -c $< $(CXXFLAGS) $(ARCH_FLAGS) -DZAIMONI_HAVE_ACCURATE_MSIZE -o $*.o

LangConf_iso.o:
	$(CXX) -c $< $(CXXFLAGS) $(ARCH_FLAGS) -DZAIMONI_FORCE_ISO -o $*.o

CSVTable_iso.o:
	$(CXX) -c $< $(CXXFLAGS) $(ARCH_FLAGS) -DZAIMONI_FORCE_ISO -o $*.o

libz_langconf.a: LangConf.o
	$(AR) $(ARFLAGS) libz_langconf.a LangConf.o

libz_langconf_iso.a: LangConf_iso.o
	$(AR) $(ARFLAGS) libz_langconf_iso.a LangConf_iso.o

libz_csvtable.a: CSVTable.o
	$(AR) $(ARFLAGS) libz_csvtable.a CSVTable.o

../../$(host_libdir)/libz_langconf.a: libz_langconf.a
	cp libz_langconf.a ../../$(host_libdir)/libz_langconf.a

../../$(host_libdir)/libz_langconf_iso.a: libz_langconf_iso.a
	cp libz_langconf_iso.a ../../$(host_libdir)/libz_langconf_iso.a

../../$(host_libdir)/libz_csvtable.a: libz_csvtable.a
	cp libz_csvtable.a ../../$(host_libdir)/libz_csvtable.a

