//
#include "counter.h"

using namespace std;
using namespace MNRL;

/*
 *
 */
Counter::Counter(string id, uint32_t target, string at_target) : SpecialElement(id), 
                                                                 mode(PULSE),
                                                                 dormant(false){

    setTarget(target);
    setMode(at_target);
    value = 0;
}

/*
 *
 */
Counter::~Counter() {

 
}



/*
 * Calculate should return whether or not this element is activated for the next cycle.
 */
bool Counter::calculate() {

    bool retval = false;

    // if any inputs are to the cnt input
    bool count = false;
    bool reset = false;

    // gather input signals from all ports
    for(auto i : inputs) {
        if((i.first.compare(i.first.size() - 4, i.first.size(), ":cnt") == 0) && 
           i.second == true) {

            if(DEBUG) {
                cout << "COUNTER ACTIVATED ON PORT: " << i.first << endl;
                cout << "\tvalue= " << value << " target= " << target << endl;
            }

            // count up
            count = true;

        } else if((i.first.compare(i.first.size() - 4, i.first.size() - 1, ":rst") == 0) &&
                  i.second == true) {

            if(DEBUG)
                cout << "COUNTER ACTIVATED ON PORT: " << i.first << endl;

            // reset counter
            reset = true;
        }  
    }

    // reset takes priority over all other signals
    if(reset) {

        dormant = false;
        latched = false;
        value = 0;
        
        // reset is not high, so count if not disabled
    } else if((count && !dormant) || latched) {
        
        // depending on the mode, take action
        if(mode == LATCH) {
            if(!latched) {
                value++;
            
                if(value == target) {
                    retval = true;
                    latched = true;
                }
            } else {
                if(DEBUG){
                    cout << "I'M LATCHED!" << endl;
                }

                retval = true;
            }
            
        } else if (mode == ROLL) {
            
            value++;

            if(value == target) {
                retval = true;

                // reset value
                value = 0;
            }
            
        } else { // pulse

            // count always increases
            value++;

            if(value == target) {
                retval = true;
                // set dormant until reset signal is received
                dormant = true;
            }            
        }
        if(DEBUG)
            cout << "\tnext value = " << value << endl;
    } else {
        if(DEBUG)
            cout << "SKIPPING COMPUTATION" << endl;
    }

    return retval;
}

/*
 * Calculate should return whether or not this element is activated for the next cycle.
 */
/*
bool Counter::calculate() {
    
    bool retval = false;
    // if any inputs are to the cnt input
    bool count = false;
    bool count_down = false; // MOD
    bool reset = false;
    
    // gather input signals from all ports
    for(auto i : inputs) {
        if((i.first.compare(i.first.size() - 4, i.first.size(), ":cnt") == 0) && 
           i.second == true) {
            
            // count up
            count = true;
            
        }
        
        if((i.first.compare(i.first.size() - 4, i.first.size() - 1, ":rst") == 0) &&
           i.second == true) {
            
            // reset counter
            reset = true;
            // START MOD
        } 
        
        if((i.first.compare(i.first.size() - 4, i.first.size(), ":cntd") == 0) && 
           i.second == true) {
            
            // count down
            count_down = true;
        }
        // END MOD
    }
    
    // reset takes priority over all other signals
    if(reset) {
        
        dormant = false;
        latched = false;
        value = 0;
        
        // reset is not high, so count if not disabled
    } else if(((count || count_down) && !dormant) || latched) { // MOD
        
        // depending on the mode, take action
        if(mode == LATCH) {
            
            if(!latched) {
                if(count) value++;
                if(count_down) value--; //MOD
                
                if(value == target) {
                    retval = true;
                    latched = true;
                }
            } else {
                retval = true;
            }
            
        } else if (mode == ROLL) {
            
            if(count) value++;
            if(count_down) value--; // MOD
            
            if(value == target) {
                retval = true;
                
                // reset value
                value = 0;
            }
            
        } else { // pulse
            
            // count always increases
            if(count) value++;
            if(count_down) value--; // MOD
            
            if(value == target) {
                retval = true;
                // set dormant until reset signal is received
                dormant = true;
            }            
        }
    }
    
    return retval;
}
*/

void Counter::setMode(string md) {
    
    if(md.compare("latch") == 0) {
        mode = LATCH;
    } else if (md.compare("roll") == 0) {
        mode = ROLL;
    } else {
        mode = PULSE;
    }
}

/*
 *
 */
void Counter::setTarget(uint32_t t) {

    target = t;
}

/*
 *
 */
uint32_t Counter::getTarget() {

    return target;
}

/*
 *
 */
uint32_t Counter::getValue() {

    return value;
}

/*
 *
 */
bool Counter::deactivate() {

    bool retval = true;

    if(latched) {
        retval = false;
    }else {
        activated = false;
    }

    return retval;
}

/*
 *
 */
string Counter::toString() {

    return "COUNTER TO STRING NOT IMPLEMENTED YET";
}

/*
 *
 */
string Counter::toANML() {


    string s("<counter ");
    
    s.append("id=\"");
    s.append(id);
    s.append("\" ");
    s.append(" target=\"");
    s.append(to_string(target));
    s.append("\">\n");

    s.append("\t");
    for(string s2 : outputs) {
        s.append("<activate-on-target element=\"");
        s.append(s2);
        s.append("\"/>\n");
    }

    if(reporting){
        s.append("\t<report-on-match/>\n");
    }

    s.append("</counter>");

    return s;

}

