/**
 * @file
 */
#include "automata.h"
#include <cassert>
//#include <bits/stdc++.h>

using namespace std;
using namespace MNRL;


/**
 * Constructs an empty Automata object.
 */
Automata::Automata() {

    // Initialize status code
    setErrorCode(E_SUCCESS);
    
    // Disable report vector by default
    setReport(false);

    // Disable profiling by default
    setProfile(false);

    // Enable output by default
    setQuiet(false);

    // Initialize cycle to start at 0
    cycle = 0;

    // End of data is false until last cycle
    setEndOfData(false);
    
    // debug
    setDumpState(false, 0);
}

/**
 * Populates all internal graph data structures based on the connections defined in the string output arrays of Elements. Should be run after any modification to the graph.
 */
void Automata::finalizeAutomata() {
    
    // Populate Elements with back references and pointers
    for(auto e : elements) {

        Element *parent = e.second;

        // For all children, add a proper edge from parent -> child
        vector<string> children = parent->getOutputs();
        for(string child : children) {

            // inputs are of the form "fromNodeId:toPort"
            if(getElement(child) == NULL){
                return;
            }

            // Add the element as a parent
            addEdge(parent->getId(), child);
        }

        // add to proper data structures
        validateStartElement(parent);
        validateReportElement(parent);
        
    }
    
    //
    // collect special elements in BFS order in orderedSpecialElements vector
    //
    unmarkAllElements();

    queue<Element *> workq;
    // add all special elements to a queue
    for(auto e : getElements()){
        // push to vector, store all children specels of these for next level
        Element *el = e.second;
        // if we're an STE
        if(el->isSpecialElement()){
            workq.push(el);
        }
    }
            
    //
    queue<Element *> next_worq;
    while(!workq.empty()){

        Element *el = workq.front();
        workq.pop();

        // if all of els parents have been marked, then mark us and move to queue
        bool ready = true;
        for(auto ins : el->getInputs()){
            Element *parent = getElement(ins.first);
            if(!parent->isSpecialElement())
                continue;

            if(!parent->isMarked()){
                ready = false;
                break;
            }
        }

        //
        if(ready){
            // add to the "sorted" special element list
            el->mark();
            orderedSpecialElements.push_back(el);
            
        }else{
            // consider again
            workq.push(el);
        }
    }

    // print out ordered special elements
    //uint32_t counter = 0;
    //for(Element *el : orderedSpecialElements){
    //    cout << counter++ << " : " << el->getId() << endl;
    //}
}

/**
 * Constructs an automata object based on a string file name and a file type. Supported file types are "anml" and "mnrl". All unrecognized file types are assumed to be anml.
 */
void Automata::parseAutomataFile(string fn, string filetype) {

    // set filename
    filename = fn;
        
    if(filetype.compare("mnrl") == 0){
        // Read in automata description from MNRL file
        MNRLAdapter parser(filename);
        // TODO:: GET THIS TO RETURN PROPER ERROR CODE
        parser.parse(elements, starts, reports, specialElements, &id, activateNoInputSpecialElements);  
    } else {
        // Read in automata description from ANML file
        ANMLParser parser(filename);
        vasim_err_t result = parser.parse(elements, starts, reports, specialElements, &id, activateNoInputSpecialElements);

        setErrorCode(result);      
    }

}

/** 
 * Constructs an Automata object from a given ANML or MNRL homogeneous automata description file and the file type "mnrl" or "anml".
 */
Automata::Automata(string fn, string filetype) : Automata() {

    parseAutomataFile(fn, filetype);

    finalizeAutomata();
}


/**
 * Constructs an Automata object from a given ANML or MNRL homogeneous automata description file. Automatically determines file type based on the file extension. Currently VASim only supports MNRL (.mnrl) and ANML (.anml) files.
 */
Automata::Automata(string fn) : Automata() {

    
    // Read Automata from file based on extension
    if(getFileExt(fn).compare("mnrl") == 0) {
        parseAutomataFile(fn, "mnrl");
    } else {
        parseAutomataFile(fn, "anml");
    }

    finalizeAutomata();
}


/**
 * Disables and deactivates all elements in the automata. TODO::handle latched STEs?
 */
void Automata::reset() {

    // deactivate and disable all elements
    for(auto ee : elements) {
        Element *e = ee.second;
        e->deactivate();
        e->disable();
    }

    // unmark all elements
    unmarkAllElements();
    
    // clear all functional maps
    while(!enabledSTEs.empty())
        enabledSTEs.pop_back();

    while(!activatedSTEs.empty())
        activatedSTEs.pop_back();

    while(!latchedSTEs.empty())
        latchedSTEs.pop_back();
    
    while(!enabledSpecialElements.empty())
        enabledSpecialElements.pop();

    while(!activatedSpecialElements.empty())
        activatedSpecialElements.pop();

    latchedSpecialElements.clear();
    activateNoInputSpecialElements.clear();    

    // Reset all simulation stats
    activationVector.clear();
    activationHist.clear();
    enabledHist.clear();
    activatedHist.clear();
    maxActivations = 0;
    enabledCount.clear();
    activatedCount.clear();

    while(!enabledLastCycle.empty())
        enabledLastCycle.pop();

    while(!activatedLastCycle.empty())
        activatedLastCycle.pop();

    while(!reportedLastCycle.empty())
        reportedLastCycle.pop();

    // clear report vector
    reportVector.clear();

    // reset cycle counter to be 0
    cycle = 0;
    
}

/**
 * Copies flags from automata input to this automata.
 */
void Automata::copyFlagsFrom(Automata *a) {

    setProfile(a->profile);
    setQuiet(a->quiet);
    setReport(a->report);
    setDumpState(a->dump_state, a->dump_state_cycle);
    setEndOfData(a->end_of_data);

}

/**
 * Adds an STE to the current automata. Adds all edges contained in STEs output list.
 */
void Automata::addSTE(STE *ste) {

    rawAddSTE(ste);
    
    // for all outputs, add a proper edge
    for(auto str : ste->getOutputs()) {
        addEdge(ste->getId(), str);
    }
}

/**
 * Adds an STE to the current automata. Adds all edges specified by input vector "outputs".
 */
void Automata::addSTE(STE *ste, vector<string> &outputs) {

    rawAddSTE(ste);
    
    // for all outputs, add a proper edge
    for(auto str : outputs) {
        addEdge(ste->getId(), str);
    }
}

/** 
 * Adds an STE to the current automata. Does not update any dangling connections.
 */
void Automata::rawAddSTE(STE *ste) {

    elements[ste->getId()] = static_cast<Element *>(ste);

    if(ste->isStart()){
        starts.push_back(ste);
    }

    if(ste->isReporting()){
        reports.push_back(ste);
    }
}

/**
 * Adds a special element to the current automata. Does not update any dangling connections.
 */
void Automata::rawAddSpecialElement(SpecialElement *specel) {

    specialElements[specel->getId()] = specel;
    elements[specel->getId()] = static_cast<Element *>(specel);

    if(specel->isReporting()){
        reports.push_back(specel);
    }

    if(specel->canActivateNoEnable()){
        activateNoInputSpecialElements.push_back(specel);
    }
}

/**
 * Adds all outputs of ste2 to ste1 then removes ste2 from the automata. Used in common prefix merging algorithm.
 */
void Automata::leftMergeSTEs(STE *ste1, STE *ste2) {

    // add all outputs from ste2 to ste1
    for(auto e : ste2->getOutputSTEPointers()){
        STE *output = static_cast<STE*>(e.first);
        addEdge(ste1, output);
    }

    // remove all edges from ste2 to output
    for(auto e : ste2->getOutputSTEPointers()){
        STE *output = static_cast<STE*>(e.first);
        removeEdge(ste2, output);
    }

    removeElement(ste2);
}

/**
 * Returns a vector of every connected component automaton as a separate Automata object. Does not delete the original automata.
 */
vector<Automata*> Automata::splitConnectedComponents() {

    // unmark all elements
    unmarkAllElements();

    vector<Automata*> connectedComponents;
    uint32_t index = 0;

    // For each start state
    for(STE *start : starts){

        // If start state was already marked, don't consider it;
        if(start->isMarked())
            continue;
        // Create new automata in vector
        Automata *m = new Automata();

        connectedComponents.push_back(m);
        index++;

        // Do a breadth first search marking all children and parents
        // BFS work queue
        queue<Element *> workq;
        // MARK BEFORE ADDING TO QUEUE
        start->mark();
        workq.push(start);
        uint32_t counter = 0;
        while(!workq.empty()){
            // mark head of queue and pop current
            Element *current = workq.front();
            workq.pop();
            counter++;

            // add to new automata
            if(!current->isSpecialElement()){
                STE *current_ste = static_cast<STE*>(current);
                connectedComponents[index-1]->rawAddSTE(current_ste);
            }else{
                SpecialElement *current_specel = static_cast<SpecialElement*>(current);
                connectedComponents[index-1]->rawAddSpecialElement(current_specel);
            }

            set<Element *> unique_workq_adds;
            // add all unique children of current to workq if not already marked
            for(auto out : current->getOutputSTEPointers()){
                if(!out.first->isMarked()){
                    //workq.push(out.first);
                    unique_workq_adds.insert(out.first);
                }
            }

            for(auto out : current->getOutputSpecelPointers()){
                if(!out.first->isMarked()){
                    //workq.push(out.first);
                    unique_workq_adds.insert(out.first);
                }
            }

            // Add all parents of current to workq if not already marked
            // Get unique parents
            for(auto input : current->getInputs()){
                // only keep track of unique inputs
                Element * to_add = getElement(input.first);

                if(to_add == nullptr){
                    cout << input.first << endl;
                    cout << Element::stripPort(input.first) << endl;
                }

                if(!to_add->isMarked()){
                    unique_workq_adds.insert(to_add);
                }
            }

            // Add unique elements to workq
            for(Element * add : unique_workq_adds){
                if(!add->isMarked()){
                    // MARK BEFORE ADDING TO QUEUE
                    add->mark();
                    workq.push(add);
                }
            }
        } // workq loop
        //cout << "Added " << counter << " Elements to new automata..." << endl;
        counter = 0;
    }

    if(!quiet)
        cout << "  Found " << connectedComponents.size() << " distinct subgraphs!" << endl;

    // transfer flags
    for(Automata *a : connectedComponents) {
        a->copyFlagsFrom(this);
    }

    // return vector
    return connectedComponents;
}

/**
 * Merges all states in input automata into current automata. "Unsafe" because it does not make sure that the two automata don't have name clashes.
 */
void Automata::unsafeMerge(Automata *a) {

    for(auto el : a->getElements()){
        if(!el.second->isSpecialElement()){
            rawAddSTE(static_cast<STE*>(el.second));
        }else{
            rawAddSpecialElement(static_cast<SpecialElement*>(el.second));
        }
    }
}

/**
 * Clones current automata graph. Does not clone current simulation state.
 */
Automata *Automata::clone() {

    Automata *ap = new Automata();

    // Create empty automata and merge us into it
    ap->unsafeMerge(this);

    ap->copyFlagsFrom(this);
    
    return ap;
}


/**
 * Removes Element from the automata. Removes the element from all data structures and removes references to this element from other elements. TODO:: this works, but probably leaves some traces behind. Also, does not destroy Element object.
 */
void Automata::removeElement(Element *el) {

    // remove traces from output elements
    for(string output : el->getOutputs()){
        removeEdge(el->getId(), output);
    }

    // remove traces from input elements
    for(pair<string, bool> in : el->getInputs()){
        removeEdge(in.first, el->getId());
    }

    // clear automata data structures
    if(el->isReporting()){
        reports.erase(find(reports.begin(), reports.end(), el));
    }

    // clear from special elements array and starts array
    if(el->isSpecialElement()){
        specialElements.erase(specialElements.find(el->getId()));
    }else{
        STE * ste = static_cast<STE*>(el);
        if(ste->isStart()){
            starts.erase(find(starts.begin(), starts.end(), ste));
        }
    }

    // Clear from global element map
    unordered_map<string, Element*>::iterator it = elements.find(el->getId());
    if(it != elements.end())
        elements.erase(elements.find(el->getId()));

    //  delete el;
}

/**
 * Returns a vector of all start elements in the automata.
 */
vector<STE *> &Automata::getStarts() {

    return starts;
}

/**
 * Returns a vector of all reporting elements in the automata.
 */
vector<Element *> &Automata::getReports() {

    return reports;
}

/**
 * Returns the report vector. Each report entry is a <cycle, Element ID> pair.
 */
std::vector<std::pair<uint64_t, std::string>> &Automata::getReportVector() {

    return reportVector;
}

/**
 * Returns a map of string IDs to every Element in the automata. This is the main automata data structure.
 */
unordered_map<string, Element *> &Automata::getElements() {

    return elements;
}

/**
 * Returns a map of string IDs to every SpecialElement in the automata.
 */
unordered_map<string, SpecialElement *> &Automata::getSpecialElements() {

    return specialElements;
}


/**
 * Returns a queue of elements that were enabled on the most recent simulated symbol cycle.
 */
queue<Element *> &Automata::getEnabledLastCycle() {

    return enabledLastCycle;
}

/**
 * Returns a queue of elements that activated on the most recent simulated symbol cycle.
 */
queue<Element *> &Automata::getActivatedLastCycle() {

    return activatedLastCycle;
}

/**
 * Returns a queue of elements that reported on the most recent simulated symbol cycle.
 */
queue<Element *> &Automata::getReportedLastCycle() {

    return reportedLastCycle;
}


/**
 * Returns the activation histogram mapping Element IDs to activation counts.
 */
unordered_map<string, uint32_t> &Automata::getActivationHist() {

    return activationHist;
}

/**
 * Get the largest number of times an element activated over the course of simulation.
 */
uint32_t Automata::getMaxActivations() {

    return maxActivations;
}

/**
 * Enables automata profiling during automata simulation.
 */
void Automata::setProfile(bool profile_flag) {

    profile = profile_flag;

    // If we're profiling, map STEs to a counter for each state
    if(profile){
	for(auto e : elements) {
            enabledCount[e.second] = 0;
            activatedCount[e.second] = 0;
        }
    }
}

/**
 * Enables report recording during automata simulation.
 */
void Automata::setReport(bool report_flag) {
    report = report_flag;
}

/**
 * Supresses all output.
 */
void Automata::setQuiet(bool quiet_flag) {
    quiet = quiet_flag;
}

/**
 * Enables dynamic state logging. Dumps all states that activated on cycle dump_cycle. Acts as a debug break point. Currently only works for STEs. 
 */
void Automata::setDumpState(bool dump_flag, uint64_t dump_cycle) {
    dump_state = dump_flag;
    dump_state_cycle = dump_cycle;
}
/**
 * Sets end of data flag. If any reporting elements only report on end of data and this flag is set, the elements will report.
 */
void Automata::setEndOfData(bool eod) {
    end_of_data = eod;
}


/**
 * Prints out all elements in the automata.
 */
void Automata::print() {

    cout << "NUMBER OF ELEMENTS: " << elements.size() << endl;

    for(auto e: elements) {
        cout << e.second->toString() << endl;
    }
}

/**
 * Simulates the automata on a single input symbol. Injects is a list of element IDs injecting enable signals into the automata. All children of each injecting Element is enabled for the next symbol cycle.
 */
