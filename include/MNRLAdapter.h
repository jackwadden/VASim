#ifndef MNRLADAPTER_H
#define MNRLADAPTER_H

#include "ste.h"
#include "and.h"
#include "or.h"
#include "nor.h"
#include "counter.h"
#include "inverter.h"

#include <mnrl.hpp>
#include <string>
#include <unordered_map>

class MNRLAdapter{
    
    private:
        std::string filename;
        uint32_t unique_ids = 0;
    
    public:
        MNRLAdapter(std::string);
        void parse(std::unordered_map<std::string, Element*>&, 
               std::vector<STE*>&,
               std::vector<Element*>&,
               std::unordered_map<std::string, SpecialElement*>&, 
               std::string *,
               std::vector<SpecialElement*>&);
        
        STE *parseSTE(std::shared_ptr<MNRL::MNRLHState>);
        Gate *parseGate(std::shared_ptr<MNRL::MNRLBoolean>);
        Counter *parseCounter(std::shared_ptr<MNRL::MNRLUpCounter>);
};

#endif