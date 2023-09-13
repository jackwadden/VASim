/**
 * @file
 */
#ifndef ELEMENT_H
#define ELEMENT_H
//
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <queue>
#include <deque>
#include <memory>

#include <mnrl.hpp>

#include "stack.h"

// type enum definitions
enum ElementType {
    STE_T,
    OR_T,
    NOR_T,
    AND_T,
    INVERTER_T,
    COUNTER_T
};

class Element {

protected:
    std::vector<std::string> outputs;
    std::vector<std::pair<Element *, std::string>> outputSTEPointers;
    std::vector<std::pair<Element *, std::string>> outputSpecelPointers;
    std::map<std::string, bool> inputs;
    std::string id;    
    uint32_t int_id;    
    bool reporting;
    std::string report_code;
    bool activated;
    bool enabled;
    bool eod;

    bool marked;

    
public:

    Element(std::string);
    //Element(const Element &);
    virtual ~Element();

    virtual ElementType getType() = 0;
    
    bool setId(std::string);
    bool setIntId(uint32_t);
    inline std::string getId() {return id; }
    inline uint32_t getIntId() {return int_id; }
    bool setReporting(bool);
    bool isReporting();
    bool setReportCode(std::string);
    std::string getReportCode();
    virtual void enable(std::string) = 0;
    virtual void disable() = 0;
    void activate();
    virtual bool deactivate();
    inline bool isActivated() { return activated; }
    inline bool isEnabled() {return enabled; }
    inline bool isEod() { return eod; }
    void setEod(bool isEod);
    virtual bool isSpecialElement() = 0;
    virtual bool canActivateNoEnable();
    std::vector<std::string> getOutputs();
    std::vector<std::pair<Element *, std::string>> getOutputSTEPointers();
    //std::vector<Element *> getOutputSTEPointers();
    std::vector<std::pair<Element *, std::string>> getOutputSpecelPointers();
    bool clearOutputs();
    bool clearOutputPointers();
    bool clearInputs();
    std::map<std::string, bool> getInputs();
    bool addOutput(std::string);
    bool addOutputPointer(std::pair<Element *, std::string>);
    bool addOutputExisting(std::string, std::map<std::string,Element*>);
    bool removeOutput(std::string);
    bool removeOutputPointer(std::pair<Element *, std::string>);
    bool removeOutputExisting(std::string, std::map<std::string,Element*>);

    virtual bool addInput(std::string);
    bool removeInput(std::string);
    static std::string stripPort(std::string);
    static std::string getPort(std::string);
    virtual std::string toString() = 0;
    virtual std::string toANML() = 0;
    virtual MNRL::MNRLNode& toMNRLObj() = 0;
    //void enableChildSTEs(std::vector<Element *> *);
    void enableChildSTEs(Stack<Element *> *);
    uint32_t enableChildSpecialElements(std::queue<Element *> *);
    bool isMarked();
    void mark();
    void unmark();
    virtual bool isStateful();
    bool isSelfRef();
    bool identicalOutputs(Element *);
    bool identicalInputs(Element *);
    
    // backport additions
    bool isCut();
    void setCut(bool);

};
#endif
