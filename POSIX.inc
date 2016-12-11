# GNU compatibility core
srcdir = /CPP_App/Franci
prefix = /CPP_App/Franci
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin
includedir = $(exec_prefix)/include
libdir = $(exec_prefix)/lib

AR = ar
CC = gcc
CXX = g++

EXEEXT = exe
OBJEXT = o

CFLAGS = -finline -O3 -Wall -Wno-long-long --pedantic -fgcse-sm -fgcse-las -fsched-spec-load -fsee -fipa-pta -ftree-loop-im -fivopts -ftree-vectorize -falign-functions=1 -falign-jumps=1  -falign-loops=1 -falign-labels=1 -Wdisabled-optimization -Winline --param max-inline-insns-single=500
CXXFLAGS =
ARCH_FLAGS = -mwindows -march=pentium -mtune=pentium3
ARCH2_FLAGS = -mconsole -march=pentium -mtune=pentium3
LINK_FLAGS = -Llib/host.franci

OTHER_INCLUDEDIR =
C_MACROS = -DNDEBUG
CXX_MACROS = -D__STDC_LIMIT_MACROS