void Automata::simulate(uint8_t symbol, vector<string> injects) {

    // enable all element children of injected signal
    for(string inject : injects) {

        Element *el = getElement(inject);
        el->enableChildSTEs(&enabledSTEs);
        if(specialElements.size() > 0)
            el->enableChildSpecialElements(&enabledSpecialElements);
        
    }

    simulate(symbol);
}

/**
 * Simulates the automata on a single input symbol.
 */
void Automata::simulate(uint8_t symbol) {

    
    // -----------------------------
    // Step 1: if STEs are enabled and we match, activate
    computeSTEMatches(symbol);
    // -----------------------------

    
    // Activation Statistics
    if(profile){
        profileActivations();
    }

    // Debug state
    if(dump_state && (dump_state_cycle == cycle)){
        dumpSTEState("stes_" + to_string(cycle) + ".state");
    }

    // -----------------------------
    // Step 2: enable children of matching STEs
    enableSTEMatchingChildren();
    // -----------------------------


    // -----------------------------
    // Step 3:  enable all-input start states
    enableStartStates(end_of_data);
    // -----------------------------

    
    // -----------------------------
    // Step 4: special element computation
    if(specialElements.size() > 0){        
        specialElementSimulation2();

        if(dump_state && (dump_state_cycle == cycle)){
            dumpSpecelState("specels_" + to_string(cycle) + ".state");
        }
    }
    // -----------------------------

    // Enabled Statistics
    if(profile){
        profileEnables();
    }
    
    // advance cycle count
    tick();
}

/**
 * Saves the Elements that are currently enabled so that they can be recovered after each complete symbol cycle.
 */
void Automata::profileEnables() {

    // clear data structures
    while(!enabledLastCycle.empty()){
        enabledLastCycle.pop();
    }
    
    // per element statistics
    queue<Element*> tmp;
    while(!enabledSTEs.empty()) {
        
        Element* s = enabledSTEs.back();
        tmp.push(s);
        enabledSTEs.pop_back();
        
        // track number of times each ste was enabled per step
        enabledCount[s] = enabledCount[s] + 1;
        
        // track the STEs that were enabled on the last cycle
        enabledLastCycle.push(s);
    }
    
    //push back onto queue to proceed to next stage
    while(!tmp.empty()) {
        enabledSTEs.push_back(tmp.front());
        tmp.pop();
    }
}

/**
 * Saves the Elements that are currently activated so that they can be recovered after each complete symbol cycle.
 */
void Automata::profileActivations() {

    // clear data structures
    while(!activatedLastCycle.empty()){
        activatedLastCycle.pop();
    }

    while(!reportedLastCycle.empty()){
        reportedLastCycle.pop();
    }
    
    // Get per cycle stats
    activatedHist.push_back(activatedSTEs.size());
    
    // Get per STE stats
    // Check number of times each ste was activated per step
    queue<STE*> tmp;
    while(!activatedSTEs.empty()) {
        
        STE* s = activatedSTEs.back();
        tmp.push(s);
        activatedSTEs.pop_back();
        
        // track number of times each STE activated
        activatedCount[s] = activatedCount[s] + 1;
        
        // track the STEs that activated on the last cycle
        activatedLastCycle.push(s);

        // if any were reports, also track reports
        if(s->isReporting()){
            reportedLastCycle.push(s);
        }
    }
    
    //push back onto queue to proceed to next stage
    while(!tmp.empty()) {
        activatedSTEs.push_back(tmp.front());
        tmp.pop();
    }        
}

/**
 * Enables start states and primes simulation. Must be executed before simulation.
 */
void Automata::initializeSimulation() {
    
    // Initiate simulation by enabling all start states
    bool enableStartOfDataStates = true;
    enableStartStates(enableStartOfDataStates);

    //
    if(profile)
        profileEnables();
    
}

/**
 * Simulates the automata on input string. Starts at start_index and runs for length symbols.
 */
void Automata::simulate(uint8_t *inputs, uint64_t start_index, uint64_t length, uint64_t total_length) {

    cycle = start_index;

    // primes all data structures for simulation
    initializeSimulation();
    
    // for all inputs
    for(uint64_t i = start_index; i < start_index + length; i = i + 1) {

        // set end of data flag if its the last byte
        if( i == total_length - 1 ) {
            setEndOfData(true);
        }    
        // set end of data flag if the byte is a "\n"
        else if( inputs[i] == (uint32_t)'\n' ) {
            setEndOfData(true);
        }
        // unset end of data otherwise
        else {
            setEndOfData(false);
        }

        // measure progress on longer runs
        if(!quiet) {

            if(i % 10000 == 0) {
                if(i != 0) {
                    cout << "\x1B[2K"; // Erase the entire current line.
                    cout << "\x1B[0E";  // Move to the beginning of the current line.
                }

                cout << "  Progress: " << i << " / " << length << "\r";
                flush(cout);
                //
            }
        }
        simulate(inputs[i]);
    }

    if(!quiet) {
        cout << "\x1B[2K"; // Erase the entire current line.
        cout << "\x1B[0E";  // Move to the beginning of the current line.
        cout << "  Progress: " << length << " / " << length << "\r";
        flush(cout);
        cout << endl;
    }
 
    if(profile) {

        cout << endl << "Dynamic Statistics: " << endl;

        // cal average active set
        uint64_t sum = 0;
        for(uint32_t acts : activatedHist){
            sum += (uint64_t)acts;
        }

        cout << "  Average Active Set: " << (double)sum / (double)length << endl;
        for(uint32_t acts : activatedHist){
            sum += (uint64_t)acts;
        }

        // cal distribution

        // build histogram of activations
        buildActivationHistogram("activation_hist.out");        
        
        // print activation stats
        calcEnableDistribution();
        
        // write to file
        writeIntVectorToFile(enabledHist, "enabled_per_cycle.out");
        writeIntVectorToFile(activatedHist, "activated_per_cycle.out");
    
        cout << endl;
    }
}

/**
 * Writes the report vector to a file. Each report consists of the cycle the report occured on, the element ID, and the report ID of the element if set, all delimited by " : ".
 */
void Automata::writeReportToFile(string fn) {

    std::ofstream out(fn);
    string str;
    for(pair<uint64_t,string> s : reportVector) {
        str += to_string(s.first) + " : " + s.second + " : " + getElement(s.second)->getReportCode() + "\n";
    }
    out << str;
    out.close();
}


/**
 * Prints report vector in the style of the Micron AP SDK batchSim automata simulator.
 */
void Automata::printReportBatchSim() {

    // print report vector
    for(auto s: reportVector) {
        uint64_t cycle = s.first + 1;
        if(id.empty()){
            cout << "Element id: " << s.second << " reporting at index " << to_string(cycle) << endl;
        }else{
            cout << "Element id: " << id << "." << s.second << " reporting at index " << to_string(cycle) << endl;
        }
    }
}


/**
 * Calculates proportions of elements that capture total amounts of automata activity. Prints automata proportions to stdout.
 */
void Automata::calcEnableDistribution() {
    
    // gather enables into vector
    vector<uint32_t> enables;
    uint64_t sum = 0;
    for(auto e : enabledCount){
        enables.push_back(e.second);
        sum += e.second;
    }
    
    // sort vector
    sort(enables.rbegin(), enables.rend());
    
    // report how many STEs it takes to capture 90, 99, 99.9, 99.99, 99.999, 99.9999, 99.99999, 99.999999% activity
    bool one = false;
    bool two = false;
    bool three = false;
    bool four = false;
    bool five = false;
    bool six = false;
    bool seven = false;
    bool eight = false;

    uint64_t run_sum = 0;
    uint32_t index = 1;
    for(uint32_t enas : enables) {
        run_sum += enas;
        if(((double)run_sum/(double)sum) > .90 &! one){
            cout << "  90%: " << index << " / " << elements.size() << endl;
            one = true;
        }
        if((double)run_sum/(double)sum > .99 &! two){
            cout << "  99%: " << index << " / " << elements.size() << endl;
            two = true;
        }
        if((double)run_sum/(double)sum > .999 &! three){
            cout << "  99.9%: " << index << " / " << elements.size() << endl;
            three = true;
        }
        if((double)run_sum/(double)sum > .9999 &! four){
            cout << "  99.99%: " << index << " / " << elements.size() << endl;
            four = true;
        }
        if(((double)run_sum/(double)sum) > .99999 &! five){
            cout << "  99.999%: " << index << " / " << elements.size() << endl;
			five = true;
        }
        if((double)run_sum/(double)sum > .999999 &! six){
            cout << "  99.9999%: " << index << " / " << elements.size() << endl;
            six = true;
        }
        if((double)run_sum/(double)sum > .9999999 &! seven){
            cout << "  99.99999%: " << index << " / " << elements.size() << endl;
            seven = true;
        }
        if((double)run_sum/(double)sum > .99999999 &! eight){
            cout << "  99.999999%: " << index << " / " << elements.size() << endl;
            eight = true;
        }
        index++;
    }
}

/**
 * Returns a data structure mapping element pointers to the total number of times they were enabled. Only populated after simulation.
 */
unordered_map<Element*, uint32_t> &Automata::getEnabledCount() {

    return enabledCount;
}

/**
 * Returns a data structure mapping element pointers to the total number of times they were activated. Only populated after simulation.
 */
unordered_map<Element*, uint32_t> &Automata::getActivatedCount() {

    return activatedCount;
}

/**
 * Constructs a histogram counting how many times each element in the automata was activated. Writes the histogram out to file.
 */
void Automata::buildActivationHistogram(string fn) {

    maxActivations = 0;

    // gather histogram
    for(auto s: activationVector) {
        list<string> l = s.second;
        //cout << s.first << "::" << endl;
        for(auto e: l) {
            activationHist[e]++;
            // keep track of the maximum number of activations
            if(activationHist[e] > maxActivations)
                maxActivations = activationHist[e];
        }
    }

    writeStringToFile(activationHistogramToString(), fn);
}


/**
 * Writes the activation histogram to a string.
 */
string Automata::activationHistogramToString() {

    string str = "";
    for(auto s: activationHist) {
        str += s.first + "\t" + to_string(s.second) + "\n";
    }

    return str;
}


/**
 * Takes an ID and returns a red color proportional to the number of total activations of this element out of the max number of activations in the automata. Used to generate color heat maps in automataToDotFile().
 */
string Automata::getElementColor(string id) {

    // get hit count
    uint32_t hits = activationHist[id];

    double ratio = (double)hits/(double)maxActivations;

    int red, green, blue;

    int scale = (int)((double)hits/(double)maxActivations * 511); 

    if(scale > 255) {
        red = 255;
        green = 511 - scale;
        blue = 0;

    } else {
        red = scale;
        green = 255;
        blue = 0;
    }

    // if less than 1% make it blue
    if(ratio < .01) {
        red = 0;
        green = 0;
        blue = 255;
    }

    // if less than .1% make it purple
    /*
      if(ratio < .001) {
      red = 233;
      green = 27;
      blue = 240;
      }
    */
    if(hits == 0){

        red = 255;
        green = 255;
        blue = 255;
    }
    
    /*
      if(scale > 255) {
      scale = scale - 256;
      red = 255;
      green = 255 - scale;
      blue = 255 - scale;

      } else {
      red = scale;
      green = 255;
      blue = scale;

      }
    */
    char hexcol[16];

    snprintf(hexcol, sizeof hexcol, "\"#%02x%02x%02x\"", red, green, blue);
    return string(hexcol);
}

/**
 * Takes an ID and returns a log-scaled red color proportional to the number of total activations of this element out of the max number of activations in the automata. Used to generate color heat maps in automataToDotFile().
 */
string Automata::getElementColorLog(string id) {

    // get hit count
    uint32_t hits = activationHist[id];

    double ratio = (double)hits/(double)maxActivations;

    int red, green, blue;

    // start off black
    red = 0;
    green = 0;
    blue = 0;

    double scale = (1.0-ratio);
    int range = 255;
    scale = (double)range * scale;
   
    red = scale;
    green = scale;
    blue = scale;
    
    if(ratio < .01){
        red = 255;
        green = 0;
        blue = 255;
    }

    if(ratio < .001){
        red = 255;
        green = 0;
        blue = 0;
    }

    if(ratio < .0001){
        red = 0;
        green = 255;
        blue = 0;
    }

    if(ratio < .00001){
        red = 0;
        green = 0;
        blue = 255;
    }

    if(hits == 0){
        red = 255;
        green = 255;
        blue = 255;
    }
    

    char hexcol[16];

    snprintf(hexcol, sizeof hexcol, "\"#%02x%02x%02x\"", red, green, blue);
    return string(hexcol);
}

/**
 * Takes an ID and returns a log-scaled color based on the number of times an element has been activated during computation. Used for generating colorized heat maps in automataToDotFile().
 */
string Automata::getLogElementColor(string id) {

    // get hit count
    uint32_t hits = activationHist[id];

    double ratio = (double)hits/(double)maxActivations;

    int red, green, blue;

    int scale = (int)(ratio * 511); 

    scale = log2(scale)/log2(512)*511;

    if(scale > 0) {

        if(scale > 255) {

            red = 255;
            green = 255 - (scale - 256);
            blue = 0;

        } else {

            red = scale;
            green = 255;
            blue = 0;
        }

    }else {

        red = 0;
        green = 0;
        blue = 255;
    }

    char hexcol[16];

    snprintf(hexcol, sizeof hexcol, "\"#%02x%02x%02x\"", red, green, blue);
    return string(hexcol);
}

/**
 * Writes automata to .dot file for visualization using GraphML.
 */
void Automata::automataToDotFile(string out_fn) {

    map<string, uint32_t> id_map;

    string str = "";
    str += "digraph G {\n";

    //add all nodes
    uint32_t id = 0;
    for(auto e : elements) {

        // map ids to string names
        id_map[e.first] = id;

        //string fillcolor = "\"#ffffff\"";
        string fillcolor = "\"#add8e6\"";
        str += to_string(id);

        // label
        str.append("[label=\"") ;
        //str.append(e.first); 

        if(e.second->isSpecialElement()) {
            str.append(e.first); 
        } else {
            STE * ste = dynamic_cast<STE *>(e.second);
            str.append(e.first); 
            str += ":" + ste->getSymbolSet();
        }


        // heatmap color:0
        str.append("\" style=filled fillcolor="); 
        if(profile) {
            //fillcolor = getElementColorLog(e.first);
            fillcolor = getElementColor(e.first);
        }
        str.append(fillcolor); 

        //start state double circle/report double octagon
        if(!e.second->isSpecialElement()) {
            STE * ste = dynamic_cast<STE *>(e.second);
            if(ste->isStart()) {
                if(ste->isReporting()) {
                    str.append(" shape=doubleoctagon"); 
                }else {
                    str.append(" shape=doublecircle"); 
                } 
            } else {
                if(ste->isReporting()) {
                    str.append(" shape=octagon");
                }else{
                    str.append(" shape=circle");
                } 
            }

        } else {
            str.append(" shape=rectangle"); 
        }

        str.append(" ];\n"); 
        id++;
    }

    // id map <string element name> -> <dot id int>
    for(auto e : id_map) {
        uint32_t from = e.second;

        for(auto to : getElement(e.first)->getOutputs()) {
            str += to_string(from) + " -> " + to_string(id_map[Element::stripPort(to)]) + ";\n";
        }
    }

    str += "}\n";

    writeStringToFile(str, out_fn);
}

