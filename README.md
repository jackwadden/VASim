# Virtual Automata Simulator (VASim)

VASim is a homogeneous non-deterministic finite automata simulator. Homogeneous automata do not contain epsilon transitions, and also have the property that all incoming transitions for a particular state have the same rule.

VASim can parse, transform, simulate, and profile homogeneous automata, and is meant to be an open tool for automata processing research. VASim can also be extended to support arbitrary automata processing elements other than traditional states.

## Installation

VASim is designed to run on 64-bit machines GCC 4.9+. Users have successfully compiled on 32-bit platforms but these builds are not officially supported. Below are the commands used to install on all tested platforms.

### Ubuntu Linux (14.04/16.04 with g++ 4.9+)
```bash

$ git clone https://github.com/jackwadden/VASim.git
$ cd VASim
$ make

```

### MacOS (10.11+)
```bash

$ brew install nasm
$ brew install gcc5
$ git clone https://github.com/jackwadden/VASim.git
$ cd VASim
$ make

```

### Windows 7 (Cygwin64)

Dependencies:
- Cygwin64
- Git (https://git-scm.com/download/win)

First you must install all required Cygwin64 software tools
```bash

C:\> chdir cygwin64
C:\cygwin64> move <path-to-setup-x86_64.exe> .
C:\cygwin64> setup-x86_64.exe -q -P wget -P gcc-g++ -P make -P diffutils -P libmpfr-devel -P libgmp-devel -P libmpc-devel -P nasm

```
Once Cygwin64 is configured, VASim should compile correctly in a Cygwin terminal.

```bash

$ git clone https://github.com/jackwadden/VASim.git
$ cd VASim
$ make


```

## Command Line Usage

VASim was developed to be used as a command line tool for automata simulation. VASim takes at least one input--an automata graph--and an optional input. If an input is provided, VASim will simulate the automata on the input.

```bash
$ vasim [options] <anml_file> [input_file]
```

## Library Usage

VASim can also be used as a library to programmatically construct and simulate automata. An example program to make and simulate an automata with two states is shown below.

```c++
#include "automata.h"

#define FROM_INPUT_STRING false

using namespace std;

/*
 *
 */
int main(int argc, char * argv[]) {

    //
    Automata ap;

    STE *start = new STE("start", "[JACK]", "all-input");
    STE *stop = new STE("stop", "[JARED]", "none");
    stop->setReporting(true);

    // Add them to data structure
    ap.rawAddSTE(start);
    ap.rawAddSTE(stop);

    // Add edge between them
    ap.addEdge(start,stop);

    // Emit automata as anml file
    ap.automataToANMLFile("example.anml");
        
    // print out how many reports we've seen (this should be 0)
    cout << ap.getReportVector().size() << endl;

    // enable report gathering for the automata
    ap.enableReport();

    // initialize simulation
    ap.initializeSimulation();

    // simulate the automata on three inputs
    ap.simulate('J');
    ap.simulate('J');
    ap.simulate('J');

    // print out how many reports we've seen (this should be 2)
    cout << ap.getReportVector().size() << endl;

}
```

## Issues

Please see https://www.github.com/jackwadden/VASim/issues for a list of known bugs or to create an issue ticket.

## Citation
If you use VASim in a publication, please use the following citation:

Wadden, J., and Skadron, K. "VASim: An Open Virtual Automata Simulator for Automata Processing Application and Architecture Research." University of Virginia, Tech Report #CS2016-03, 2016.

```
@techreport{vasim,
    Author = {Wadden, Jack and Skadron, Kevin},
    Institution = {University of Virginia},
    Number = {CS2016-03},
    Title = {{VASim: An Open Virtual Automata Simulator for Automata Processing Application and Architecture Research}},
    Year = {2016}}

```
