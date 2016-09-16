//
// emulator.h holds the configuration parameters for the modeled reporting architecture
#include "emulator.h"

using namespace std;

Emulator::Emulator(vector<pair<uint32_t, string>> report_vector, 
                   uint32_t in_cycles, 
                   vector<Element *> report_els) {

    symbol_cycles = in_cycles;
    report_elements = report_els;
    num_reporting_elements = report_elements.size();
    reports = report_vector;

    
    // Greedily assign blocks to report regions
    //assignReportsToL1Buffers();
    assignReportsToL1BuffersRR();

    // report vector division?
    calcEventVectorDivisor();

    cout << words_per_vector_L1 << " words per report vector region..." << endl;
    cout << num_buffers_L1 << " regions required..." << endl;

    
}


void Emulator::initDefault() {

    // defaults are set in header file
}

void Emulator::initPerfect() {
    
}


Emulator::Emulator(string config_fn) {

}

Emulator::~Emulator() {


}

/*
 * TODO: have emulator read from arch config and trace file
 */
void Emulator::parseTraceFile(string fn){

    cout << "PARSING TRACE FILE" << endl;

    // open the file
    ifstream file(fn);
    
    string token;
    uint32_t index = 0;
    uint32_t report_cycle;
    string element_id;
    string report_code;
    /*
    while(file >> token) {
        cout << token << endl;
        switch(index) {
        case 0 :
            // there will always be a line number
            report_cycle = 
            break;
        case 1 :
            break;
        default :

        }
    }
    */
}

/*
 *
 */
double Emulator::calcTotalRuntime() {

    
    double cycles = 0;
    uint32_t L1_vector_counter = 0;
    uint32_t report_index = 0;
    bool stalled = false;
    bool first_report = true;
    
    // for each symbol cycle
    uint32_t symbol_index = 0;
    for(symbol_index = 0; symbol_index < symbol_cycles; symbol_index++){
        
        if(DEBUG)
            printf("symbol: %d\n", symbol_index);
        
        // penalty for the cycle
        cycles++;
        
        // --------------
        // AP computation
        // --------------
        // Does the AP have a report?
        // TODO: DO THIS CHECK FOR EACH REGION BUFFER
        if(report_index < reports.size() &&
           reports[report_index].first == symbol_index){
            
            if(DEBUG)
                printf("report\n");
            
            // push 
            L1_vector_counter++;
            
            // stall if we've filled the buffer
            if(L1_vector_counter > max_vectors_L1){

                // Empty L1 to L2
                cycles += startup_penalty_L1;
                cycles += L1_vector_counter * (word_read_penalty_L1 * words_per_vector_L1);
                L1_vector_counter = 0;
            }
            
            // find next unique report cycle
            uint32_t old_index = reports[report_index].first;
            while(old_index == reports[report_index].first)
                report_index++;
        }
        
           
    }

    // Empty L1 to L2
    if(L1_vector_counter > 0) {
        cycles += startup_penalty_L1;
        cycles += L1_vector_counter * (word_read_penalty_L1 * words_per_vector_L1);
        L1_vector_counter = 0;
    }

    //
    return cycles;
}

/*
 * Calculates total runtime based on the original D480 reporting architecture
 *  This algorithm was correlated with real hardware up to 481 input symbols.
 *  Further correlation is required, but current hardware is not stable enough
 *  to conduct a more stringent validation.
 */
