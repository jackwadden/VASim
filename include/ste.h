//
#ifndef STE_H
#define STE_H

#include "element.h"
#include "util.h"
#include <vector>
#include <string>
//#include <boost/regex.hpp>
#include <regex>
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
    //std::vector<bool> bit_column;
    //bool bit_column[256];
    std::bitset<256> bit_column;
    std::regex *matcher;
    //const boost::regex *matcher;
    bool latched;    
    Start start;
    bool isAllInput;
    //bool report;

public:

    STE(std::string id, std::string symbol_set, std::string start);
    //STE(const STE &);
    ~STE();

    std::vector<std::string> getActivateOnMatch();
    bool setSymbolSet(std::string);
    std::string getSymbolSet();
    std::string getRegexSymbolSet();
    //bool * getBitColumn();
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
    bool match(uint32_t);

    /*
     * Optimized match using bit vector
     */
    inline bool match2(uint8_t input) {
        return bit_column.test(input);
    }
    
    virtual bool deactivate();
    std::vector<uint32_t> getIntegerSymbolSet();
    std::string toString();
    virtual std::string toANML();
    int compare(STE*);
    int compareSymbolSet(STE*);
    void merge(STE*);
    STE* clone();
    void follow(uint32_t, std::set<STE*> *);
    
};


#endif
