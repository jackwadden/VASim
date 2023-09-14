/**
 * @file
 */
#ifndef AND_H
#define AND_H

#include "gate.h"
#include <vector>
#include <string>
#include <iostream>

class AND: public Gate {

public:
    AND(std::string id);
    ~AND();

    virtual ElementType getType();

    virtual bool calculate();
    virtual std::string toString();
    virtual std::string toANML();
    virtual MNRL::MNRLNode& toMNRLObj();
};
#endif
