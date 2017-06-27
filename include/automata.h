#ifndef AUTOMATA_H
#define AUTOMATA_H

#include "stack.h"
#include "ste.h"
#include "specialElement.h"
#include "ANMLParser.h"
#include "MNRLAdapter.h"
#include <cmath>

#include <iostream>
#include <string>
#include <map>
#include <unordered_map>
#include <queue>
#include <deque>
#include <set>
#include <unordered_set>
#include <vector>
#include <list>
#include <fstream>
#include <algorithm>
#include <mnrl.hpp>



class Automata {

private:
    std::string filename;
    std::string id;
    bool profile;
    bool quiet;
    bool report;
    bool dump_state;
    uint32_t dump_state_cycle;
    
    std::unordered_map<std::string, Element*> elements;
    std::vector<STE*> starts;
    std::vector<Element*> reports;
    std::unordered_map<std::string, SpecialElement*> specialElements;
    std::unordered_map<std::string, SpecialElement*> firstLayerSpecialElements;
    
    // Functional element stacks
    Stack<Element *> enabledSTEs;
    Stack<STE*> activatedSTEs;
    Stack<STE*> latchedSTEs;
    Stack<Element *> enabledElements;
    std::queue<Element*> enabledSpecialElements;
    std::queue<SpecialElement*> activatedSpecialElements;
    std::vector<SpecialElement*> latchedSpecialElements;
    std::vector<SpecialElement*> activateNoInputSpecialElements;


    // Simulation Statistics
    std::vector<std::pair<uint64_t, std::string>> reportVector;
    std::map<uint32_t, std::list<std::string>> activationVector;
    std::unordered_map<std::string, uint32_t> activationHist;    
    std::vector<uint32_t> enabledHist;
    std::vector<uint32_t> activatedHist;
    uint32_t maxActivations;

    uint32_t totalEnables;
    std::map<Element*, uint32_t> enabledCount;
    
    uint64_t cycle;
    
public:
    Automata();
    Automata(std::string);
    
    // Get/set
    std::vector<STE *> &getStarts();
    std::vector<Element *> &getReports();
    std::unordered_map<std::string, Element *> &getElements();
    std::unordered_map<std::string, SpecialElement *> &getSpecialElements();
    Stack<Element *> &getEnabledElements();
    Stack<Element *> &getEnabledSTEs();
    Stack<STE *> &getActivatedSTEs();
    std::unordered_map<std::string, uint32_t> &getActivationHist();
    std::vector<std::pair<uint64_t, std::string>> &getReportVector();
    uint32_t getMaxActivations();
    void enableProfile();
    void enableReport();
    void enableQuiet();
    void enableDumpState(uint64_t);
    void disableProfile();
    
    // I/O
    void writeStringToFile(std::string str, std::string fn);
    void appendStringToFile(std::string str, std::string fn);
    void writeIntVectorToFile(std::vector<uint32_t> &, std::string fn);
    void print();
    void printReport();
    void writeReportToFile(std::string fn);
    void printReportBatchSim();
    void printActivations();
    std::string activationHistogramToString();
    void automataToDotFile(std::string fn);
    void automataToNFAFile(std::string fn);
    void automataToANMLFile(std::string fn);
    void automataToMNRLFile(std::string fn);
    void automataToHDLFile(std::string fn);
    void automataToBLIFFile(std::string fn);
    void automataToGraphFile(std::string fn);

    std::vector<std::string> automataToRegex(std::string fn);

    // Simulation
    void simulate(uint8_t *, uint64_t, uint64_t, bool);
    void simulate(uint8_t);
    void reset();
    void enableStartStates(); // formerly stageOne
    void computeSTEMatches(uint8_t); // formerly stageTwo
    void enableSTEMatchingChildren(); // formerly stageThree
    void specialElementSimulation(); // formerly stageFour/Five
    uint64_t tick();


    // Statistics and Profiling
    void buildActivationHistogram(std::string fn);
    void calcEnableDistribution();
    void printActivationHistogram();
    void printGraphStats();
    void printSTEComplexity();
    void dumpSTEState(std::string fn);
    void dumpSpecelState(std::string fn);

    // Optimization and manipulation
    void leftMinimizeStartStates();
    uint32_t leftMinimize();
    uint32_t leftMinimizeChildren(STE*, int);

    void addEdge(Element *from, Element *to);
    void removeEdge(Element *from, Element *to);
    
    void defrag();
    Automata * generateDFA();
    std::set<STE*>* follow(uint32_t, std::set<STE*>*);
    void cutElement(Element *);
    void addSTE(STE *, std::vector<std::string>&);
    void rawAddSTE(STE *);
    void rawAddSpecialElement(SpecialElement *);
    void leftMergeSTEs(STE*,STE*);
    void rightMergeSTEs(STE*,STE*);
    void removeElement(Element *);
    void removeOrGates();
    void removeCounters();
    void convertAllInputStarts();
    std::vector<Automata*> splitConnectedComponents();
    std::vector<Automata*> generateGNFAs();
    void unsafeMerge(Automata *);
    Automata *clone();

    // Fan-in/fan-out relaxation
    void enforceFanIn(uint32_t fanin_max);
    void enforceFanOut(uint32_t fanout_max);

    // Util
    bool isTailNode(Element *);
    std::string getElementColor(std::string);
    std::string getElementColorLog(std::string);
    std::string getLogElementColor(std::string);
    void validate();

};

#endif