/**
 * UNFINISHED:: Removes OR gates from the automata. OR gates are syntactic sugar introduced by Micron's optimizing compiler and other automata engines may not support them, so we allow their removal.
 */
uint32_t Automata::removeOrGates() {

    // for each special element that is an OR gate
    queue<OR *> ORGates;
    uint32_t removed = 0;
    for(auto el : specialElements) { 
        SpecialElement * specel = el.second;

        // if we're not an OR gate, continue
        if(dynamic_cast<OR*>(specel) == NULL) {
            continue;
        }

        ORGates.push(static_cast<OR*>(specel));

    }

    // remove all the OR gates
    while(!ORGates.empty()){

        OR *or_gate = ORGates.front();
        ORGates.pop();
        
        // if it reports
        if(or_gate->isReporting()){

            //// make all of its parents report with the same ID
            for(auto e : or_gate->getInputs()){
                Element *parent = getElement(e.first);
                parent->setReporting(true);
                parent->setReportCode(or_gate->getReportCode());
                // remove or gate from outputs
                parent->removeOutput(or_gate->getId());
                parent->removeOutputPointer(make_pair(or_gate, or_gate->getId()));
            }
        }

        // add edges between all parents and children
        for(string output : or_gate->getOutputs()){
            for(auto input : or_gate->getInputs()){
                addEdge(input.first, output);
            }
        }

        // remove OR gate
        removed++;
        removeElement(or_gate);

    }

    return removed;
}

/**
 * UNFINISHED:: Removes Counters from the automata. Counters can sometimes be replaced by an equivalent number of matching elements.
 * TODO
 */
void Automata::replaceCounters() {

    // for each special element that is an OR gate
    queue<Element *> toRemove;
    for(auto el : specialElements) {
        SpecialElement * specel = el.second;

        // if we're not an Counter, continue
        if(dynamic_cast<Counter*>(specel) == NULL) {
            //cout << specel->toString() << endl;
            continue;
        }

        cout << "FOUND A COUNTER!" << endl;

        // ONLY REMOVE IF WE HAVE ONE CNT AND LESS THAN ONE RST
        uint32_t cnts = 0;
        uint32_t rsts = 0;

        for(auto in : specel->getInputs()){

            string input = in.first;
            cout << input << endl;
            if(Element::getPort(input).compare(":cnt") == 0){
                cnts++;
            }

            if(Element::getPort(input).compare(":rst") == 0){
                rsts++;
            }
        }

        if(cnts == 1 && rsts <= 1){
            cout << "FOUND COUNTER TO REMOVE!" << endl;
            toRemove.push(specel);
        }
    }

    // remove Counters from the automata
    // replace with the same number of STEs
    while(!toRemove.empty()) { 

        Counter * counter = static_cast<Counter *>(toRemove.front());
        toRemove.pop();

        // for the guaranteed one STE on the count input
        STE *input;
        for(auto in : counter->getInputs()){
            input = static_cast<STE*>(getElement(in.first));

            // remove output to counter
            string output = counter->getId() + Element::getPort(in.first);
            input->removeOutput(output);
            input->removeOutputPointer(make_pair(counter, output));

            // remove input from counter
            counter->removeInput(in.first);
        }
        cout << counter->toString() << endl;
        cout << input->toString() << endl;
        validate();

        // create a string of STEs the length of the counter target
        STE * input_prev = input;
        for(uint32_t i = 0; i < counter->getTarget(); i++){

            STE *input_next = new STE(input->getId() + "_cnt" + to_string(i),
                                      input->getSymbolSet(),
                                      "none");


            cout << "Adding new ste: " << i << endl;            
            // adjust pointers
            // remove all current pointers from cloned node
            input_next->clearOutputs();
            input_next->clearOutputPointers();
            input_next->clearInputs();

            // add input from input_prev
            input_next->addInput(input_prev->getId());

            // add output to input_prev
            input_prev->addOutput(input_next->getId());
            input_prev->addOutputPointer(make_pair(input_next, input_next->getId()));

            // add STE to the automata
            rawAddSTE(input_next);

            // march along
            input_prev = input_next;
        }

        // add outputs of counter to outputs of last node
        for(string out : counter->getOutputs()){
            input_prev->addOutput(out);
            input_prev->addOutputPointer(make_pair(getElement(out), out));
        }

        //if counter reported, make last node report
        if(counter->isReporting()){
            input_prev->setReporting(true);
            input_prev->setReportCode(counter->getReportCode());
        }

        // remove counter
        removeElement(counter);
        validate();
    }
}

/**
 * Writes automata to a file in an NFA style readable by Michela Becchi's NFA/DFA/HFA engine and iNFAnt GPU automata processing engine.
 */
void Automata::automataToNFAFile(string out_fn) {

    // This only works on automata that do not have special elements
    for(auto e : elements){
        if(e.second->isSpecialElement()){
            cout << "VASim Error: Automata network contains special elements unsupported by other NFA tools. Please attempt to remove redundant Special Elements using -x option." << endl;
            setErrorCode(E_ELEMENT_NOT_SUPPORTED);
            return;
        }
    }
    
    unordered_map<string, int> id_map;
    unordered_map<string, bool> marked;
    queue<string> to_process;
    unsigned int state_counter = 0;
    unsigned int accept_counter = 1;

    string str = "";

    // Header
    str += "#NFA\n";

    /* 
     * Because each AP state is an NFA edge, we must instantiate
     * a start NFA state and add transitions to all start states 
     * in the automata.
     */
    str += to_string(state_counter++) + ": initial\n";

    // TODO: NEED TO DISTINGUISH BETWEEN START OF DATA AND START ON ALL STATES
    // TODO: does not work for "start of input" start STEs.
    // TODO: Need to handle these in a separate loop.
    // Create a transition from this start state to itself
    str += "0 -> 0 : 0|255\n";

    // and the rest of the start states
    for(STE *start : starts) {

        id_map[start->getId()] = state_counter++;
        //add a transition rule for every int in the int_set
        //cout << state_counter << " :: " << start->getSymbolSet() << endl;

        for(uint32_t i : start->getIntegerSymbolSet()) {
            str += "0 -> " + to_string(id_map[start->getId()]);
            str += " : " + to_string(i) + "\n";
        }
    }

    // for every start state, start building the nfa
    for(STE *start : starts) {

        //create a new state and make a transition
        if(id_map.find(start->getId()) == id_map.end())
            id_map[start->getId()] = state_counter++;

        marked[start->getId()] = true;

        string state = to_string(id_map[start->getId()]);         

        if(start->isReporting()) {
            str += state + " : ";
            // always default back to 0
            str += "accepting " + to_string(accept_counter++) + "\n";
        }

        // for every output, 
        for(string s : start->getOutputs()) {
            STE * ste = dynamic_cast<STE *>(getElement(s));

            //create a new state and make a transition
            if(id_map.find(s) == id_map.end())
                id_map[s] = state_counter++;

            string to_state = to_string(id_map[s]);             
            //add a transition rule for every int in the int_set
            bool first = true;
            for(uint32_t i : ste->getIntegerSymbolSet()) {
                if(first) {
                    str += state + " -> " + to_state;
                    str += " : " + to_string(i);
                    first = false;
                }else{
                    str += " " + to_string(i);
                }
            }

            if(!first)
                str += "\n";

            //push to todo list if we haven't already been here
            if(marked[s] != true) {
                to_process.push(s);

            }
        }
    }

    while(!to_process.empty()) {

        // get an element
        string id = to_process.front();
        //cout << elements[id]->toString() << endl;
        STE * ste = dynamic_cast<STE *>(getElement(id));

        // make sure not to double add states
        // still not sure why this is necessary but it is...
        // maybe because of self references in output lists?
        if(marked[id] == true) {
            to_process.pop();
            continue;
        }

        marked[id] = true;
        to_process.pop();

        if(id_map.find(id) == id_map.end())
            id_map[id] = state_counter++;

        //add element as an output node
        string state = to_string(id_map[id]);             
        if(ste->isReporting()){
            str += to_string(id_map[id]) + " : ";
            str += "accepting " + to_string(accept_counter++) + "\n";
        }

        // for every output, 
        for(string s : ste->getOutputs()) {
            STE * ste_to = dynamic_cast<STE *>(getElement(s));
            //create a new state and make a transition
            if(id_map.find(s) == id_map.end())
                id_map[s] = state_counter++;

            string to_state = to_string(id_map[s]);             
            bool first = true;
            for(uint32_t i : ste_to->getIntegerSymbolSet()) {
                if(first) {
                    str += state + " -> " + to_state;
                    str += " : " + to_string(i);
                    first = false;
                }else{
                    str += " " + to_string(i);
                }
            }

            if(!first)
                str += "\n";

            //push to todo list
            if(marked[s] != true)
                to_process.push(s);
        }
    }

    // emit the number of states at the head of the file
    str = to_string(state_counter) + "\n" + str;


    // write NFA to file
    writeStringToFile(str, out_fn);
}

/**
 * Writes automata to ANML file.
 */
void Automata::automataToANMLFile(string out_fn) {

    string str = "";

    // xml header
    str += "<anml version=\"1.0\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\n";
    str += "<automata-network id=\"vasim\">\n";

    vector< pair<string, Element *>> els;

    // first sort elements by ID
    for(auto el : elements) {
        els.push_back(el);
    }
    sort(els.begin(), els.end());
    
    for(auto el : els) {
        str += el.second->toANML();
        str += "\n";
    }

    // xml footer
    str += "</automata-network>\n";
    str += "</anml>\n";

    // write NFA to file
    writeStringToFile(str, out_fn);
}

/**
 * Writes automata to the MNRL file format.
 */
void Automata::automataToMNRLFile(string out_fn) {
    MNRLNetwork net("vasim");
    
    // add all the elements
    for(auto el : elements) {
        net.addNode((el.second)->toMNRLObj());
    }
    
    // add all the connections
    for(auto el : elements) {
        for(auto dst : el.second->getOutputs()) {
            // We're going to make some assumptions here
            
            string dst_port = Element::getPort(dst);
            
            // determine the corret destination port
            switch(getElement(dst)->getType()) {
                case STE_T:
                    dst_port = MNRLDefs::H_STATE_INPUT;
                    break;
                case COUNTER_T:
                    dst_port = Element::getPort(dst);
                    break;
                default:
                    // for Boolean
                    dst_port = "b0"; // this is the first boolean port
                    break;
            }
            
            net.addConnection(
                el.second->getId(),// src id
                MNRLDefs::H_STATE_OUTPUT,// src port
                Element::stripPort(dst),// dest id
                dst_port// dest port
            );
        }
        
    }
    
    // write the net to a file
    net.exportToFile(out_fn);
    
}

/**
 * Outputs automata to Vitis-HLS
 * 
**/

void writeHeaderFile(int num_components, string return_type, string tree_header) {
    // Generate header file
    string str = "";
    str += "#ifndef _AUTOMATA_HPP_\n";
    str += "#define _AUTOMATA_HPP_\n";
    str += "\n";
    str += "#include \"../krnl_automata.hpp\"\n";
    str += "\n";

    // Fill in the tree header if there is one
    str += tree_header;
    str += "\n\n";

    // Fill in the functions for each automaton
    for(int i = 0; i < num_components; i++) {
        str += "ap_uint<1> automata_" + std::to_string(i) + "(uint8_t input);\n";
    }
    str += "\n";
    str += "#endif";

    // Write header to file 'automata.hpp'
    writeStringToFile(str, "automata.hpp");
}

string generateHTree(int num_automata, int split_factor) {
    // Do we want to balance partitioning by # states?
    // For now we're gonna go the lazy route
    string str = "";
    string header_str = "// ROOT (LEVEL 0)\n";
    
    str += "////////////////////////////////////////\n";
    str += "//  Copyright goes here\n";
    str += "//  This HLS was emitted by VASim\n";
    str += "////////////////////////////////////////\n";
    str += "\n";

    // Print headers
    str += "#include \"automata.hpp\"\n";
    str += "\n";

    // Some details for debugging
    str += "// This file was generated for " + std::to_string(num_automata) + " automata.\n";
    str += "// with split factor " + std::to_string(split_factor) + "\n\n";

    // Root function automata(input)
    str += "automata_output automata(uint8_t input){\n";
    header_str += "automata_output automata(uint8_t input);\n\n";
    str += "\t#pragma HLS INLINE OFF\n\n";

    // We assume that we have more than split_factor automata
    assert(num_automata > split_factor);

    // This is how we'll pass ranges
    struct range {
        int range[2];
        std::string name;
    };

    // Start breaking the automata up recursively
    std::queue<range> q;
    int split_size = num_automata / split_factor;
    int left_overs = num_automata % split_factor;

    std::cout << "Num automata: " << num_automata << std::endl;
    std::cout << "Split Size: " << split_size << std::endl;
    std::cout << "Left Overs: " << left_overs << std::endl;

    // The root function has a report = OR(all automata)
    str += "\tstatic uint8_t report = ";
    for(int i = 0; i < split_factor; i++) {
        string automata_branch = "automata_tree_" + std::to_string(i);
        str += automata_branch + "(input)";

        // If not the last branch for this level, then |
        if(i != (split_factor - 1)) {
            str += " | ";
        }

        range tmp;
        tmp.range[0] = split_size * i;
        tmp.range[1] = split_size * i + split_size - 1;
        tmp.name = automata_branch;
        q.push(tmp);
    }
    str += ";\n\n";
    str += "\treturn report;\n";
    str += "}\n\n";

    // Now lets start processing the branches / leaves

    std::cout << "Contents of the Queue" << std::endl;
    while (!q.empty()) {
        range front = q.front();
        int automata_left = (front.range[1] - front.range[0] + 1);
        split_size = automata_left / split_factor;
        left_overs = automata_left % split_factor;
        std::cout << "\t Range:[" << front.range[0] << ", " << front.range[1] << "], Left: " << automata_left << " Name:" << front.name << std::endl;
        
        // Check if leaf
        if(automata_left <= split_factor){
            str += "uint8_t " + front.name + "(uint8_t input){\n";
            header_str += "uint8_t " + front.name + "(uint8_t input);\n";
            str += "\t#pragma HLS INLINE OFF\n\n";
            str += "\tstatic uint8_t report = ";
            std::cout <<"Found a LEAF!!: " << front.range[0] << "-" << front.range[1] << std::endl;
            for(int i = front.range[0]; i <= front.range[1]; i++){
                str += "automata_" + std::to_string(i) + "(input)";
                if(i != front.range[1]) { 
                    str += " | ";
                }
            }
            str += ";\n\n";
            str += "\treturn report;\n";
            str += "}\n\n";
        }
        // Generate name of function based on depth
        else {
            str += "uint8_t " + front.name + "(uint8_t input){\n";
            header_str += "uint8_t " + front.name + "(uint8_t input);\n";
            str += "\t#pragma HLS INLINE OFF\n\n";
            str += "\tstatic uint8_t report = ";

            for(int i = 0; i < split_factor; i++){
                string new_name = front.name + "_" + std::to_string(i);
                str += new_name + "(input)";
                if(i != (split_factor - 1)){
                    str += " | ";
                }
                range tmp;
                tmp.range[0] = front.range[0] + (split_size * i);
                tmp.range[1] = front.range[0] + (split_size * i + split_size - 1);
                tmp.name = new_name;
                q.push(tmp);
            }
            str += ";\n\n";
            str += "\treturn report;\n";
            str += "}\n\n";
        }

        q.pop();
    }

    writeStringToFile(str, "automata_tree.cpp");

    return header_str;
}

