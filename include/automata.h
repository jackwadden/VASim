#ifndef AUTOMATA_H
#define AUTOMATA_H

#include "stack.h"
#include "ste.h"
#include "specialElement.h"
#include "ANMLParser.h"
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



class Automata {

private:
    std::string filename;
    std::string id;
    bool profile;
    bool quiet;
    bool report;
    
    std::unordered_map<std::string, Element*> elements;
    std::vector<STE*> starts;
    std::vector<Element*> reports;
    std::unordered_map<std::string, SpecialElement*> specialElements;
    std::vector<SpecialElement*> activateNoInputSpecialElements;
    
    // Functional element stacks
    //std::vector<Element *> enabledSTEs;
    Stack<Element *> enabledSTEs;
    //std::vector<STE*> activatedSTEs;
    Stack<STE*> activatedSTEs;
    //std::vector<STE*> latchedSTEs;
    Stack<STE*> latchedSTEs;
    //std::vector<Element*> enabledElements;
    Stack<Element *> enabledElements;
    std::queue<Element*> enabledSpecialElements;
    std::queue<SpecialElement*> activatedSpecialElements;
    std::vector<SpecialElement*> latchedSpecialElements;

    // Simulation Statistics
    std::vector<std::pair<uint32_t, std::string>> reportVector;
    std::map<uint32_t, std::list<std::string>> activationVector;
    std::unordered_map<std::string, uint32_t> activationHist;    
    // at each cycle, how many STEs were enabled?
    std::vector<uint32_t> enabledHist;
    std::vector<uint32_t> activatedHist;
    uint32_t maxActivations;

    uint64_t cycle;
    uint8_t input;
    
public:
    Automata();
    Automata(std::string);
    
    // Get/set
    std::vector<STE *> &getStarts();
    std::vector<Element *> &getReports();
    std::unordered_map<std::string, Element *> &getElements();
    std::unordered_map<std::string, SpecialElement *> &getSpecialElements();
    std::unordered_map<std::string, uint32_t> &getActivationHist();
    std::vector<std::pair<uint32_t, std::string>> &getReportVector();
    uint32_t getMaxActivations();
    void enableProfile();
    void enableReport();
    void enableQuiet();
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
    void automataToHDLFile(std::string fn);
    void automataToGraphFile(std::string fn);
    std::vector<std::string> automataToRegex(std::string fn);

    // Simulation
    void simulate(uint8_t *, uint32_t, uint32_t, bool);
    void simulate(uint8_t);
    void reset();
    void stageOne();
    void stageTwo();
    void stageThree();
    void stageFour();
    uint32_t stageFive();
    uint64_t tick();


    // Statistics and Profiling
    void buildActivationHistogram(std::string fn);
    void printActivationHistogram();
    void printGraphStats();
    void printSTEComplexity();

    // Optimization and manipulation
    void leftMinimize(uint32_t);
    void leftMinimizeChildren(STE*, int);
    void defrag();
    // Jack's DFA alg
    Automata * generateDFA();
    std::set<STE*>* follow(uint32_t, std::set<STE*>*);
    //
    void leftMinimize2();
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
    void mergeDisjointReportElements();
    void mergeDisjointReportElements2();

    // Regex construction
    void collapseSelfLoops();
    void collapseSequentialStates();
    void collapseSequentialSTEs(STE * parent, STE * child);
    void collapseAdjacentStates();
    void collapseHopStates();
    void duplicateEndStates();
    void collapseStartStates();
    void removeCycles();
    // Automata construction
    

    // Util
    bool isTailNode(Element *);
    std::string getElementColor(std::string);
    std::string getElementColorLog(std::string);
    std::string getLogElementColor(std::string);
    void validate();

};

#endif
