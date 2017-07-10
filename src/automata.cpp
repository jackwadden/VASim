#include "automata.h"
#include "util.h"

#ifndef DEBUG
#define DEBUG false
#endif

using namespace std;
using namespace MNRL;

static string getFileExt(const string& s) {

   size_t i = s.rfind('.', s.length());
   if (i != string::npos) {
      return(s.substr(i+1, s.length() - i));
   }

   return("");
}


/*
 *
 */
Automata::Automata() {

    // Disable profiling by default
    report = false;
    profile = false;
    quiet = false;
    cycle = 0;

    // debug
    dump_state = false;
    dump_state_cycle = 0;
}


/*
 *
 */
Automata::Automata(string filename): filename(filename), 
                                     cycle(0), 
                                     dump_state(false), 
                                     dump_state_cycle(0) {

    if(getFileExt(filename).compare("mnrl") == 0) {
        // Read in automata description from MNRL file
        MNRLAdapter parser(filename);
        parser.parse(elements, starts, reports, specialElements, &id, activateNoInputSpecialElements);
    } else {
        // Read in automata description from ANML file
        ANMLParser parser(filename);
        parser.parse(elements, starts, reports, specialElements, &id, activateNoInputSpecialElements);
    }

    // Disable report vector by default
    report = false;

    // Disable profiling by default
    profile = false;

    // Enable output by default
    quiet = false;

    // for all elements
    for(auto e : elements) {

        Element *el = e.second;

        // for all edges
        for(auto str : el->getOutputs()) {

            // inputs are of the form "fromNodeId:toPort"
            if(elements[Element::stripPort(str)] == NULL){
                cout << "COULD NOT FIND ELEMENT WITH NAME: " << Element::stripPort(str) << "  DURING PARSE." << endl;
                exit(1);
            }
            elements[Element::stripPort(str)]->addInput(el->getId() + Element::getPort(str));
        }
    }

    // once the basic data structure is created,
    // fill each elements outputPointers data structure so we don't
    // have to consult the global map when propagating signals
    for(auto e : elements) {
        Element *el = e.second;

        // for all edges
        for(string str : el->getOutputs()) { 
            Element * e = elements[Element::stripPort(str)];
            if(Element::getPort(str).empty()){
                pair<Element *, string> output(e, ""); 
                el->addOutputPointer(output);
            }else{
                pair<Element *, string> output(e, Element::getPort(str)); 
                el->addOutputPointer(output);
            }
        }
    }
}




/*
 * Disables and deactivates all elements in the automata
 */
void Automata::reset() {

    // deactivate and disable all elements
    for(auto ee : elements) {
        Element *e = ee.second;
        e->deactivate();
        e->disable();
        e->unmark();
    }

    // clear all functional maps
    STE *s;
    while(!enabledSTEs.empty())
        enabledSTEs.pop_back();

    //enabledSTEs.clear();
    while(!activatedSTEs.empty())
        activatedSTEs.pop_back();

    while(!enabledElements.empty())
        enabledElements.pop_back();

    while(!activatedSpecialElements.empty())
        activatedSpecialElements.pop();

    // clear report vector
    reports.clear();

    // reset cycle counter to be 0
    cycle = 0;
    
}

/*
 *
 */
void Automata::addSTE(STE *ste, vector<string> &outputs) {

    elements[ste->getId()] = static_cast<Element *>(ste);

    if(ste->isStart()){
        starts.push_back(ste);
    }

    if(ste->isReporting()){
        reports.push_back(ste);
    }

    // for all edges
    for(auto str : outputs) {

        ste->addOutput(str);

        // inputs are of the form "fromNodeId:toPort"
        elements[Element::stripPort(str)]->addInput(ste->getId() + Element::getPort(str));
    }
}

/*
 *
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

/*
 *
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



/*
 * Converts an automata with multiple start or end states into multiple
 *   automata with single start and single end states
 */
