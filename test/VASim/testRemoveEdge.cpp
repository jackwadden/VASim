#include "automata.h"
#include "test.h"

using namespace std;

string testname = "TEST_REMOVE_EDGE";

/**
 * Tests whether removeEdge function works correctly.
 */
int main(int argc, char * argv[]) {

    // 
    Automata ap;

    STE *start = new STE("start", "[A]", "all-input");
    STE *stop = new STE("stop", "[A]", "none");
    stop->setReporting(true);

    // Add them to data structure 
    ap.rawAddSTE(start);
    ap.rawAddSTE(stop);

    // Make sure ap correctly handles elements in internal data structures
    ap.validateElement(start);
    ap.validateElement(stop);
    
    // Add edge between them
    ap.addEdge(start,stop);

    if(start->getOutputs().size() != 1)
        fail(testname);

    if(stop->getInputs().size() != 1)
        fail(testname);
    
    // Remove edge
    ap.removeEdge(start,stop);
    
    if(start->getOutputs().size() != 0)
        fail(testname);

    if(stop->getInputs().size() != 0)
        fail(testname);
    
    // enable report gathering for the automata
    ap.setReport(true);//enableReport();

    // initialize simulation
    ap.initializeSimulation();
    
    // simulate the automata on three inputs
    ap.simulate('A');
    ap.simulate('A');
    ap.simulate('A');

    // print out how many reports we've seen (this should be 0)
    uint32_t numReports = ap.getReportVector().size();
    if(numReports != 0)
        fail(testname);

    // see what happens when we try to remove the edge again
    ap.removeEdge(start,stop);
    
    // if we haven't failed, pass the test
    pass(testname);
}
