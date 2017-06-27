//
#include "ste.h"
//#include "util.h"
#define MAX_ERROR_MSG 0x1000

using namespace std;
using namespace MNRL;

/*
 *
 */
STE::STE(string id, string symbol_set, string strt) : Element(id), 
                                                      symbol_set(symbol_set),
                                                      latched(false) {

    setStart(strt);
   
    if(start == ALL_INPUT)
        isAllInput = true;
    else
        isAllInput = false;

    bitset<256> bit_column2;
    for(uint32_t i = 0; i < 256; i++) {
        bit_column.set(i,0);
        bit_column2.set(i,0);
    }

    parseSymbolSet(bit_column, symbol_set);
    
    // SANITY CHECK
    // compares charset parsing against <regex>
    // useful for debugging purposes, but <regex> will *not* behave correctly
    if(0){    
        try{
            matcher = new regex(symbol_set);
        }catch(const std::regex_error& e){
            cout << symbol_set << endl;
        }
        for(uint32_t i = 0; i < 256; i++) {
            bit_column2.set(i,match(i));
            if(bit_column2.test(i) != bit_column.test(i)){
                cout << "FAILED AT: " << i << " " << symbol_set << " hand_coded: " << bit_column.test(i) << endl;
                exit(1);
            }
        }
    }
}

/*
STE::STE(const STE &old) : Element(old) {

    //STE
    symbol_set = old.symbol_set;
    bit_column = old.bit_column;
    matcher = old.matcher;
    latched = old.latched;
    start = old.start;
    enabled = old.enabled;
}
*/

STE::~STE() {

    // for some reason this is segfaulting within regex.h
    //delete matcher;
    
}

/*
 *
 */
bool STE::setSymbolSet(string symbol_set) {

    this->symbol_set = symbol_set;

    return true;
}

/*
 *
 */
string STE::getSymbolSet() {

    return symbol_set;
}

/*
 *
 */
string STE::getRegexSymbolSet() {

    if(symbol_set.compare("*") == 0)
        return ".";
    else
        return symbol_set;
}

/*
 *
 */
bool STE::setStart(string strt) {
    
    if(strt.compare("start-of-data") == 0) {
        start = START_OF_DATA;
    } else if (strt.compare("all-input") == 0) {
        start = ALL_INPUT;
    } else {
        start = NONE;
    }

    return true;
}

/*
 *
 */
bool STE::setStart(Start s) {
    
    start = s;

    return true;
}

/*
 *
 */
STE::Start STE::getStart() {

    return start;
}

/*
 *
 */
string STE::getStringStart() {

    if(start == START_OF_DATA){
        return "start-of-data";
    }else if(start == ALL_INPUT){
        return "all-input";
    }else{
        return "none";
    }

}

/*
 *
 */
bool STE::isStart() {
    
    bool result = false;

    if(start != NONE){
        result = true;
    }

    return result;
}

/*
 *
 */
bool STE::startIsAllInput() {

    return isAllInput;
}

/*
 *
 */
bool STE::startIsStartOfData() {

    return (start == START_OF_DATA);
}

/*
 *
 */
bool STE::isSpecialElement() {

    return false;
}

/*
 *
 */
bool STE::isActivateNoInput() {

    if(isStart()){
        return true;
    }else{
        return false;
    }
}

/*
 *
 */
bool STE::match(uint32_t input) {

    bool retval = false;

    // DOES NOT GENERICIZE TO MORE BITS TODO
    string tmp(1, (char)input);
    if(regex_match(tmp, *matcher)) {
        retval = true;
    }

    return retval;
}


/*
 *
 */
bool STE::deactivate() {

    bool retval = true;

    if(latched) {
        retval = false;
    } else {
        activated = false;
    }

    return retval;
}

/*
 *
 */
vector<uint32_t> STE::getIntegerSymbolSet() {

    vector<uint32_t> ret;
    //this is lazy and slow, but sososososo clean and easy
    //try to match on every character. If we match
    // add to the return set
    for(uint32_t i = 0; i < 256; i++) {
        if(match2((uint8_t)i))
            ret.push_back(i);

    }
        
    return ret;
}

/*
 *
 */
string STE::toString() {

    string s("");
    
    s.append("STE: id=");
    s.append(id);
    s.append(" symbol-set=");
    s.append(symbol_set);
    s.append(" start=");
    if(start == ALL_INPUT){
        s.append("all-input");
    }else if( start == START_OF_DATA) {
        s.append("start-of-data");
    } else {
        s.append("none");
    }
        
    s.append("\n\t");
    s.append("activate-on-match=\n\t  ");
  //  for(string s2 : outputs) {
  //      s.append(s2);
  //      s.append("\n\t  ");
  //  }
    s.append("reporting=");
    if(reporting)
        s.append("true");
    else
        s.append("false");
    s.append("\n\t  ");
    s.append("enabled=");
    if(isEnabled()) {
        s.append("true");
    } else {
        s.append("false");
    }

    s.append("\nINPUTS:\n");
    for(auto s2 : inputs) {
        s.append(s2.first);
        s.append("\n");
    }
    return s;
}