vector<Automata *> Automata::generateGNFAs(){

    //
    cout << "  GENERATING GNFAs..." << endl;

    cout << "  Original size: " << elements.size() << endl;

    // maps nodes to a list of start/end nodes
    unordered_map<string, vector<string>> start_markers;
    unordered_map<string, vector<string>> end_markers;

    // for each start node
    for(STE *start : starts){

        // label all reachable states using breadth first search
        unordered_map<string, bool> visited;
        for(auto el : elements){
            STE *ste = static_cast<STE*>(el.second); 
            visited[ste->getId()] = false;
        }

        queue<STE*> work_queue;
        work_queue.push(start);
        visited[start->getId()] = true;
        //
        while(!work_queue.empty()){
            STE *node = work_queue.front();
            work_queue.pop();

            //cout << "BFS considering: " << node->getId() << endl;

            //mark node with start id
            // if this is the first time we've seen this node
            if(start_markers.find(node->getId()) == start_markers.end()){
                vector<string> tmp;
                tmp.push_back(start->getId());
                start_markers[node->getId()] = tmp;
            }else{
                start_markers[node->getId()].push_back(start->getId());
            }

            // push children to work_queue if not visited
            for( auto el : node->getOutputSTEPointers()){
                STE *child = static_cast<STE*>(el.first);
                if(!visited[child->getId()]){
                    work_queue.push(child);
                    visited[child->getId()] = true;
                }
            }
        }
    }

    // find all end states
    vector<STE*> ends;
    for(auto el : elements){
        STE *ste = static_cast<STE*>(el.second); 
        if(ste->isReporting())
            ends.push_back(ste);
    }

    //for each accept/end state
    // label all backwards reachable states
    for(STE *end : ends){

        // label all reachable states using reverse breadth first search
        unordered_map<string, bool> visited;
        for(auto el : elements){
            STE *ste = static_cast<STE*>(el.second); 
            visited[ste->getId()] = false;
        }
        queue<STE*> work_queue;
        work_queue.push(end);
        visited[end->getId()] = true;
        //
        while(!work_queue.empty()){
            STE *node = work_queue.front();
            work_queue.pop();

            //mark node with end id
            // if this is the first time we've seen this node
            if(end_markers.find(node->getId()) == end_markers.end()){
                vector<string> tmp;
                tmp.push_back(end->getId());
                end_markers[node->getId()] = tmp;
            }else{
                end_markers[node->getId()].push_back(end->getId());
            }

            // push parents to work_queue if not visited
            for(auto e : node->getInputs()){
                string parent_id = e.first;
                STE *parent = static_cast<STE*>(elements[parent_id]);
                if(!visited[parent_id]){
                    work_queue.push(parent);
                    visited[parent_id] = true;
                }
            }
        }
    }

    // construct vector of automata for each [start,accept] pair
    automataToANMLFile("tmp.anml");

    vector<Automata *> GNFAs;
    unsigned int counter = 0;
    for(STE * start : starts) {
        for(STE * end: ends) {

            // clone original automata
            Automata *a = new Automata("tmp.anml");
            cout << "  Pruning nodes in automata of size: " << a->getElements().size() << endl;

            //remove all states that aren't reachable from both the start and end state
            queue<STE *> to_remove;
            for(auto el : a->getElements()){

                STE *ste = static_cast<STE*>(el.second);

                // if the STE is a start or end state that is not a part of our pair
                // remove this status
                if((ste->getId().compare(start->getId()) != 0) &&
                   ste->isStart()){
                    ste->setStart("none");
                }
                if((ste->getId().compare(end->getId()) != 0) &&
                   ste->isReporting()){
                    ste->setReporting(false);
                }

                vector<string> markers = start_markers[ste->getId()];
                if(find(markers.begin(), markers.end(), start->getId()) == markers.end()){
                    // remove STE
                    to_remove.push(ste);
                }else{
                    markers = end_markers[ste->getId()];
                    if(find(markers.begin(), markers.end(), end->getId()) == markers.end()){
                        to_remove.push(ste);
                    }else{
                        //Keep the node!
                    }
                }
            }

            while(!to_remove.empty()){
                STE * remove_this = to_remove.front();
                to_remove.pop();
                a->removeElement(remove_this);
            }

            cout << "    extracted automata with size: " << a->getElements().size() << endl;

            GNFAs.push_back(a);
            //a->automataToDotFile("automata_" + to_string(counter) + ".dot");
            //a->validate();
            counter++;
        }
    }

    remove("tmp.anml");

    // return vector of GNFAs
    return GNFAs;
}


/*
 * Adds all outputs of ste2 to ste1
 * then removes ste2 from the automata.
 */
