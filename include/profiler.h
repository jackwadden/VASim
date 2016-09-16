//
#ifndef PROFILER_H
#define PROFILER_H

#include "automata.h"
#include <iostream>
#include <string>
#include <unordered_map>
#include <map>
#include <vector>
#include <list>

class Profiler {

private:
   
    Automata offload;
    std::list<std::pair<uint32_t, std::list<std::string>>> reportVector;

    uint64_t cycle;
    uint32_t input;
    
public:
    Profiler(std::list<std::pair<uint32_t, std::list<std::string>>> &, Automata &);
    uint32_t profile(uint32_t *, uint32_t);
    void step(char);
};

#endif
