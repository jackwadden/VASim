#include "automata.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include "getopt.h"
#include <chrono>
#include <ctime>
#include <thread>
#include "errno.h"

#define FROM_INPUT_STRING false

using namespace std;

void usage(char * argv) {

    printf("USAGE: %s [OPTIONS] <automata anml> <input file/string> \n", argv);
    printf("  -i, --input               Input chars are taken from command line\n");
    printf("  -s, --step                Useful if debugging; simulation advances one cycle per when a key is pressed\n");
    printf("  -t, --time                Time simulation\n");
    printf("  -r, --report              Print reports to stdout\n");
    printf("  -b, --batchsim            Output report mimics format of batchsim\n");
    printf("  -q, --quiet               Suppress all non-debugging output\n");
    printf("  -p, --profile             Profiles automata, storing activation and enable histograms in .out files\n");
    printf("  -c, --charset             Compute charset complexity of automata using Quine-McCluskey Algorithm\n");
    
    printf("\n OUTPUT FORMATS:\n");
    printf("  -d, --dot                 Output automata as dot file. Builds a heat map if profiling is turned on\n");
    printf("  -a, --anml                Output automata as anml file. Useful for storing graphs after long running optimizations\n");
    printf("  -n, --nfa                 Output automata as nfa readable by Michela Becchi's tools\n");    
    printf("  -D, --dfa                 Convert automata to DFA\n");
    printf("  -f, --hdl                 Output automata as one-hot encoded verilog HDL for execution on an FPGA (EXPERIMENTAL)\n");    
    printf("      --graph               Output automata as .graph file for HyperScan.\n");

    printf("\n OPTIMIZATIONS:\n");    
    printf("  -O, --left-min-before     Enable left minimization before connected component search\n");
    printf("  -L, --left-min-after      Enable left minimization after within connected components\n");
    printf("  -x, --remove_ors          Remove all OR gates\n");

    printf("\n TRANSFORMATIONS:\n");
    printf("      --enforce-fanin=<int> Enforces a fan-in limit, replicating nodes until no node has a fan-in of larger than <int>.\n");
    printf("      --enforce-fanout=<int> Enforces a fan-out limit, replicating nodes until no node has a fan-out of larger than <int>.\n");

    printf("\n MULTITHREADING:\n");
    printf("  -T, --threads             Specify number of threads to compute connected components of automata\n");
    printf("  -P, --packets             Specify number of threads to compute input stream. NOT SAFE. TODO: allow for overlap between packets\n");

    printf("\n MISC:\n");
    printf("  -h, --help                Print this help and exit\n");
    printf("\n");
}

uint32_t fileSize(string fn) {

    // open the file:
    std::ifstream file(fn, ios::binary);

    // Stop eating new lines in binary mode!!!
    file.unsetf(std::ios::skipws);

    // get its size:
    std::streampos fileSize;

    file.seekg(0, ios::end);
    fileSize = file.tellg();
    file.seekg(0, ios::beg);

    return fileSize;
}
void inputFileCheck() { 
    if(errno == ENOENT) {
        cout<< "VAsim Error: no such input file." << endl;
        exit(-1);
    }	
}
vector<unsigned char> file2CharVector(string fn) {

    // open the file:
    std::ifstream file(fn, ios::binary);
    if(file.fail()){
        inputFileCheck();
    }

    // get its size:
    std::streampos fileSize;

    file.seekg(0, ios::end);
    fileSize = file.tellg();
    file.seekg(0, ios::beg);

    // Stop eating new lines in binary mode!!!
    file.unsetf(std::ios::skipws);

    // reserve capacity
    std::vector<unsigned char> vec;
    vec.reserve(fileSize);

    // read the data:
    vec.insert(vec.begin(),
               std::istream_iterator<unsigned char>(file),
               std::istream_iterator<unsigned char>());

    return vec;

}

/*
 *
 */
void simulateAutomaton(Automata *a, uint8_t *input, uint32_t start_index, uint32_t size, bool step) {
    a->simulate(input, start_index, size, step);
}

/*
 * Returns the input stream byte array, stores length in size pointer input
 */