double Emulator::calcTotalRuntime2() {

    
    double cycles = 0;
    
    // Per region trackers
    uint32_t * vector_counter_L1 = new uint32_t[num_buffers_L1];
    bool * vector_report_L1 = new bool[num_buffers_L1];
    for(int i = 0; i < num_buffers_L1; i++) {
        vector_counter_L1[i] = 0;
        vector_report_L1[i] = false;
    }
    
    
    uint32_t report_index = 0;
    bool stalled = false;
    
    // for each symbol cycle
    uint32_t symbol_index = 0;
    for(symbol_index = 0; symbol_index < symbol_cycles; symbol_index++){
        
        if(DEBUG)
            printf("symbol: %d\n", symbol_index);
        
        // penalty for the cycle
        cycles++;
        
        // --------------
        // AP computation
        // --------------
        // Does the AP have a report?
        if(report_index < reports.size() &&
           reports[report_index].first == symbol_index){
            
            if(DEBUG)
                printf("report!\n");
            
            
            // for each report at this symbol index
            while(reports[report_index].first == symbol_index){
                
                // get the region id of the report
                uint32_t buffer_id = idRegionMap[reports[report_index].second];

                // push the vector to the appropriate L1
                if(vector_report_L1[buffer_id] == false) {
                    
                    vector_counter_L1[buffer_id]++;
                }
                
                // mark that the region had a report
                vector_report_L1[buffer_id] = true;
                
                report_index++;
            }
            
            // clean report structures
            for(int i = 0; i < num_buffers_L1; i++) {
                vector_report_L1[i] = false;
            }
            
            // stall if we've filled any of the buffers
            bool stall = false;
            for(int i = 0; i < num_buffers_L1; i++) {
                if(vector_counter_L1[i] >= max_vectors_L1){
                    stall = true;
                }
            }

            if(stall){

                // check if the region is empty
                //cycles += query_penalty_L1 * 3;
                cycles += query_penalty_L1 * 3;
                cycles += startup_penalty_L1;
                for(int i = 0; i < num_buffers_L1; i++) {
                    // Empty L1 to L2    
                    cycles += (double)vector_counter_L1[i] * (word_read_penalty_L1 * (double)words_per_vector_L1);
                    vector_counter_L1[i] = 0;
                }

                stall = false;
            }
            
            // find next unique report cycle
            //uint32_t old_index = reports[report_index].first;
            //while(reports[report_index].first == old_index){
            //    report_index++;
            //}
        }
    }

    // We're done, empty all buffers
    // check if the region is empty
    cycles += query_penalty_L1 * 3;
    for(int i = 0; i < num_buffers_L1; i++) {
        // Empty L1 to L2
        cycles += startup_penalty_L1;
        cycles += (double)vector_counter_L1[i] * (word_read_penalty_L1 * (double)words_per_vector_L1);
        vector_counter_L1[i] = 0;
    }
    
    return cycles;
}

/*
 * D480 Architecture but with a double buffered L1 buffer
 */