bool sort_automata_by_states (Automata* i,Automata* j) { 
    return (i->getElements().size() < j->getElements().size());
}

// Function for automataToHLSFiles to generate ranges of values for symbol sets
vector<pair<int, int> > getRanges(vector<uint32_t> symbolSet){

    // Sort the symbols
    std::sort (symbolSet.begin(), symbolSet.end());

    // We're going to return pairs representing continuous numerical ranges
    vector<pair<int, int> > ranges;

    int first, second;

    for(int i = 0; i < symbolSet.size(); i++){
        int current = symbolSet[i];

        if(i == 0){ // Start
            first = current;
            second = first;
        }
        // If contiguous
        else if(current == (second + 1)){
            second = current;
        }
        // Not contiguous
        else {
            pair<int, int> temp(first, second);
            ranges.push_back(temp);

            first = current;
            second = current;
        }
    }

    pair<int, int> temp(first, second);
    ranges.push_back(temp);
    return ranges;
}

void Automata::automataToHLSFiles(int N, int split_factor) {

    bool or_all = false;
    bool one_function_per_file = false;
    bool inlined = false;
    bool single_file = true;
    bool sort_automata = false;

    // This is representing character sets in the AP way
    // They are stored in memories and evaluated at runtime; this is SLOW
    bool bitwise = false;

    vector<Automata*> connected_components = splitConnectedComponents();

    if(sort_automata){
        std::sort (connected_components.begin(), connected_components.end(), sort_automata_by_states);
    }

    int index = 0;
    for(auto cc: connected_components)
        std::cout << "Automata " << index++ << " size: " << std::to_string(cc->getElements().size()) << std::endl;

    assert(N <= connected_components.size());

    Automata first = Automata(*connected_components[0]);

    // Chooses the first N components
    vector<Automata*> subset;
    for (int i = 0; i < N; i++) {
        subset.push_back(connected_components[i]);
        first.unsafeMerge(connected_components[i]);
    }
    first.finalizeAutomata();

    // Drop the N components into an Automata file
    first.automataToANMLFile("Automata.anml");

    // Update num_components
    int num_components = subset.size();

    std::cout << "Splitting automata into " << num_components << " components" << std::endl;
    std::cout << "Where each component is represented by a separate HLS function" << std::endl;

    // Set the return type; one bit per automaton
    // TODO: we may have multiple report states in the future
    string return_type = "ap_uint<" + std::to_string(N) + ">";
    
    // Generate kernel call string
    string str = "";
    str += "";
    
    int i = 0;
    // For each automata component in the graph
    for(auto aut : subset){
        string automata_name = "automata_" + std::to_string(i);

        if(!single_file)
            str = "";

        if(!single_file || i == 0){
            // Print copyright
            str += "////////////////////////////////////////\n";
            str += "//  Copyright goes here\n";
            str += "//  This HLS was emitted by VASim\n";
            str += "////////////////////////////////////////\n";
            str += "\n";
        }
        string report_string = "return (";

        // Print headers
        if(!single_file || i == 0){
            str += "#include \"automata.hpp\"\n";
        }

        str += "\n";

        // Print function 
        // TO DO: Future automata may have multiple return states
        str += "ap_uint<1> " + automata_name + "(uint8_t input) {\n";

        if(!inlined)
            str += "\t#pragma HLS INLINE OFF\n";

        // Add pipeline pragma to make sure automaton finishes in one cycle!
        str += "\t#pragma HLS pipeline II=1\n";

        str += "\n";

        // Stamp out states
        str += "\t//States\n";
        str += "\tstatic uint8_t input_r = 0;\n";
        str += "\tstatic const ap_uint<1> start_state = 1;\n";

        // For each state...
        for(auto e : aut->getElements()){
            Element *el = e.second;

            // TODO: In the future support counters and gates
            if(el->isSpecialElement()){
                std::cout << "Element " << el->getId() + " is not an STE; FAIL\n";
                exit(-1);
            }
            STE *s = static_cast<STE*>(el);
            string s_id = s->getId();
            char start = (s->isStart()) ? '1' : '0'; 
            vector<uint32_t> integerSymbolSet = s->getIntegerSymbolSet();

            // Create state register for STE
            str += "\tstatic ap_uint<1> state_" + s_id + "_enable = " + start + ";\n";
            
            // Create symbol set array for STE
            if(bitwise) {
                str += "\tconst uint8_t state_" + s_id + "_char[" + std::to_string(integerSymbolSet.size()) + "] = {";
                // Populate symbol set array with symbol set of STE
                for(int i = 0; i < integerSymbolSet.size(); i++) {
                    str += std::to_string(integerSymbolSet[i]);
                    if(i != (integerSymbolSet.size() - 1)){
                        str += ",";
                    }
                }
                str += "};\n";
                str += "\n";
            }
        }
        
        // Add State Logic
        str += "\t // State Logic\n";

        for(auto e : aut->getElements()){
            Element *el = e.second;

            // TODO: For now only support NFA states; future add counters and OR/AND gates
            if(el->isSpecialElement()){
                std::cout << "Element " << el->getId() + " is not an STE; FAIL\n";
                exit(-1);
            }

            STE *s = static_cast<STE*>(el);
            string s_id = s->getId();
            vector<uint32_t> integerSymbolSet = s->getIntegerSymbolSet();

            str += "\tap_uint<1> ste_" + s_id + " = (state_" + s_id + "_enable) &&\n";
            str += "\t\t(";

            // This is a * state
            if(bitwise){
                if(integerSymbolSet.size() == 256) {
                    str += "1";
                }
                else {
                    // There is room to make this more concise
                    for(int i = 0; i < integerSymbolSet.size(); i++){
                        str += "(input_r == state_" + s_id + "_char[" + std::to_string(i) + "])";
                        if(i != integerSymbolSet.size() - 1){
                            str += " || \n";
                        }
                    }
                }
            }
            else{
                vector<pair<int, int>> ranges = getRanges(integerSymbolSet);
                for(int i = 0; i < ranges.size(); i++){
                    pair<int,int> pair = ranges[i];
                    if(pair.first == pair.second){
                        str += "(input_r == " + std::to_string(pair.first) + ")";
                    }
                    else{
                        str += "(input_r >= " + std::to_string(pair.first) + "  && input_r <= " + std::to_string(pair.second) + ")";
                    }
                    if(i != (ranges.size() - 1)){
                        str += "|| ";
                    }
                }

            }
            str += ");\n";
            str += "\n";
        }

        // Register updates (edges)
        str += "\t// Edges\n";
        str += "\tinput_r = input;\n";

        bool first = true;
        for(auto e: aut->getElements()){
            Element *el = e.second;
            if(el->isSpecialElement()){
                std::cout << "Element " << el->getId() + " is not an STE; FAIL\n";
                exit(-1);
            }
            STE *s = static_cast<STE*>(el);
            string s_id = s->getId();

            if(s->isReporting()){
                if(!first){
                    report_string += " || ";
                }
                else{
                    first = false;
                }
                report_string += "ste_" + s_id;
            }

            str += "\tstate_" + s_id + "_enable = ";
            if(s->isStart()) { // For now we only support all-data
                str += "start_state;\n"; // Every cycle this state is enabled
            }
            else {
                str += "(";
                auto in_edges = s->getInputs();
                bool first = true;
                for(auto in : in_edges){
                    if(!first){
                        str += " || ";
                    }
                    else {
                        first = false;
                    }
                    str += "ste_" + in.first;
                }
                str += ");\n";
            }
        }

        report_string += ");";
        str += "\t" + report_string + "\n";
        str += "}\n\n";

        if(!single_file)
            writeStringToFile(str, "automata_" + std::to_string(i) + ".cpp");
        i++;
    }
    if(single_file)
        writeStringToFile(str, "automata_single_file.cpp");
    
    string tree_header = "";
    
    // What should this threashold me?
    if(N > 512){
        tree_header += generateHTree(N, split_factor);
    }

    writeHeaderFile(num_components, return_type, tree_header);
}

/**
 * Outputs automata to Verilog HDL description following the algorithm originally developed by Xiaoping Huang and Mohamed El-Hadedy.
 */
void Automata::automataToHDLFile(string out_fn) {

    string str = "";

    // This is a temp fix for Vinh. Ideally the user should be able to pass in a module name
    string module_name = out_fn.substr(0, out_fn.length() - 2);

    unordered_map<string, string> id_reg_map;

    // print copyright
    str += "////////////////////////////////////////\n";
    str += "//  Copyright goes here\n";
    str += "//  This HDL was emitted by VASim\n";
    str += "////////////////////////////////////////\n";
    // print timescale
    str += "`timescale 1ns/100ps\n";

    // NECESSARY? print a module for every connected component

    //// print module header
    str += "// The start of the module\n";
    str += "module " + module_name + "(\n";
    str += "\tClk,\n";
    str += "\tRst_n,\n";
    str += "\tSymbol";
    
    if(getReports().size() > 0){
        // print output signals
        for(Element * el: getReports()){
            string reg_name = module_name + "$" + el->getId();
            str += ",\n\t" + reg_name;
        }
    }

    str += "\n\t);\n\n";


    //// define ports
    str += "\t// Port definitions\n";
    // inputs
    str += "\tinput\tClk;\n";
    str += "\tinput\tRst_n;\n";
    str += "\tinput [0:7]\tSymbol;\n";
    
    // outputs
    for(Element *el : getReports()){
        str += "\toutput\t" + module_name + "$" + el->getId() + ";\n";
    }

    //
    str += "\n";

    // define output signals
    str += "\t// Output signal definitions\n";
    for(Element *el : getReports()){
        string reg_name = module_name + "$" + el->getId();
        str += "\treg\t" + reg_name + ";\n";
        id_reg_map[el->getId()] = reg_name;
    }

    // define ste registers
    str += "\n";
    str += "\t// Internal variable reg definitions\n";
    for(auto e : getElements()){
        Element *el = e.second;
        if(el->isReporting())
            continue;
        if(el->isStateful()) {
            str += "\treg\t" + el->getId() + ";\n";
        }else{
            // if we are stateless logic 
            // just declare a wire
            str += "\twire\t" + el->getId() + ";\n";
        }
        
        id_reg_map[el->getId()] = el->getId();
    }

    // define special signal registers
    str += "\n";
    str += "\t// cycle counter\n";
    str += "\treg\t[0:31] Cycle;\n";
    string start_of_data = "start_of_data";
    str += "\n";
    str += "\t// start of data signal\n";
    str += "\treg\t" + start_of_data + ";\n";
    str += "\n\n";

    // Cycle counter and start of data logic
    str += "\n\n\t// Cycle counter logic\n";
    str += "\t(*dont_touch = \"true\"*) always @(posedge Clk) // should not be optimized\n";
    str += "\tbegin\n";
    str += "\t\tif (Rst_n == 1'b1)\n";
    str += "\t\tbegin\n";
    str += "\t\t\t Cycle <= 32'b00000000000000000000000000000000;\n";
    str += "\t\t\t " + start_of_data + " <= 1'b1;\n";
    str += "\t\tend\n";
    str += "\t\telse\n";
    str += "\t\tbegin\n";
    str += "\t\t\t Cycle <= Cycle + 1;\n";
    str += "\t\t\t " + start_of_data + " <= 1'b0;\n";
    str += "\t\tend\n";
    str += "\tend\n\n";

    
    // print logic for every Element
    // ONLY HANDLES:
    // STES
    // Counters
    // Inverters
    for(auto e : this->getElements()){

        //
        Element *el = e.second;
        if(el->isSpecialElement()){
            //FIXME this is such a hack.  Fix this for good OO design!
            SpecialElement *s = static_cast<SpecialElement*>(el);
            str += s->toHDL(id_reg_map);
        } else {
            STE *s = static_cast<STE*>(el);

            // header
            str += "\t////////////////\n";
            str += "\t// STE: " + s->getId() + "\n";
            str += "\t////////////////\n";
            // print input enables
            str += "\t// Input enable OR gate\n";
            string enable_name = s->getId() + "_EN";
            str += "\twire\t" + enable_name + ";\n";
            if(s->startIsAllInput()){
                str += "\tassign " + enable_name + " = 1'b1;";
            } else {
                str += "\tassign " + enable_name + " = ";
                
                // for all the inputs
                bool first = true;
                for(auto in : s->getInputs()){
                    if(first){
                        str += id_reg_map[in.first];
                        first = false;
                    }else{
                        str += " | " + id_reg_map[in.first];
                    }
                }
    
                if(s->startIsStartOfData()) 
                    str += " | " + start_of_data;
    
                str += ";\n";
            }
                
            //
            str += "\n\t// Match logic and activation register\n";
    
            //
            str += "\t(*dont_touch = \"true\"*) always @(posedge Clk) // should not be optimized\n";
    
            string reg_name = id_reg_map[s->getId()];
            str += "\tbegin\n";
            str += "\t\tif (Rst_n == 1'b0)\n";
            str += "\t\t\t" + reg_name + " <= 1'b0;\n";
            str += "\t\telse if ("+ enable_name +" == 1'b1)\n";
            str += "\t\t\tcase (Symbol)\n";
            // emit decoder for each STE
            for(uint32_t i = 0; i < 256; i++){
                if(s->getBitColumn().test(i)){
                    str += "\t\t\t\t8'd" + to_string(i) + ": " + reg_name + " <= 1'b1;\n";
                }
            }
            str += "\t\t\t\tdefault: " + reg_name + " <= 1'b0;\n";
            str += "\t\t\tendcase\n";
            str += "\t\telse " + reg_name + " <= 1'b0;\n";
            str += "\tend\n\n";
    
            // build or gate for enable inputs
            
        }

    }


    //// print module footer
    str += "endmodule\n";
    
    cout << "Writing Verilog to file: " << out_fn << endl << endl; 

    // write NFA to file
    writeStringToFile(str, out_fn);
}

/**
 * Write automata to .blif circuit readable by "Automata-to-Routing" (https://github.com/jackwadden/Automata-To-Routing).
 */
