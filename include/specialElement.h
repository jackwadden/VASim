/**
 * @file
 */
//
#ifndef SPECIAL_ELEMENT_H
#define SPECIAL_ELEMENT_H

#include "element.h"
#include <vector>
#include <string>
#include <iostream>
#include <unordered_map>

class SpecialElement: public Element {

public:
    SpecialElement(std::string id);
    ~SpecialElement();
    virtual void enable(std::string id);
    virtual void disable();
    virtual bool calculate() = 0;
    virtual bool isSpecialElement();
    virtual std::string toString() = 0;
    virtual std::string toANML() = 0;
    virtual MNRL::MNRLNode& toMNRLObj() = 0;
    virtual std::string toHDL(std::unordered_map<std::string, std::string> id_reg_map);
};
#endif
