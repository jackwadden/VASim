//
#ifndef EMULATOR_H
#define EMULATOR_H

#include <string>
#include <stdint.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <cmath>
#include "element.h"
#include "ste.h"

//#include "automata.h"

class Emulator {

protected:
    
    // Configuration files
    std::string config_fn;
    std::string trace_fn;
    
    // Report Vector
    std::vector<std::pair<uint32_t, std::string>> reports;
    uint32_t symbol_cycles;

    // Architectural parameters
    std::string arch;
    double mhz = 60.0;
    //double mhz = 133.0;
    std::vector<Element *> report_elements;
    uint32_t num_reporting_elements;

    // L1 output buffer organization
    uint32_t num_buffers_L1 = 6;
    uint32_t num_arch_buffers_L1 = 6;
    uint32_t bits_per_word_L1 = 64;
    uint32_t words_per_vector_L1 = 16; // 16 for reports, 1 for the flow input/byte offset
    //double vector_divisor_L1 = 16;
    uint32_t max_vectors_L1 = 481;
    //uint32_t num_vectors_L1_buffer = 1024;
    double word_read_penalty_L1 = 2.5;
    double query_penalty_L1 = 2.0;
    double startup_penalty_L1 = 15.0;

    // L2 output buffer organization

    // Data structures to keep track of assigned output regions
    std::unordered_map<std::string, uint32_t> idRegionMap;
    
public:

    Emulator(std::string fn);
    Emulator(std::vector<std::pair<uint32_t, std::string>> report_vector, 
             uint32_t cycles,
             std::vector<Element *> report_elements);
    ~Emulator();

    //
    void initDefault();
    void initPerfect();

    //
    void assignReportsToL1Buffers();
    void assignReportsToL1BuffersRR();
    void calcEventVectorDivisor();

    void parseTraceFile(std::string fn);
    double calcTotalRuntime();
    double calcTotalRuntime2();
    double calcTotalRuntime3();
    double calcTotalRuntime4();
    void printEmulationStats();
};

#endif
