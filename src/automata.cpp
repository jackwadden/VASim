#include "automata.h"
#include "util.h"

#ifndef DEBUG
#define DEBUG false
#endif

using namespace std;


/*
 *
 */
Automata::Automata() {

    // Disable profiling by default
    report = false;
    profile = false;
    quiet = false;
    cycle = 0;

}


/*
 *
 */
Automata::Automata(string filename): filename(filename), cycle(0) {


    // Read in automata description from ANML file
    ANMLParser parser(filename);
    parser.parse(elements, starts, reports, specialElements, &id, activateNoInputSpecialElements);

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
 *
 */
void Automata::cutElement(Element *e ) {

    // set as cut
    e->setCut(true);
    // make sure parents are reporting
    for(auto it : e->getInputs()) {
        elements[it.first]->setReporting(true);
    }
}

/*
 *
 */
bool Automata::isTailNode(Element *e) {

    bool tail = true;
    if(e->isCut()) {
        return false;
    }

    if(e->getOutputs().size() == 0) {
        return true;
    }

    for(string out : e->getOutputs()) {
        //if our output isn't cut, and it's not us
        // we are not a tail node
        if(!elements[out]->isCut() && (out.compare(e->getId()) != 0))
            return false;
    }

    return tail;

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

    // SIMULATION STATS SHOULD NOT BE DELETED

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
std::vector<std::pair<uint32_t, std::string>> &Automata::getReportVector() {

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

void Automata::simulate(uint8_t symbol) {

    input = symbol;

    if(DEBUG)
        cout << "CONSUMING INPUT: " << input << " @cycle: " << cycle << endl;

    // PARALLEL STAGE 1
    // WRITING
    // enable start states
    stageOne();

    // CYCLE STATISTICS
    if(profile)
        enabledHist.push_back(enabledSTEs.size());

    // PARALLEL STAGE 2
    // READING
    // if STEs are enabled and we match, activate
    stageTwo();

    if(profile)
        activatedHist.push_back(activatedSTEs.size());

    // PARALLEL STAGE 3
    // WRITING
    // propagate activation to other STEs, logic, and counters
    stageThree();

    // SPECIAL ELEMENT STAGES
    if(specialElements.size() > 0){
        // PARALLEL STAGE 4
        // READING
        // calculate logic and counter functions
        stageFour();

        // PARALLEL STAGE 5
        // WRITING
        // propagate logic and counter activations to other STEs, logic, and counters
        stageFive();            
    }

    tick();
}


/*
 *
 */
void Automata::simulate(uint8_t *inputs, uint32_t start_index, uint32_t length, bool step) {

    if(DEBUG)
        cout << "STARTING SIMULATION..." << endl;

    cycle = start_index;

    // for all inputs
    for(int i = start_index; i < start_index + length; ++i) {

        // measure progress on longer runs
        if(!quiet) {

            if(i % 10000 == 0) {
                if(i != 0) {
                    cout << "\x1B[2K"; // Erase the entire current line.
                    cout << "\x1B[0E";  // Move to the beginning of the current line.
                }
                cout << "  Progress: " << i << "/" << length << "\r";
                flush(cout);
                //
            }
        }

        simulate(inputs[i]);

        /*
          if(!quiet){
          if(i == length-1){
          cout << endl;
          }
          }
        */

        // if we are stepping, wait for a key to be pressed
        if(step)
            getchar();

    }

    if(!quiet) {
        cout << "\x1B[2K"; // Erase the entire current line.
        cout << "\x1B[0E";  // Move to the beginning of the current line.

        cout << "  Progress: " << length << "/" << length << "\r";
        flush(cout);
        cout << endl;
    }


    if(profile) {

        // cal average active set
        uint64_t sum = 0;
        for(uint32_t acts : activatedHist){
            sum += (uint64_t)acts;
        }

        cout << "Average Active Set: " << (double)sum / (double)length << endl;

        //write
        buildActivationHistogram("activation_hist.out");        
        writeIntVectorToFile(enabledHist, "enabled_per_cycle.out");
        writeIntVectorToFile(activatedHist, "activated_per_cycle.out");
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
    for(pair<uint32_t,string> s : reportVector) {
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
        uint cycle = s.first + 1;
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

    unordered_map<string, int> id_map;
    unordered_map<string, bool> marked;
    queue<string> to_process;
    uint state_counter = 0;
    uint accept_counter = 1;

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

bool determineUniquenessVector(set<STE*>* potential_set, vector<set<STE*>*>  all_sets) {

    bool unique = true;
    if(all_sets.empty()) {
        return unique;
    }
    for(int i = 0 ; i < all_sets.size(); i++) {
        bool matches = true;

        // if new candidate state set is not the same size
        if(potential_set->size() != all_sets[i]->size()) {
            matches = false;
            //compare each individual element and see if they match
        } else {
            set<STE*>::iterator it;
            set<STE*>::iterator it2 = potential_set->begin();
            for(it = all_sets[i]->begin(); it != all_sets[i]->end(); it++) {
                if((*it)->getIntId() != (*it2)->getIntId()) {
                    matches = false;
                    break;
                }
                it2++;
            }
        }
        if(matches){
            unique = false;
            break;
        }
    }
    return unique;
}

/*
 *
 */
bool determineUniquenessMap(set<STE*>* potential_set, unordered_map<STE*, set<STE*>*> all_sets) {
    bool is_equal = true;
    if(all_sets.empty()) {
        return is_equal;
    }
    unordered_map<STE*, set<STE*>*>::iterator it;
    for(it = all_sets.begin(); it != all_sets.end(); it++) {
        bool is_equal2 = true;
        if(it->second->size() != potential_set->size()) {
            is_equal2 = false;
        } else {
            set<STE*>::iterator it2;
            set<STE*>::iterator it3 = it->second->begin();
            for(it2 = potential_set->begin(); it2 != potential_set->end(); it2++) {
                //if((*it2)->getIntId() != (*it3)->getIntId()) {
                if((*it2) != (*it3)) {
                    is_equal2 = false;
                    break;
                }
                it3++;
            }
        }
        if(is_equal2) {
            is_equal = false;
            break;
        }
    }
    return is_equal;
}

/*
 *
 */
STE* findMatchedSet(set<STE*>* set_to_be_found, unordered_map<STE*, set<STE*>*> all_sets) {
    unordered_map<STE*, set<STE*>*>::iterator it;
    bool is_equal = true;
    for(it = all_sets.begin(); it != all_sets.end(); it++) {
        bool is_equal2 = true;
        if(it->second->size() != set_to_be_found->size()) {
            is_equal2 = false;
        } else {
            set<STE*>::iterator it2;
            set<STE*>::iterator it3 = it->second->begin();
            for(it2 = set_to_be_found->begin(); it2 != set_to_be_found->end(); it2++) {
                if((*it2)->getIntId() != (*it3)->getIntId()) {
                    is_equal2 = false;
                    break;
                }
                it3++;
            }
        }
        if(is_equal2) {
            is_equal = false;
            break;
        }
    }
    if(!is_equal) 
        return (*it).first;
    else
        return NULL;
}

/*
 *
 */
inline set<STE*>* Automata::follow(uint32_t character, set<STE*>* dstate) {

    set<STE*>* potential_dfa = new set<STE*>;

    if(dstate == NULL) {
        for(STE *s : getStarts()){
            s->follow(character, potential_dfa);
        }
    } else {

        set<STE*>::iterator it;
        for(it = dstate->begin(); it != dstate->end(); it++) {
            (*it)->follow(character, potential_dfa);
        }
    }

    return potential_dfa;
}

/*
 *
 */
inline bool fncomp(set<STE*>* lhs, set<STE*>* rhs) {

    if(lhs->size() != rhs->size()) 
        return (lhs->size() < rhs->size());

    set<STE*>::iterator it;
    set<STE*>::iterator it2 = rhs->begin();
    for(it = lhs->begin(); it != lhs->end(); it++) {
        if((void*)(*it) != (void*)(*it2)) {
            return ((void*)(*it)) < ((void*)(*it2));
        }
        it2++;
    }

    return false;
}

/*
 * NFA to DFA conversion algorithm based on subset_construction
 * keeps homogeneity of NFA when DFA is created
 */
Automata* Automata::generateDFA() {

    Automata* ap = new Automata();
    uint32_t unique_ids = 0; //id of newly created dfa
    convertAllInputStarts(); // method to add star state
    vector<uint32_t> alphabet; //character set
    queue<STE*> new_dfa_stes; //queue of new dfa states
    queue<set<STE*>*> workq; //queue of new dfa sets
    bool(*fn_pt)(set<STE*>*, set<STE*>*) = fncomp; //comparison method for logarithmic search
    set<set<STE*>*,  bool(*)(set<STE*>*, set<STE*>*)> all_dfa_sets (fn_pt); // final vector of dfa sets
    unordered_map<set<STE*>*, STE*> reverse_ste_set; //map of dfa sets to their STEs
    set<set<STE*>*>::iterator dfa_it;//iterator through the set of new dfa states

    for(uint32_t i = 0; i < 256; i++) 
        alphabet.push_back(i);

    //checks to see if starts match on a given character in alphabet
    for(uint32_t c : alphabet) {
        set<STE*>* potential_dfa = follow(c, NULL);
        bool is_in;

        if(all_dfa_sets.empty()) {
            is_in = false;
        } else {
            dfa_it = all_dfa_sets.find(potential_dfa);
            is_in = (dfa_it != all_dfa_sets.end());
        }
        //if the dfa set is not already created then create the dfa set and add it to all_dfa_sets
        if(!is_in) {
            all_dfa_sets.insert(potential_dfa);
            string dfa_state_name = to_string(unique_ids);
            STE * s = new STE(dfa_state_name, "", "start-of-data"); 
            s->setIntId(unique_ids);
            s->addSymbolToSymbolSet(c);
            unique_ids++;
            for(set<STE*>::iterator it = potential_dfa->begin(); it != potential_dfa->end(); it++) {
                if((*it)->isReporting() == true) {
                    s->setReporting(true);
                    break;
                }
            }
            ap->rawAddSTE(s);
            new_dfa_stes.push(s);
            reverse_ste_set[potential_dfa] = s;
        }

        // if it does exist, grab the set and add my character to its charset
        else {	
            delete potential_dfa;
            set<STE*> *tmp = (*dfa_it);
            STE* s = reverse_ste_set[tmp];
            s->addSymbolToSymbolSet(c);
        }
    }

    //push all sets to queue to loop through for all_dfa_sets children
    for(set<STE*>* e : all_dfa_sets) {
        workq.push(e);
    }

    //while there are still potential dfa sets to be created
    while(!workq.empty()) {
        set<STE*>* d_state = workq.front();
        workq.pop();
        STE* popped = new_dfa_stes.front();
        new_dfa_stes.pop();

        //check every character in the alphabet
        for(uint32_t c : alphabet) {
            //find all the output stes that follow that specific character return as a set
            set<STE*>* pot_DFA = follow(c, d_state);

            dfa_it = all_dfa_sets.find(pot_DFA);
            bool is_in = (dfa_it != all_dfa_sets.end());

            if(!is_in) {
                string dfa_state_name = to_string(unique_ids);
                STE * s = new STE(dfa_state_name, "", ""); 
                s->setIntId(unique_ids);
                s->addSymbolToSymbolSet(c);
                unique_ids++;
                for(set<STE*>::iterator it = pot_DFA->begin(); it != pot_DFA->end(); it++) {
                    if((*it)->isReporting() == true) {
                        s->setReporting(true);
                        break;
                    }
                }

                popped->addOutput(s->getId());
                popped->addOutputPointer(make_pair(s, s->getId()));
                reverse_ste_set[pot_DFA] = s;
                ap->rawAddSTE(s);
                workq.push(pot_DFA);
                new_dfa_stes.push(s);
                all_dfa_sets.insert(pot_DFA);

            } else {
                delete pot_DFA;
                bool check = false;
                STE * f = reverse_ste_set[(*dfa_it)];
                f->addSymbolToSymbolSet(c);

                //check if output edge already exists
                for(auto e : popped->getOutputSTEPointers()) {
                    STE* s = static_cast<STE*>(e.first);    
                    if(f == s)  {
                        check = true;
                        break;
                    }
                }

                //if no edge exists then create an edge 
                if(!check) {
                    popped->addOutput(f->getId());
                    popped->addOutputPointer(make_pair(f, f->getId()));
                }

            }
        }
        //use as a way to test if the code is hanging
        //cout<< "Total DFA size " << all_dfa_sets.size() <<endl;
    }

    ap->addInputToDFA(); //add corresponding inputs to match output
    return ap;
}

/*
 *
 */
Automata* Automata::generateDFA_Linear() {

    Automata* ap = new Automata();
    uint32_t unique_ids = 0; //id of newly created dfa
    convertAllInputStarts(); // method to add star state
    vector<uint32_t> alphabet; //character set
    queue<STE*> new_dfa_stes; //queue of new dfa states
    queue<set<STE*>*> workq;
    vector<set<STE*>*> all_dfa_sets; // final vector of dfa sets
    unordered_map<STE*, set<STE*>*> ste_set; //map of dfa and corresponding set of nfa stes

    for(uint32_t i = 0; i < 256; i++) 
        alphabet.push_back(i);

    //checks to see if starts match on a given character in alphabet
    for(uint32_t c : alphabet) {
        set<STE*>* potential_dfa = follow(c, NULL);
        if(determineUniquenessVector(potential_dfa, all_dfa_sets)) {
            all_dfa_sets.push_back(potential_dfa);
            string dfa_state_name = to_string(unique_ids);
            STE * s = new STE(dfa_state_name, "", "start-of-data"); 
            s->setIntId(unique_ids);
            s->addSymbolToSymbolSet(c);
            unique_ids++;
            for(set<STE*>::iterator it = potential_dfa->begin(); it != potential_dfa->end(); it++) {
                if((*it)->isReporting() == true) {
                    s->setReporting(true);
                    break;
                }
            }
            ap->rawAddSTE(s);
            new_dfa_stes.push(s);
            ste_set.insert({s, potential_dfa});
        }

        // if it does exist, grab it and add my character to its charset
        else {	
            if(!determineUniquenessMap(potential_dfa, ste_set)) {
                STE* s = findMatchedSet(potential_dfa, ste_set);
                s->addSymbolToSymbolSet(c);
            }
        }
    }

    for(set<STE*>* e : all_dfa_sets) {
        workq.push(e);
    }
    while(!workq.empty()) {
        set<STE*>* d_state = workq.front();
        workq.pop();
        STE* popped = new_dfa_stes.front();
        new_dfa_stes.pop();
        //check every character in the alphabet
        for(uint32_t c : alphabet) {
            //find all the output stes that follow that specific character return as a set
            set<STE*>* pot_DFA = follow(c, d_state);
            if(determineUniquenessVector(pot_DFA, all_dfa_sets)) {
                string dfa_state_name = to_string(unique_ids);
                STE * s = new STE(dfa_state_name, "", ""); 
                s->setIntId(unique_ids);
                s->addSymbolToSymbolSet(c);
                unique_ids++;
                for(set<STE*>::iterator it = pot_DFA->begin(); it != pot_DFA->end(); it++) {
                    if((*it)->isReporting() == true) {
                        s->setReporting(true);
                        break;
                    }
                }
                popped->addOutput(s->getId());
                popped->addOutputPointer(make_pair(s, s->getId()));
                ste_set.insert({s, pot_DFA});
                ap->rawAddSTE(s);
                workq.push(pot_DFA);
                new_dfa_stes.push(s);
                all_dfa_sets.push_back(pot_DFA);
            }
            else {
                if(!determineUniquenessMap(pot_DFA, ste_set)) {
                    bool check = false;
                    STE* f = findMatchedSet(pot_DFA, ste_set);
                    f->addSymbolToSymbolSet(c);
                    for(auto e : popped->getOutputSTEPointers()) {
                        STE* s = static_cast<STE*>(e.first);
                        if(f->getIntId() == 14)    
                            cout << f->getIntId() << " " << s->getIntId() << endl;
                        if(f->getIntId() == s->getIntId())
                            {
                                check = true;
                                break;
                            }
                    }
                    //if no edge exists then create an edge and add the character to the ste
                    if(!check) {	
                        popped->addOutput(f->getId());
                        popped->addOutputPointer(make_pair(f, f->getId()));
                    }
                }
            }
        }
        //cout<< "Total DFA size " << all_dfa_sets.size() <<endl;
    }
    ap->addInputToDFA();
    return ap;


}

/*
 *
 */
void Automata::addInputToDFA() {	

    // for all elements
    for(auto e : elements) {

        Element *el = e.second;

        // for all edges
        for(auto str : el->getOutputs()) {

            //inputs are of the form "fromNodeId:toPort"
            if(elements[Element::stripPort(str)] == NULL){
                cout << "COULD NOT FIND ELEMENT WITH NAME: " << Element::stripPort(str) << "  DURING PARSE." << endl;
                exit(1);
            }
            elements[Element::stripPort(str)]->addInput(el->getId() + Element::getPort(str));
        }
    }
}


/*************************************
 *            Simulation             *
 *************************************/

/*
 *
 */
inline void Automata::stageOne() {

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
inline void Automata::stageTwo() {

    if(DEBUG)
        cout << "STAGE TWO:" << endl;


    //for each enabled ste
    while(!enabledSTEs.empty()) {

        STE * s = static_cast<STE *>(enabledSTEs.back());

        if(DEBUG)
            cout << s->getId() << " IS ENABLED. CHECKING FOR MATCH WITH INPUT: \"" << input << "\"" << endl;

        // if we match on the input character
        // the STE will activate and we record this
        // ste should also report
        //if(s->match(input)) {
        if(s->match2(input)) {

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
inline void Automata::stageThree() {

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
 * Calculate logic and counter functions.
 * Special elements report if they go high. 
 */
inline void Automata::stageFour() {
    
    if(DEBUG)
        cout << "STAGE FOUR:" << endl;
    
    
    // CONSIDER ELEMENTS THAT DONE NECESSARILY NEED ENABLE INPUTS
    if(activateNoInputSpecialElements.size() > 0){
        // for each special element that must be considered even if not enabled
        for(SpecialElement *specel : activateNoInputSpecialElements){
            
            if(!specel->isEnabled()){
                enabledSpecialElements.push(specel);
            }
        }
    }
    
    // for all enabled special elements
    while(!enabledSpecialElements.empty()){
        
        //SpecialElement *spel = e.second;
        SpecialElement *spel = static_cast<SpecialElement*>(enabledSpecialElements.front());
        
        if(DEBUG)
            cout << "CONSIDERING SPECIAL ELEMENT: " << spel->toString() << endl;
        
        // calulate underlying special function
        bool emitOutput = spel->calculate();
        if(DEBUG)
            cout << "CALCULATED: " << emitOutput << endl;
        
        // add to high elements list
        if(emitOutput) {
            
            // activate only if we weren't already activated
            if(!spel->isActivated()) {
                spel->activate();
                activatedSpecialElements.push(spel);
            }
            
            if(DEBUG)
                cout << "SPECIAL ELEMENT ACTIVATED: " << spel->getId() << endl;
            
            // report
            if(report && spel->isReporting()) {
                if(DEBUG)
                    cout << "\tSPECEL REPORTING: " << spel->getId() << endl;
                
                reportVector.push_back(make_pair(cycle, spel->getId()));
            }
        }
        
        // disable
        spel->disable();
        
        // remove Specel from the queue
        enabledSpecialElements.pop();
    }
    
}

/*
 * Propagate special element activations to other STEs and special elements
 *  Returns the number of special elements enabled. If zero, stage six can
 *  skipped. For now, stage six does not exist and we don't consider
 *  concatenated logic.
 */
inline uint32_t Automata::stageFive() {
    
    if(DEBUG)
        cout << "STAGE FIVE:" << endl;
    
    // keep track of specels enabled in this stage
    uint32_t numEnabledSpecEls = 0;
    
    while(!activatedSpecialElements.empty()) {
        
        SpecialElement *spel = activatedSpecialElements.front();
        activatedSpecialElements.pop();

        if(DEBUG)
            cout << "ACTIVATED SPECIAL: " << spel->getId() << endl;

        spel->enableChildSTEs(&enabledSTEs);    
        numEnabledSpecEls += spel->enableChildSpecialElements(&enabledSpecialElements);    

        // suggest that the logic deactivate
        if(!spel->deactivate()) {
            // store for later stages
            latchedSpecialElements.push_back(spel);
        }
    }

    // refil activated elements
    while(!latchedSpecialElements.empty()) {
        activatedSpecialElements.push(latchedSpecialElements.back());
        latchedSpecialElements.pop_back();
    }

    return numEnabledSpecEls;
}

/*
 * If we enabled any special elements, activate them and loop to stage 5
 */
/*
  void Automata::stageSix() {

  if(DEBUG)
  cout << "STAGE SIX:" << endl;

  // for all special elements that have at least one enabled port
  while(!enabledSpecialElements.empty()) {

  SpecialElement *spel = static_cast<SpecialElement*>(enabledSpecialElements.front());
  enabledSpecialElements.pop();

  if(DEBUG)
  cout << "CONSIDERING SPECIAL ELEMENT: " << spel->toString() << endl;

  // calulate underlying special function
  bool emitOutput = spel->calculate();
  if(DEBUG)
  cout << "CALCULATED: " << emitOutput << endl;

  // add to high elements list
  if(emitOutput) {

  // activate
  if(!spel->isActivated()) {
  spel->activate();
  activatedSpecialElements.push(spel);
  }
  if(DEBUG)
  cout << "SPECIAL ELEMENT ACTIVATED: " << spel->getId() << endl;

  // report
  if(report && spel->isReporting()) {
  if(DEBUG)
  cout << "\tSPECEL REPORTING: " << spel->getId() << endl;

  reportVector.push_back(make_pair(cycle, spel->getId()));
  }
  }

  spel->disable();
  }    
  }
*/

/*
 *
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
 * If automata is DNA read with only 4 bp,
 *  can combine STEs to recognize 4 bytes at once
 */
/*
  void Automata::strideDNA(uint32_t stride_len) {

  //TODO
  */

/*
 * Breadth first seach on the automata, combining states with identical
 * inputs and properties, but varying outputs. Essentially creates a tree
 * structure where possible.
 * Right now this is an N^2 algorithm, but could be made Nlog(N) by applying
 * a merge sort like divide and conquer to the workq foreach loop
 */
void Automata::leftMinimize(uint32_t max_level) {

    int orig_size = elements.size();
    if(!quiet){
        cout << "  Original size: " << orig_size << endl;
        cout << "  Applying left minimization..." << endl;
    }

    // keeps track of visited elements
    unordered_map<uint32_t, bool> visited;
    unordered_map<uint32_t, bool> merged;
    // maps STE IDs to a unique bucket associated with a symbol set
    unordered_map<uint32_t, uint32_t> symbol_set_map;

    /*
     * Gather all STEs into buckets according to their symbol set.
     *  This allows the algorithm to only compare STEs that have
     *  the same symbol set, greatly improving runtime for larger
     *  automata.
     */

    //symbol sets seen
    // each index in the vector is a unique symbol set
    // to a particular bucket
    vector<STE *> symbol_sets_seen;

    for(auto e : elements){

        // don't merge specels for now
        if(e.second->isSpecialElement())
            continue;

        STE * s = static_cast<STE*>(e.second);

        visited[s->getIntId()] = false;
        merged[s->getIntId()] = false;

        //see if we've seen this symbol set before
        int counter = -1;
        for(int i = 0; i < symbol_sets_seen.size(); i++){
            if(s->compareSymbolSet(symbol_sets_seen[i]) == 0){
                counter = i;
                break;
            }
        }

        //if we have seen it, add it to the symbol set map
        if(counter < 0) {
            symbol_sets_seen.push_back(s);
            symbol_set_map[s->getIntId()] = symbol_sets_seen.size()-1;
        }else{
            symbol_set_map[s->getIntId()] = counter;
        }

    }

    if(!quiet)
        cout << "  Found " << symbol_sets_seen.size() << " unique symbol sets..." << endl;

    /*
     * Do a breadth first search from start STEs. 
     *  If two STEs have identical properties but their 
     *  output edges are different, merge them into a single STE
     */
    //workq_map maps symbol set buckets to workqs
    //each location in the vector is a bucket
    vector<list<STE *>> workq_map;
    vector<list<STE *>> next_workq_map;

    //initialize empty lists
    for(int i = 0; i < symbol_sets_seen.size(); i++){
        list<STE *> workq;
        workq_map.push_back(workq);
        list<STE *> workq2;
        next_workq_map.push_back(workq2);
    }

    // Start BFS by filling the queue with start STEs
    // and "visiting" them
    for(Element *el: starts) {
        if(!el->isSpecialElement()){
            STE *s = static_cast<STE*>(el);
            // mark before adding to queue
            // ADD
            visited[s->getIntId()] = true;
            workq_map[symbol_set_map[s->getIntId()]].push_back(s);
        }
    }

    int level = 0;

    bool work_left = false;
    for(int i = 0; i < workq_map.size(); i++){
        if(workq_map[i].size() > 0){
            work_left = true;
            break;
        }
    }

    // Global invariant: do we have any STEs left that are
    //  candidates for merging?
    while(work_left) {

        if(!quiet) {
            cout << "  Merging level " << level << "\r";
            flush(cout);
        }

        // for every symbol set bucket
        for(int i = 0; i < workq_map.size(); i++){

            // for every element in the bucket workq
            while(!workq_map[i].empty()){
                STE *el = workq_map[i].back();    

                // remove from queue
                workq_map[i].pop_back();

                if(merged[el->getIntId()] == true)
                    continue;

                // add all outputs from candidate to next level
                // ignore specels for now
                for(auto e : el->getOutputSTEPointers()) {
                    // if we haven't visited it yet
                    if(visited[e.first->getIntId()] == false){
                        STE *s = static_cast<STE*>(e.first);
                        // "visit these"
                        visited[s->getIntId()] = true;
                        next_workq_map[symbol_set_map[s->getIntId()]].push_back(s);
                    }
                }

                // Compare this STE to all others in the workq
                //  merging if they are "left identical"
                // store the rest in a tmp workq to restore later
                list<STE *> workq_tmp;
                while(!workq_map[i].empty()){
                    STE *el2 = workq_map[i].back();
                    workq_map[i].pop_back();    

                    // ignore elements that are us,
                    //  or that we've visited
                    /* WADDEN
                       if(visited[el2->getIntId()] == true)
                       continue;
                    */
                    if(el->getId().compare(el2->getId()) == 0){
                        visited[el2->getIntId()] = true;
                        continue;
                    }

                    if(el->compare(el2) == 0) {

                        // add all outputs from candidate merges
                        //  to the next level queue
                        for(auto e : el2->getOutputSTEPointers()) {
                            // if we haven't visited it yet
                            // and it's not a special element
                            if(visited[e.first->getIntId()] == false){
                                STE *s = static_cast<STE*>(e.first);
                                // ADD
                                visited[s->getIntId()] = true;
                                next_workq_map[symbol_set_map[s->getIntId()]].push_back(s);
                            }
                        }

                        // Merge the left identical states
                        //cout << "MERGING: " << endl;
                        //cout << el->toString() << endl;
                        //cout << el2->toString() << endl;
                        leftMergeSTEs(el, el2);
                        merged[el2->getIntId()] = true;

                        if(elements.find(el2->getId()) != elements.end())
                            cout << "NOT REMOVED" << endl;

                    }else{
                        // consider on next round
                        workq_tmp.push_back(el2);
                    }
                }

                //reset workq with saved elements
                while(!workq_tmp.empty()){
                    workq_map[i].push_back(workq_tmp.back());
                    workq_tmp.pop_back();
                }

            }
        }

        level++;

        if(level > max_level)
            break;

        // copy next level queue to current workq map
        for(int i = 0; i < next_workq_map.size(); i++){
            while(!next_workq_map[i].empty()){
                workq_map[i].push_back(next_workq_map[i].back());
                next_workq_map[i].pop_back();
            }
        }    

        // check the invariant
        work_left = false;        
        for(int i = 0; i < workq_map.size(); i++){
            if(!workq_map[i].empty()){
                work_left = true;
                break;
            }
        }

        if(!quiet){
            if(work_left) {
                cout << "\x1B[2K"; // Erase the entire current line.
                cout << "\x1B[0E";  // Move to the beginning of the current line.
                flush(cout);
            }else{
                cout << endl;
                flush(cout);
            }
        }
    }

    if(!quiet)
        cout << "  Final size: " << elements.size() << "/" << orig_size << endl;

}

/*
 * depth first search on the automata, combining states with identical
 * inputs and properties, but varying outputs. Essentially creates a tree
 * structure where possible.
 */
void Automata::leftMinimize2() {


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

    //pop two and compare
    while(!workq.empty()) { 

        STE * first = workq.front();
        workq.pop();

        while(!workq.empty()) {
            STE * second = workq.front();
            workq.pop();
            //if the same merge and place into second queue

            if(first->compare(second) == 0) {
                leftMergeSTEs(first, second);
                //if the same merge and place into second queue
            } else {
                workq_tmp.push(second);
            }	 
        }

        while(!workq_tmp.empty()) {
            workq.push(workq_tmp.front());
            workq_tmp.pop();
        }
    }

    for(auto e : starts){
        STE * s = static_cast<STE *>(e);
        leftMinimizeChildren(s, 0);
    }
}

/*
 * Recursive call that merges the current child level, and calls left minimization
 *  on every subsequent unique child in a depth-first fashion.
 */
void Automata::leftMinimizeChildren(STE * s, int level) {

    queue<STE *> workq;
    queue<STE *> workq_tmp;

    vector<STE *> outputSTE;

    for(auto e : s->getOutputSTEPointers()) {
        STE * node = static_cast<STE*>(e.first);
        if(!node->isMarked()) {
            node->mark();
            workq.push(node);	
        }
    }

    //pop two and compare
    while(!workq.empty()) { 
        STE * first = workq.front();
        workq.pop();
        while(!workq.empty()) {
            STE * second = workq.front();
            workq.pop();
            //if the same merge and place into second queue

            if(first->compare(second) == 0) {
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

    for(auto e : outputSTE) {
        if(e->getOutputSTEPointers().size() != 0) {
            for(auto f : s->getOutputSTEPointers()) {
                STE * node = static_cast<STE*>(f.first);
                leftMinimizeChildren(node, level + 1);
            }
        }
    }
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
    uint64_t sum_in = 0;
    for(auto el : elements){
        sum_out += el.second->getOutputs().size();
        sum_in += el.second->getInputs().size();
    }


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

