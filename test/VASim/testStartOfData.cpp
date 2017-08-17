#include "automata.h"
#include "test.h"

using namespace std;

string testname = "TEST_START_OF_DATA";

/**
 * TEST DESCRIPTION: start-of-data start states should only ever be specially enabled at the start of simulation (after initializeSimulation is called).
 */
int main(int argc, char * argv[]) {

    // 
    Automata ap;

    STE *start = new STE("start", "[JACK]", "start-of-data");
    STE *stop = new STE("stop", "[JARED]", "none");
    stop->setReporting(true);

    // Add them to data structure 
    ap.rawAddSTE(start);
    ap.rawAddSTE(stop);

    // Make sure ap correctly handles elements in internal data structures
    ap.validateElement(start);
    ap.validateElement(stop);

    // Add edge between them
    ap.addEdge(start,stop);

    // SIMULATION
    
    // enable report gathering for the automata
    ap.enableReport();
    
    // initialize simulation
    ap.initializeSimulation();
    
    // simulate the automata on three inputs
    ap.simulate('J');
    ap.simulate('J');
    ap.simulate('J');

    // print out how many reports we've seen (this should be 1)
    uint32_t numReports = ap.getReportVector().size();
    if(numReports != 1)
        fail(testname);

    // if we haven't failed, pass the test
    pass(testname);
}