void Automata::leftMergeSTEs(STE *ste1, STE *ste2) {


    // for all output edges in ste2;
    // 1) add outputs to ste1
    // 2) adjust inputs of output target nodes to reflect ste1, not ste2
    for(string str : ste2->getOutputs()) {
        
        // add outputs from ste2 to outputs list of ste1
        if(Element::stripPort(str).compare(ste2->getId()) != 0){
            ste1->addOutput(str);
            Element * e = elements[Element::stripPort(str)];
            // we may have already removed this
            if( e != NULL ){
                pair<Element *, string> output(e, Element::getPort(str)); 
                ste1->addOutputPointer(output);
            }
            
            // adjust inputs of output to reflect new parent
            // inputs are of the form "fromNodeId:toPort"
            // remove old input to child from ste2            
            
            string port = Element::getPort(str);
            //if(!port.empty())
            //port = ":" + port;
            Element * e2 = elements[Element::stripPort(str)]; 
            if(e2 != NULL){
                e2->removeInput(ste2->getId() + port);
                // add new input to child from ste1
                e2->addInput(ste1->getId() + port);
            }
        }
    }
    
    // for all input edges; 
    // 1) remove the output to ste2 from the parent node if it exists
    //    output to ste1 must necessarily exist
    for(auto e : ste2->getInputs()) {
        
        // if the input node is not ourself
        if(Element::stripPort(e.first).compare(ste2->getId()) != 0) {
            
            // get parent node if it exists
            Element * parent = elements[Element::stripPort(e.first)];
            
            string port = Element::getPort(e.first);
            if(!port.empty())
                port = ":" + port;
            parent->removeOutput(ste2->getId() + port);
            pair<Element *, string> output(ste2, ste2->getId() + port); 
            parent->removeOutputPointer(output);
        }
    }
    
    // SANITY CHECK IF WE STILL EXIST
    for(auto e : ste2->getInputs()) {
        if(Element::stripPort(e.first).compare(ste2->getId()) != 0) {

            // get parent node if it exists
            Element * parent = elements[Element::stripPort(e.first)];
            for(string e2 : parent->getOutputs()) {
                if(e2.compare(ste2->getId()) == 0)
                    cout << "WAS NOT REMOVED" << endl;
            }
        }
    }

    // remove merged ste from global maps
    removeElement(ste2);
}

/*
 * Returns a vector of every separate connected component in the 
 *   automaton. Does not delete the original automata.
 */
vector<Automata*> Automata::splitConnectedComponents() {

    // unmark all elements
    for(auto e : getElements()){
        e.second->unmark();
    }

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
                Element * to_add = elements[Element::stripPort(input.first)];

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
        if(quiet){
            a->enableQuiet();
        }

        if(profile){
            a->enableProfile();
        }

        if(report){
            a->enableReport();
        }
        
        if(dump_state){
            a->enableDumpState(dump_state_cycle);
        }
    }

    // return vector
    return connectedComponents;
}

/*
 * Merges two automata but does not make sure
 *   that the two automata don't have name clashes.
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

/*
 * Clones current automata
 */
Automata *Automata::clone() {

    Automata *ap = new Automata();

    // Create empty automata and merge us into it
    ap->unsafeMerge(this);

    if(profile)
        ap->enableProfile();

    if(quiet)
        ap->enableQuiet();

    if(report)
        ap->enableReport();

    if(dump_state)
        ap->enableDumpState(dump_state_cycle);

    return ap;
}


/*
 * Removes STE from the global element map
 * TODO:: this works, but probably leaves some traces behind
 * does not destroy object
 */
void Automata::removeElement(Element *el) {

    //cout << "removing element: " << el->getId() << endl;
    //cout << el->toString() << endl;

    // remove traces from output elements
    for(auto output : el->getOutputSTEPointers()){
        output.first->removeInput(el->getId());
    }

    for(auto output : el->getOutputSpecelPointers()){
        output.first->removeInput(el->getId());
    }

    // remove traces from input elements
    for(auto in : el->getInputs()){
        Element *input = elements[Element::stripPort(in.first)];
        input->removeOutput(el->getId());
        input->removeOutputPointer(make_pair(el, el->getId()));
    }

    // clear input data structures
    el->clearInputs();
    el->clearOutputs();
    el->clearOutputPointers();

    // clear automata data structures
    if(el->isReporting()){
        reports.erase(find(reports.begin(), reports.end(), el));
    }

    if(el->isSpecialElement()){
        specialElements.erase(specialElements.find(el->getId()));
    }else{
        STE * ste = static_cast<STE*>(el);
        if(ste->isStart()){
            starts.erase(find(starts.begin(), starts.end(), ste));
        }
    }

    unordered_map<string, Element*>::iterator it = elements.find(el->getId());
    if(it != elements.end())
        elements.erase(elements.find(el->getId()));

    //  delete el;
}

/*
 *
 */
vector<STE *> &Automata::getStarts() {

    return starts;
}

/*
 *
 */
vector<Element *> &Automata::getReports() {

    return reports;
}

/*
 *
 */
std::vector<std::pair<uint64_t, std::string>> &Automata::getReportVector() {

    return reportVector;
}

/*
 *
 */
unordered_map<string, Element *> &Automata::getElements() {

    return elements;
}

/*
 *
 */
unordered_map<string, SpecialElement *> &Automata::getSpecialElements() {

    return specialElements;
}

/*
 *
 */
Stack<Element *> &Automata::getEnabledElements() {

    return enabledElements;
}

/*
 *
 */
Stack<Element *> &Automata::getEnabledSTEs() {

    return enabledSTEs;
}

/*
 *
 */
Stack<STE *> &Automata::getActivatedSTEs() {

    return activatedSTEs;
}


