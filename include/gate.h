//
#ifndef GATE_H
#define GATE_H

#include "specialElement.h"
#include <map>
#include <string>
#include <iostream>

class Gate: public SpecialElement {

protected:
    std::shared_ptr<MNRL::MNRLBoolean> toMNRLBool(MNRL::MNRLDefs::BooleanMode);

public:

    Gate(std::string id);
    virtual ~Gate();
    virtual bool calculate() = 0;
    bool isGate();
   
    virtual std::string toString() = 0;
    virtual std::string toANML() = 0;
    virtual std::shared_ptr<MNRL::MNRLNode> toMNRLObj() = 0;
    virtual bool isStateful();
};
#endif