void Automata::automataToBLIFFile(string out_fn) {

    string str = "";

    // ------------------------
    // Hardware constraints
    // ------------------------
    // STE enable input limit
    // 16 is the default limit imposed by the D480AP (d480.xml)
    uint32_t ste_enable_limit = 16;
    // ------------------------


    // emit header for module
    str += ".model blif_by_VASim\n";
    
    // emit inputs
    str += ".inputs ";

    // emit clock
    str += "top.clock ";
    str += "\n";

    // emit outputs
    str += ".outputs outpin\n";
    // THIS DESIGN HAS NO REAL PHYSICAL OUTPUT PINS
    str += "\n";

    // emit unconnected dummy wire
    str += ".names unconn";
    str += "\n\n";

    //------------------------------
    // emit STEs
    //------------------------------
    // data structure to track enable inputs for each ste
    unordered_map<string, uint32_t> enable_counter; 

    // initialize portnumber map to all 0
    for(auto e : elements){

        Element *el = e.second;
        if(el->isSpecialElement()){
            continue;
        }

        STE *s = static_cast<STE*>(el);
        
        enable_counter[s->getId()] = 0;    
    }

    // for every STE, emit a proper .subckt
    for(auto e : elements){

        Element *el = e.second;
        if(el->isSpecialElement()){
            continue;
        }

        STE *s = static_cast<STE*>(el);
    
        str += ".subckt ste ";

        // INPUTS
        // add global enable ports to start states
        // for each input
        uint32_t input_counter = 0;
        for(auto in : s->getInputs()){

            string parent = in.first;
            string child = s->getId();

            // ignore self refs
            if(parent.compare(child) == 0)
                continue;

            // emit proper signal
            string wire = parent; 
            uint32_t portnumber = enable_counter[s->getId()];
            str += "enable[" + to_string(portnumber) + "]=" + wire + " ";
            enable_counter[s->getId()] = portnumber + 1;

            // do a check to see if this automata is legal given hw constraints
            input_counter++;
            if(input_counter > ste_enable_limit) {
                cout << "ERROR:: Automata fan-in is too large. STE " << s->getId() << " has too many inputs. HW limit is " << ste_enable_limit << ". Exiting..." << endl;
                exit(1);
            }
        }

        // Fill rest of inputs with unconn dummy nets
        for(int i = 0; i < ste_enable_limit; i++){
            uint32_t portnumber = enable_counter[s->getId()];
            if(i < portnumber)
                continue;
            str += "enable[" + to_string(i) + "]=unconn ";
        }

        // OUTPUTS
        string parent = s->getId();
        string wire = s->getId();
        if(!s->isReporting())
            str += "active=" + wire + " ";
        
        // CLOCK
        str += "clock=top.clock ";

        str += "\n\n";
    }

    str += "\n";

    // end STE netlist
    str += "\n";
    str += ".end\n";
    str += "\n";
    str += "\n";

    // emit the STE blackbox model
    str += ".model ste\n";
    str += ".inputs ";
    for(int i = 0; i < ste_enable_limit; i++)
        str += "enable[" + to_string(i) + "] " ;
    str += "clock\n";
    str += ".outputs active\n";
    str += ".blackbox\n";
    str += ".end\n";
    str += "\n";

    writeStringToFile(str, out_fn);
}


/**
 * Write out automata to file readable by graphgrep (https://github.com/jackwadden/graphgrep).
 */
void Automata::automataToGraphFile(string out_fn) {


    string str = "";
    
    // num nodes header
    str += to_string(elements.size()) + "\n";

    for(auto e : elements){
        
        Element *el = e.second;
        if(!el->isSpecialElement()){
            STE *s = static_cast<STE*>(el);
            
            // emit ID
            str += s->getId() + " ";
            
            // emit char reach
            for(int i = 255; i >= 0; i--){
                if(s->match(i)){
                    str += "1";
                }else{
                    str += "0";
                }
            }
            str += " ";

            // emit start
            if(s->isStart()){
                str += "1 ";
            }else{
                str += "0 ";
            }

            // emit startDs
            if(s->isStart()){
                str += "1 ";
            }else{
                str += "0 ";
            }

            // emit accept
            if(s->isReporting()){
                str += "1 ";
            }else{
                str += "0 ";
            }

            str += "\n";
        }
    }

    // emit all edges
    for(auto e : elements){
        
        Element *el = e.second;
        if(!el->isSpecialElement()){
            STE *s = static_cast<STE*>(el);
            
            str += s->getId() + " ";

            for(string out : s->getOutputs()){
                str += out + " ";
            }

            str += "\n";
        }
    }

    writeStringToFile(str, out_fn);
}

/**
 * Converts each "all-input" type start element to "start-of-data" type. Preserves automata semantics by installing self referencing star states that act like "all-input" start states.
 */
void Automata::convertAllInputStarts() {

    // create new star ste start state
    STE *star_start = new STE("STAR_START", "*", "start-of-data");
    rawAddSTE(star_start);
    star_start->addOutput(star_start->getId());
    star_start->addOutputPointer(make_pair(star_start, star_start->getId()));
    star_start->addInput(star_start->getId());

    // for all starts
    for(STE *s : getStarts()) {

        // convert all start states to start-of-data
        if(!s->startIsStartOfData()){
            s->setStart("start-of-data");
        }

        // make sure star start points to them
        star_start->addOutput(s->getId());
        star_start->addOutputPointer(make_pair(s, s->getId()));
        s->addInput(star_start->getId());
    }
}

/*
 *
 */
inline bool set_comp(set<STE*>* lhs, set<STE*>* rhs) {

    return (*lhs < *rhs);
}

/*
 * return true if lhs is strictly before rhs
 */
inline bool state_comp(pair<set<STE*>*, STE*> lhs, pair<set<STE*>*, STE*> rhs) {

    // if the sets are not the same, return the difference
    if(*(lhs.first) != *(rhs.first))
        return (*(lhs.first) < *(rhs.first));

    // if the sets are the same, compare the bitsets
    bitset<256> lhs_column = lhs.second->getBitColumn();
    bitset<256> rhs_column = rhs.second->getBitColumn();

    for(uint32_t i = 0; i < 256; i++){
        if(lhs_column[i] == false &&  rhs_column[i] == true)
            return true;
        if(lhs_column[i] == true && rhs_column[i] == false)
            return false;
    }

    // if they're equal, return false
    return false;
}


/**
 * Returns the set of STEs reachable from the input set of STEs on this input symbol.
 */
set<STE*>* Automata::follow(uint32_t symbol, set<STE*>* state_set) {


    set<STE*>* follow_set = new set<STE*>;

    // for each start
    for(STE *start : getStarts()){
        if(start->match((uint8_t)symbol)){
            // add to follow_set
            //cout << start->getId() << " matched on input " << endl; 
            follow_set->insert(start);
        }
    }
    
    // for each STE in the set
    for(STE *ste : *state_set){
        // for each child
        for(auto e : ste->getOutputSTEPointers()){
            STE * child = static_cast<STE*>(e.first);
            // if they match this character
            if(child->match((uint8_t)symbol)){
                // add to follow_set
                follow_set->insert(child);
            }
        }
    }

    return follow_set;
}


/**
 * Constructs an equivalent homogeneous DFA from the current automata. This algorithm is worst case exponential in space and time and so may not be feasible for even medium-sized automata.
 */
Automata* Automata::generateDFA() {

    if(!quiet)
        cout << "Generating DFA..." << endl;

    // DFAs need a failure state and cannot use all-input start nodes
    //  we convert all-inputs to start-of-data plus a failure node to
    //  keep the automata going
    //convertAllInputStarts();

    Automata* dfa = new Automata();
    //
    bool(*fn_pt)(set<STE*>*, set<STE*>*) = set_comp; //custom comparison method for set
    //
    bool(*fn_pt2)(pair<set<STE*>*, STE*>, pair<set<STE*>*, STE*>) = state_comp; //custom comparison method for set

    // global data structure to hold DFA states (used for searching for uniqueness)
    set<pair<set<STE*>*, STE*>, bool(*)(pair<set<STE*>*,STE*>, pair<set<STE*>*,STE*>)> dfa_states (fn_pt2);

    // counter for dfa integer IDs
    uint32_t dfa_state_ids = 0;

    // map DFA state ID to STE in dfa
    unordered_map<uint32_t, STE*> dfa_ste_map;

    // work queues for subset construction alg
    queue<pair<set<STE*>*, STE*>> workq;
    queue<pair<set<STE*>*, STE*>> next_workq;

    // initialize failure state
    set<STE*>* start_state = new set<STE*>;
    
    // push impl start onto workq
    workq.push(make_pair(start_state, (STE*)NULL));

    uint32_t dfa_state_counter = 0;
    
    // main loop
    while(!workq.empty()){

        if(!quiet)
            cout << "DFA States:" <<  dfa_state_counter << " -- Stack:" << workq.size() << endl;

        // get current working DFA state set and STE
        set<STE*>* dfa_state = workq.front().first;
        STE* dfa_ste = workq.front().second;
        workq.pop();
        dfa_state_counter++;

        //
        // find unique next DFA states for each character class
        //

        // holds all potential DFA states from this node
        set<set<STE*>*, bool(*)(set<STE*>*, set<STE*>*)> potential_dfa_states (fn_pt);
        // maps sets to STEs
        unordered_map<set<STE*>*, STE*> ste_table;
        
        // for each character
        for(uint32_t i = 0; i < 256; i++){
            
            //get the follow state
            set<STE*>* potential_dfa_state = follow(i, dfa_state);

            bool found = false;          
            uint32_t unique_state_id = 0;
            
            // look through all existing potential DFA states
            // is this potential DFA state unique?
            set<set<STE*>*>::const_iterator got;
            got = potential_dfa_states.find(potential_dfa_state);
            if(got != potential_dfa_states.end()){
                found = true;
            }
        
            // if DFA state is unique
            if(!found){
                //cout << "found unique!" << endl;

                // create new ste
                STE * new_dfa_ste = new STE("temp", "","");
                // add this character to its symbol set
                new_dfa_ste->addSymbolToSymbolSet(i);
                // if any of the sets STEs were reporting, set us to reporting
                for(STE * nfa_state : *potential_dfa_state){
                    if(nfa_state->isReporting()){
                        new_dfa_ste->setReporting(true);
                        break;
                    }
                }

                // if we come from the first implicit state make us a start state
                if(dfa_ste == NULL){
                    new_dfa_ste->setStart("start-of-data");
                }

                // add dfa_state set to temp data structure
                potential_dfa_states.insert(potential_dfa_state);
                ste_table[potential_dfa_state] = new_dfa_ste;

            }else{
                
                // we already created this potential DFA state so retrieve it
                STE* existing_dfa_ste = ste_table[(*got)];

                // add new symbol to its charset (idempotent)
                existing_dfa_ste->addSymbolToSymbolSet(i);
            
                // delete the potential object
                delete potential_dfa_state;
                
            }            
            
        } // character for loop

        // once we have constructed the potential new DFA states
        //  we must check if they already exist in the global DFA data struct
        //  this includes comparing NFA states, and also character set of DFA STE
        for(set<STE*>* potential_dfa_state : potential_dfa_states){
            
            // see if we exist in the global dfa states set
            set<pair<set<STE*>*, STE*>>::const_iterator got2;
            STE * potential_ste = ste_table[potential_dfa_state];
            pair<set<STE*>*, STE*> potential_pair = make_pair(potential_dfa_state, potential_ste);
            got2 = dfa_states.find(potential_pair);
            STE * existing_dfa_ste;
            bool found = false;
            if(got2 != dfa_states.end()){
                found = true;
                existing_dfa_ste = (*got2).second;
            }else{
                existing_dfa_ste = potential_ste;
            }

            // if we found a unique new state
            if(!found){
                // add it to the global state structure
                dfa_states.insert(potential_pair);
                // give STE unique ID
                existing_dfa_ste->setId(to_string(dfa_state_ids));
                existing_dfa_ste->setIntId(dfa_state_ids);
                dfa_state_ids++;
                // add STE to the dfa
                dfa->rawAddSTE(existing_dfa_ste);
                // push it onto the workq for processing
                next_workq.push(potential_pair);
            }

            //add physical edge from current DFA state to new DFA state
            if(dfa_ste != NULL){
                dfa_ste->addOutput(existing_dfa_ste->getId());
                dfa_ste->addOutputPointer(make_pair(existing_dfa_ste, existing_dfa_ste->getId()));
                existing_dfa_ste->addInput(dfa_ste->getId());
            }

        }


        // push next workq into workq
        while(!next_workq.empty()){
            workq.push(next_workq.front());
            next_workq.pop();
        }

    }

    return dfa;
 
}


/**
 * Enable all elements that are start states. Start states initiate computation by being enabled on the first cycle (for start-of-data type) or every cycle (for all-input type).
 */
void Automata::enableStartStates(bool enableStartOfData) {

    //for each start element
    for(STE * s: starts) {

        // Enable if start is "all input"
        if(s->startIsAllInput() || (enableStartOfData && s->startIsStartOfData())) { 
           
            // add to enabled queue if we were not already enabled
            if(!s->isEnabled()){
                s->enable();
                enabledSTEs.push_back(static_cast<Element *>(s));
            }
        }
    }

}

/**
 * If an STE is enabled and matches on the current input, activate. If the STE is a report STE, record a report in the report vector. 
 */
void Automata::computeSTEMatches(uint8_t symbol) {

    //for each enabled ste
    while(!enabledSTEs.empty()) {

        STE * s = static_cast<STE *>(enabledSTEs.back());

        // if we match on the input character
        // the STE will activate and we record this
        // ste should also report
        if(s->match(symbol)) {

            //activate and push to queue only if we werent already
            if(!s->isActivated()) {
                s->activate();
                activatedSTEs.push_back(s);
            }

            if(profile)
                activationVector[cycle].push_back(s->getId());

            // report
            if(report && s->isReporting()) {
                if(s->isEod()) {
                    if(end_of_data)
                        reportVector.push_back(make_pair(cycle, s->getId()));
                }else{
                    reportVector.push_back(make_pair(cycle, s->getId()));
                }
            }

        }

        //disable 
        s->disable();

        // remove STE from the queue
        enabledSTEs.pop_back();        
    }
}

/**
 * Propagate activation signal of STEs that match on the current input symbol. Enables Element children of active STEs.
 */
void Automata::enableSTEMatchingChildren() {

    //for each activated ste
    while(!activatedSTEs.empty()) {

        STE *s = activatedSTEs.back();
        // remove from activated queue
        activatedSTEs.pop_back();

        s->enableChildSTEs(&enabledSTEs);

        if(specialElements.size() > 0)
            s->enableChildSpecialElements(&enabledSpecialElements);

        // suggest that the STE deactivate
        // if we don't, add to the queue
        if(!s->deactivate()) {
            // don't mark for removal from activated map
            latchedSTEs.push_back(s);
        }
        
    }
    
    // refil activated elements
    while(!latchedSTEs.empty()) {
        activatedSTEs.push_back(latchedSTEs.back());
        latchedSTEs.pop_back();
    }
}

/**
 * BFS that only considers special Elements once per post STE cycle
 *  Special Element compute order starts with those that only have STE parents
 *  and then progresses, adding new elements when all parent special elements
 *  have been considered.
 */
void Automata::specialElementSimulation2() {

    // Calculate all specels in order
    for(Element *spel : orderedSpecialElements){
        
        // calculate
        bool result = static_cast<SpecialElement*>(spel)->calculate();

        // DO WE ACTIVATE?
        if(result){

            // activate
            if(!spel->isActivated()){
                spel->activate();
            }
            
            // report?
            if(report && spel->isReporting()) {
                reportVector.push_back(make_pair(cycle, spel->getId()));
            }
        }

        // disable
        spel->disable();
        
        // for all children
        // enable them if we activated
        if(result){
            spel->enableChildSTEs(&enabledSTEs);
            spel->enableChildSpecialElements(&enabledSpecialElements);
        } 
    }
}

