# Table of Contents
1. [About](#about)
2. [Reference](#ref)
3. [Bibliography](#bib)
## About
VASim is an open-source, freely licensed, homogeneous automata simulator. VASim is the perfect tool if you want to:

* Optimize homogeneous automata using prefix merging
* Gather statistics about automata graph properties
* Experiment with DFA conversion
* Simulate and debug automata graphs on an input stream
* Convert automata from one standard file-type to another
* Emit automata as synthesizable HDL for an FPGA platform
* Profile and track automata behavior during simulation

VASim operates exclusively on _homogeneous automata_(#homogeneous-automata)

## Reference
### Homogeneous Automata
Usually, automata are taught using as a set of states with transition rules between states. A picture of an automata state is shown below.

Homogeneous automata (aka Glushkov automata) are special automata graphs where every transitions into a state has the same rule (i.e. all incoming transition rules are "homogeneous"). This property allows us to associate an incoming transition rule with a state, rather than with an edge.