/*
 *
 */
string STE::toANML() {

    string s("<state-transition-element ");
    
    s.append("id=\"");
    s.append(id);
    s.append("\" ");
    s.append(" symbol-set=\"");
    s.append(symbol_set);
    s.append("\" ");
    s.append(" start=\"");
    if(start == ALL_INPUT){
        s.append("all-input");
    }else if( start == START_OF_DATA) {
        s.append("start-of-data");
    } else {
        s.append("none");
    }
    s.append("\">\n");

    if(reporting){
        if(!report_code.empty()){
            s.append("\t<report-on-match reportcode=\"");
            s.append(report_code);
            s.append("\"/>\n");
        }else{
            s.append("\t<report-on-match/>\n");
        }
    }
    
    for(string s2 : outputs) {
        s.append("\t<activate-on-match element=\"");
        s.append(s2);
        s.append("\"/>\n");
    }

    s.append("</state-transition-element>");

    return s;
}

/*
 * Note that this doesn't contain the connections
 */
shared_ptr<MNRLNode> STE::toMNRLObj() {
    
    MNRLDefs::EnableType en;
    switch(start) {
        case NONE:
            en = MNRLDefs::EnableType::ENABLE_ON_ACTIVATE_IN;
            break;
        case START_OF_DATA:
            en = MNRLDefs::EnableType::ENABLE_ON_START_AND_ACTIVATE_IN;
            break;
        case ALL_INPUT:
            en = MNRLDefs::EnableType::ENABLE_ALWAYS;
            break;
    }
    
    shared_ptr<MNRLHState> s =
        shared_ptr<MNRLHState>(new MNRLHState(
            symbol_set,
            en,
            id,
            reporting,
            false, // latched
            report_code,
            shared_ptr<map<string,string>>(new map<string,string>())
        ));
    
    return s;
}

/*
 *
 */
//bool * STE::getBitColumn(){
//    return bit_column;
//}

/*
 *
 */
bitset<256> STE::getBitColumn(){
    return bit_column;
}

/*
 * Adds a character to a symbol set.
 *  If symbol set is wrapped with brackets, they are removed
 *  and then added after the symbol is added.
 */
bool STE::addSymbolToSymbolSet(uint32_t symbol) {

    bool already_exists;
    already_exists = bit_column.test(symbol);

    if(!already_exists){
        
        // add to bit column
        bit_column.set(symbol);
        
        // add to "symbol set"
        // remove brackets if they exist
        bool had_brackets = false;
        if(symbol_set[0] == '[' &&
           symbol_set[symbol_set.length() - 1] == ']'){
            symbol_set = symbol_set.substr(1, symbol_set.length() - 2);
            had_brackets = true;
        }
        
        // add symbol as hex code
        std::stringstream stream;
        stream << std::hex 
               << std::setfill ('0') << std::setw(sizeof(uint8_t)*2) 
               << symbol;
        std::string result( stream.str() );
        symbol_set = symbol_set + "\\x" + result;
        
        // add back brackets if charset had them
        if(had_brackets){
            symbol_set = "[" + symbol_set + "]";
        }
    }

    return already_exists;
}


/*
 * Implements compare for STEs (greater than)
 */