/**
 * Simulates SpecialElements. SpecialElements are unbuffered and behave as traditional electrical circuit elements. Therefore, all special elements need to continuously calculate based on their inputs until a steady state is reached. SpecialElement simulation is extremely slow compared to STE-only simulation.
 */
void Automata::specialElementSimulation() {

    // circuit simulation happens between automata processing
    // all circuit elements (specels) are considered
    // elements are considered from root nodes (specels that are children of STEs)
    // computation proceeds until all elements enable STEs and have no more circuit children

    map<uint32_t, bool> calculated;
    map<uint32_t, bool> queued;

    queue<SpecialElement *> work_q;
    
    // initialize tracking structures
    for( auto e : elements) {
        
        // initialize claculated map
        calculated[e.second->getIntId()] = false;

        // initialize queued map
        queued[e.second->getIntId()] = false;
    }

    // fill work_q with special children of STEs
    for(auto e : elements) {
        if(!e.second->isSpecialElement()){

            for( auto sp : e.second->getOutputSpecelPointers() ) {

                SpecialElement *specel = static_cast<SpecialElement*>(sp.first);
                if(!queued[specel->getIntId()]){
                    work_q.push(specel);
                    queued[specel->getIntId()] = true;
                }
            }
            
            // indicate that this parent has already calculated
            calculated[e.second->getIntId()] = true;
        }
    } 

    // while workq is not empty
    while(!work_q.empty()){

        // get front
        SpecialElement * spel = work_q.front();
        work_q.pop();

        // if all parents have already calculated
        bool ready = true;
        for(auto in : spel->getInputs()){

            if(!calculated[getElement(in.first)->getIntId()]){
                ready = false;
                break;
            }
        }

        // execute if all our inputs are ready
        if(ready){
            
            // calculate
            calculated[spel->getIntId()] = true;
            bool emitOutput = spel->calculate();

            // if we calculated true
            if(emitOutput) {

                // activate
                if(!spel->isActivated()){
                    spel->activate();
                }
                
                // report?
                if(report && spel->isReporting()) {
                    reportVector.push_back(make_pair(cycle, spel->getId()));
                }
                
            }
            
            // disable
            spel->disable();

            // for all children
            // enable them if we activated
            if(emitOutput){
                spel->enableChildSTEs(&enabledSTEs);
                spel->enableChildSpecialElements(&enabledSpecialElements);
            }

            //// if child is specel
            for(auto e : spel->getOutputSpecelPointers()){
            
                ////// push to queue to consider 
                SpecialElement *spel_child = static_cast<SpecialElement*>(e.first);
                if(!queued[spel_child->getIntId()]){
                    work_q.push(spel_child);
                    queued[spel_child->getIntId()] = true;
                }

            }
        }
    }
}


/**
 * Tick advances the cycle count representing an automata "symbol cycle."
 */
uint64_t Automata::tick() {

    return cycle++;
}

/**
 * Merges identical prefixes of automaton. Uses a breadth first search on the automata, combining states with identical inputs and properties, but varying outputs. Does not currently merge prefixes with back references (loops).
 */
uint32_t Automata::mergeCommonPrefixes() {

    // number of merged elements
    uint32_t merged = 0;
    
    // work queue items are candidate sets of mergeable elements
    queue<queue<STE*>*> workq;

    //
    unmarkAllElements();

    // load all start states into first set
    queue<STE*> *first = new queue<STE*>;
    for(STE *ste : starts){
        ste->mark();
        first->push(ste);
    }
    
    workq.push(first);

    // now for each candidate set, try to merge all elements
    while(!workq.empty()){

        // grab candidate set
        queue<STE*> *candidates = workq.front();
        queue<STE*> candidates_tmp;
        
        // try to merge all candidates
        while(!candidates->empty()) {

            STE * first = candidates->front();
            candidates->pop();
            
            while(!candidates->empty()) {
                STE * second = candidates->front();
                candidates->pop();
                
                //if the two STEs have identical prefixes, merge
                if(first->leftCompare(second)) {
                    merged++;
                    leftMergeSTEs(first, second);
                    //else push back onto workq
                } else {
                    candidates_tmp.push(second);
                }	 
            }

            // Add all children of first to new candidate set
            queue<STE*> *next_candidate_set = new queue<STE*>;
            for(auto c : first->getOutputSTEPointers()) {
                STE * child = static_cast<STE*>(c.first);
                if(!child->isMarked()){
                    child->mark();
                    next_candidate_set->push(child);
                }
            }

            // consider candidate set at the back of the queue
            if(next_candidate_set->size() > 0)
                workq.push(next_candidate_set);
            else
                delete next_candidate_set;
            
            // try another candidate in this candidate set
            while(!candidates_tmp.empty()){
                candidates->push(candidates_tmp.front());
                candidates_tmp.pop();
            }
        }

        // free the candidate set
        delete candidates;
        
        // pop the workq now that we're done with it
        workq.pop();
    }
    
    return merged;
}

/*
uint32_t Automata::mergeCommonPrefixes() {

    uint32_t merged = 0;
    
    unmarkAllElements();

    // start search by considering all start states
    queue<STE*> workq;
    for(STE *ste : starts){
        ste->mark();
        workq.push(ste);
    }

    merged += mergeCommonPrefixes(workq);

    return merged;
}
*
/**
 * Recursive function that considers merging all candidate STEs in the current workq. Recursively calls itselfe with a new workq with all child candidates that could possibly be merged.
 */
/*
uint32_t Automata::mergeCommonPrefixes(queue<STE *> &workq) {

    queue<STE*> next_level;
    queue<STE*> workq_tmp;
    
    uint32_t merged = 0;
    
    // merge identical children of next level
    while(!workq.empty()) { 
        STE * first = workq.front();
        workq.pop();
        
        while(!workq.empty()) {
            STE * second = workq.front();

            workq.pop();
            //if the two STEs have identical prefixes, merge
            if(first->leftCompare(second)) {
                merged++;
                leftMergeSTEs(first, second);
                //else push back onto workq
            } else {
                workq_tmp.push(second);
            }	 
        }

        // Add all children of first to the next level
        for(auto c : first->getOutputSTEPointers()) {
            STE * child = static_cast<STE*>(c.first);
            if(!child->isMarked()){
                child->mark();
                next_level.push(child);
            }
        }

        // Recurse downward
        if(next_level.size() > 0)
            merged += mergeCommonPrefixes(next_level);

        // Try another candidate
        while(!workq_tmp.empty()) {
            workq.push(workq_tmp.front());
            workq_tmp.pop();
        }
    }

    return merged;
}
*/

/**
 * Merges identical suffixes of automaton. Uses a depth first search on the automata, combining states with identical outputs and properties, but varying inputs. Does not currently merge suffixes with back references (loops).
 */
uint32_t Automata::mergeCommonSuffixes() {

        // number of merged elements
    uint32_t merged = 0;
    
    // work queue items are candidate sets of mergeable elements
    queue<queue<STE*>*> workq;

    //
    unmarkAllElements();
    
    // start search by considering all start states
    queue<STE*> *first = new queue<STE*>;
    for(Element *el : reports){
        if(!el->isSpecialElement()) {
            STE *ste = static_cast<STE*>(el);
            ste->mark();
            first->push(ste);
        }
    }

    workq.push(first);

    // now for each candidate set, try to merge all elements
    while(!workq.empty()){

        // grab candidate set
        queue<STE*> *candidates = workq.front();
        queue<STE*> candidates_tmp;
        
        // try to merge all candidates
        while(!candidates->empty()) {

            STE * first = candidates->front();
            candidates->pop();
            
            while(!candidates->empty()) {
                STE * second = candidates->front();
                candidates->pop();
                
                //if the two STEs have identical prefixes, merge
                if(first->rightCompare(second)) {
                    merged++;
                    rightMergeSTEs(first, second);
                    //else push back onto workq
                } else {
                    candidates_tmp.push(second);
                }	 
            }

            // Add all children of first to new candidate set
            queue<STE*> *next_candidate_set = new queue<STE*>;
            for(auto c : first->getInputs()) {
                Element *el = getElement(c.first);
                if(!el->isSpecialElement()) {
                    STE * child = static_cast<STE*>(el);
                    if(!child->isMarked()){
                        child->mark();
                        next_candidate_set->push(child);
                    }
                }
            }

            // consider candidate set at the back of the queue
            if(next_candidate_set->size() > 0)
                workq.push(next_candidate_set);
            else
                delete next_candidate_set;
            
            // try another candidate in this candidate set
            while(!candidates_tmp.empty()){
                candidates->push(candidates_tmp.front());
                candidates_tmp.pop();
            }
        }

        // free the candidate set
        delete candidates;
        
        // pop the workq now that we're done with it
        workq.pop();
    }
    
    return merged;
}

/**
 * Merges identical suffixes of automaton. Uses a depth first search on the automata, combining states with identical outputs and properties, but varying inputs. Does not currently merge suffixes with back references (loops).
 */
/*
uint32_t Automata::mergeCommonSuffixes() {

    uint32_t merged = 0;
    
    unmarkAllElements();

    // start search by considering all start states
    queue<STE*> workq;
    for(Element *el : reports){
        if(!el->isSpecialElement()) {
            STE *ste = static_cast<STE*>(el);
            ste->mark();
            workq.push(ste);
        }
    }

    merged += mergeCommonSuffixes(workq);

    return merged;
}
*/
/**
 * Recursive function that considers merging all candidate STEs in the current workq. Recursively calls itselfe with a new workq with all child candidates that could possibly be merged.
 */
/*
uint32_t Automata::mergeCommonSuffixes(queue<STE *> &workq) {

    queue<STE*> next_level;
    queue<STE*> workq_tmp;
    
    uint32_t merged = 0;
    
    // merge identical parents of next level
    while(!workq.empty()) { 
        STE * first = workq.front();
        workq.pop();
        
        while(!workq.empty()) {
            STE * second = workq.front();

            workq.pop();
            //if the two STEs have identical sufffixes, merge
            if(first->rightCompare(second)) {
                merged++;
                rightMergeSTEs(first, second);
                //else push back onto workq
            } else {
                workq_tmp.push(second);
            }	 
        }

        // Add all parents of first to the next level
        for(auto c : first->getInputs()) {
            Element *el = getElement(c.first);
            if(!el->isSpecialElement()) {
                STE * child = static_cast<STE*>(el);
                if(!child->isMarked()){
                    child->mark();
                    next_level.push(child);
                }
            }
        }

        // Recurse downward
        if(next_level.size() > 0)
            merged += mergeCommonSuffixes(next_level);

        // Try another candidate
        while(!workq_tmp.empty()) {
            workq.push(workq_tmp.front());
            workq_tmp.pop();
        }
    }

    return merged;
}
*/

/**
 * If two STEs share the same parents and the same children, they can be combined into
 *   one STE with the union of their character sets.
 */
uint32_t Automata::mergeCommonPaths() {

    uint32_t merged = 0;
    
    //
    unmarkAllElements();

    //
    queue<STE*> to_remove;
    
    // for each element
    for(auto e : elements) {
        
        //
        Element *el = e.second;

        // skip if we've been removed
        if(el->isMarked())
            continue;

        el->mark();
        
        // skip special elements
        if(el->isSpecialElement())
            continue;

        // skip reporting elements
        if(el->isReporting())
            continue;
        
        // find candidates to compare
        // all children's parents
        for(string c : el->getOutputs()){

            Element *child = getElement(c);
            if(child->isSpecialElement())
                continue;

            
            for(auto p : child->getInputs()){
                Element * childs_parent = getElement(p.first);

                if(childs_parent->isSpecialElement())
                    continue;

                // skip if we've already considered this one
                if(childs_parent->isMarked())
                    continue;
                
                // if the two elements share identical parents and children lists
                if(el->identicalInputs(childs_parent) &&
                   el->identicalOutputs(childs_parent)) {

                    STE *ste1 = static_cast<STE*>(el);
                    STE *ste2 = static_cast<STE*>(childs_parent);
                    
                    // add charset of childs_parent to el
                    for(uint32_t symbol = 0; symbol < 256; symbol++) {
                        if(ste2->match(symbol))
                            ste1->addSymbolToSymbolSet(symbol);
                    }

                    // delete ste2
                    ste2->mark();
                    to_remove.push(ste2);

                    merged++;
                }
            }
        }
    }

    // remove all elements marked for deletion
    while(!to_remove.empty()){
        removeElement(to_remove.front());
        to_remove.pop();
    }

    return merged;
    
}

/**
 * Checks Automata graph for inconsistencies and errors. Sets the Automata error code to something other than E_SUCCESS if the automata has an error.
 */
void Automata::validate() {


    for(auto e : elements) {

        Element *el = e.second;

        // check inputs
        for(auto ins : el->getInputs()){
            // does my input exist?
            if(getElement(ins.first) == NULL){
                cout << "FAILED INPUTS EXISTANCE TEST!" << endl;
                cout << "  " << Element::stripPort(ins.first) << " input of element: " << e.first << " does not exist in the element map." << endl;
                setErrorCode(E_MALFORMED_AUTOMATA);
                return;
            }

            Element * parent = getElement(ins.first);

            // does my input have me as an output?
            bool has_ref = false;
            for(string out : parent->getOutputs()){
                if(el->getId().compare(Element::stripPort(out)) == 0){
                    has_ref = true;
                    break;
                }
            }

            if(!has_ref){
                cout << "FAILED INPUTS MATCH TEST!" << endl;
                cout << "  " << el->getId() << " did not exist in outputs list of " << parent->getId() << endl;
                setErrorCode(E_MALFORMED_AUTOMATA);
                return;
            }
        }


        // check outputs
        for(string output_tmp : el->getOutputs()){

            string output = Element::stripPort(output_tmp);

            // does my output exist in the map?
            if(getElement(output) == NULL){
                cout << "FAILED OUTPUTS TEST!" << endl;
                cout << "  " << output << " output of element: " << e.first << " does not exist in the element map." << endl;
                setErrorCode(E_MALFORMED_AUTOMATA);
                return;
            }

            Element * child = getElement(output);

            // does my output have me as an input?
            bool has_ref = false;
            for(auto e : child->getInputs()){
                if(el->getId().compare(Element::stripPort(e.first)) == 0){
                    has_ref = true;
                    break;
                }
            }

            if(!has_ref){
                cout << "FAILED OUTPUTS MATCH TEST!" << endl;
                cout << "  " << el->getId() << " did not exist in inputs list of its child " << child->getId() << endl;
                cout << child->toString() << endl;
                setErrorCode(E_MALFORMED_AUTOMATA);
                return;
            }

        }

    }

    automataToDotFile("failed_verification.dot");
}

/**
 * Gathers and displays the average STE character set complexity using the Quine-McKlusky algorithm as a measure of "complexity."
 */
void Automata::printSTEComplexity() {

    // gather charset complexity
    // NAIVE METHOD
    uint32_t complexity = 0;
    unordered_map<string, uint32_t> score_cache;
    for(auto el : elements){

        if(!el.second->isSpecialElement()){

            STE *ste = static_cast<STE*>(el.second);
            // count number of bits set in column
            cout << ste->getSymbolSet() << endl;

            if(!score_cache[ste->getSymbolSet()])
                score_cache[ste->getSymbolSet()] = QMScore(ste->getBitColumn());

            complexity += score_cache[ste->getSymbolSet()];
        }
    }

    cout << "  Average STE Complexity: " << (double)complexity / (double)elements.size() << endl;

}

