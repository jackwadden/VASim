/**
 * @file
 */
//
#ifndef INVERTER_H
#define INVERTER_H

#include "gate.h"

class Inverter: public Gate {

public:
    Inverter(std::string);
    ~Inverter();

    virtual ElementType getType();

    virtual bool calculate();
    virtual std::string toString();
    virtual std::string toANML();
    virtual MNRL::MNRLNode& toMNRLObj();
    virtual std::string toHDL(std::unordered_map<std::string, std::string>);
    virtual bool canActivateNoEnable();
};

#endif
