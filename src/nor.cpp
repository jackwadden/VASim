/**
 * @file
 */
#include "nor.h"

using namespace std;
using namespace MNRL;

/*
 *
 */
NOR::NOR(string id) : Gate(id) {

}

/*
 *
 */
NOR::~NOR() {

}

/*
 * DOES NOR OPERATION OVER INPUTS
 */
bool NOR::calculate() {

    bool result = false;
    

    for(auto e : inputs) {
        result = result || e.second;
    }

    // above code was copied from OR
    // simply invert OR op
    return (!result);
}

/*
 *
 */
string NOR::toString() {

    string str("NOR:");
    str.append("\tid=");
    str.append(id);


    return str;
}

/*
 *
 */
string NOR::toANML() {

    string str("<nor ");
    str.append("id=\"");
    str.append(id);
    str.append("\">\n");

    for(string s2 : outputs) {
        str.append("<activate-on-match element=\"");
        str.append(s2);
        str.append("\"/>\n");
    }

    if(reporting){
        str.append("\t<report-on-high reportcode=\"");
        str.append(report_code);
        str.append("\"/>\n");
    }
    str.append("</nor>");

    return str;
}

shared_ptr<MNRLNode> NOR::toMNRLObj() {
    return toMNRLBool(MNRLDefs::BooleanMode::NOR);
}

/*
 *
 */
bool NOR::canActivateNoEnable() {

    return true;
}

/*
 *
 */
ElementType NOR::getType() {

    return ElementType::NOR_T;
}
