#include "automata.h"
#include "test.h"

using namespace std;

string testname = "TEST_COUNTER";

/**
 * Tests adding an and gate and different numbers of inputs.
 */
int main(int argc, char * argv[]) {

    // 
    Automata ap;

    STE *one = new STE("one", "[abc]", "all-input");
    STE *two = new STE("two", "[bc]", "all-input");
    STE *three = new STE("three", "[c]", "all-input");

    // Add STEs to AP 
    ap.rawAddSTE(one);
    ap.rawAddSTE(two);
    ap.rawAddSTE(three);


    // Build OR gate
    AND *andgate = new AND("and");
    andgate->setReporting(true);
    
    // Add counter to automata
    ap.rawAddSpecialElement(andgate);
    
    // Add edge between them
    ap.addEdge(one, andgate);
    ap.addEdge(two, andgate);
    ap.addEdge(three, andgate);
    
    // SIMULATION

    // enable report gathering for the automata
    ap.enableReport();

    // initialize simulation
    ap.initializeSimulation();
    
    assert(ap.getReportVector().size() == 0, testname, "1");

    // simulate the automata on three inputs
    ap.simulate('a'); // 

    assert(ap.getReportVector().size() == 0, testname, "2");
    
    ap.simulate('b'); // 

    assert(ap.getReportVector().size() == 0, testname, "3");

    ap.simulate('c'); // 

    assert(ap.getReportVector().size() == 1, testname, "4");

    // NOW TEST ONE ELEMENT
    ap.reset();
    ap.removeElement(two);
    ap.removeElement(three);

    // SIMULATION
    ap.initializeSimulation();

    assert(ap.getReportVector().size() == 0, testname, "5");

    ap.simulate('a');

    assert(ap.getReportVector().size() == 1, testname, "6");
    
    // if we haven't failed, pass the test
    pass(testname);
}