double Emulator::calcTotalRuntime3() {

    
    double cycles = 0;
    
    // Per region trackers
    uint32_t vector_counter_L1[2][num_buffers_L1];
    uint32_t current_L1[num_buffers_L1];
    uint32_t next_L1[num_buffers_L1];
    uint32_t L1_penalties[2][num_buffers_L1];
    bool vector_report_L1[num_buffers_L1];
    for(uint32_t i = 0; i < num_buffers_L1; i++) {
        vector_counter_L1[1][i] = 0;
        vector_counter_L1[0][i] = 0;
        L1_penalties[0][i] = 0;
        L1_penalties[1][i] = 0;
        current_L1[i] = 0;
        next_L1[i] = 1;
        vector_report_L1[i] = false;
    }
    
    uint32_t report_index = 0;
    bool stalled = false;
    
    // for each symbol cycle
    uint32_t symbol_index = 0;
    for(symbol_index = 0; symbol_index < symbol_cycles; symbol_index++){
        
        if(DEBUG)
            printf("symbol: %d\n", symbol_index);
        
        // penalty for the cycle
        cycles++;
        // count down penalties in double buffered structures
        for(uint32_t i = 0; i < num_buffers_L1; i++) {
            if(L1_penalties[current_L1[i]][i] > 0)
                L1_penalties[current_L1[i]][i]--;

            if(L1_penalties[next_L1[i]][i] > 0)
                L1_penalties[next_L1[i]][i]--;
        }

        // --------------
        // AP computation
        // --------------
        // Does the AP have a report?
        if(report_index < reports.size() &&
           reports[report_index].first == symbol_index){
            
            if(DEBUG)
                printf("report!\n");
            

            /////////////////////////////////////
            // PUSH VECTORS TO L1
            /////////////////////////////////////            
            // for each report at this symbol index
            while(reports[report_index].first == symbol_index){
                
                // get the buffer id of the report
                uint32_t buffer_id = idRegionMap[reports[report_index].second];

                // push the vector to the appropriate L1
                if(vector_report_L1[buffer_id] == false) {

                    vector_counter_L1[current_L1[buffer_id]][buffer_id]++;
                }
                
                // mark that the region had a report
                vector_report_L1[buffer_id] = true;
                
                report_index++;
            }
            
            // clean report structures
            for(uint32_t i = 0; i < num_buffers_L1; i++) {
                vector_report_L1[i] = false;
            }

            //////////////////////////////////////
            // IF ANY L1 FILLS, BEGIN EXPORT TO L2
            //////////////////////////////////////           
            bool stall = false;            
            // switch buffers if we filled any of the buffers
            for(uint32_t i = 0; i < num_buffers_L1; i++) {
                if(vector_counter_L1[current_L1[i]][i] > max_vectors_L1){

                    // make sure that the next buffer has no cycle penalties left
                    //L1_penalties[current_L1[i]][i] += query_penalty_L1;

                    // if the next buffer is empty
                    if(L1_penalties[next_L1[i]][i] == 0){
                        
                        // initiate an empty for this buffer
                        // record cycle penalties
                        L1_penalties[current_L1[i]][i] += startup_penalty_L1;
                        L1_penalties[current_L1[i]][i] += (double)vector_counter_L1[current_L1[i]][i] * (word_read_penalty_L1 * (double)words_per_vector_L1);

                        // reset vector counter
                        vector_counter_L1[current_L1[i]][i] = 0;

                    }else{
                        // if it's not empty, stall the chip and finish emptying next buffer
                        cycles += L1_penalties[next_L1[i]][i];
                        L1_penalties[next_L1[i]][i] = 0;         
                    }
                     
                    // switch buffers
                    uint32_t tmp = current_L1[i];
                    current_L1[i] = next_L1[i];
                    next_L1[i] = tmp;
                }
            }

            /////////////////////////////////////
            // 
            /////////////////////////////////////            

        }
    }

    // We're done, empty all buffers
    // finish emptying currently emptying buffers
    for(uint32_t i = 0; i < num_buffers_L1; i++) {
        cycles += query_penalty_L1;
        cycles += L1_penalties[next_L1[i]][i];
    }

    // empty all partially filled buffers
    for(uint32_t i = 0; i < num_buffers_L1; i++) {
        // check if the region is empty
        cycles += query_penalty_L1;
        
        // Empty L1 to L2
        cycles += startup_penalty_L1;
        cycles += (double)vector_counter_L1[current_L1[i]][i] * (word_read_penalty_L1 * (double)words_per_vector_L1);
        vector_counter_L1[current_L1[i]][i] = 0;
    }
    
    return cycles;
}

/*
 * D480 Architecture as described in the literature.
 *  different from the model in 2/3 because penalties aren't tripled
 */