uint8_t * parseInputStream(bool simulate, bool input_string, uint32_t *size, char ** argv, uint32_t optind) {

    uint8_t * input;
    
    if(simulate){
        // From command line
        if(input_string){
            string input2 = argv[optind];
            *size = (uint32_t)input2.length();
            uint32_t counter = 0;
            input = (uint8_t*)malloc(sizeof(uint8_t) * *size);
            // copy bytes to unsigned ints
            for(unsigned char val : input2){
                input[counter] = (uint8_t)val;
                counter++;
            }
            // From file
        } else {
            string input_fn = argv[optind];
            vector<unsigned char> input2 = file2CharVector(input_fn);
            *size = input2.size();
            input = (uint8_t*)malloc(sizeof(uint8_t) * input2.size());
            // copy bytes to unsigned ints
            uint32_t counter = 0;
            for(uint8_t val : input2){
                input[counter] = (uint8_t)val;
                counter++;
            }
        }
    }

    return input;
}

/*
 *
 */
int main(int argc, char * argv[]) {


    bool input_string = false;
    bool step = false;
    bool quiet = false;
    bool batchsim = false;
    bool report = false;
    bool profile = false;
    bool charset_complexity = false;
    bool to_dot = false;
    bool to_anml = false;
    bool time = false;
    bool optimize = false;
    bool optimize_after = false;
    bool remove_ors = false;
    bool to_nfa = false;
    bool to_dfa = false;
    bool to_hdl = false;
    uint32_t max_level = 10000; // artificial (and arbitrary) max depth of attempted left-minimization
    uint32_t num_threads = 1;
    uint32_t num_threads_packets = 1;
    bool to_graph = false;
    int32_t fanin_limit = -1;
    int32_t fanout_limit = -1;

    int c;
    const char * short_opt = "thsqrbnfcdDeaxipOLl:T:P:";

    struct option long_opt[] = {
        {"help",          no_argument, NULL, 'h'},
        {"step",          no_argument, NULL, 's'},
        {"quiet",          no_argument, NULL, 'q'},
        {"report",         no_argument, NULL, 'r'},
        {"batchsim",         no_argument, NULL, 'b'},
        {"input",         no_argument, NULL, 'i'},
        {"dot",         no_argument, NULL, 'd'},
        {"anml",         no_argument, NULL, 'a'},
        {"nfa",         no_argument, NULL, 'n'},
        {"dfa",			no_argument, NULL, 'D'},
        {"hdl",         no_argument, NULL, 'f'},
        {"profile",         no_argument, NULL, 'p'},
        {"charset",         no_argument, NULL, 'c'},
        {"time",         no_argument, NULL, 't'},
        {"left-min-before",         no_argument, NULL, 'O'},
        {"left-min-after",         no_argument, NULL, 'L'},
        {"remove-ors",         no_argument, NULL, 'x'},
        {"level",         required_argument, NULL, 'l'},
        {"thread-width",         required_argument, NULL, 'T'},
        {"thread-height",         required_argument, NULL, 'P'},
        {"graph",         no_argument, NULL, 0},
        {"enforce-fanin",         required_argument, NULL, 0},
        {"enforce-fanout",         required_argument, NULL, 0},
        {NULL,            0,           NULL, 0  }
    };
    
    int long_ind;
    while((c = getopt_long(argc, argv, short_opt, long_opt, &long_ind)) != -1) {
        switch(c) {
        case -1:       /* no more arguments */
        case 0:        /* long options toggles */
            if(long_opt[optind].flag != 0){
                break;
            }

            //
            if(strcmp(long_opt[long_ind].name,"graph") == 0){
                to_graph = true;
            }
            if(strcmp(long_opt[long_ind].name,"enforce-fanin") == 0){
                fanin_limit = atoi(optarg);
                if(fanin_limit < 1){
                    cout << "Error: Fanin limit cannot be less than 1/n" << endl;
                    exit(1);
                }
            }
            if(strcmp(long_opt[long_ind].name,"enforce-fanout") == 0){
                fanout_limit = atoi(optarg);
                if(fanout_limit < 1){
                    cout << "Error: Fanout limit cannot be less than 1/n" << endl;
                    exit(1);
                }
            }
            break;
            
        case 'i':
            input_string = true;
            break;
            
        case 'q':
            quiet = true;
            break;

        case 's':
            step = true;
            break;

        case 'r':
            report = true;
            break;

        case 'b':
            report = true;
            batchsim = true;
            break;

        case 'p':
            profile = true;
            break;

        case 'd':
            to_dot = true;
            break;

        case 'a':
            to_anml = true;
            break;

        case 't':
            time = true;
            break;

        case 'c':
            charset_complexity = true;
            break;

        case 'O':
            optimize = true;
            break;

        case 'L':
            optimize_after = true;
            break;

        case 'x':
            remove_ors = true;
            break;

        case 'l':
            max_level = atoi(optarg);
            break;

        case 'T':
            num_threads = atoi(optarg);
            break;

        case 'P':
            num_threads_packets = atoi(optarg);
            break;

        case 'D':
            to_dfa = true;
            break;

        case 'n':
            to_nfa = true;
            break;

        case 'f':
            to_hdl = true;
            break;

        case 'h':
            usage(argv[0]);
            return(0);

        case ':':
        case '?':
            fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
            return(-2);
            
        default:
            fprintf(stderr, "%s: invalid option -- %c\n", argv[0], c);
            usage(argv[0]);
            return(-2);
        };
    };

    if(optind >= argc) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    
    // Parse command line args
    string fn(argv[optind++]);
    uint8_t *input;
    uint32_t size;

    bool simulate = true;

    if(optind >= argc) {
        simulate = false;
    }

    // Parse automata input file or input from command line
    input = parseInputStream(simulate, input_string, &size, argv, optind);

    // Build automata
    if(!quiet){
     
        cout << "|------------------------|" << endl;
        cout << "|        Parsing         |" << endl;
        cout << "|------------------------|" << endl;

        cout << "Building automata from file: " << fn << endl;
    
    }


    // 
    Automata ap(fn);
    uint32_t automata_size = ap.getElements().size();
    uint32_t orig_automata_size = ap.getElements().size();

    if(!quiet){
        ap.printGraphStats();
    }

    if(quiet)
        ap.enableQuiet();
        
    // Optimize automata before identifying connected components
    // "Global" optimizations
    // Start optimizations
    if(optimize) {
        if(!quiet){
            cout << "|------------------------|" << endl;
            cout << "|       Optimization     |" << endl;
            cout << "|------------------------|" << endl;
         
            cout << "Starting Global Optimizations..." << endl; 

        }

        /***********************
         * GLOBAL OPTIMIZATIONS
         ***********************/
        
        if(!quiet)
            cout << "Left-merging automata..." << endl;
        ap.leftMinimize2();
        
        while(automata_size != ap.getElements().size()) {
            automata_size = ap.getElements().size();
            ap.leftMinimize2();
        }
        
        if(!quiet)
            cout << endl;
        
        // ******
        // ADD OTHER OPTIMIZATIONS
        // ******
    }
    
    
    // Partition automata into connected components
    if(!quiet)
        cout << "Finding distinct subgraphs..." << endl;
    vector<Automata*> ccs;
    ccs = ap.splitConnectedComponents();
    

    if(!quiet)
        cout << endl;

    // Combine connected components into N automata
    if(!quiet)
        cout << "Combining " << ccs.size() << " distinct subgraphs into " << num_threads << " graphs..." << endl;
    
    // Check if there are too many threads for the available subgraphs
    if(ccs.size() < num_threads){
        if(!quiet){
            cout << "VASim WARNING: Not enough subgraphs to satisfy all threads!" << endl;
            cout << "VASim WARNING: Adjusting threadcount to match subgraphs.\n" << endl;
        }
        
        num_threads = ccs.size();
    }

    unsigned int counter = 0;
    vector<Automata*> merged(num_threads);
    for(Automata *a : ccs) {

        if(merged[counter % num_threads] == NULL){
            merged[counter % num_threads] = ccs[counter];
        }else{
            merged[counter % num_threads]->unsafeMerge(a);
            
            if(quiet)
                merged[counter % num_threads]->enableQuiet();
        }
        counter++;
    }
    
    if(!quiet)
        cout << endl;
    
    // Preprocess all automata partitions
    // Set up multi-dimensional structure for parallelization
    Automata *automata[num_threads][num_threads_packets];
    
    counter = 0;
    for(Automata *a : merged) {        
        /*****************
         * LOCAL OPTIMIZATIONS
         *****************/     
        // Optimize after connected component merging
        if(optimize_after) {
            if(!quiet)
                cout << "Starting Local Optimizations..." << endl; 

            // Left minimization
            automata_size = a->getElements().size();
            a->leftMinimize2();
            while(automata_size != a->getElements().size()) {
                automata_size = a->getElements().size();
                a->leftMinimize2();
            }

            if(!quiet)
                cout << endl;
            
            // ******
            // ADD OTHER OPTIMIZATIONS
            // ******
            
        }

        /*****************
         * TRANSFORMATIONS
         *****************/

        // Remove OR gates, which are just syntactic sugar (benefitial for some hardware)
        if(remove_ors) {
            if(!quiet){
                cout << "Removing OR gates..." << endl;
                cout << endl;
            }

            a->removeOrGates();
        }

        // Enforce fan-in limit
        if(fanin_limit > 0){
            if(!quiet){
                cout << "Enforcing fan-in of " << fanin_limit << "..." << endl; 
                cout << endl;
            }

            a->enforceFanIn(fanin_limit);
        }

        // Enforce fan-out limit
        if(fanout_limit > 0){
            if(!quiet){
                cout << "Enforcing fan-out of " << fanout_limit << "..." << endl; 
                cout << endl;
            }

            a->enforceFanOut(fanout_limit);
        }

        // Convert automata to DFA
        if(to_dfa) {
            if(!quiet) {
                cout<< "Converting automata to DFA..." <<endl;
                cout<<endl;
            }
            
            Automata * dfa = a->generateDFA();
            a = dfa;
        }

        /*******************
         * OUTPUT FORMATS
         *******************/
        // Save automata anml if desired
        if(to_anml) {
            a->automataToANMLFile("automata_" + to_string(counter) + ".anml");
        }

        // Emit in Becchi format NFA
        if(to_nfa){
            a->automataToNFAFile("automata_" + to_string(counter) + ".nfa");
        }
        
        // Emit as HDL
        if(to_hdl) {
            a->automataToHDLFile("automata_" + to_string(counter) + ".v");
        }

        // Emit as graph file readable by augmented HyperScan
        if(to_graph){
            if(!quiet)
                cout << "Emitting automata in .graph format for HyperScan ingestion..." << endl << endl;
            a->automataToGraphFile("automata_" + to_string(counter) + ".graph");
        }

        // Save each parallel automata and replicate for multiple streams
        // *** TODO: this is a lazy way of avoiding writing a copy constructor
        const char * tmp_fn = "temp_vasim_unique_temp_file_name.anml"; 
        a->automataToANMLFile(tmp_fn);
        
        // Insert automata into correct index for multiple input streams
        automata[counter][0] = a;
        for (int packet = 1; packet < num_threads_packets; ++packet) {
            automata[counter][packet] = new Automata(tmp_fn);
        }
        // Delete temp file
        remove(tmp_fn);

        // Print final stats
	if(!quiet)
            a->printGraphStats();

        counter++;
    }

    /****************************
     * GRAPH STATISTICS
     ***************************/
    if(!quiet){
        if(num_threads == 1){
            // Compressability
            cout << "Compressability: " << 1.0 - ((double)automata[0][0]->getElements().size()/(double)orig_automata_size) << endl;
            
            // STE complexity
            if(charset_complexity)
                automata[0][0]->printSTEComplexity();
            cout << endl;
        }
    }

    /****************************
     * SIMULATION
     ***************************/
    if(simulate){
        if(!quiet){
            cout << "|------------------------|" << endl;
            cout << "|       Simulation       |" << endl;
            cout << "|------------------------|" << endl;
            cout << "Starting simulation using " << num_threads << "x" << num_threads_packets << "=" << num_threads*num_threads_packets << " thread(s)..." << endl; 
        }
        thread threads[num_threads][num_threads_packets];

        // Start timer
        chrono::high_resolution_clock::time_point start_time;
        if(time) {
            start_time = chrono::high_resolution_clock::now();
        }
        
        // Simulate all automata
        for (int tid= 0; tid < num_threads; tid++) {
            //for(Automata *a : merged) {

            uint64_t packet_offset = 0;              
            uint64_t packet_size = size/num_threads_packets;              
            // For each input packet of the input stream
            //for (int tid= 0; tid < num_threads; tid++) {
            for(int packet = 0; packet < num_threads_packets; packet++) {

                Automata *a = automata[tid][packet];

                // print
                if(DEBUG)
                    a->print();
                
                // enable runtime profiling
                if(profile){
                    a->enableProfile();
                }
                
                if(report)
                    a->enableReport();
       
                // Handle odd divisors
                uint64_t length = packet_size;
                if(packet == num_threads_packets - 1)
                    length += size % packet_size;

                //cout << "Launching thread:" << endl;
                //cout << "  packet: " << packet << endl;
                //cout << "  packet_offset: " << packet_offset << endl;
                //cout << "  length: " << length << endl;
                //cout << "  packet_size: " << packet_size << endl;

                // Launch thread
                threads[tid][packet] = thread(simulateAutomaton, 
                                              a,
                                              input,
                                              packet_offset,
                                              length, 
                                              step);
            
                packet_offset += packet_size;
            }
        }

        // Join threads
        for (int i = 0; i < num_threads; ++i) {
            for(int j = 0; j < num_threads_packets; j++){
                threads[i][j].join();
            }
        }

        // Stop timer
        if(time) {
            chrono::high_resolution_clock::time_point end_time = chrono::high_resolution_clock::now();
            double duration = chrono::duration<double, std::milli>(end_time - start_time).count();
            std::cout << "Simulation Time: " << duration << " ms" << std::endl;
            std::cout << "Throughput: " << (size/1000)/(duration) << " MB/s" << std::endl;
        }
    }
     
    /**********************************
     * POST SIMULATION PROCESSING
     **********************************/
    uint32_t num_reports = 0;
    uint32_t match_cycles = 0;
    for (int tid= 0; tid < num_threads; tid++) {
        for(int packet = 0; packet < num_threads_packets; packet++) {
            
            Automata *a = automata[tid][packet];
            
            // quiet supresses all non-debug output
            if(report){
                // number of reports
                num_reports += a->getReportVector().size();

                // number of reporting cycles
                uint32_t cur = 0;
                for(auto e : a->getReportVector()) {
                    if(e.first != cur){
                        match_cycles++;
                        cur = e.first;
                    }
                }

                if(batchsim){
                    a->printReportBatchSim();
                }else{
                    //a->printReport();
                    string reportFile = "reports_" +
                        to_string(tid) + "tid_" +
                        to_string(packet) + "packet.txt";
 
                    a->writeReportToFile(reportFile);
                    //e.parseTraceFile(reportFile);
                }

            }
            
        }
    }       

    if(report && !quiet && simulate) {

        cout << "|------------------------|" << endl;
        cout << "|        Results         |" << endl;
        cout << "|------------------------|" << endl;
        
        std::cout << "Reports: " << num_reports << std::endl;
        std::cout << "Reporting Cycles: " << match_cycles << std::endl;

    }


    // Emit heatmap dot graphs
    for (int tid= 0; tid < num_threads; tid++) {
        
        // Remember, the y direction are identical, so only need y = 0
        Automata *a = automata[tid][0];
        
        // print graphviz
        // this is done after simulation to account for addition of profiling metadata (e.g. heatmaps)
        if(to_dot) {
            a->automataToDotFile("automata_" + to_string(tid) + ".dot");
        }
        
    }

    if(simulate){
        delete input;
    }
}
