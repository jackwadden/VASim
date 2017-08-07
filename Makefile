# COMPILERS
CC = g++-5
AR = ar
#CC=icpc -mmic
#CC=icpc

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
_OBJ = errors.o util.o ste.o ANMLParser.o MNRLAdapter.o automata.o element.o specialElement.o gate.o and.o or.o nor.o counter.o inverter.o 

MAIN_CPP = main.cpp

ODIR=$(SRCDIR)/obj

DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

CXXFLAGS += $(OPTS)

all: vasim_release

vasim_release: mnrl_release
	$(info  )
	$(info Compiling VASim Library...)
	$(MAKE) $(TARGET)

mnrl_release:
	$(info  )
	$(info Compiling MNRL Library...)
	$(MAKE) $(LIBMNRL)

$(TARGET): $(SRCDIR)/$(MAIN_CPP) $(SNAME) $(MNRL)/libmnrl.a 
	$(info  )
	$(info Compiling VASim executable...)
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

clean: cleanvasim cleanmnrl

cleanvasim:
	$(info Cleaning VASim...)
	rm -f $(ODIR)/*.o $(TARGET) $(SNAME)

cleanmnrl:
	$(info Cleaning MNRL...)
	rm -f $(MNRL)/libmnrl.a $(MNRL)/libmnrl.so $(MNRL)/src/obj/*.o


.PHONY: clean cleanvasim cleanmnrl vasim_release mnrl_release