/**
 * Prints various automata graph summary statistics.
 */
void Automata::printGraphStats() {


    cout << "Automata Statistics:" << endl;
    cout << "  Elements: " << elements.size() << endl;
    cout << "  STEs: " << elements.size() - specialElements.size() << endl;
    cout << "  SpecialElements: " << specialElements.size() << endl;

    // gather edge statistics
    uint64_t sum_out = 0;
    uint32_t max_out = 0;
    uint32_t max_in = 0;
    uint64_t sum_in = 0;
    for(auto el : elements){

        uint32_t outputs = 0;
        uint32_t inputs = 0;
        outputs = el.second->getOutputs().size();
        inputs = el.second->getInputs().size();

        if(el.second->isSelfRef()){
            outputs--;
            inputs--;
        }
        
        if(outputs > max_out){
            max_out = outputs;
        }
        
        sum_out += outputs;

        if(inputs > max_in){
            max_in = inputs;
        }
        sum_in += inputs;
    }

    cout << "  Max Fan-in (not including self loops): " << max_in << endl;
    cout << "  Max Fan-out (not including self loops): " << max_out << endl;
    cout << "  Average Node Degree: " << (double)sum_out / (double)elements.size() << endl << endl;

}


/**
 * Adds all inputs of ste2 to ste1 then removes ste2 from the automata.
 */
void Automata::rightMergeSTEs(STE *ste1, STE *ste2){

    // add all inputs to ste1
    for(auto input : ste2->getInputs()){
        STE *in_ste = static_cast<STE*>(getElement(input.first));
        addEdge(in_ste, ste1);
    }

    // 
    for(auto input : ste2->getInputs()){
        STE *in_ste = static_cast<STE*>(getElement(input.first));
        removeEdge(in_ste, ste2);
    }

    removeElement(ste2);
}

/**
 * Adds all members of ste2's charset to ste1's charset and then deletes ste2
 */
void Automata::mergeSTEs(STE *ste1, STE *ste2){

    // add charset of ste2 to ste1
    for(uint32_t i = 0; i < ste2->getBitColumn().size(); i++) {
        if(ste2->match(i)){
            ste1->addSymbolToSymbolSet(i);
        }
    }

    removeElement(ste2);
}

/**
 * Guarantees that the fan-in for every node does not exceed fanin_max.
 */
void Automata::enforceFanIn(uint32_t fanin_max){
    
    // BFS queue of elements to process 
    queue<STE*> workq;

    // unmark all elements
    unmarkAllElements();
    
    // push all start states to workq
    for(auto el : getElements()){ 

        // ignore special elements
        if(el.second->isSpecialElement()){
            continue;
        }
        
        STE * s = static_cast<STE*>(el.second);

        // push start states to workq    
        if(s->isStart()){
            // mark node
            s->mark();
            // add to workq
            workq.push(s);
        }
    }

    // look for elements with fanins that violate fanin_max
    while(!workq.empty()){
       
        // get node to work on
        STE * s = workq.front();
        workq.pop();

        // if we have more inputs than the max
        // (does not include self references)
        uint32_t fanin = 0;
        bool selfref = false;
        for(auto e : s->getInputs()){
            if(e.first.compare(s->getId()) == 0){
                selfref = true;
            }else{
                fanin++;
            }
        }

        // add all children of original node to workq if they are not marked (visited)
        // NOTE: this implicitly does not handle special element children
        for(auto e : s->getOutputSTEPointers()){
            if(!e.first->isMarked()){
                STE * child = static_cast<STE*>(e.first);
                workq.push(child);
                child->mark();
            }
        }
        
        //
        //cout << "FAN IN: " << fanin << endl;
        if(fanin > fanin_max){

            // adjust node
            // figure out how many new nodes we'll need
            uint32_t new_nodes = ceil((double)fanin / (double)fanin_max);

            //add all inputs to queue
            // except self refs
            queue<string> old_inputs;
            for(auto e : s->getInputs()){
                if(e.first.compare(s->getId()) != 0)
                    old_inputs.push(e.first);
            }
            
            // create new nodes
            for(uint32_t i = 0; i < new_nodes; i++){
                
                string id = s->getId() + "_" + to_string(i);
                
                STE *new_node = new STE(id,
                                        s->getSymbolSet(),
                                        s->getStringStart());
                if(s->isReporting()){
                    //cout << "REPORTING SPLIT" << endl;
                    new_node->setReporting(true);
                    new_node->setReportCode(s->getReportCode());
                }
                
                // add to automata
                rawAddSTE(new_node);
                
                // mark the node in case there are loops
                new_node->mark();

                // replicate output edges from old node to new node
                for(string output : s->getOutputs()){
                    // ignore selfref output, we'll handle it later
                    if(output.compare(s->getId()) != 0){
                        Element *to = getElement(output);
                        addEdge(new_node, to);

                        // make sure we reconsider the output even if it was already marked
                        if(!to->isSpecialElement())
                            workq.push(static_cast<STE*>(to));
                    }
                }
                   
                // add a portion of the inputs to new node
                uint32_t input_counter = 0;
                while(input_counter < fanin_max && !old_inputs.empty()){
                    // add edge from input node to new node
                    Element *from = getElement(old_inputs.front());
                    addEdge(from ,new_node);

                    old_inputs.pop();
                    input_counter++;
                }

                // if the split node is a self looping node, make new node self looping
                if(selfref){
                    addEdge(new_node, new_node);
                }
            }
            
            // delete old node
            removeElement(s);
        }
    }
}

/**
 * Guarantees that the fan-in for every node does not exceed fanin_max.
 */
void Automata::enforceFanOut(uint32_t fanout_max){

    // BFS queue of elements to process 
    queue<STE*> workq;

    // unmark all elements
    unmarkAllElements();
    
    // push report states to workq
    for(auto el : getElements()){ 

        if(el.second->isSpecialElement()){
            continue;
        }
        STE * s = static_cast<STE*>(el.second);

        // push report states to workq    
        if(s->isReporting()){
            // mark node
            s->mark();
            // add to workq
            workq.push(s);
        }
    }

    // look for elements with fanouts that violate fanout_max
    while(!workq.empty()){
       
        // get node to work on
        STE * s = workq.front();
        workq.pop();

        // add all parents to workq if they are not marked (visited)
        // NOTE: this implicitly does not handle special element children
        for(auto in : s->getInputs()){
            Element *el = getElement(in.first);
            // if not marked
            if(!el->isMarked()){
                // add parent to workq
                STE * parent = static_cast<STE*>(el);
                workq.push(parent);
                parent->mark();
            }
        }

        // calc fanout
        // (does not include self references)
        uint32_t fanout = 0;
        bool selfref = false;
        for(string out : s->getOutputs()){
            if(out.compare(s->getId()) == 0){
                selfref = true;
            }else{
                fanout++;
            }
        }
       
        //
        // if fanout violates bound
        if(fanout > fanout_max){
            
            // adjust node
            // figure out how many new nodes we'll need
            uint32_t new_nodes = ceil((double)fanout / (double)fanout_max);
            
            //add all outputs from node to queue
            // except self refs
            queue<string> old_outputs;
            for(string out : s->getOutputs()){
                if(out.compare(s->getId()) != 0)
                    old_outputs.push(out);
            }
            
            // create new nodes
            for(uint32_t i = 0; i < new_nodes; i++){
                
                string id = s->getId() + "_" + to_string(i);
                
                STE *new_node = new STE(id,
                                        s->getSymbolSet(),
                                        s->getStringStart());
                if(s->isReporting()){
                    //cout << "REPORTING SPLIT" << endl;
                    new_node->setReporting(true);
                    new_node->setReportCode(s->getReportCode());
                }
                
                // add to automata
                rawAddSTE(new_node);
                
                // mark the node in case there are loops
                new_node->mark();

                // add all inputs from s to new node
                // replicate input edges from old node to new node
                for(auto in : s->getInputs()){
                    string input = in.first;
                    // ignore selfref output, we'll handle it later
                    if(input.compare(s->getId()) != 0){

                        //
                        Element *from = getElement(input);
                        addEdge(from, new_node);

                        // make sure we reconsider the input node, even if it was already marked
                        // adding an output edge to this input may have violated the fan-out max!
                        if(!from->isSpecialElement()){
                            workq.push(static_cast<STE*>(from));
                        }
                    }
                }
                
                // add a portion of the outputs to new node
                uint32_t output_counter = 0;
                while(output_counter < fanout_max && !old_outputs.empty()){

                    // add output edge to new node
                    addEdge(new_node, getElement(old_outputs.front()));

                    old_outputs.pop();
                    output_counter++;
                }

                // if the split node is a self looping node, make new node self looping
                if(selfref){
                    //
                    addEdge(new_node, new_node);
                }                
            }
            
            // delete old node
            removeElement(s);
        }
    }
}


/**
 * Dumps active states on parameter designated cycle to file stes_<cycle>.state
 */
void Automata::dumpSTEState(string filename) {

    string s = "";

    queue<STE*> temp;

    // print activated STEs
    while(!activatedSTEs.empty()){

        STE * ste = activatedSTEs.back();
        temp.push(ste);
        activatedSTEs.pop_back();
        // print ID
        s += ste->getId();
        s += "\n";
    }

    // restore activated STEs
    while(!temp.empty()){
        activatedSTEs.push_back(temp.front());
        temp.pop();
    }

    writeStringToFile(s, filename);
}

/**
 * Dumps active special elements on parameter designated cycle to file stes_<cycle>.state
 *  *always* dumps counters and prints the current counter value and target.
 */
void Automata::dumpSpecelState(string filename) {

    string s = "";

    // print activated Specials
    for(auto e : elements){

        if(e.second->isSpecialElement()){
            SpecialElement * specel = static_cast<SpecialElement*>(e.second);
           
            // if counter, print ID and target
            if(dynamic_cast<Counter*>(specel)){
                s += specel->getId();
                Counter *c = static_cast<Counter*>(specel);
                s += " " + to_string(c->getValue()) + " " + to_string(c->getTarget());
                s += "\n";
            }else{
                // if its just a specel, only print its ID if it activated
                if(specel->isActivated()){
                     s += specel->getId();
                     s += "\n";
                }
            }


        }
    }

    writeStringToFile(s, filename);
}

/**
 * Removes a directed edge between two elements.
 */
void Automata::removeEdge(Element* from, Element *to) {

    from->removeOutput(to->getId());
    from->removeOutputPointer(make_pair(to, to->getId()));
    to->removeInput(from->getId());
}

/**
 * Removes a directed edge between two elements. Either string input can define a connection to a specific Element port using the form "ElementID:port". 
 */
void Automata::removeEdge(string from_str, string to_str) {

    Element *from = getElement(from_str);
    Element *to = getElement(to_str);

    string to_port = Element::getPort(to_str);
    string from_port = Element::getPort(from_str);

    // either string may specify the port
    // TODO: assert that they match if either has info
    if(to_port.empty()){
        to_port = from_port;
        to_str += to_port;
    }
    
    // remove outputs from parent
    from->removeOutput(to_str);
    from->removeOutputPointer(make_pair(to, to_port));

    // remove inputs from child
    to->removeInput(from->getId() + to_port);
}

/**
 * Adds a directed edge between two elements.
 */
void Automata::addEdge(Element* from, Element *to){

    from->addOutput(to->getId());

    // output pointers are paired with the output port
    // for two element pointers the output port is blank
    from->addOutputPointer(make_pair(to, ""));
    to->addInput(from->getId());
}

/**
 * Adds a directed edge between two elements. Either string input can define a connection to a specific Element port using the form "ElementID:port". 
 */
void Automata::addEdge(string from_str, string to_str) {

    Element *from = getElement(from_str);
    Element *to = getElement(to_str);

    string to_port = Element::getPort(to_str);
    string from_port = Element::getPort(from_str);

    // either string may specify the port
    // TODO: assert that they match if either has info
    if(to_port.empty()){
        to_port = from_port;
        to_str += to_port;
    }

    // add proper outputs to parent
    from->addOutput(to_str);
    // output pointers are paired with their ports
    from->addOutputPointer(make_pair(to, to_port));
    
    // add proper input to child as "fromId:toport
    to->addInput(from->getId() + to_port);
}


/**
 * Updates an element's string ID.
 */
void Automata::updateElementId(Element *el, string newId) {

    string oldId = el->getId();
    vector<string> children;
    vector<string> parents;

    // remove old outputs
    for(string output : el->getOutputs()) {
        removeEdge(el->getId(), output);
        // save child
        children.push_back(output);
    }

    // remove old inputs
    for(pair<string,bool> input : el->getInputs()){
        removeEdge(input.first, el->getId());
        // save parent
        parents.push_back(input.first);
    }

    // remove from data structures that have string->ptr maps
    if(el->isSpecialElement()){
        specialElements.erase(el->getId());
    }

    elements.erase(el->getId());

    // CHANGE ID
    el->setId(newId);

    // Re-insert element into proper automata data structures
    validateElement(el);
    
    // add back in all child edges
    for(string child : children){
        addEdge(el->getId(), child);
    }

    // add back in parent edges
    for(string parent : parents){
        addEdge(parent, el->getId());
    }
}


/**
 * Makes sure that the element is in (or out of) the data structure that tracks start elements.
 */
void Automata::validateStartElement(Element* el){

    // return if we're a specel
    if(el->isSpecialElement())
        return;

    STE *ste = static_cast<STE*>(el);
    
    if(ste->isStart()){
        // make sure we're in the array
        bool contains = (find(starts.begin(), starts.end(), ste) != starts.end());
        
        if(!contains){
            starts.push_back(ste);
        }

    }else{

        // make sure we're not in the array
        auto iter = find(starts.begin(), starts.end(), ste);
        bool contains = (iter != starts.end());
        
        if(contains){
            starts.erase(iter);
        }
    } 
}

/**
 * Makes sure that the element is in (or out of) the data structure that tracks reporting elements.
 */
void Automata::validateReportElement(Element* el){

    if(el->isReporting()){
        // make sure we're in the array
        bool contains = (find(reports.begin(), reports.end(), el) != reports.end());
        
        if(!contains){
            reports.push_back(el);
        }

    }else{

        // make sure we're not in the array
        auto iter = find(reports.begin(), reports.end(), el);
        bool contains = (iter != reports.end());
        
        if(contains){
            reports.erase(iter);
        }
    }
}

/**
 * Makes sure that the input element is in the proper Automata data structures given its internal properties.
 */
void Automata::validateElement(Element* el) {

    elements[el->getId()] = el;
    
    // make sure we're in the start array if we're a start and vice versa
    validateStartElement(el);
    
    // if we're a report, make sure we're in the report array
    validateReportElement(el);

    // make sure we're in the special element array with the right ID
    if(el->isSpecialElement())
        specialElements[el->getId()] = static_cast<SpecialElement*>(el);
}

/**
 * Returns an element
 */
Element *Automata::getElement(std::string elementId) {

    Element *el = elements[Element::stripPort(elementId)];

    if(el == NULL){
        setErrorCode(E_ELEMENT_NOT_FOUND);
        if(!quiet)
            cout << "WARNING: Element " << elementId << " was not found." << endl;
    }

    return el;
}

