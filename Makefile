# TARGET NAMES
TARGET = vasim
SNAME = libvasim.a

# DIRECTORIES
IDIR = ./include
SRCDIR = ./src
MNRL = ./MNRL/C++
PUGI = ./pugixml-1.6/src

# DEPENDENCIES
LIBMNRL = $(MNRL)/libmnrl.a

# FLAGS
CXXFLAGS= -I$(IDIR) -I$(MNRL)/include -I$(PUGI) -pthread --std=c++11
OPTS = -Ofast -march=native -m64 #-flto
DEBUG = -g
PROFILE = $(DEBUG) -pg
ARFLAGS = rcs

_DEPS = *.h
_OBJ = util.o ste.o ANMLParser.o MNRLAdapter.o automata.o element.o specialElement.o gate.o and.o or.o nor.o counter.o inverter.o

MAIN_CPP = main.cpp

CC = g++-5
AR = ar
#CC=icpc -mmic
#CC=icpc

ODIR=$(SRCDIR)/obj

DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


all: CXXFLAGS += -DDEBUG=false $(OPTS)
all: $(TARGET)

debug: CXXFLAGS += -DDEBUG=true $(DEBUG) 
debug: $(TARGET)

profile: CXXFLAGS += -DDEBUG=false $(OPTS) $(PROFILE) 
profile: $(TARGET)

library: CXXFLAGS += -DDEBUG=false $(OPTS)
library: $(SNAME) 

$(TARGET): $(SRCDIR)/$(MAIN_CPP) $(SNAME) $(MNRL)/libmnrl.a 
	$(CC) $(CXXFLAGS) $^ -o $@  

$(SNAME): $(ODIR)/pugixml.o $(OBJ)
	$(AR) $(ARFLAGS) $@ $^ 

$(ODIR)/%.o: $(SRCDIR)/%.cpp $(DEPS) $(LIBMNRL)
	@mkdir -p $(ODIR)	
	$(CC) $(CXXFLAGS) -c -o $@ $< 

$(ODIR)/pugixml.o: $(PUGI)/pugixml.cpp
	@mkdir -p $(ODIR)	
	$(CC) $(CXXFLAGS) -c -o $@ $< $(CXXFLAGS)

$(LIBMNRL):
	git submodule init
	git submodule update
	$(MAKE) -C ./MNRL/C++/

.PHONY: clean

cleanlight:
	mv $(ODIR)/pugixml.o $(ODIR)/pugixml.o.tmp
	rm -f $(ODIR)/*.o $(TARGET)
	mv $(ODIR)/pugixml.o.tmp $(ODIR)/pugixml.o

clean:
	rm -f $(ODIR)/*.o $(TARGET)
	rm $(SNAME)
	rm $(MNRL)/libmnrl.a $(MNRL)/libmnrl.so
	rmdir $(ODIR)
