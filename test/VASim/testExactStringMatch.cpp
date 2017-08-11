#include "automata.h"
#include "test.h"

/**
 * Adds STEs to an automata to match an exact string. Simulates the automata on that string. Should obviously have one report at the end of simulation.
 */

using namespace std;

string testname = "TEST_EXACT_MATCH";

string wrapWithBrackets(char c) {
    
    string str = string("[" + string(1,c) + "]");
    return str;
}

/*
 *
 */
void addStringToAutomata(Automata *ap, string str, uint32_t *id_counter) {

    vector<STE*> stes;

    string charset = "[";
    
    // make STEs
    for(char c : str){
        stes.push_back(new STE("__" + to_string(*id_counter) + "__",
                               wrapWithBrackets(c),
                               "none"));
        (*id_counter)++;
    }

    // Make first STE start
    stes[0]->setStart("all-input");

    // Make last STE report
    stes[str.size() - 1]->setReporting(true);
    
    // add STEs to automata
    for(STE *ste : stes) {
        ap->rawAddSTE(ste);
    }

    // add edges between STEs
    for(int i = 0; i < str.size() - 1; i++){
        ap->addEdge(stes[i], stes[i+1]);
    }
}

/*
 *
 */
int main(int argc, char * argv[]) {

    // 
    Automata ap;
    ap.enableQuiet();
    
    // global ID counter
    uint32_t id_counter = 0;

    // Add a an exact match string to the automata
    string str = "Jack";
    addStringToAutomata(&ap, str, &id_counter);
        
    // get how many reports we've seen 
    uint32_t numReports = ap.getReportVector().size();
    //(this should be 0)
    if(numReports != 0)
        fail(testname);
    
    // enable report gathering for the automata
    ap.enableReport();

    // initialize simulation
    ap.initializeSimulation();
    
    // simulate the automata on the input string
    ap.simulate(reinterpret_cast<uint8_t*>(&str[0]), 0, str.size(), false);

    // print out how many reports we've seen 
    numReports = ap.getReportVector().size();
    //(this should be 1)
    if(numReports != 1)
        fail(testname);

    // if we haven't failed, pass the test
    pass(testname);
}
