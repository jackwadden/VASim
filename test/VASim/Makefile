CC=g++
FLAGS=-O3 --std=c++11 -pthread -g
VASIM=../../../VASim
MNRL=$(VASIM)/libs/MNRL/C++
LIBMNRL=$(MNRL)/libmnrl.a
PUGI=$(VASIM)/libs/pugixml/src
LIBVASIM=$(VASIM)/libvasim.a
IDIRS=-I$(VASIM)/include -I$(MNRL)/include -I$(PUGI) -I.

SRCS = $(wildcard *.cpp)

PROGS = $(patsubst %.cpp, %,$(SRCS))

all: $(PROGS)

%: %.cpp
	$(CC) $(IDIRS) $(FLAGS) $< $(LIBVASIM) $(LIBMNRL)  -o $@Runner
