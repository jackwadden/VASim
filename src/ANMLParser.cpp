/**
 * @file
 */
#include "ANMLParser.h"


using namespace std;

ANMLParser::ANMLParser(string filename) : filename(filename) {

}

/**
 *
 */
STE *ANMLParser::parseSTE(pugi::xml_node ste) {

    string id;
    string symbol_set;
    string start;
    bool eod = false;
    
    // gather attributes
    for (pugi::xml_attribute attr = ste.first_attribute(); 
         attr; 
         attr = attr.next_attribute()) {
        
        string str = attr.name();

        if(str.compare("id") == 0) { 
            id = attr.value();
        } else if(str.compare("symbol-set") == 0) {
            symbol_set = attr.value();
        }else if(str.compare("start") == 0) {
            start = attr.value();
        } 

        if(str.compare("high-only-on-eod") == 0) {
            eod = true;
        }

    }


    // create new STE
    STE *s = new STE(id, symbol_set, start);
    s->setIntId(unique_ids++);
    s->setEod(eod);
    
    for(pugi::xml_node aom : ste.children("activate-on-match")) {
        s->addOutput(aom.attribute("element").value());
    }
    
    for(pugi::xml_node aom : ste.children("report-on-match")) {
        s->setReporting(true);
    }

    for(pugi::xml_node aom : ste.children("report-on-match")) {
        s->setReportCode(aom.attribute("reportcode").value());
    }

    return s;    
}

/**
 *
 */
template <typename T>
T *ANMLParser::parseGate(pugi::xml_node a) {

    string id;
    bool eod = false;
    
    // gather attributes
    for (pugi::xml_attribute attr = a.first_attribute(); 
         attr; 
         attr = attr.next_attribute()) {
        
        string str = attr.name();
        
        if(str.compare("id") == 0) { 
            id = attr.value();
        }

        if(str.compare("high-only-on-eod") == 0) {
            eod = true;
        }
    }

    // create new gate
    T *s = new T(id);
    s->setEod(eod);
    s->setIntId(unique_ids++);

    for(pugi::xml_node aom : a.children("activate-on-high")) {
        s->addOutput(aom.attribute("element").value());
    }
    
    for(pugi::xml_node aom : a.children("report-on-high")) {
        s->setReporting(true);
    }

    for(pugi::xml_node aom : a.children("report-on-high")) {
        s->setReportCode(aom.attribute("reportcode").value());
    }

    for(pugi::xml_node aom : a.children("report-on-high")) {
        s->setReportCode(aom.attribute("reportcode").value());
    }


    return s;
}

/**
 *
 */
Counter *ANMLParser::parseCounter(pugi::xml_node a) {

    string id;
    uint32_t target;
    string at_target;


    // gather attributes
    for (pugi::xml_attribute attr = a.first_attribute(); 
         attr; 
         attr = attr.next_attribute()) {
        
        string str = attr.name();
        
        if(str.compare("id") == 0) { 

            id = attr.value();
            //cout << "PARSING COUNTER: " << id << endl; 
        } else if(str.compare("target") == 0) { 
        
            target = attr.as_uint();
        }else if(str.compare("at-target") == 0) { 

            at_target = attr.value();
        }



    }

    // create new Counter gate
    Counter *c = new Counter(id, target, at_target);
    c->setIntId(unique_ids++);

    for(pugi::xml_node aom : a.children("activate-on-target")) {
        c->addOutput(aom.attribute("element").value());
    }
    
    for(pugi::xml_node aom : a.children("report-on-target")) {
        c->setReporting(true);
    }

    for(pugi::xml_node aom : a.children("report-on-target")) {
        c->setReportCode(aom.attribute("reportcode").value());
    }

    return c;
}

/**
 *
 */
vasim_err_t ANMLParser::parse(unordered_map<string, Element*> &elements, 
                       vector<STE*> &starts, 
                       vector<Element*> &reports, 
                       unordered_map<string, SpecialElement*> &specialElements,
                       string * id,
                       vector<SpecialElement*> &activateNoInputSpecialElements)
{

    // Open xml document
    pugi::xml_document doc;
    if (!doc.load_file(filename.c_str(),pugi::parse_default|pugi::parse_declaration)) {
        cout << "Could not load .xml file: " << filename << endl;
        return E_FILE_OPEN;
    }

    // can handle finding automata-network at one or two layers under root
    pugi::xml_node nodes = doc.child("anml").child("automata-network");
    if(nodes.name() == ""){
        nodes = doc.child("automata-network");
    }

    string id_tmp = nodes.attribute("id").value();

    *id = id_tmp;

    for (pugi::xml_node node = nodes.first_child(); node; node = node.next_sibling()) {
        
        string str = node.name();
        
        if(str.compare("state-transition-element") == 0) {
            STE *s = parseSTE(node);
            elements[s->getId()] =  dynamic_cast<Element*>(s);
            if(s->isStart()) {
                starts.push_back(s);
            }
            if(s->isReporting()){
                reports.push_back(s);
            }
        } else if(str.compare("and") == 0) {
            AND *a = parseGate<AND>(node);
            elements[a->getId()] = dynamic_cast<Element*>(a);
            if(a->isReporting()){
                reports.push_back(a);
            }
            specialElements[a->getId()] = a;
        } else if(str.compare("or") == 0) {
            //OR *a = parseOR(node);
            OR *a = parseGate<OR>(node);
            elements[a->getId()] = dynamic_cast<Element*>(a);
            if(a->isReporting()){
                reports.push_back(a);
            }
            specialElements[a->getId()] = a;
        }else if(str.compare("nor") == 0) {
            NOR *a = parseGate<NOR>(node);
            elements[a->getId()] = dynamic_cast<Element*>(a);
            if(a->isReporting()){
                reports.push_back(a);
            }
            specialElements[a->getId()] = a;
            activateNoInputSpecialElements.push_back(a);
        } else if(str.compare("counter") == 0) {
            Counter *a = parseCounter(node);
            elements[a->getId()] = dynamic_cast<Element*>(a);
            if(a->isReporting()){
                reports.push_back(a);
            }
            specialElements[a->getId()] = a; 
        } else if (str.compare("inverter") == 0) {
            Inverter *a = parseGate<Inverter>(node);
            elements[a->getId()] = dynamic_cast<Element*>(a);
            if(a->isReporting()){
                reports.push_back(a);
            }
            specialElements[a->getId()] = a; 
            activateNoInputSpecialElements.push_back(dynamic_cast<SpecialElement*>(a));
        } else if(str.compare("description") == 0) {
            
            // DO NOTHING
        } else {
            cout << "NODE: " << str << " NOT IMPLEMENTED IN PARSER..." << endl;
        }
        
    }

    return E_SUCCESS;


}
