/**
 * @file
 */
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

    bool result = false;
    

    for(auto e : inputs) {
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

    str += Gate::toANML();

    str.append("</or>");

    return str;
}

MNRLNode& OR::toMNRLObj() {
    return toMNRLBool(MNRLDefs::BooleanMode::OR);
}

/*
 *
 */
ElementType OR::getType() {

    return ElementType::OR_T;
}
