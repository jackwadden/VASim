//
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

string SpecialElement::toHDL(unordered_map<string, string> id_reg_map) {
    // fail
    cout << "CANNOT EMIT SPECIAL ELEMENTS AS HDL YET..." << getId() << endl;
    exit(1);
}
