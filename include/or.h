//
#ifndef OR_H
#define OR_H

#include "gate.h"
#include <vector>
#include <string>
#include <iostream>

class OR: public Gate {

public:
    OR(std::string id);
    ~OR();
    virtual bool calculate();
    virtual std::string toString();
    virtual std::string toANML();
};
#endif
