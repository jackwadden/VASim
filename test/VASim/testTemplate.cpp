#include "automata.h"
#include "test.h"

using namespace std;

string testname = "TEST_TEMPLATE";

/**
 * TEST DESCRIPTION: all tests can use the fail(string) and pass(string) methods provided in test.h to indicate failing and passing conditions. Failing immediately quits the test program so multiple failures are not allowed per file.
 */
int main(int argc, char * argv[]) {

    // 
    Automata ap;

    STE *start = new STE("start", "[JACK]", "all-input");
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

    // get how many reports we've seen (this should be 0)
    uint32_t numReports = ap.getReportVector().size();

    if(numReports != 0)
        fail(testname);
    
    // enable report gathering for the automata
    ap.setReport(true);//enableReport();

    // initialize simulation
    ap.initializeSimulation();
    
    // simulate the automata on three inputs
    ap.simulate('J');
    ap.simulate('J');
    ap.simulate('J');

    // print out how many reports we've seen (this should be 2)
    numReports = ap.getReportVector().size();
    if(numReports != 2)
        fail(testname);

    // if we haven't failed, pass the test
    pass(testname);
}