double Emulator::calcTotalRuntime4() {

    
    double cycles = 0;
    
    // Per region trackers
    uint32_t L1_vector_counter[num_buffers_L1];
    uint32_t L1_penalties[num_buffers_L1];
    bool L1_vector_reported[num_buffers_L1];
    
    // init region trackers
    for(uint32_t i = 0; i < num_buffers_L1; i++) {
        L1_vector_counter[i] = 0;
        L1_penalties[i] = 0;
        L1_vector_reported[i] = false;
    }
    
    uint32_t report_vector_index = 0;
    bool stalled = false;
    
    // for each symbol cycle
    uint32_t symbol_index = 0;
    for(symbol_index = 0; symbol_index < symbol_cycles; symbol_index++){
        
        if(DEBUG)
            printf("symbol: %d\n", symbol_index);
        
        // penalty for the cycle
        cycles++;

        // --------------
        // AP computation
        // --------------
        // Does the AP have a report?
        if(report_vector_index < reports.size() &&
           reports[report_vector_index].first == symbol_index){
            
            if(DEBUG)
                printf("report!\n");

            /////////////////////////////////////
            // PUSH VECTOR TO L1
            /////////////////////////////////////            
            // for each report at this symbol index
            while(reports[report_vector_index].first == symbol_index){
                
                // get the buffer id of the report
                //cout << "report vector index: " << report_vector_index << endl;
                //cout << reports[report_vector_index].second << endl;
                uint32_t buffer_id = idRegionMap[reports[report_vector_index].second];
                // push the vector to the appropriate L1
                if(L1_vector_reported[buffer_id] == false) {
                    L1_vector_counter[buffer_id]++;
                }
                
                // mark that the region had a report
                L1_vector_reported[buffer_id] = true;
                
                report_vector_index++;
            }
            
            // clean report structures for next cycle
            for(uint32_t i = 0; i < num_buffers_L1; i++) {
                L1_vector_reported[i] = false;
            }
            
            //////////////////////////////////////
            // IF ANY L1 FILLS, 
            // INITIATE A TRANSFER OF ALL VECTORS
            //////////////////////////////////////           
            // stall the machine and initiate
            bool stall = false;         
            for(uint32_t i = 0; i < num_buffers_L1; i++) {
                if(L1_vector_counter[i] > max_vectors_L1){
                    if(!stall)
                        cycles += startup_penalty_L1;
                    stall = true;
                }
            }

            if(stall){
                // for each buffer
                for(uint32_t i = 0; i < num_arch_buffers_L1; i++) {
                    
                    // for all potentially active buffers
                    if(i < num_buffers_L1){
                        
                        if(L1_vector_counter[i] > 0){
                            // record cost for export
                            cycles += (double)L1_vector_counter[i] * (word_read_penalty_L1 * (double)words_per_vector_L1);
                            // reset vector counter
                            L1_vector_counter[i] = 0;
                        }else{
                            cycles += query_penalty_L1;
                        }
                        
                    }else{
                        // if we were empty, record the query penalty
                        cycles += query_penalty_L1;
                    }
                }
            }

            stall = false;

        } // if report
    } // symbol cycle loop


    // We're done, empty all buffers
    cycles += startup_penalty_L1;
    // for each buffer
    for(uint32_t i = 0; i < num_arch_buffers_L1; i++) {    
        // for all potentially active buffers
        if(i < num_buffers_L1){
            if(L1_vector_counter[i] > 0){
                // record cost for export
                cycles += (double)L1_vector_counter[i] * (word_read_penalty_L1 * (double)words_per_vector_L1);
                // reset vector counter
                L1_vector_counter[i] = 0;
            }else{
                // if we were empty, record the query penalty
                cycles += query_penalty_L1;
            }
            
        }else{
            // record the query penalty for unused buffers on chip
            cycles += query_penalty_L1;
        }
    }

    return cycles;
}



