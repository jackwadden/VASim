/**
 * @file
 */
#include "element.h"
#include "ste.h"

using namespace std;

/*
 *
 */
Element::Element(string id) : id(id), 
                              reporting(false), 
                              activated(false),
                              enabled(false),
                              marked(false){

}

/*
 *
 *
Element::Element(const Element &old) {

    //Element
    outputs = old.outputs;
    inputs = old.inputs;
    id = old.id;
    int_id = old.int_id;
    reporting = old.reporting;
    report_code = old.report_code;
    activated = old.activated;
}
*/

/*
 *
 */
Element::~Element(){

}

/*
 *
 */
bool Element::setId(string s) {
    
    id = s;

    return true;
}

/*
 *
 */
bool Element::setIntId(uint32_t i) {

    int_id = i;

    return true;
}


/*
 *
 */
bool Element::setReporting(bool b) {
    
    reporting = b;

    return true;
}

/*
 *
 */
bool Element::isReporting() {

    return reporting;
}

/*
 *
 */
bool Element::setReportCode(string s) {

    report_code = s;
    return true;
}

/*
 *
 */
string Element::getReportCode() {

    return report_code;
}


/*
 *
 */
void Element::activate() {

    activated = true;
}

/*
 * 
 */
bool Element::deactivate() {

    activated = false;
    return true;
}


/*
 *
 */
vector<string> Element::getOutputs() {

    return outputs;
}

/*
 *
 */
vector<pair<Element *, string>> Element::getOutputSTEPointers() {

    return outputSTEPointers;
}

/*
 *
 */
vector<pair<Element *, string>> Element::getOutputSpecelPointers() {

    return outputSpecelPointers;
}

/*
 *
 */
map<string, bool> Element::getInputs() {

    return inputs;
}

/*
 *
 */
bool Element::addOutput(string s) {

    // should be idempotent (cannot add edges twice)
    for(string output : outputs){
        if(output.compare(s) == 0){
            return false;
        }
    }

    outputs.push_back(s);
    return true;
}

/*
 *
 */
bool Element::addOutputPointer(pair<Element *, string> el) {

    if(el.first->isSpecialElement()){
        // check if it already exists
        for(auto e : outputSpecelPointers){
            if(e.first == el.first)
                return false;
        }
        
        outputSpecelPointers.push_back(el);
    }else{
        // check if it already exists
        for(auto e : outputSTEPointers){
            if(e.first == el.first)
                return false;
        }
        
        outputSTEPointers.push_back(el);
    }
    return true;
}

/*
 *
 */
bool Element::addOutputExisting(string s, map<string, Element *> elements) {
    
    addOutput(s);
    pair<Element *, string> p(elements[s], s);
    addOutputPointer(p);

    return true;
}

/*
 *
 */
bool Element::removeOutputExisting(string s, map<string, Element*> elements){

    removeOutput(s);
    pair<Element *, string> p(elements[s], s);
    removeOutputPointer(p);

    return true;
}

/*
 *
 */
bool Element::removeOutput(string s) {

    bool retval = true;

    vector<string>::iterator result = find(outputs.begin(), outputs.end(), s);

    while (result != outputs.end()){
        if(*result == s)
            result = outputs.erase(result);
        else
            result++;
    }

    return retval;
}

/*
 *
 */
bool Element::removeOutputPointer(pair<Element *,string> p) {

    bool retval = false;
    bool found = false;

    pair<Element *, string> to_remove;

    int position = 0;
    if(p.first->isSpecialElement()){
        for(pair<Element *, string> el : outputSpecelPointers) {
            if(el.first->getId().compare(p.first->getId()) == 0){
                found = true;
                break;
                // DOESNT WORK... WHY? TODO THIS IS A BUG IN HOW WE STORE PORTS
                /*
                  if(el.second.compare(p.second) == 0){
                  
                  }
                */
            }
            position++;
        }
        
        if(found){     
            outputSpecelPointers.at(position) = outputSpecelPointers.back();
            outputSpecelPointers.pop_back();
        }
    }else{
        for(pair<Element *, string> el : outputSTEPointers) {
            if(el.first->getId().compare(p.first->getId()) == 0){
                found = true;
                break;
                // DOESNT WORK... WHY? TODO THIS IS A BUG IN HOW WE STORE PORTS
                /*
                  if(el.second.compare(p.second) == 0){
                  
                  }
                */
            }
            position++;
        }
        
        if(found){     
            outputSTEPointers.at(position) = outputSTEPointers.back();
            outputSTEPointers.pop_back();
        }
    }
    
    return found;
}

bool Element::clearOutputs() {

    outputs.clear();
    return true;
}

bool Element::clearOutputPointers() {

    outputSTEPointers.clear();
    outputSpecelPointers.clear();
    return true;
}

bool Element::clearInputs() {

    inputs.clear();
}

/*
 *
 */
bool Element::addInput(string s) {
    
    inputs[s] = false;
    return true;
}

/*
 *
 */
bool Element::removeInput(string s) {

    bool retval = true;

    uint32_t size = inputs.size();

    int num_entries = 0;

    num_entries = inputs.erase(s);

    /*
    if(size == inputs.size()) {
        cout << "SIZE MATCH AFTER ERASE" << endl;
        cout << "attempted to erase: " << s << endl;

    }
    */
    if(num_entries == 0)
        retval = false;
    return retval;
}


/*
 *
 */
string Element::stripPort(string in) {


    size_t found = in.find(":");

    string result;

    if(found != string::npos) {
        result = in.substr(0, found);
    } else {
        result = in;
    }

    return result;
}

/*
 *
 */
string Element::getPort(string in) {


    size_t found = in.find(":");

    string result;

    if(found != string::npos) {
        result = in.substr(found, in.size());
    } else {
        result = "";
    }

    return result;
}

/*
 *
 */
//void Element::enableChildSTEs(vector<Element *> *enabledSTEs) {
void Element::enableChildSTEs(Stack<Element*> *enabledSTEs) {
    
    for(const pair<Element *, string> e : outputSTEPointers) {

        Element * child = e.first;
        
        // only enable if not previously enabled
        if(!child->isEnabled()){
            enabledSTEs->push_back(child);
            static_cast<STE *>(child)->enable();        
        }
    }
}

/*
 * Returns the number of specels enabled
 */
uint32_t Element::enableChildSpecialElements(queue<Element *> *enabledSpecialEls) {
    
    uint32_t numEnabledSpecEls = 0;

    for(auto e : outputSpecelPointers) {
        Element * child = e.first;
        
        // consider all special elements
        numEnabledSpecEls++;

        // adds enable signal as "fromElementId:toPort"
        child->enable(getId() + e.second);
    }

    return numEnabledSpecEls;
}

/*
 *
 */
bool Element::isMarked() {

    return marked;
}

/*
 *
 */
void Element::mark() {

    marked = true;
}

/*
 *
 */
void Element::unmark() {

    marked = false;
}

    
/*
 *
 */
bool Element::canActivateNoEnable(){

    return false;
}

/*
 *
 */
bool Element::isStateful(){

    return true;
}

/*
 *
 */
bool Element::isSelfRef(){

    bool selfref = false;
    
    for(auto input : getInputs()){
        if(input.first.compare(getId()) == 0)
            selfref = true;
    }

    return selfref;
}