/*
 *
 */
unordered_map<string, uint32_t> &Automata::getActivationHist() {

    return activationHist;
}

/*
 *
 */
uint32_t Automata::getMaxActivations() {

    return maxActivations;
}

/*
 *
 */
void Automata::enableProfile() {

    profile = true;

    // If we're profiling, map STEs to a counter for each state
    if(profile){
	for(auto e : elements) {
            enabledCount[e.second] = 0;
            activatedCount[e.second] = 0;
        }
    }
}

/*
 *
 */
void Automata::enableReport() {
    report = true;
}

/*
 *
 */
void Automata::enableQuiet() {
    quiet = true;
}

/*
 *
 */
void Automata::enableDumpState(uint64_t dump_cycle) {
    dump_state = true;
    dump_state_cycle = dump_cycle;
}

/*
 *
 */
void Automata::disableProfile() {
    profile = false;
}

/*
 *
 */
void Automata::writeStringToFile(string str, string fn) {

    std::ofstream out(fn);
    out << str;
    out.close();
}

/*
 *
 */
void Automata::appendStringToFile(string str, string fn) {

    std::ofstream out(fn, ios::out | ios::app);
    out << str;
    out.close();
}

/*
 *
 */
void Automata::writeIntVectorToFile(vector<uint32_t> &vec, string fn) {

    std::ofstream out(fn);
    for(uint32_t val : vec)
        out << val << endl;
    out.close();
}

/*
 *
 */
void Automata::print() {

    cout << "NUMBER OF ELEMENTS: " << elements.size() << endl;

    for(auto e: elements) {
        cout << e.second->toString() << endl;
    }
}

/*
 *
 */
void Automata::simulate(uint8_t symbol) {

    if(DEBUG)
        cout << "CONSUMING INPUT: " << symbol << " @cycle: " << cycle << endl;

    // -----------------------------
    // Step 1: if STEs are enabled and we match, activate
    computeSTEMatches(symbol);
    // -----------------------------
    
    // Activation Statistics
    if(profile){
        profileActivations();
    }
    
    if(dump_state && (dump_state_cycle == cycle)){
        dumpSTEState("stes_" + to_string(cycle) + ".state");
    }

    // -----------------------------
    // Step 2: enable children of matching STEs
    enableSTEMatchingChildren();
    // -----------------------------

    // -----------------------------
    // Step 3:  enable start states
    enableStartStates();
    // -----------------------------

    // -----------------------------
    // Step 4: special element computation
    if(specialElements.size() > 0){        
        specialElementSimulation();
    }
    // -----------------------------

    // Enabled Statistics
    if(profile){
        profileEnables();
    }
    
    // advance cycle count
    tick();
}

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

void Automata::initializeSimulation() {
    
    // Initiate simulation by enabling all start states
    enableStartStates();

    //
    if(profile)
        profileEnables();
    
}

/*
 *
 */