void Emulator::printEmulationStats() {
    double cycles;
    // Original
    /*
    cycles = calcTotalRuntime2();
    cout << "Predicted cycles: " << to_string(cycles) << endl;
    cout << "Predicted time: " << to_string(cycles / (mhz * 1000)) << " ms" << endl;
    cout << "Percent greater than ideal: " << to_string(((cycles / symbol_cycles) - 1) * 100) << endl;
    cout << endl;

    // Original with perfect packing
    
    cycles = calcTotalRuntime4();
    cout << "Predicted cycles: " << to_string(cycles) << endl;
    cout << "Predicted time: " << to_string(cycles / (mhz * 1000)) << " ms" << endl;
    cout << "Percent greater than ideal: " << to_string(((cycles / symbol_cycles) - 1) * 100) << endl;
    cout << endl;
    */
    
    // Improved Model with double buffering
    cycles = calcTotalRuntime3();
    cout << "DB_cycles: " << to_string(cycles) << endl;
    cout << "DB_time: " << to_string(cycles / (mhz * 1000)) << " ms" << endl;
    cout << "Percent greater than ideal+: " << to_string(((cycles / symbol_cycles) - 1) * 100) << endl;
    

    // Model as published by the AP website
    cycles = calcTotalRuntime4();
    cout << "Predicted_cycles: " << to_string(cycles) << endl;
    cout << "Predicted_time: " << to_string(cycles / (mhz * 1000)) << " ms" << endl;
    cout << "Output_penalty: " << to_string(((cycles / symbol_cycles) - 1) * 100) << endl;
    cout << endl;
}

/*
 *
 */
void Emulator::assignReportsToL1Buffers() {

    cout << "Assigning reports to L1 buffers..." << endl;

    // blocks per region
    uint32_t blocks_per_L1 = 32;
    uint32_t block_counter = 0;
    uint32_t block_counter_total = 0;
    uint32_t region_counter = 0;
    uint32_t region_counter_old = 0;

    // map IDs to regions
    // this is a class var
    //unordered_map<string, uint32_t> idRegionMap;

    // map blocks to regions
    unordered_map<string, uint32_t> blockRegionMap;

    // print suffixes
    for(Element * el : report_elements) {

        uint32_t last_underscore = el->getId().find_last_of("_");
        //cout << el->getId() << " : " << last_underscore << endl;
        uint32_t second_last_underscore = el->getId().substr(0,last_underscore).find_last_of("_"); 
        //cout << el->getId() << " : " << second_last_underscore << endl;
        //cout << el->getId() << " : " <<  el->getId().substr(second_last_underscore + 1) << endl;
        

        // Extract subgraph ID
        uint32_t subgraph_index = el->getId().rfind("_S");
        uint32_t block_index = el->getId().rfind("_B");
        uint32_t row_index = el->getId().rfind("R");
        //cout << el->getId() << endl;
        //cout << subgraph_index << " " << block_index << " " << row_index << endl;
        string subgraph_id = el->getId().substr(subgraph_index + 2, (block_index) - (subgraph_index + 2));
        string block_id = el->getId().substr(subgraph_index + 2, (row_index) - (subgraph_index + 2));
        string row_id = el->getId().substr((row_index) + 1);
        //cout << "Subgraph ID: " << subgraph_id << endl;
        //cout << "Row ID: " << row_id << endl;

        // If we haven't seen this block before
        if(blockRegionMap.find(block_id) == blockRegionMap.end()){
            // Assign it to a region
            blockRegionMap[block_id] = region_counter;
            idRegionMap[el->getId()] = region_counter;

            block_counter++;
            block_counter_total++;

            // if we've exceeded the blocks per region, move to new region
            if(block_counter == blocks_per_L1){
                block_counter = 0;
                region_counter_old = region_counter;
                region_counter++;
            }

        } else {
            idRegionMap[el->getId()] = region_counter;
        }


        //cout << "Block ID: " << block_id << endl;
        //cout << "Element ID: " << idRegionMap[el->getId()] << endl;
        //cout << "  region: " << blockRegionMap[block_id] << endl;
        //cout << "  block_counter: " << block_counter << endl;
    }

    cout << "Assigned " << report_elements.size() << " report elements" << endl;
    cout << "  in " << block_counter_total << " blocks " << endl;
    cout << "  to " << region_counter + 1 << " regions..." << endl;
    
    num_buffers_L1 = (region_counter + 1);
}

/*
 * Uses a round robin scheme across the chip instead of greedy
 */
