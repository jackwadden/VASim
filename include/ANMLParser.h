/**
 * @file
 */
#ifndef ANMLPARSER_H
#define ANMLPARSER_H

#include "ste.h"
#include "and.h"
#include "or.h"
#include "nor.h"
#include "counter.h"
#include "inverter.h"
#include "errors.h"
#include "pugixml.hpp"

#include <string>
#include <unordered_map>
#include <iostream>

class ANMLParser {

private:
    std::string filename;
    uint32_t unique_ids = 0;

public:
    ANMLParser(std::string);
    vasim_err_t parse(std::unordered_map<std::string, Element*>&, 
                      std::vector<STE*>&,
                      std::vector<Element*>&,
                      std::unordered_map<std::string, SpecialElement*>&, 
                      std::string *,
                      std::vector<SpecialElement*>&);
    STE *parseSTE(pugi::xml_node);
    template<typename T> T*parseGate(pugi::xml_node);
    AND *parseAND(pugi::xml_node);
    OR *parseOR(pugi::xml_node);
    NOR *parseNOR(pugi::xml_node);
    Inverter *parseInverter(pugi::xml_node);
    Counter *parseCounter(pugi::xml_node);
};

#endif
