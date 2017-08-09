/**
 * @file
 */
#include "gate.h"

using namespace std;
using namespace MNRL;

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
bool Gate::isStateful(){

    return false;
}

/*
 * Give it a bool type and it's happy
 */
shared_ptr<MNRLBoolean> Gate::toMNRLBool(MNRLDefs::BooleanMode m) {
    shared_ptr<MNRLBoolean> b =
        shared_ptr<MNRLBoolean>(new MNRLBoolean(
            m,
            1, // port count
            id,
            MNRLDefs::EnableType::ENABLE_ON_ACTIVATE_IN,
            reporting,
            report_code,
            shared_ptr<map<string,string>>(new map<string,string>())
        ));
    
    return b;
}
