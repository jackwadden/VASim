# Table of Contents
1. [About](#about)
2. [Automata Model](#automata-model)
3. [Transformations](#transformations)
4. [Simulation](#simulation)
5. [Command Line Usage](#command-line-usage)
6. [Library Usage](#library)
7. [Reference](#reference)
8. [Bibliography](#bibliography)

## About
VASim is an open-source, freely licensed, homogeneous automata simulator. VASim is the perfect tool if you want to:

* Optimize homogeneous automata using prefix merging
* Gather statistics about automata graph properties
* Experiment with DFA conversion
* Simulate and debug automata graphs on an input stream
* Convert automata from one standard file-type to another
* Emit automata as synthesizable HDL for an FPGA platform
* Profile and track automata behavior during simulation


## Automata Model
VASim operates exclusively on [_homogeneous automata_](#homogeneous-automata) and can potentially read any homogeneous automata graph format. Given that homogeneous automata are a subset of classical automata, VASim can be made to emit homogeneous automata graphs in any automata file format.

VASim offers an object oriented view of automata graphs. A full description of the class hierarchy can be found in the doxygen documentation in VASim/docs/Doxygen. Each automata is made up of state transition elements (STEs). STEs represent homogeneous automata states and compute a matching function based on the symbols in their character set.

Automata are networks of STEs. VASim also allows the addition of arbitrary combinational or sequential circuit elements.

## Transformations
VASim comes with a variety of well-known and important automata transformations. A few of the most important transformations are listed below.

### Prefix merging (aka left minimization)
Prefix merging attempts to remove common prefixes in automata graphs. This is analogous to circuit minimization in modern synthesis tools. Two identical automata prefixes can be merged without changing the automata behavior. This optimization not only reduces redundant states, but implicitly reduces redundant computation in these states.

### Fan-in/Fan-out Enforcement
Fan-in and Fan-out from elements in the automat graph can be modified by duplicating parent elements or child elements respectively. VASim can automatically enforece a particular fan-in or fan-out in a graph.

## Simulation
VASim can simulate automata using an input byte stream provided by the user. Simulation in VASim involves four basic stages that are executed sequentially for each byte in the input byte stream:

### Initialization

### Stage One
Stage one computes all matching functions for every automata state. Each state that had a parent state 

### Stage Two

### Stage Three

## Command Line Usage

## Library Usage

## Reference
### Homogeneous Automata
Usually, finite automata theory is taught using as a set of states with transition rules between states, and epsilon transitions. A picture of what we refer to as a _classical_ automata formulation is shown below.

Homogeneous automata (aka Glushkov automata) are special restriction on classical automata graphs where every transition into a state must have the same rule (i.e. all incoming transition rules are "homogeneous"). This property allows us to associate an incoming transition rule with a state, rather than with an edge. Below is a picture of the classical automata represented in a way that enforces homogeneity.

Now, instead of edges computing transitions between nodes, nodes now compute transition functions. This formulation is analogous to a sequential circuit. Each element in a sequential circuit buffers signals on input edges, computes a boolean function on those inputs, and then transmits that result to the inputs of other circuit elements.

Note that homogeneous automata are just as powerful as classical NFAs and DFAs, and are simply a different machine representation with a few useful restrictions. We use homogeneous automata because they allow us a few benefits over classical automata:
1. No epsilon transitions. Epsilon transitions only complicate simulation of finite automata, can easily be removed from classical automata, and are thus generally ignored.
2. A "circuit" view of automata. Each edge can be thought of as a wire, communicating a boolean value from one circuit element state to another. This view of the world allows us to trivially add _other_ circuit elements to the graph such as AND, OR, NOT gates, counters, that can contribute to computation.
3. Easily simulated and optimized. Homogeneous automata make some optimizations such as prefix merging much easier to implement. Many other transformations are more naturally expressed using homogeneous automata.
4. Optimization friendly. Informally, if you apply an algorithm designed for classical automata on homogeneous automata, as long as no step introduces non-homogeneity, the resulting automata is homogeneous. This benefit needs a more formal proof.

## Bibliography

