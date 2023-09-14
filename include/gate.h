/**
 * @file
 */
//
#ifndef GATE_H
#define GATE_H

#include "specialElement.h"
#include <map>
#include <string>
#include <iostream>

class Gate: public SpecialElement {

protected:
    MNRL::MNRLBoolean& toMNRLBool(MNRL::MNRLDefs::BooleanMode);

public:

    Gate(std::string id);
    virtual ~Gate();
    virtual bool calculate() = 0;
    bool isGate();
   
    virtual std::string toString() = 0;
    virtual std::string toANML();
    virtual MNRL::MNRLNode& toMNRLObj() = 0;
    virtual bool isStateful();
};
#endif
