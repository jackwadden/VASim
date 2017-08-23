#include "automata.h"
#include "test.h"

using namespace std;

string testname = "TEST_BITSET_TO_CHARSET";

/**
 * TEST DESCRIPTION: Tests correctness of util function that converts bitsets to string charsets for STEs.
 */
int main(int argc, char * argv[]) {

    // 
    STE *ste1 = new STE("start", "[JACKL]", "all-input");
    STE *ste2 = new STE("start", "*", "all-input");

    assert(bitsetToCharset(ste1->getBitColumn()).compare("[\\x41\\x43\\x4a-\\x4c]") == 0, testname + "0");
    
    assert(bitsetToCharset(ste2->getBitColumn()).compare("[\\x00-\\xff]") == 0, testname + "1");
    
    // add a symbol to the range
    ste1->addSymbolToSymbolSet((uint32_t)'M');
    
    assert(bitsetToCharset(ste1->getBitColumn()).compare("[\\x41\\x43\\x4a-\\x4d]") == 0, testname + "2");

    // pass if no failures
    pass(testname);
}
