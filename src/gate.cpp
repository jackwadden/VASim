//
#include "gate.h"

using namespace std;

/*
 *
 */
Gate::Gate(string id) : SpecialElement(id){

}

/*
 *
 */
Gate::~Gate() {

}

/*
 *
 */
bool Gate::isGate() {

    return true;
}


/*
 *
 */
/*
void Gate::enable(string s) {

    enabled = true;
    inputs[s] = true;
}
*/

/*
 *
 */
/*
void Gate::disable() {
    
    if(DEBUG)
        cout << "DISABLING " << getId() << endl;

    for(auto e: inputs) {
        inputs[e.first] = false;
    }

     enabled = false;
}
*/
/*
 *
 */
bool Gate::isStateful(){

    return false;
}