/*
 *
 */
shared_ptr<MNRLNode> Counter::toMNRLObj() {
    
    MNRLDefs::CounterMode m;
    switch(mode) {
        case LATCH:
            m = MNRLDefs::CounterMode::HIGH_ON_THRESHOLD;
            break;
        case ROLL:
            m = MNRLDefs::CounterMode::ROLLOVER_ON_THRESHOLD;
            break;
        case PULSE:
            m = MNRLDefs::CounterMode::TRIGGER_ON_THRESHOLD;
            break;
    }
    
    shared_ptr<MNRLUpCounter> c =
        shared_ptr<MNRLUpCounter>(new MNRLUpCounter(
            target,
            m,
            id,
            MNRLDefs::EnableType::ENABLE_ON_ACTIVATE_IN,
            reporting,
            report_code,
            shared_ptr<map<string,string>>(new map<string,string>())
        ));
        
    return c;
}

/**
 * return a string containing Verliog to implement a counter
 *
 * NOTE:  This only supports latch counters as the moment
 */
string Counter::toHDL(unordered_map<string, string> id_reg_map) {
    // fail for things we don't support
    if( mode != LATCH ) {
        cout << "CANNOT EMIT THIS COUNTER AS HDL YET..." << endl;
        exit(1);
    }
    
    // build up a string with everything
    string ctr = "";
    
    // header
    ctr += "\t////////////////\n";
    ctr += "\t// COUNTER: " + getId() + "\n";
    ctr += "\t////////////////\n";
    
    // print input enables
    ctr += "\t// Input enable OR gate\n";
    string enable_name = getId() + "_CNT";
    ctr += "\twire\t" + enable_name + ";\n";
    
    // iterate through the enables
    // for all the inputs
    string enable_string = "";
    bool first = true;
    for(auto in : inputs){
        // only do this for the cnt inputs
        if(in.first.compare(in.first.size() - 4, in.first.size(), ":cnt") == 0) {
            if(first){
                enable_string += id_reg_map[in.first.substr(0,in.first.size()-4)];
                first = false;
            }else{
                enable_string += " | " + id_reg_map[in.first.substr(0,in.first.size()-4)];
            }
        }
    }
    
    if(enable_string.compare("") != 0) {
        ctr += "\tassign " + enable_name + " = " + enable_string + ";\n";
    } else {
        ctr += "\tassign " + enable_name + " = 1'b0;\n";
    }
    
    // print input resets
    ctr += "\t// Input reset OR gate\n";
    string reset_name = getId() + "_RST";
    ctr += "\twire\t" + reset_name + ";\n";
    
    
    
    // iterate through the enables
    // for all the inputs
    string reset_string = "";
    
    first = true;
    for(auto in : inputs){
        // only do this for the cnt inputs
        if(in.first.compare(in.first.size() - 4, in.first.size(), ":rst") == 0) {
            if(first){
                reset_string += id_reg_map[in.first.substr(0,in.first.size()-4)];
                first = false;
            }else{
                reset_string += " | " + id_reg_map[in.first.substr(0,in.first.size()-4)];
            }
        }
    }
    
    if(reset_string.compare("") != 0) {
        ctr += "\tassign " + reset_name + " = " + reset_string + ";\n";
    } else {
        ctr += "\tassign " + reset_name + " = 1'b0;\n";
    }
    
    
    string reg_name = id_reg_map[getId()];
    
    ctr += "\n\t// Register to hold current count\n";
    ctr += "\treg\t[0:11] " + reg_name + "_val;\n";
    
    //
    ctr += "\n\t// Match logic and activation register\n";

    //
    ctr += "\t(*dont_touch = \"true\"*) always @(posedge Clk) // should not be optimized\n";

    
    ctr += "\tbegin\n";
    ctr += "\t\tif (Rst_n == 1'b1)\n";
    ctr += "\t\tbegin\n";
    ctr += "\t\t\t" + reg_name + "_val <= 12'b000000000000;\n";
    ctr += "\t\t\t" + reg_name + " <= 1'b0;\n";
    ctr += "\t\tend\n";
    
    // if the counter is being reset
    ctr += "\t\telse if (" + reset_name +" == 1'b1)\n";
    ctr += "\t\tbegin\n";
    ctr += "\t\t\t" + reg_name + "_val <= 12'b000000000000;\n";
    ctr += "\t\t\t" + reg_name + " <= 1'b0;\n";
    ctr += "\t\tend\n";
    
    // else if someone is driving the counter
    ctr += "\t\telse if ("+ enable_name +" == 1'b1)\n";
    
    // check if the counter has reached the threshold
    ctr += "\t\tbegin\n";
    ctr += "\t\t\tif (" + reg_name + "_val < 12'b111111111111)\n";
    ctr += "\t\t\t\t " + reg_name + "_val <= " + reg_name + "_val + 1;\n";
    ctr += "\t\t\tif (" + reg_name + "_val == " + to_string(target) + ")\n";
    ctr += "\t\t\t\t " + reg_name + " <= 1'b1;\n";
    ctr += "\t\tend\n";
    
    ctr += "\t\telse " + reg_name + "_val <= 12'b000000000000;\n";
    ctr += "\tend\n\n";
        
    return ctr;
}
