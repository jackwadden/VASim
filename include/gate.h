//
#ifndef GATE_H
#define GATE_H

#include "specialElement.h"
#include <map>
#include <string>
#include <iostream>

class Gate: public SpecialElement {

public:

    Gate(std::string id);
    virtual ~Gate();
    virtual bool calculate() = 0;
    bool isGate();
    void enable(std::string);
    void disable();
    virtual std::string toString() = 0;
    virtual std::string toANML() = 0;
    virtual bool isStateful();
};
#endif
