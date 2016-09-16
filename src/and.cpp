//
#include "and.h"

using namespace std;

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

    if(DEBUG)
        cout << "AND: id=" << id << " CALCULATING" << endl;

    bool result = true;
    

    for(auto e : inputs) {
    
        if(DEBUG)
            cout << e.second << endl;
        result = result && e.second;
    }

    return result;
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