/**
 * Sets the status error code.
 */
void Automata::setErrorCode(vasim_err_t err) {

    error = err;
}

/**
 * Returns the current status error code.
 */
vasim_err_t Automata::getErrorCode() {

    return error;
}

/**
 * Unmarks all elements in the automata.
 */
void Automata::unmarkAllElements() {

    for(auto e : elements){
        e.second->unmark();
    }
}

/**
 * Removes elements that are unreachable or cannot result in a match.
 */
void Automata::eliminateDeadStates() {

    // if we can't reach a report state, we should be eliminated
    // for each state, check if we can reach a report state
    queue<Element *> toRemove;
    for(auto e : elements){

        // unmark all elements
        unmarkAllElements();
        
        Element *el = e.second;

        // Can we reach a report state from el?
        bool report_unreachable = true;
        
        // are we a report state?
        if(el->isReporting()) {
            el->mark();
            report_unreachable = false;
        }
        
        queue<Element *> workq;
        // push outputs to workq
        for(string out : el->getOutputs()){
            Element *output = getElement(out);
            if(output->isReporting()) {
                report_unreachable = false;
            }

            // push to queue
            if(!output->isMarked()) {
                output->mark();
                workq.push(output);
            }
        }

        // BFS and attempt to find a report state
        while(!workq.empty() && report_unreachable){

            Element *child = workq.front();
            workq.pop();
            
            for(string out : child->getOutputs()){
                Element *output = getElement(out);
                if(output->isReporting()) {
                    report_unreachable = false;
                }
                
                // push to queue
                if(!output->isMarked()) {
                    output->mark();
                    workq.push(output);
                }
            }
        }

        //
        if(report_unreachable){
            toRemove.push(el);
        }
    }

    // Remove all dead states from the automata
    while(!toRemove.empty()){
        Element *el = toRemove.front();
        toRemove.pop();
        removeElement(el);
    }


    // we also need to find elements that are unreachable from start states
    //   even if they can lead to reports

    // unmark all elements
    unmarkAllElements();

    // BFS all reachable states from the start states
    for(STE *el : getStarts()){

        el->mark();
        
        queue<Element *> workq;
        
        // push outputs to workq
        for(string out : el->getOutputs()){
            Element *output = getElement(out);

            // push to queue
            if(!output->isMarked()) {
                output->mark();
                workq.push(output);
            }
        }

        // BFS and attempt to find a report state
        while(!workq.empty()){

            Element *child = workq.front();
            workq.pop();
            
            for(string out : child->getOutputs()){
                Element *output = getElement(out);
                
                // push to queue
                if(!output->isMarked()) {
                    output->mark();
                    workq.push(output);
                }
            }
        }

    }

    // Remove all dead states from the automata
    for(auto e : elements){
        // if an element wasn't marked, delete it
        if(!e.second->isMarked()){
            toRemove.push(e.second);
        }
    }
    
    // remove all unmarked elements
    while(!toRemove.empty()){
        Element *el = toRemove.front();
        toRemove.pop();
        removeElement(el);
    }
}

/**
 * Redundant edges include those that connect to "all-input" start states.
 *   all input start states are enabled on every cycle and so incoming edges
 *   are always redundant.
 */
void Automata::removeRedundantEdges() {

    // remove any input edge to an all-input start state
    for(STE *ste: getStarts()){

        if(ste->getStringStart().compare("all-input") == 0){

            // remove all incoming edges
            for(auto in : ste->getInputs()){
                removeEdge(getElement(in.first), ste);
            }
        }
    }

    // add other instances here.
}

/**
 *
 */
void Automata::optimize(bool remove_ors,
                        bool left,
                        bool right,
                        bool common_path
                        ){

    // REMOVE OR GATES
    uint32_t removed_ors = 0;
    if(remove_ors) {
        if(!quiet)
            cout << " * Removing OR gates..." << endl;

        removed_ors = removeOrGates();

        if(!quiet)
            cout << "     removed " << removed_ors << " OR gates..." << endl;

    }


    // NFA REDUCTION ALGORITHMS
    uint32_t automata_size_total = 0;
    while(automata_size_total != elements.size()) {
        automata_size_total = elements.size();
        // PREFIX MERGING
        if(left) {
            if(!quiet) {
                cout << " * Merging common prefixes..." << endl;
            }
            
            uint32_t automata_size = 0;
            uint32_t merged = 0;
            while(automata_size != elements.size()) {
                automata_size = elements.size();
                
                // prefix merge call
                merged += mergeCommonPrefixes();
                
            }
            
            if(!quiet)
                cout << "     removed " << merged << " elements..." << endl;
            
        }
        
        // SUFFIX MERGING
        if(right) {
            if(!quiet) {
                cout << " * Merging common suffixes..." << endl;
            }
            
            uint32_t automata_size = 0;
            uint32_t merged = 0;
            while(automata_size != elements.size()) {
                automata_size = elements.size();
                
                // prefix merge call
                merged += mergeCommonSuffixes();
                
            }
            
            if(!quiet)
                cout << "     removed " << merged << " elements..." << endl;
            
        }
        
        if(common_path) {
            
            if(!quiet) {
                cout << " * Merging common paths..." << endl;
            }
            
            uint32_t automata_size = 0;
            uint32_t merged = 0;
            while(automata_size != elements.size()) {
                automata_size = elements.size();
                
                // common path merge call
                merged += mergeCommonPaths();
                
            }
            
            if(!quiet)
                cout << "     removed " << merged << " elements..." << endl;
            
        }
    }

    //
    
    if(!quiet)
        cout << endl;
}


/**
 * "Widens" an automata by padding with STEs that recognize zeroes
 *   this is often required for YARA malware automata.
 */
void Automata::widenAutomata() {

    queue<STE*> toWiden;
    
    //
    for(auto e : elements) {

        //
        if(e.second->isSpecialElement())
            continue;
        STE *ste = static_cast<STE*>(e.second);
        toWiden.push(ste);

    }

    while(!toWiden.empty()){

        STE *ste = toWiden.front();
        toWiden.pop();
        
        // build new STE
        string id = ste->getId() + "_widened";
        STE *pad = new STE(id, "[\\x00]", "none");
        
        // add all parents of ste as parents of pad
        queue<Element *> toRemove;
        for(auto e : ste->getOutputSTEPointers()) {
            addEdge(pad, e.first);
            toRemove.push(e.first);
        }
        
        for(auto e : ste->getOutputSpecelPointers()) {
            addEdge(pad, e.first);
            toRemove.push(e.first);
        }

        // remove all parents of ste
        while(!toRemove.empty()){
            removeEdge(ste, toRemove.front());
            toRemove.pop();
        }

        // add edge from ste to pad
        addEdge(ste, pad);

        // if STE was reporting, make pad report instead
        if(ste->isReporting()){
            ste->setReporting(false);
            pad->setReporting(true);
            pad->setReportCode(ste->getReportCode());
        }

        // add STE to automata
        rawAddSTE(pad);
    }

    // we modified the graph, so finalize data structures
    finalizeAutomata();
}

/**
 * 2-Stride Automata
 *  Striding converts the original automata to an equivalent automata that
 *  consumes two symbols per cycle, as a single symbol. This effectively 
 *  doubles the alphabet size. In VASim, this can only be applied when the
 *  alphabet is less than 2x 256. 2-striding can be applied sequentially to
 *  4-stride (in the case of 2-bit symbols) , or even 8-stride (in the case 
 *  of bit-level automata).
 */
Automata *Automata::twoStrideAutomata() {


    // Check if special elements exist
    for( auto e : getElements()) {
        if(e.second->isSpecialElement()) {
            cout << "WARNING: Could not stride automata because of special elements. In reality, we totally could, we just dont support it right now." << endl;
            exit(1);
        }
    }

    // What's the largest symbol we need? 
    uint32_t largest = 0;
    for( auto e : getElements()) {
        STE *ste = static_cast<STE*>(e.second);
        for(uint32_t i = 0; i < 256; i++){
            if(ste->match(i)){
                if(i > largest){
                    largest = i;
                }
            }
        }
    }
    
    if(largest > 127){
        cout << "WARNING: Could not 2-stride automata because symbols are too big." << endl;
        exit(1);
    }else{
        cout << "  Largest symbol used is: " << largest << endl;
    }
    
    // Identify what power of 2 is required to hold all symbols
    uint32_t bits_per_symbol;
    uint32_t num_symbols;
    for(uint32_t i = 0; i < 8; i++){
        uint32_t bits = pow(2, i);
        if(bits >= largest){
            //bits_per_symbol = bits;
            //num_symbols = pow(2, bits);
            bits_per_symbol = i;
            num_symbols = bits;
            break;
        }
    }
    
    cout << "  Automata requires " << bits_per_symbol << " bits per symbol. " << endl;
    cout << "  This means we can two stride to form " << bits_per_symbol * 2 << " bit symbols." << endl;

    // START STRIDING ALGORITHM
    
    // Unmark all elements
    unmarkAllElements();
    
    // Start striding
    Automata *strided_automata = new Automata();
    unordered_map<STE *,vector<STE*>> head_node_to_pair;
    unordered_map<STE *,vector<STE*>> pair_to_tail_node;
    queue<STE *> workq;

    // Push start states to work queue
    for( auto e : getElements()) {

        STE *s = static_cast<STE*>(e.second);

        if(s->isStart()) {
            s->mark();
            workq.push(s);
        }
    }

    //
    uint32_t id_counter = 0;
    
    bool warn_odd_length = false;
    
    // Iterate over all states in breadth first manner
    while(!workq.empty()){
        
        //
        STE *s1 = workq.front();
        workq.pop();

        //
        //cout << "Considering s1: " << s1->getId() << endl;
        
        // handle if there is an "odd length"
        // in other words, there is an unvisited STE but no children
        // We'll print a warning, too
        if(s1->getOutputSTEPointers().empty()) {
          warn_odd_length = true;
          
          // we just have to shift the charset
          string id;
          string charset ="";
          string start = "none";
          STE *new_ste = new STE("__" + to_string(id_counter++) + "__", charset, start);
          strided_automata->rawAddSTE(new_ste);
          
          new_ste->setReporting(s1->isReporting());
          new_ste->setReportCode(s1->getReportCode());
          new_ste->setStart(s1->getStart());
          
          for(uint32_t c1 = 0; c1 < num_symbols; c1++) {
              if(s1->match(c1)){
                  new_ste->addSymbolToSymbolSet(c1 << bits_per_symbol );
              }
          }
          
          // now that we have the new node map the original head node to it
          if(head_node_to_pair.find(s1) == head_node_to_pair.end()){
              vector<STE*> vec {};
              head_node_to_pair[s1] = vec;
          }
          
          // if the list doesn't have it
          vector<STE*> tmp = head_node_to_pair[s1];
          if(find(tmp.begin(), tmp.end(), new_ste) == tmp.end())
              head_node_to_pair[s1].push_back(new_ste);

        
          
        }
        
        // for each child node
        for(auto e : s1->getOutputSTEPointers()) {
            
            //
            STE *s2 = static_cast<STE*>(e.first);

            //cout << "Considering s1 child: " << s2->getId() << endl;
            
            // combine these states into a new node
            string id;
            string charset ="";
            string start = "none";
            STE *new_ste = new STE("__" + to_string(id_counter++) + "__", charset, start);
            strided_automata->rawAddSTE(new_ste);

            //
            //cout << "Combining : " << s1->getId() << " : " << s2->getId() << endl;
            
            if(s1->isReporting() || s2->isReporting()) {
                new_ste->setReporting(true);
                if(!s1->getReportCode().empty()){
                    new_ste->setReportCode(s1->getReportCode());
                }
                if(!s2->getReportCode().empty()){
                    new_ste->setReportCode(s2->getReportCode());
                }
            }

            if(s1->isStart())
                new_ste->setStart(s1->getStart());
            
            // get combined charset
            for(uint32_t c1 = 0; c1 < num_symbols; c1++) {
                if(s1->match(c1)){
                    for(uint32_t c2 = 0; c2 < num_symbols; c2++) {
                        if(s2->match(c2)){
                            new_ste->addSymbolToSymbolSet(c2 << bits_per_symbol | c1);
                        }
                    }
                }
            }

            // now that we have the new node map the original head node to it
            if(head_node_to_pair.find(s1) == head_node_to_pair.end()){
                vector<STE*> vec {};
                head_node_to_pair[s1] = vec;
            }
            // if the list doesn't have it
            vector<STE*> tmp = head_node_to_pair[s1];
            if(find(tmp.begin(), tmp.end(), new_ste) == tmp.end())
                head_node_to_pair[s1].push_back(new_ste);

            // map the pair to the second node
            if(pair_to_tail_node.find(new_ste) == pair_to_tail_node.end()){
                vector<STE*> vec {};
                pair_to_tail_node[new_ste] = vec;
            }
            // if the list doesn't have our key, push it
            tmp = pair_to_tail_node[new_ste];
            if(find(tmp.begin(), tmp.end(), s2) == tmp.end())
                pair_to_tail_node[new_ste].push_back(s2);
            
            // push the children of s2 to the queue
            for(auto next : s2->getOutputSTEPointers()){
                // only push if we haven't been seen yet!
                STE * next_ste = static_cast<STE*>(next.first);
                if(!next_ste->isMarked()){
                    next_ste->mark();
                    workq.push(next_ste);
                }
            }
        }
    }
    
    if (warn_odd_length) {
      cout << "  WARNING: potential odd length input. Be sure to pad!" << endl;
    }

    // Once we have all of the proper states
    // we make a second pass to construct correct edges
    for(auto e : strided_automata->getElements()) {

        STE *strided_parent = static_cast<STE*>(e.second);
        
        // get the 2nd node of each strided pair
        vector<STE*> tails = pair_to_tail_node[strided_parent];

        //
        for(STE * tail : tails) {
            // for each output of the second node, add an edge tail->pair[output]
            for(auto e2 : tail->getOutputSTEPointers()) {
                
                STE *head = static_cast<STE*>(e2.first);

                // get the strided pair from each 1st node
                vector<STE*> strided_children = head_node_to_pair[head];
                for(STE* strided_child : strided_children) {
                    strided_automata->addEdge(strided_parent, strided_child);
                }
            }
        }
    }
    
    return strided_automata;
    
}

/**
 * Removes counters from a design. Does not replace counters with STEs.
 */
void Automata::removeCounters(){

    queue<Element*> to_remove;
    
    // Find all counters
    for(auto e : getElements()){
        Element *el = e.second;
        if(dynamic_cast<Counter*>(el) != NULL){
            // now we've found a counter
            Counter *c = static_cast<Counter*>(el);

            // connect all inputs to outputs
            // yes, this includes both CNT and RST ports
            // unclear what the implications are for your design
            for(auto i : c->getInputs()){
                Element *in = getElement(i.first);
                for(string o : c->getOutputs()){
                    Element *out = getElement(o);
                    addEdge(in, out);
                    
                    // if it reports, make all parent STEs report
                    if(c->isReporting()){
                        in->setReporting(true);
                        in->setReportCode(c->getReportCode());
                    }
                }
            }

            // remove counter
            to_remove.push(c);
        }
    }

    while(!to_remove.empty()){
        removeElement(to_remove.front());
        to_remove.pop();
    }
}
