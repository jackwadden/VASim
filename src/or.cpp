//
#include "or.h"

using namespace std;
using namespace MNRL;

/*
 *
 */
OR::OR(string id) : Gate(id) {

}

/*
 *
 */
OR::~OR() {

}

/*
 * DOES OR OPERATION OVER INPUTS
 */
bool OR::calculate() {

    if(DEBUG)
        cout << "OR: id=" << id << " CALCULATING" << endl;

    bool result = false;
    

    for(auto e : inputs) {
    
        if(DEBUG)
            cout << e.second << endl;
        result = result || e.second;
    }

    return result;
}

/*
 *
 */
string OR::toString() {

    string str("OR:");
    str.append("\tid=");
    str.append(id);


    return str;
}

/*
 *
 */
string OR::toANML() {

    string str("<or ");
    str.append("id=\"");
    str.append(id);
    str.append("\">\n");

    for(string s2 : outputs) {
        str.append("<activate-on-high element=\"");
        str.append(s2);
        str.append("\"/>\n");
    }

    if(reporting){
        if(!report_code.empty()){
            str.append("\t<report-on-high reportcode=\"");
            str.append(report_code);
            str.append("\"/>\n");
        }else{
            str.append("\t<report-on-high/>\n");
        }
    }

    str.append("</or>");

    return str;
}

shared_ptr<MNRLNode> OR::toMNRLObj() {
    return toMNRLBool(MNRLDefs::BooleanMode::OR);
}
