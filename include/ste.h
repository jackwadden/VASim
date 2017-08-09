/**
 * @file
 */
//
#ifndef STE_H
#define STE_H

#include "element.h"
#include "util.h"
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <bitset>
#include <set>
#include <sstream>
#include <iomanip>

class STE: public Element {

    enum Start { NONE, START_OF_DATA, ALL_INPUT };

protected:

    std::string symbol_set;
    std::bitset<256> bit_column;
    bool latched;    
    Start start;

public:

    STE(std::string id, std::string symbol_set, std::string start);
    ~STE();

    virtual ElementType getType();
    
    std::vector<std::string> getActivateOnMatch();
    bool setSymbolSet(std::string);
    std::string getSymbolSet();
    std::string getRegexSymbolSet();
    std::bitset<256> getBitColumn();
    bool addSymbolToSymbolSet(uint32_t);
    bool setStart(std::string);
    bool setStart(Start);
    bool isStart();
    Start getStart();
    std::string getStringStart();
    bool startIsAllInput();
    bool startIsStartOfData();
    inline void enable(std::string s) { enabled = true; }
    inline void enable() { enabled = true; }
    inline void disable() { enabled = false; }
    inline bool isEnabled() { return enabled; }
    bool isSpecialElement();
    bool isActivateNoInput();

    /*
     * Optimized match using bit vector
     */
    inline bool match(uint8_t input) {
        return bit_column.test(input);
    }
    
    virtual bool deactivate();
    std::vector<uint32_t> getIntegerSymbolSet();
    std::string toString();
    virtual std::string toANML();
    virtual std::shared_ptr<MNRL::MNRLNode> toMNRLObj();
    int compare(STE*);
    int compareSymbolSet(STE*);
    void merge(STE*);
    STE* clone();
    void follow(uint32_t, std::set<STE*> *);
    
};


#endif
