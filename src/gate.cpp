/**
 * @file
 */
#include "gate.h"

using namespace std;
using namespace MNRL;

/*
 *
 */
Gate::Gate(string id) : SpecialElement(id){

}

/*
 *
 */
Gate::~Gate() {

}

/*
 *
 */
bool Gate::isGate() {

    return true;
}

/*
 *
 */
bool Gate::isStateful(){

    return false;
}

/*
 *
 */
string Gate::toANML() {
    string str = "";
    //string str("<" + type + " ");
    str.append("id=\"");
    str.append(id);

    if(isEod()){
        str.append(" high-only-on-eod=\"true\"");
    }
    
    str.append("\">\n");

    for(string s2 : outputs) {
        str.append("<activate-on-high element=\"");
        str.append(s2);
        str.append("\"/>\n");
    }

    if(reporting){
        if(!report_code.empty()){
            str.append("\t<report-on-high reportcode=\"");
            str.append(report_code);
            str.append("\"/>\n");
        }else{
            str.append("\t<report-on-high/>\n");
        }
    }
    
    //str.append("</" + type + ">");

    return str;
}

/*
 * Give it a bool type and it's happy
 */
shared_ptr<MNRLBoolean> Gate::toMNRLBool(MNRLDefs::BooleanMode m) {
    shared_ptr<MNRLBoolean> b =
        shared_ptr<MNRLBoolean>(new MNRLBoolean(
            m,
            1, // port count
            id,
            MNRLDefs::EnableType::ENABLE_ON_ACTIVATE_IN,
            reporting,
            report_code,
            shared_ptr<map<string,string>>(new map<string,string>())
        ));
    if(eod) {
      s->setReportEnable(MNRLDefs::ReportEnableType::ENABLE_ON_LAST);
    }
    return b;
}
