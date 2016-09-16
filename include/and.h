//
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
    virtual bool calculate();
    virtual std::string toString();
    virtual std::string toANML();
};
#endif