void Automata::simulate(uint8_t *inputs, uint64_t start_index, uint64_t length, bool step) {

    if(DEBUG)
        cout << "STARTING SIMULATION..." << endl;

    cycle = start_index;

    // primes all data structures for simulation
    initializeSimulation();
    
    // for all inputs
    for(uint64_t i = start_index; i < start_index + length; i = i + 1) {

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

        // if we are stepping, wait for a key to be pressed
        if(step)
            getchar();

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

/*
 *
 */
void Automata::printReport() {

    // print report vector
    //cout << "REPORTS:" << endl;
    for(pair<uint32_t,string> s : reportVector) {
        // cycle space colon space ID
        cout << s.first << " : " << s.second << " : " << elements[s.second]->getReportCode() << endl;
    }
}

/*
 *
 */
void Automata::writeReportToFile(string fn) {

    std::ofstream out(fn);
    string str;
    for(pair<uint64_t,string> s : reportVector) {
        str += to_string(s.first) + " : " + s.second + " : " + elements[s.second]->getReportCode() + "\n";
    }
    out << str;
    out.close();
}


/*
 *
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

/*
 *
 */
void Automata::printActivations() {

    // print activation vector
    cout << "ACTIVATIONS:" << endl;
    for(auto s: activationVector) {
        list<string> l = s.second;
        cout << s.first << "::" << endl;
        for(auto e: l) {
            cout << "\t" << e << endl;
        }
    }
}

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

/*
 *
 */
unordered_map<Element*, uint32_t> &Automata::getEnabledCount() {

    return enabledCount;
}

/*
 *
 */
unordered_map<Element*, uint32_t> &Automata::getActivatedCount() {

    return activatedCount;
}


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

/*
 *
 */
void Automata::defrag() {

    vector<pair<uint32_t, string>> sortedEls;

    for(auto e : activationHist) {
        sortedEls.push_back(make_pair(e.second, e.first));
    }

    // sort based on activation
    sort(sortedEls.rbegin(), sortedEls.rend());

    // emit as anml file
    automataToANMLFile("automata_defrag.anml");

}

/*
 *
 */
string Automata::activationHistogramToString() {

    string str = "";
    for(auto s: activationHist) {
        str += s.first + "\t" + to_string(s.second) + "\n";
    }

    return str;
}


/*
 *
 */
void Automata::printActivationHistogram() {

    // print histogram vector
    cout << "ACTIVATIONS HISTOGRAM:" << endl;
    cout << activationHistogramToString();
}

/*
 * Takes an ID and returns a red color proportional to the number
 * of total activations of this element out of the max number of
 * activations in the automata.
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

/*
 * Takes an ID and returns a red color proportional to the number
 * of total activations of this element out of the max number of
 * activations in the automata.
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

/*
 * 
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

/*
 *
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


        // heatmap color
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

        for(auto to : elements[Element::stripPort(e.first)]->getOutputs()) {
            str += to_string(from) + " -> " + to_string(id_map[Element::stripPort(to)]) + ";\n";
        }
    }

    str += "}\n";

    writeStringToFile(str, out_fn);
}

/*
 * Removes OR gates from the automata. OR gates are syntactic sugar introduced
 *  by Micron's optimizing compiler. Other automata engines may not support
 *  them, so we allow their removal.
 * TODO:: THIS IS NOT SAFE
 */
void Automata::removeOrGates() {

    // for each special element that is an OR gate
    queue<Element *> toRemove;
    for(auto el : specialElements) { 
        SpecialElement * specel = el.second;

        // if we're not an OR gate, continue
        if(dynamic_cast<OR*>(specel) == NULL) {
            //cout << specel->toString() << endl;
            continue;

        }

        // FOR NOW ONLY REMOVE IF WE DONT HAVE CHILDREN
        // TODO
        if(specel->getOutputs().size() > 0)
            continue;

        // if it reports
        if(specel->isReporting()){

            //// make all of its parents report with the same ID
            for(auto e : specel->getInputs()){
                Element *parent = elements[Element::stripPort(e.first)];
                parent->setReporting(true);
                parent->setReportCode(specel->getReportCode());
                // remove or gate from outputs
                parent->removeOutput(specel->getId());
                parent->removeOutputPointer(make_pair(specel, specel->getId()));
            }

            //// mark OR gate for removal from the automata
            toRemove.push(specel);
        }

        // else if it does not report

        //// for each input element

        ////// for each output element

        //////// connect input to output

        //////// remove OR gate from the automata
    }

    // remove OR gates from the automata
    while(!toRemove.empty()) { 
        removeElement(toRemove.front());
        toRemove.pop();
    }
}

/*
 * Removes Counters from the automata. Counters can sometimes be replaced by 
 *  an equivalent number of matching elements. TODO.
 */
void Automata::removeCounters() {

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
            input = static_cast<STE*>(elements[Element::stripPort(in.first)]);

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

            vector<string> does_nothing;
            addSTE(input_next, does_nothing);

            // march along
            input_prev = input_next;
        }

        // add outputs of counter to outputs of last node
        for(string out : counter->getOutputs()){
            input_prev->addOutput(out);
            input_prev->addOutputPointer(make_pair(elements[out], out));
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

/*
 * Converts the automata to a theoretical NFA style
 *   readable by Michela Becchi's NFA/DFA/HFA engine
 */
void Automata::automataToNFAFile(string out_fn) {

    // This only works on automata that do not have special elements
    for(auto e : elements){
        if(e.second->isSpecialElement()){
            cout << "VASim Error: Automata network contains special elements unsupported by other NFA tools. Please attempt to remove redundant Special Elements using -x option." << endl;
            exit(1);
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
            STE * ste = dynamic_cast<STE *>(elements[s]);

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
        STE * ste = dynamic_cast<STE *>(elements[id]);

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
            STE * ste_to = dynamic_cast<STE *>(elements[s]);
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

/*
 * Outputs automata to ANML file
 *  meant to be called after optimization passes
 */
void Automata::automataToANMLFile(string out_fn) {

    string str = "";

    // xml header
    str += "<anml version=\"1.0\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\n";
    str += "<automata-network id=\"vasim\">\n";

    for(auto el : elements) {
        str += el.second->toANML();
        str += "\n";
    }

    // xml footer
    str += "</automata-network>\n";
    str += "</anml>\n";

    // write NFA to file
    writeStringToFile(str, out_fn);
}

/*
 * Outputs automata to MNRL file
 * Meant to be called after optimization passes
 */
void Automata::automataToMNRLFile(string out_fn) {
    MNRLNetwork net("vasim");
    
    // add all the elements
    for(auto el : elements) {
        net.addNode(el.second->toMNRLObj());
    }
    
    // add all the connections
    for(auto el : elements) {
        for(auto dst : el.second->getOutputs()) {
            // We're going to make some assumptions here
            
            string dst_port = Element::getPort(dst);
            
            net.addConnection(
                el.second->getId(),// src id
                MNRLDefs::H_STATE_OUTPUT,// src port
                Element::stripPort(dst),// dest id
                dst_port.compare("") == 0 ? MNRLDefs::H_STATE_INPUT : dst_port// dest port
            );
        }
        
    }
    
    // write the net to a file
    net.exportToFile(out_fn);
    
}

/*
 * Outputs automata to Verilog HDL description following the algorithm
 *   originally developed by Xiaoping Huang and Mohamed El-Hadedy
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
                //cout << "START OF DATA STE NOT SUPPORTED YET!" << endl;
                //exit(1);
           
    
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

/*
 * Outputs automata to .blif circuit description file.
 *  Currently works for STE-only designs.
 *  Reporting architecture assumes 2 static ports per row.
 */
void Automata::automataToBLIFFile(string out_fn) {

    string str = "";

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
        }

        // Fill rest of inputs with unconn dummy nets
        for(int i = 0; i < 16; i++){
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
    for(int i = 0; i < 16; i++)
        str += "enable[" + to_string(i) + "] " ;
    str += "clock\n";
    str += ".outputs active\n";
    str += ".blackbox\n";
    str += ".end\n";
    str += "\n";

    writeStringToFile(str, out_fn);
}


/*
 *
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

/*
 * Converts all-input start elements to "start-of-data"
 *   preserves functionality by installing self referencing star states
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


/*
 *
 */
set<STE*>* Automata::follow(uint32_t character, set<STE*>* current_dfa_state) {


    set<STE*>* follow_set = new set<STE*>;

    // for each all-input start
    for(STE *start : getStarts()){
        if(start->match((uint8_t)character)){
            // add to follow_set
            //cout << start->getId() << " matched on input " << endl; 
            follow_set->insert(start);
        }
    }
    
    // for each STE in the DFA state
    for(STE *ste : *current_dfa_state){
        // for each child
        for(auto e : ste->getOutputSTEPointers()){
            STE * child = static_cast<STE*>(e.first);
            // if they match this character
            if(child->match((uint8_t)character)){
                // add to follow_set
                follow_set->insert(child);
            }
        }
    }

    return follow_set;
}


/*
 *
 */
Automata* Automata::generateDFA() {

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


/*************************************
 *            Simulation             *
 *************************************/

/*
 *
 */
void Automata::enableStartStates() {

    if(DEBUG)
        cout << "STAGE ONE:" << endl;

    //for each start element
    for(STE * s: starts) {

        // Enable if start is "all input"
        // or if it's the first cycle, enable start-of-data STEs
        if(s->startIsAllInput() ||         
           (cycle == 0 && s->startIsStartOfData())) {

            if(DEBUG)
                cout << "SETTING ENABLE: " << s->toString() << endl;

            // add to enabled queue if we were not already enabled
            if(!s->isEnabled()){
                s->enable();
                enabledSTEs.push_back(static_cast<Element *>(s));
            }
        }
    }

}

/*
 * If an STE is enabled and matches on the current input, activate.
 * If we're a reporter, report. 
 */
void Automata::computeSTEMatches(uint8_t symbol) {

    if(DEBUG)
        cout << "STAGE TWO:" << endl;


    //for each enabled ste
    while(!enabledSTEs.empty()) {

        STE * s = static_cast<STE *>(enabledSTEs.back());

        if(DEBUG)
            cout << s->getId() << " IS ENABLED. CHECKING FOR MATCH WITH INPUT: \"" << symbol << "\"" << endl;

        // if we match on the input character
        // the STE will activate and we record this
        // ste should also report
        if(s->match(symbol)) {

            if(DEBUG)
                cout << "MATCHED!" << endl;


            //activate and push to queue only if we werent already
            if(!s->isActivated()) {
                s->activate();
                activatedSTEs.push_back(s);
            }

            if(profile)
                activationVector[cycle].push_back(s->getId());

            // report
            if(report && s->isReporting()) {

                reportVector.push_back(make_pair(cycle, s->getId()));
            }

            if(DEBUG)
                cout << s->getId() << " ACTIVATED" << endl;
        }

        //disable 
        s->disable();

        // remove STE from the queue
        enabledSTEs.pop_back();        
    }
}

/*
 * Propagate enable signal of active STEs to all other elements
 */
void Automata::enableSTEMatchingChildren() {

    if(DEBUG)
        cout << "STAGE THREE:" << endl;

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


/*
 *
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

    // fill work_q with specel children of STEs
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
            if(!calculated[elements[Element::stripPort(in.first)]->getIntId()]){
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
                    if(DEBUG)
                        cout << "\tSPECEL REPORTING: " << spel->getId() << endl;
                    
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


/*
 * Tick advances the cycle count representing an automata "symbol cycle"
 */
uint64_t Automata::tick() {

    return cycle++;
}


/*************************************
 *          Optimizations            *
 *************************************/

bool ste_sort(STE a, STE b) {

    return a.compare(&b);
}

void vector_sort(vector<STE*> &vec) {

    STE * swap;
    unsigned int c,d;
    unsigned int n = vec.size();

    for (c = 0 ; c < ( n - 1 ); c++){
        for (d = 0 ; d < n - c - 1; d++){
            if (vec[d]->compare(vec[d+1])) {
                swap       = vec[d];
                vec[d]   = vec[d+1];
                vec[d+1] = swap;
            }
        }
    }
}


/*
 * depth first search on the automata, combining states with identical
 * inputs and properties, but varying outputs. Essentially creates a tree
 * structure where possible.
 */
uint32_t Automata::leftMinimize() {


    leftMinimizeStartStates();

    uint32_t merged = 0;

    if(!quiet)
        cout << "  Merging inner states..." << endl;

    for(auto e : starts){

        // unmark all elements
        for(auto e : getElements()){
            e.second->unmark();
        }

        STE * s = static_cast<STE *>(e);
        merged += leftMinimizeChildren(s, 0);
    }
    cout << "    merged " << merged << " inner states..." << endl;

    return merged;
}

/*
 * Recursive call that merges the current child level, and calls left minimization
 *  on every subsequent unique child in a depth-first fashion.
 */
uint32_t Automata::leftMinimizeChildren(STE * s, int level) {

    queue<STE *> workq;
    queue<STE *> workq_tmp;

    vector<STE *> outputSTE;

    uint32_t merged = 0;

    // add all children to the workq
    for(auto e : s->getOutputSTEPointers()) {
        STE * node = static_cast<STE*>(e.first);
        if(!node->isMarked()) {
            node->mark();
            workq.push(node);	
        }
    }

    // merge identical children of next level
    while(!workq.empty()) { 
        STE * first = workq.front();
        workq.pop();
        while(!workq.empty()) {
            STE * second = workq.front();
            workq.pop();
            //if the same merge and place into second queue

            if(first->compare(second) == 0) {
                merged++;
                leftMergeSTEs(first, second);
                //else push back onto workq
            } else {
                workq_tmp.push(second);
            }	 
        }

        outputSTE.push_back(first);

        while(!workq_tmp.empty()) {
            workq.push(workq_tmp.front());
            workq_tmp.pop();
        }
    }

    // left minimize children
    for(auto e : outputSTE) {
        if(e->getOutputSTEPointers().size() != 0) {
            for(auto f : s->getOutputSTEPointers()) {
                STE * node = static_cast<STE*>(f.first);
                merged += leftMinimizeChildren(node, level + 1);
            }
        }
    }

    return merged;
}

/*
 *
 *
 *
 */
void Automata::leftMinimizeStartStates() {

    if(!quiet)
        cout << "  Merging start states..." << endl;

    uint32_t merge_count = 0;

    // unmark all elements
    for(auto e : getElements()){
        e.second->unmark();
    }

    queue<STE *> workq;
    queue<STE *> workq_tmp;

    //push all start STE's onto the queue
    for(auto e : starts) {
        STE * s = static_cast<STE*>(e);
        workq.push(s);
    }

    // Merge start states
    while(!workq.empty()) { 

        STE * first = workq.front();
        bitset<256> first_column = first->getBitColumn();
        workq.pop();

        while(!workq.empty()) {
            STE * second = workq.front();
            workq.pop();
            
            //if the same, merge and place into second queue
            if(first_column == second->getBitColumn() && 
               first->getStart() == second->getStart()){ 
                // merge
                merge_count++;
                leftMergeSTEs(first, second);
            } else {
                workq_tmp.push(second);
            }	 
        }

        while(!workq_tmp.empty()) {
            workq.push(workq_tmp.front());
            workq_tmp.pop();
        }
    }

    if(!quiet)
        cout << "    merged " << merge_count << " start states!" << endl;
}

/*
 *
 */
void Automata::validate() {


    for(auto e : elements) {

        Element *el = e.second;

        // check inputs
        for(auto ins : el->getInputs()){
            // does my input exist?
            if(elements[Element::stripPort(ins.first)] == NULL){
                cout << "FAILED INPUTS EXISTANCE TEST!" << endl;
                cout << "  " << Element::stripPort(ins.first) << " input of element: " << e.first << " does not exist in the element map." << endl;
                exit(1);
            }

            Element * parent = elements[Element::stripPort(ins.first)];

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
                exit(1);
            }
        }


        // check outputs
        for(string output_tmp : el->getOutputs()){

            string output = Element::stripPort(output_tmp);

            // does my output exist in the map?
            if(elements[output] == NULL){
                cout << "FAILED OUTPUTS TEST!" << endl;
                cout << "  " << output << " output of element: " << e.first << " does not exist in the element map." << endl;
                exit(1);
            }

            Element * child = elements[output];

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
                exit(1);
            }

        }

    }

    automataToDotFile("failed_verification.dot");

    //cout << "PASSED VERIFICATION!" << endl;

}

/*
 *
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

/*
 *
 */
void Automata::printGraphStats() {


    cout << "Automata Statistics" << endl;
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


/*
 * Adds all inputs of ste2 to ste1
 * then removes ste2 from the automata
 */
void Automata::rightMergeSTEs(STE *ste1, STE *ste2){

    // for all input edges in ste2
    // 1) add inputs to ste1
    // 2) adjust outputs of input target node to reflect ste1, not ste2
    for(auto e : ste2->getInputs()) {
        string input_str = e.first;
        // add inputs from ste2 to inputs list of ste1
        Element * input_el;
        if(Element::stripPort(input_str).compare(ste2->getId()) != 0){
            ste1->addInput(input_str);
            input_el = elements[Element::stripPort(input_str)];
        }else{
            continue;
        }

        // adjust outputs of input to reflect new child
        // outputs are of the form "toNodeId:toPort"
        // remove old output from parent from ste2
        string port = Element::getPort(input_str);
        if(!port.empty())
            port = ":" + port;
        //
        if( input_el != NULL) {

            // remove old outputs 
            input_el->removeOutput(ste2->getId() + port);
            input_el->removeOutputPointer(make_pair(ste2, ste2->getId() + port));
        
            // add new outputs if it doesn't exist already
            // NOT SURE WHY THIS IS NECESSARY
            // note: removing before adding is a way to guarantee there is only one copy
            //       in the outputs list. This was a bug that occured on Fermi.1chip.
            //       Again, not sure where the bug is. TODO/FIXME.
            input_el->removeOutput(ste1->getId() + port);
            input_el->addOutput(ste1->getId() + port);
            pair<Element *, string> output = make_pair(ste1,ste1->getId() + port);
            input_el->removeOutputPointer(output);
            input_el->addOutputPointer(output);
        }
    }

    removeElement(ste2);
}

/*
 * Guarantees that the fan-in for every node does not exceed fanin_max
 */
void Automata::enforceFanIn(uint32_t fanin_max){
    
    // BFS queue of elements to process 
    queue<STE*> workq;

    // push all start states to workq
    for(auto el : getElements()){ 

        // unmark all elements
        el.second->unmark();

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

            if(DEBUG){
                cout << "FAN-IN MAX VIOLATION: " << s->getId() << " has fan-in of " << fanin << endl;
            }
                
            // adjust node
            // figure out how many new nodes we'll need
            uint32_t new_nodes = ceil((double)fanin / (double)fanin_max);

            if(DEBUG){
                cout << "  will be split into " << new_nodes << " new nodes..." << endl;
            }
            
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
                        Element *to = elements[output];
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
                    Element *from = elements[old_inputs.front()];
                    addEdge(from ,new_node);

                    old_inputs.pop();
                    input_counter++;
                }

                // if the split node is a self looping node, make new node self looping
                if(selfref){
                    if(DEBUG){
                        cout << "SELF REF" << endl;
                    }
                    addEdge(new_node, new_node);
                }

                //
                if(DEBUG){
                    cout << "NEW NODE FANIN: " << new_node->getInputs().size() << endl;
                }

            }
            
            // delete old node
            removeElement(s);
        }
    }
}

/*
 * Guarantees that the fan-in for every node does not exceed fanin_max
 */
void Automata::enforceFanOut(uint32_t fanout_max){

    // BFS queue of elements to process 
    queue<STE*> workq;

    // unmark all stes
    // push report states to workq
    for(auto el : getElements()){ 
        el.second->unmark();
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
            Element *el = elements[in.first];
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
                        Element *from = elements[input];
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
                    addEdge(new_node, elements[old_outputs.front()]);

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


/*
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

/*
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

/*
 *
 */
void Automata::removeEdge(Element* from, Element *to){

    from->removeOutput(to->getId());
    from->removeOutputPointer(make_pair(to, to->getId()));
    to->removeInput(from->getId());
}

/*
 *
 */
void Automata::addEdge(Element* from, Element *to){

    from->addOutput(to->getId());
    from->addOutputPointer(make_pair(to, to->getId()));
    to->addInput(from->getId());
}
