# makefile taken from glaurung
GPP     = g++
CFLAGS  = -g -O3 -funroll-loops -fomit-frame-pointer -fstrict-aliasing -Wall
DEFINES = #-DTRACEPERFT
CC      = $(GPP) $(CFLAGS) $(DEFINES)

OBJS = \
	.o/attacks.o \
	.o/bitboard.o \
	.o/data.o \
	.o/eval.o \
	.o/hash.o \
	.o/history.o \
	.o/mersenne.o \
	.o/make.o \
	.o/movegen.o \
	.o/position.o \
	.o/search.o \
	.o/ui.o \
	.o/util.o \
	.o/zobrist.o

all: benthos perft epdtest

benthos: .o $(OBJS) .o/main.o
	$(CC) $(OBJS) .o/main.o -o benthos

perft: .o $(OBJS) .o/perft.o
	$(CC) $(OBJS) .o/perft.o -o perft

epdtest: .o $(OBJS) .o/epdtest.o
	$(CC) $(OBJS) .o/epdtest.o -o epdtest

.o/%.o: Makefile %.cpp
	$(CC) -c $*.cpp -o .o/$*.o

.o:
	mkdir .o

clean:
	rm -rf .o benthos.exe perft.exe epdtest.exe benthos perft epdtest
