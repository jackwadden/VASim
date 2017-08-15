/**
 * @file
 */
#include "and.h"

using namespace std;
using namespace MNRL;

/*
 *
 */
AND::AND(string id) : Gate(id) {

}

/*
 *
 */
AND::~AND() {

}

/*
 * DOES AND OPERATION OVER INPUTS
 */
bool AND::calculate() {
    
    bool result = true;
    
    for(auto e : inputs) {
        result = result && e.second;
    }

    if(inputs.size() > 0)
        return result;
    else{
        return false;
    }
}

/*
 *
 */
string AND::toString() {

    string str("AND:");
    str.append("\tid=");
    str.append(id);

    return str;
}

/*
 *
 */
string AND::toANML() {

    string str("AND: NOT IMPLEMENTED YET\n");
    return str;
}

shared_ptr<MNRLNode> AND::toMNRLObj() {
    return toMNRLBool(MNRLDefs::BooleanMode::AND);
}

/*
 *
 */
ElementType AND::getType() {

    return ElementType::AND_T;
}
