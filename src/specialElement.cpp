/**
 * @file
 */
#include "specialElement.h"

using namespace std;

/*
 *
 */
SpecialElement::SpecialElement(string id) : Element(id) {

}

/*
 *
 */
SpecialElement::~SpecialElement() {

}


/*
 *
 */
bool SpecialElement::isSpecialElement() {

    return true;
}

/*
 *
 */
void SpecialElement::enable(string s) {

    enabled = true;
    inputs[s] = true;

}

/*
 *
 */
void SpecialElement::disable() {

    for(auto e : inputs) {

        inputs[e.first] = false;
    }
    
    enabled = false;
}


string SpecialElement::toHDL(unordered_map<string, string> id_reg_map) {
    // fail
    cout << "CANNOT EMIT SPECIAL ELEMENTS AS HDL YET..." << getId() << endl;
    exit(1);
}
