/**
 * @file
 */
//
#ifndef NOR_H
#define NOR_H

#include "gate.h"
#include <vector>
#include <string>
#include <iostream>

class NOR: public Gate {

public:
    NOR(std::string id);
    ~NOR();

    virtual ElementType getType();

    virtual bool calculate();
    virtual std::string toString();
    virtual std::string toANML();
    virtual MNRL::MNRLNode& toMNRLObj();
    virtual bool canActivateNoEnable();
};
#endif
