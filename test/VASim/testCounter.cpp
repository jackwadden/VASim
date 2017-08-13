#include "automata.h"
#include "test.h"

using namespace std;

string testname = "TEST_COUNTER";

/**
 * Tests adding a counter and different functions on ported inputs.
 */
int main(int argc, char * argv[]) {

    // 
    Automata ap;

    STE *count = new STE("count", "[c]", "all-input");
    STE *reset = new STE("reset", "[r]", "all-input");
    STE *report = new STE("report", "*", "none");
    report->setReporting(true);

    // Add STEs to AP 
    ap.rawAddSTE(count);
    ap.rawAddSTE(reset);
    ap.rawAddSTE(report);


    // Build counter element
    uint32_t target = 2;
    Counter *counter = new Counter("counter", target, "pulse");

    // Add counter to automata
    ap.rawAddSpecialElement(counter);
    
    // Add edge between them
    ap.addEdge(count->getId(), counter->getId() + ":cnt");
    ap.addEdge(counter->getId(), report->getId());
    
    // SIMULATION

    // enable report gathering for the automata
    ap.enableReport();

    // initialize simulation
    ap.initializeSimulation();

    assert(ap.getReportVector().size() == 0, testname, "1");
    
    // simulate the automata on three inputs
    ap.simulate('c'); // count goes to 1

    assert(ap.getReportVector().size() == 0, testname, "2");
    
    ap.simulate('c'); // count goes to 2 (activates)

    assert(ap.getReportVector().size() == 0, testname, "3");

    ap.simulate('c'); // reporting element activates

    assert(ap.getReportVector().size() == 1, testname, "4");
     
    // if we haven't failed, pass the test
    pass(testname);
}
