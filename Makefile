TARGET = vasim
IDIR =./include
SRCDIR =./src
MNRL =./MNRL/C++
PUGI =./pugixml-1.6/src
CXXFLAGS=-I$(IDIR) -I$(MNRL)/include -I$(PUGI) -pthread --std=c++11

_DEPS = *.h
_OBJ = util.o ste.o ANMLParser.o MNRLAdapter.o automata.o element.o specialElement.o gate.o and.o or.o nor.o counter.o inverter.o  main.o

CC=g++-5
#CC=icpc -mmic
#CC=icpc

ODIR=$(SRCDIR)/obj

DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


all: CXXFLAGS += -DDEBUG=false -Ofast -march=native -m64 -flto #-fprofile-use
all: $(TARGET)

debug: CXXFLAGS += -g -DDEBUG=true
debug: $(TARGET)

profile: CXXFLAGS += -g -pg -DDEBUG=false -O3
profile: $(TARGET)

$(TARGET):  $(OBJ) $(ODIR)/pugixml.o $(MNRL)/libmnrl.a
	$(CC) $(CXXFLAGS) -o $@ $^ 

$(ODIR)/%.o: $(SRCDIR)/%.cpp $(DEPS)
	@mkdir -p $(ODIR)	
	$(CC) $(CXXFLAGS) -c -o $@ $< 

$(ODIR)/pugixml.o: $(PUGI)/pugixml.cpp
	@mkdir -p $(ODIR)	
	$(CC) $(CXXFLAGS) -c -o $@ $< $(CXXFLAGS)

$(MNRL)/libmnrl.a:
	git submodule init
	git submodule update
	$(MAKE) -C ./mnrl/C++/

.PHONY: clean

cleanlight:
	mv $(ODIR)/pugixml.o $(ODIR)/pugixml.o.tmp
	rm -f $(ODIR)/*.o $(TARGET)
	mv $(ODIR)/pugixml.o.tmp $(ODIR)/pugixml.o

clean:
	rm -f $(ODIR)/*.o $(TARGET)
	rmdir $(ODIR)
