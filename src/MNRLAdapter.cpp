/**
 * @file
 */
#include "MNRLAdapter.h"
#include <iostream>

using namespace std;
using namespace MNRL;

static string convertStart(MNRLDefs::EnableType st) {
    switch(st) {
        case MNRLDefs::EnableType::ENABLE_ALWAYS:
            return "all-input";
        case MNRLDefs::EnableType::ENABLE_ON_ACTIVATE_IN:
            return "none";
        case MNRLDefs::EnableType::ENABLE_ON_START_AND_ACTIVATE_IN:
            return "start-of-data";
        case MNRLDefs::EnableType::ENABLE_ON_LAST:
            return "on-last";
        default:
            cerr << "Non-ANML-like enable signal. Exiting." << endl;
            exit(1);
    }
}

static string convertThreshold(MNRLDefs::CounterMode m) {
    switch(m) {
        case MNRLDefs::CounterMode::TRIGGER_ON_THRESHOLD:
            return "pulse";
        case MNRLDefs::CounterMode::HIGH_ON_THRESHOLD:
            return "latch";
        case MNRLDefs::CounterMode::ROLLOVER_ON_THRESHOLD:
            return "roll";
    }
}

static void addOutputs(shared_ptr<MNRLNode> n, Element *e) {    
    for(auto p : *(n->getOutputConnections())) {
        for(auto c : p.second->getConnections()) {
            string id = c.first->getId();
            string p_id = c.second->getId();
            
            if(
               p_id.compare( MNRLDefs::UP_COUNTER_COUNT ) == 0 ||
               p_id.compare( MNRLDefs::UP_COUNTER_RESET ) == 0 ) {
                
                // we want to keep the port
                e->addOutput(id + ":" + p_id);
            } else {
                // we just drop the port
                e->addOutput(id);
            }
            
        }
    }
}

MNRLAdapter::MNRLAdapter(string filename) : filename(filename) {}

STE *MNRLAdapter::parseSTE(shared_ptr<MNRLHState> hState) {
    
    string id = hState->getId();
    string symbol_set = hState->getSymbolSet();
    string start = convertStart(hState->getEnable());

    // if the start/enable type indicates an "on last" MNRL state
    //   we interpret this as a tail anchored reporting state
    //   tail anchored states need to be enabled on the first input
    //   and also after any "\n" end of line character.
    //   NOTE: until MNRL is fixed, this will not allow a state that
    //         is both EOD report and a start state
    bool is_eod = false;
    if(start.compare("on-last") == 0){
        start = "none";
        is_eod = true;
    }
    
    // create new STE
    STE *s = new STE(id, symbol_set, start);
    s->setIntId(unique_ids++);
    
    s->setReporting(hState->getReport());
    
    addOutputs(hState, s);
    
    s->setReportCode(hState->getReportId()->toString());
    
    s->setEod(is_eod);
    
    return s;
}

Gate *MNRLAdapter::parseGate(shared_ptr<MNRLBoolean> a) {
    string id = a->getId();
    
    Gate *s;
    
    switch(a->getMode()) {
        case MNRLDefs::BooleanMode::AND:
            s = new AND(id);
            break;
        case MNRLDefs::BooleanMode::OR:
            s = new OR(id);
            break;
        case MNRLDefs::BooleanMode::NOR:
            s = new NOR(id);
            break;
        case MNRLDefs::BooleanMode::NOT:
            s = new Inverter(id);
            break;
        default:
            cerr << "Unknown Gate Type. Exiting." << endl;
            exit(1);
    }
    
    s->setIntId(unique_ids++);
    
    addOutputs(a, s);
    
    s->setReporting(a->getReport());
    
    s->setReportCode(a->getReportId()->toString());
    
    return s;
}

Counter *MNRLAdapter::parseCounter(shared_ptr<MNRLUpCounter> cnt) {
    string id = cnt->getId();
    uint32_t target = cnt->getThreshold();
    string at_target = convertThreshold(cnt->getMode());
    
    // create new Counter gate
    Counter *c = new Counter(id, target, at_target);
    c->setIntId(unique_ids++);
    
    addOutputs(cnt, c);
    
    c->setReporting(cnt->getReport());
    
    c->setReportCode(cnt->getReportId()->toString());
    
    return c;
}

void MNRLAdapter::parse(unordered_map<string, Element*> &elements, 
                       vector<STE*> &starts, 
                       vector<Element*> &reports, 
                       unordered_map<string, SpecialElement*> &specialElements,
                       string * id,
                       vector<SpecialElement*> &activateNoInputSpecialElements)
{
    
    try {
        // load the MNRL file
        shared_ptr<MNRLNetwork> net = loadMNRL(filename);
        
        *id = net->getId();
        
        for(auto node : net->getNodes()) {
            Element *s;
            switch(node.second->getNodeType()) {
                case MNRLDefs::NodeType::HSTATE:
                    s = parseSTE(dynamic_pointer_cast<MNRLHState>(node.second));
                    if(dynamic_cast<STE *>(s)->isStart()) {
                        starts.push_back(dynamic_cast<STE *>(s));
                    }
                    break;
                case MNRLDefs::NodeType::BOOLEAN:
                    s = parseGate(dynamic_pointer_cast<MNRLBoolean>(node.second));
                    specialElements[s->getId()] = dynamic_cast<Gate *>(s);
                    
                    // If this is a NOR or Inverter, we need to add it ot another vector
                    switch(dynamic_pointer_cast<MNRLBoolean>(node.second)->getMode()) {
                        case MNRLDefs::BooleanMode::NOR:
                        case MNRLDefs::BooleanMode::NOT:
                            activateNoInputSpecialElements.push_back(dynamic_cast<SpecialElement*>(s));
                            break;
                        default:
                            break;
                    }
                    
                    break;
                case MNRLDefs::NodeType::UPCOUNTER:
                    s = parseCounter(dynamic_pointer_cast<MNRLUpCounter>(node.second));
                    specialElements[s->getId()] = dynamic_cast<Gate *>(s);
                    break;
                default:
                    cerr << "Unsupported Node: " << node.second->getId() << endl;
                    exit(1);
            }
            
            elements[s->getId()] = dynamic_cast<Element*>(s);
            
            
            if(s->isReporting()){
                reports.push_back(s);
            }
        }
        
    } catch (...) {
        cerr << "Unexpected Parsing Error. Exiting." << endl;
        exit(1);
    }
    
}