void Emulator::assignReportsToL1BuffersRR() {

    cout << "Assigning reports to L1 buffers: round robin..." << endl;

    // blocks per region
    uint32_t blocks_per_L1 = 32;
    uint32_t block_counter = 0;
    uint32_t block_counter_total = 0;
    uint32_t half_core_id = 0;
    uint32_t region_counter = 0;

    // map IDs to regions
    // this is a class var
    //unordered_map<string, uint32_t> idRegionMap;

    // map blocks to regions
    unordered_map<string, uint32_t> blockRegionMap;

    // count reports per block
    unordered_map<string, uint32_t> blockReports;


    // 
    for(Element * el : report_elements) {

        uint32_t last_underscore = el->getId().find_last_of("_");
        //cout << el->getId() << " : " << last_underscore << endl;
        uint32_t second_last_underscore = el->getId().substr(0,last_underscore).find_last_of("_"); 
        //cout << el->getId() << " : " << second_last_underscore << endl;
        //cout << el->getId() << " : " <<  el->getId().substr(second_last_underscore + 1) << endl;
        

        // Extract subgraph ID
        uint32_t subgraph_index = el->getId().rfind("_S");
        uint32_t block_index = el->getId().rfind("_B");
        uint32_t row_index = el->getId().rfind("R");
        //cout << el->getId() << endl;
        //cout << subgraph_index << " " << block_index << " " << row_index << endl;
        string subgraph_id = el->getId().substr(subgraph_index + 2, (block_index) - (subgraph_index + 2));
        string block_id = el->getId().substr(subgraph_index + 2, (row_index) - (subgraph_index + 2));
        string row_id = el->getId().substr((row_index) + 1);
        //cout << "Subgraph ID: " << subgraph_id << endl;
        //cout << "Row ID: " << row_id << endl;
        
        // If we haven't seen this block before

        if(blockRegionMap.find(block_id) == blockRegionMap.end()){

            uint32_t region_id = (block_counter % 3) + (half_core_id * 3);
            
            // assign it to a region
            blockRegionMap[block_id] = region_id;
            block_counter++;
            block_counter_total++;

            // if half core is full
            // switch to next core
            if(block_counter_total > 96){
                block_counter = 0;
                half_core_id = 1;
            }

        }else{
        
            // map id to this region
            idRegionMap[el->getId()] = blockRegionMap[block_id];
        }
    }


    cout << "Assigned " << report_elements.size() << " report elements" << endl;
    cout << "  in " << block_counter_total << " blocks " << endl;

}

/*
 *
 */
void Emulator::calcEventVectorDivisor() {
    
    cout << "Calculating event vector divisor..." << endl;
    
    uint32_t reportsPerRegion[num_buffers_L1];
    for(int i = 0; i < num_buffers_L1; i++)
        reportsPerRegion[i] = 0;
    
    
    // find reporting elements per region?
    uint32_t max_region = 0;
    uint32_t max_value = 0;
    for(auto e : idRegionMap){
        reportsPerRegion[e.second]++;
        if(reportsPerRegion[e.second] > max_value){
            max_value = reportsPerRegion[e.second];
            max_region = e.second;
            //cout << max_value << endl;
        }
    }
    
    cout << "  Max reports per region: " << max_value << endl;
    cout << "  Largest region: " << max_region << endl;
    
    // calulate event vector divisor
    if(max_value <= 64){
        words_per_vector_L1 = 1;
    }
    //
    else if(max_value <= 128) {
        words_per_vector_L1 = 2;
    }
    //
    else if(max_value <= 256) {
        words_per_vector_L1 = 4;
    }
    //
    else if(max_value <= 512) {
        words_per_vector_L1 = 8;
    }
    //
    else if(max_value <= 768) {
        words_per_vector_L1 = 12;
    }
    //
    else{
        words_per_vector_L1 = 16;
    }

    cout << "  Num words per event vector: " << words_per_vector_L1 << endl;
}