int STE::compare(STE *other) {

    // check bit vector
    //bool * other_column = other->getBitColumn();
    bitset<256> other_column = other->getBitColumn();
    for(int i = 0; i < 256; i++) {
        // if we find a true before we find a true in the other column, return true
        if(bit_column.test(i) == true && other_column.test(i) == false) {
            //cout << "charset greater: " << getSymbolSet() << " :: " << other->getSymbolSet() << endl;
            return 1;
        }else if(bit_column.test(i) == false && other_column.test(i) == true) {
            //cout << "charset greater: " << getSymbolSet() << " :: " << other->getSymbolSet() << endl;
            return -1;
        } 
   }

    // check start
    // Greater conditions
     bool debug_out = false;
    if(getStart() != other->getStart()){
        if(getStart() == ALL_INPUT){
            return 1;
        }
        if((getStart() == START_OF_DATA) && (other->getStart() == NONE)){
            return 1;
        }
        // Less conditions
        if(getStart() == NONE){
            return -1;
        }
        if((getStart() == START_OF_DATA) && (other->getStart() == ALL_INPUT)){
            return -1;
        }
    }else {
        if(getStart() > 0 || other->getStart() > 0){
            //cout << "STARTS EQUALED: " << getStart() << endl;
            //debug_out = true;
        }
    }

    // check report
  
    if(isReporting() == true && other->isReporting() == false) {
        if(debug_out)
            cout << "report mismatch" << endl;
        return 1;
    }else if(isReporting() == false && other->isReporting() == true) {
        if(debug_out)
            cout << "report mismatch2" << endl;
        return -1;
        //never merge reports
    }
    // never merge two reports
    // NOTE: this is *not* the behavior of the micron compiler TODO
    else if(isReporting() || other->isReporting()){
        if(debug_out)
            cout << "report mismatch3" << endl;
        return -1;
    }
    
    // check input sizes
    if(inputs.size() != other->getInputs().size()) {

        //can merge start states that self reference
        if(inputs.size() > other->getInputs().size()){
        if(debug_out)
            cout << "input size mismatch1" << endl;
            return 1;
        } else {
        if(debug_out)
            cout << "input size mismatch2" << endl;
            return -1;
        }
    }

    // for each input key, check if output has it;
    // if it doesn't, just say we are greater; 
    // disregard self references because these are safe
    // TODO:: this seems very inefficient but isn't too slow in practice

    vector<string> keys;
    vector<string> other_keys;

    bool input_check = false;
    for(auto e : inputs) { 
        if(e.first.compare(getId()) != 0){ // filter self loop
            keys.push_back(e.first);
        }
    }
    sort(keys.begin(), keys.end());

    for(auto e : other->getInputs()) { 
        if(e.first.compare(other->getId()) != 0) { // filter self loop
            other_keys.push_back(e.first);
        }
    }
    sort(other_keys.begin(), other_keys.end());

    // check input sizes again
    if(keys.size() != other_keys.size()) {
        if(debug_out)
        cout << "inputs don't match" << endl;
        if(keys.size() > other_keys.size())
            return 1;
        else 
            return -1;
    }

    // compare sorted input keys, returning lexocographic ordering of first mismatch
    /*if(debug_out){
        for(int i = 0; i < keys.size(); i++){
    	    cout << keys[i] << "asf" << endl;
            cout << other_keys[i] << endl;
        }
    }
*/
    for(int i = 0; i < keys.size(); i++){
	if(debug_out) 
	cout<< keys[i] << " " << other_keys[i]<<endl;
        int comp_result = keys[i].compare(other_keys[i]);
        if(comp_result > 0){
            if(debug_out){
                cout << "inputs don't match2" << endl;
            }                
            return 1;
            
        }else if(comp_result < 0) {
        if(debug_out)
            cout << "inputs don't match3" << endl;
            return -1;
        }
    }

    // match returns 0
    /*
    cout << "*************" << endl;
    cout << toString() << "\n" << other->toString();
    cout << "*************" << endl;
    */
    /*
    if(symbol_set.compare(other->getSymbolSet()) != 0){
        cout << "MISMATCH" << endl;
        cout << "*************" << endl;
        cout << toString() << "\n" << other->toString();
        for(int i = 0; i < 256; i++){
            cout << bit_column.test(i) << endl;
        }
        cout << "*************" << endl;
        
    }
    */

    return 0;
}

/*
 * Implements compare for STE columns (greater than)
 */
int STE::compareSymbolSet(STE *other) {

    // check bit vector
    //bool * other_column = other->getBitColumn();
    bitset<256> other_column = other->getBitColumn();
    for(int i = 0; i < 256; i++) {
        // if we find a true before we find a true in the other column, return true
        if(bit_column[i] == true && other_column[i] == false) {
            //cout << "charset greater: " << getSymbolSet() << " :: " << other->getSymbolSet() << endl;
            return 1;
        }else if(bit_column[i] == false && other_column[i] == true) {
            //cout << "charset greater: " << getSymbolSet() << " :: " << other->getSymbolSet() << endl;
            return -1;
        } 
   }

    return 0;
}

STE * STE::clone() {

    STE * clone = new STE(getId(), getSymbolSet(), getStringStart());
    
    if(isReporting()){
        clone->setReporting(true);
        clone->setReportCode(getReportCode());
    }

    // id's are the same, but we don't want that
    // append unique int id to string id;
    string new_id = clone->getId() +"_" + to_string(clone->getIntId()); 
    clone->setId(new_id);

    /*
    // add all inputs
    for(auto in : inputs){
        clone->addInput(in.first);
    }

    // add all outputs
    for(auto out : getOutputSTEPointers()){
        clone->addOutput(out.second);
        clone->addOutputPointer(out.first, out.second);
    }
    */
                
    return clone;
}

/*
 *
 */
void STE::follow(uint32_t character, set<STE*> *follow_set){

    for(auto e : outputSTEPointers){

        STE *s = static_cast<STE*>(e.first);
        if(s->match2(character))
            follow_set->insert(s);
    }
}
