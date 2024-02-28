#include "automata.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include "getopt.h"
#include <chrono>
#include <ctime>
#include <thread>
#include <cassert>

#include "errno.h"

#define FROM_INPUT_STRING false

using namespace std;

void usage(char * argv) {

    printf("USAGE: %s [OPTIONS] <automata anml> <input file/string> \n", argv);
    printf("  -i, --input               Input chars are taken from command line\n");
    printf("  -t, --time                Time simulation\n");
    printf("  -r, --report              Print reports to stdout\n");
    printf("  -b, --batchsim            Output report mimics format of batchsim\n");
    printf("  -q, --quiet               Suppress all non-debugging output\n");
    printf("  -p, --profile             Profiles automata, storing activation and enable histograms in .out files\n");
    printf("  -c, --charset             Compute charset complexity of automata using Quine-McCluskey Algorithm\n");

    printf("\n DEBUG:\n");
    printf("      --dump-state=<int>    Prints state of automata on cycle <int> to stes_<cycle>.state and specels_<cycle>.state files.\n");

    printf("\n OUTPUT FORMATS:\n");
    printf("  -d, --dot                 Output automata as dot file. Builds a heat map if profiling is turned on\n");
    printf("  -a, --anml                Output automata as anml file. Useful for storing graphs after long running optimizations\n");
    printf("  -m, --mnrl                Output automata as MNRL file. Useful for storing graphs after long running optimizations\n");
    printf("  -n, --nfa                 Output automata as nfa readable by Michela Becchi's tools\n");    
    printf("  -D, --dfa                 Convert automata to DFA\n");
    printf("  -f, --hdl                 Output automata as one-hot encoded verilog HDL for execution on an FPGA (EXPERIMENTAL)\n");    
    printf("  -F, --hls <num automata>  Output automata as VITIS HLS-compatible C++ (EXPERIMENTAL - only supports STEs) Provide number of automata\n");
    printf("  -B, --blif                Output automata as .blif circuit for place-and-route using VPR.\n");
    printf("      --graph               Output automata as .graph file for HyperScan.\n");
    printf("  -S, --split               Specify number of seperate automata files to split automata into.\n");
    
    printf("\n OPTIMIZATIONS:\n");    
    printf("  -O, --optimize-global     Run all optimizations on all automata subgraphs.\n");
    printf("  -L, --optimize-logal      Run all optimizations on automata subgraphs after partitioned among parallel threads.\n");
    printf("  -x, --remove_ors          Remove all OR gates. Only applied globally.\n");

    printf("\n TRANSFORMATIONS:\n");
    printf("      --enforce-fanin=<int> Enforces a fan-in limit, replicating nodes until no node has a fan-in of larger than <int>.\n");
    printf("      --enforce-fanout=<int> Enforces a fan-out limit, replicating nodes until no node has a fan-out of larger than <int>.\n");
    printf("      --widen               Pads each state with a zero state for patterns where the input is 16 bits (common in YARA rules).\n");
    printf("      --2-stride            Two strides automata if possible.\n");
    
    printf("\n MULTITHREADING:\n");
    printf("  -T, --threads             Specify number of threads to compute connected components of automata\n");
    printf("  -P, --packets             Specify number of threads to compute input stream. NOT SAFE. TODO: allow for overlap between packets\n");

    printf("\n MISC:\n");
    printf("  -h, --help                Print this help and exit\n");
    printf("\n");
}

/*
 *
 */
void simulateAutomaton(Automata *a, uint8_t *input, uint64_t start_index, uint64_t sim_length, uint64_t total_length) {
    a->simulate(input, start_index, sim_length, total_length);
}

/*
 *
 */
int main(int argc, char * argv[]) {


    bool input_string = false;
    bool quiet = false;
    bool batchsim = false;
    bool report = false;
    bool profile = false;
    bool charset_complexity = false;
    bool to_dot = false;
    bool to_anml = false;
    bool to_mnrl = false;
    bool time = false;
    bool optimize_global = false;
    bool prefix_merge_global = false;
    bool prefix_merge_local = false;
    bool suffix_merge_global = false;
    bool suffix_merge_local = false;
    bool common_path_merge_global = false;
    bool common_path_merge_local = false;
    bool optimize_local = false;
    bool remove_ors = false;
    bool to_nfa = false;
    bool to_dfa = false;
    bool to_hdl = false;
    bool to_hls = false;
    bool to_blif = false;
    bool cc = false;
    uint32_t num_threads = 1;
    uint32_t num_threads_packets = 1;
    bool to_graph = false;
    int32_t fanin_limit = -1;
    int32_t fanout_limit = -1;
    bool dump_state = false;
    uint32_t dump_state_cycle = 0;
    bool widen = false;
    bool two_stride = false;
    bool split = false;
    uint32_t num_automata = 0;
    uint32_t split_count = 0;
    
    // long option switches
    const int32_t graph_switch = 1000;
    const int32_t fanin_switch = 1001;
    const int32_t fanout_switch = 1002;
    const int32_t dump_state_switch = 1003;
    const int32_t widen_switch = 1004;
    const int32_t two_stride_switch = 1005;
    
    int c;
    const char * short_opt = "thsqrbnfcdBDeamxipF:OLT:P:S:";

    struct option long_opt[] = {
        {"help",          no_argument, NULL, 'h'},
        {"quiet",          no_argument, NULL, 'q'},
        {"report",         no_argument, NULL, 'r'},
        {"batchsim",         no_argument, NULL, 'b'},
        {"input",         no_argument, NULL, 'i'},
        {"dot",         no_argument, NULL, 'd'},
        {"anml",         no_argument, NULL, 'a'},
        {"mnrl",         no_argument, NULL, 'm'},
        {"nfa",         no_argument, NULL, 'n'},
        {"dfa",			no_argument, NULL, 'D'},
        {"hdl",         no_argument, NULL, 'f'},
        {"hls",         no_argument, NULL, 'F'},
        {"blif",         no_argument, NULL, 'B'},
        {"profile",         no_argument, NULL, 'p'},
        {"charset",         no_argument, NULL, 'c'},
        {"time",         no_argument, NULL, 't'},
        {"optimize-global",         no_argument, NULL, 'O'},
        {"optimize-local",         no_argument, NULL, 'L'},
        {"remove-ors",         no_argument, NULL, 'x'},
        {"thread-width",         required_argument, NULL, 'T'},
        {"thread-height",         required_argument, NULL, 'P'},
        {"split",         required_argument, NULL, 'S'},
        {"graph",         no_argument, NULL, graph_switch},
        {"enforce-fanin",         required_argument, NULL, fanin_switch},
        {"enforce-fanout",         required_argument, NULL, fanout_switch},
        {"dump-state",         required_argument, NULL, dump_state_switch},
        {"widen",         no_argument, NULL, widen_switch},
        {"2-stride",         no_argument, NULL, two_stride_switch},
        {NULL,            0,           NULL, 0  }
    };
    
    int long_ind;
    while((c = getopt_long(argc, argv, short_opt, long_opt, &long_ind)) != -1) {
        switch(c) {
        case -1:       /* no more arguments */
        case 0:        /* long options toggles */
            break;

        case 'i':
            input_string = true;
            break;
            
        case 'q':
            quiet = true;
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
        
        case 'm':
            to_mnrl = true;
            break;

        case 't':
            time = true;
            break;

        case 'c':
            charset_complexity = true;
            break;

        case 'O':
            prefix_merge_global = true;
            suffix_merge_global = true;
            common_path_merge_global = true;
            optimize_global = true;
            break;

        case 'L':
            prefix_merge_local = true;
            suffix_merge_local = true;
            common_path_merge_local = true;
            optimize_local = true;
            break;

        case 'x':
            remove_ors = true;
            optimize_global = true;
            break;

        case 'T':
            num_threads = atoi(optarg);
            break;

        case 'P':
            num_threads_packets = atoi(optarg);
            break;

        case 'S':
            split = true;
            split_count = atoi(optarg);
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
        
        case 'F':
            to_hls = true;
            num_automata = atoi(optarg);
            break;

        case 'B':
            to_blif = true;
            break;
        
        case 'C':
            cc = true;
            break;

        case 'h':
            usage(argv[0]);
            return(0);

        case ':':
        case '?':
            fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
            return(-2);

        case graph_switch:
            to_graph = true;
            break;

        case fanin_switch:
            fanin_limit = atoi(optarg);
            if(fanin_limit < 1){
                cout << "Error: Fanin limit cannot be less than 1/n" << endl;
                exit(1);
            }
            break;

        case fanout_switch:
            fanout_limit = atoi(optarg);
            if(fanout_limit < 1){
                cout << "Error: Fanout limit cannot be less than 1/n" << endl;
                exit(1);
            }
            break;

        case dump_state_switch:
            dump_state = true;
            dump_state_cycle = atoi(optarg);
            break;

        case widen_switch:
            widen = true;
            break;

        case two_stride_switch:
            two_stride = true;
            break;
            
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
    uint64_t size;

    bool simulate = true;

    if(optind >= argc) {
        simulate = false;
    }

    // Parse Input
    if(!quiet && simulate){
     
        cout << "|------------------------|" << endl;
        cout << "|     Parsing  Input     |" << endl;
        cout << "|------------------------|" << endl;

        if(!input_string && simulate){
            cout << "Reading input stream from file: " << argv[optind] << endl;
        }
    
    }


    if(simulate){
        // Parse automata input file or input from command line
        input = parseInputStream(simulate, input_string, &size, argv, optind);
        
        if(size < 1){
            if(!quiet) {
                cout << "WARNING: Input file is empty! Refusing to simulate..." << endl;
            }
            simulate = false;
        }else{
            if(!quiet) {
                cout << "  Found " << size << " input symbols." << endl;
            }
        }

        if(!quiet){
            cout << endl;
        }
    }

    // Build automata
    if(!quiet){
     
        cout << "|----------------------------|" << endl;
        cout << "|      Parsing  Automata     |" << endl;
        cout << "|----------------------------|" << endl;

        cout << "Building automata from file: " << fn << endl;
    
    }


    // 
    Automata ap(fn);
    uint32_t automata_size = ap.getElements().size();
    uint32_t orig_automata_size = ap.getElements().size();

    ap.setQuiet(quiet);
    
    
    if(!quiet){
        ap.printGraphStats();
    }
    
    // Optimize automata before identifying connected components
    // "Global" optimizations
    // Start optimizations
    if(optimize_global) {
        if(!quiet){
            cout << "|--------------------------|" << endl;
            cout << "|   Global Optimizations   |" << endl;
            cout << "|--------------------------|" << endl;
         
            cout << "Starting Global Optimizations..." << endl; 
        }

        ap.optimize(remove_ors,
                    prefix_merge_global,
                    suffix_merge_global,
                    common_path_merge_global);
    }

    if(!quiet){
        cout << "|---------------------------|" << endl;
        cout << "|   Automata Partitioning   |" << endl;
        cout << "|---------------------------|" << endl;
        
    }

    // Partition automata into connected components
    if(!quiet)
        cout << "Finding connected components..." << endl;

    vector<Automata*> ccs;
    ccs = ap.splitConnectedComponents();    

     std::cout << "Full Automata " << std::to_string(ap.getElements().size()) << std::endl;

    int index = 0;
    for(auto cc: ccs)
        std::cout << "Automata " << index++ << " size: " << std::to_string(cc->getElements().size()) << std::endl;

    if(!quiet)
        cout << endl;

    // Combine connected components into N automata
    if(!quiet)
        cout << "Distributing " << ccs.size() << " distinct subgraphs among " << num_threads << " threads..." << endl;
    
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
            merged[counter % num_threads]->copyFlagsFrom(a);
        }

        counter++;
    }

    // std::cout << "Post merging" << std::endl;
    // index = 0;
    // for(auto cc: ccs)
    //     std::cout << "Automata " << index++ << " size: " << std::to_string(cc->getElements().size()) << std::endl;


    // finalize copied automata
    for(Automata *a : merged) {
        a->finalizeAutomata();
    }

    // std::cout << "Post finalizing" << std::endl;
    // index = 0;
    // for(auto cc: ccs)
    //     std::cout << "Automata " << index++ << " size: " << std::to_string(cc->getElements().size()) << std::endl;
    
    if(!quiet)
        cout << endl;
    
    // Preprocess all automata partitions
    // Set up multi-dimensional structure for parallelization
    Automata *automata[num_threads][num_threads_packets];
    

    if(optimize_local) {
        if(!quiet){
            cout << "|-------------------------|" << endl;
            cout << "|   Local Optimizations   |" << endl;
            cout << "|-------------------------|" << endl;
        }
    }

    counter = 0;
    for(Automata *a : merged) {
        /*********************
         * LOCAL OPTIMIZATIONS
         *********************/     
        // Optimize after connected component merging
        if(optimize_local) {
            if(!quiet)
                cout << "Starting Local Optimizations for Thread " << counter << "..." << endl; 

            //
            a->optimize(false, // never do or gate removal locally
                        prefix_merge_local,
                        suffix_merge_local,
                        common_path_merge_local);
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

        // Widen
        if(widen){
            if(!quiet) {
                cout<< "Widening automata..." <<endl;
                cout<<endl;
            }
            a->widenAutomata();
        }

        // 2-Stride
        if(two_stride){
            if(!quiet) {
                cout<< "2-Striding automata..." << endl;
            }
            Automata *strided = a->twoStrideAutomata();
            delete a;
            a = strided;
            if(!quiet){
                cout << "  Done!" << endl << endl;
            }

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
        
        // Save automata mnrl if desired
        if(to_mnrl) {
            a->automataToMNRLFile("automata_" + to_string(counter) + ".mnrl");
        }

        // Emit in Becchi format NFA
        if(to_nfa){
            a->automataToNFAFile("automata_" + to_string(counter) + ".nfa");
        }
        
        // Emit as HDL
        if(to_hdl) {
            a->automataToHDLFile("automata_" + to_string(counter) + ".v");
        }

        // Emit as HLS
        if(to_hls) {
            int split_factor = 5;
            a->automataToHLSFiles(num_automata, split_factor);
        }

        // Emit as split components
        if(split) {

            vector<Automata*> components = ap.splitConnectedComponents();    

            int num_components = components.size();
            int num_automata_per_file = num_components / split_count;

            for(int i = 0; i < split_count; i++){
                Automata first = Automata(*components[i * num_automata_per_file]);
                for(int j = 0; j < num_automata_per_file; j++)
                    first.unsafeMerge(components[i * num_automata_per_file + j]);
                if(i == (split_count - 1)){
                    // Put the remaining automata in the last split file
                    for(int j = split_count * num_automata_per_file; j < num_components; j++){
                        first.unsafeMerge(components[j]);
                    }
                }
                first.finalizeAutomata();
                if(to_mnrl)
                    first.automataToMNRLFile("automata_split_" + to_string(i) + ".mnrl");
                else
                    first.automataToANMLFile("automata_split_" + to_string(i) + ".anml");
            }
        }

        // Emit as .blif circuit file
        if(to_blif){
            if(!quiet)
                cout << "Emitting automata as .blif circuit..." << endl << endl;
            a->automataToBLIFFile("automata_" + to_string(counter) + ".blif");
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
        for (int tid = 0; tid < num_threads; tid++) {
            //for(Automata *a : merged) {

            uint64_t packet_offset = 0;              
            uint64_t packet_size = size/num_threads_packets;              
            // For each input packet of the input stream
            //for (int tid= 0; tid < num_threads; tid++) {
            for(int packet = 0; packet < num_threads_packets; packet++) {

                Automata *a = automata[tid][packet];

                /***************************
                 * RUNTIME FLAGS
                 ***************************/

                // enable runtime profiling
                a->setProfile(profile);

                // enable state dumping
                a->setDumpState(dump_state, dump_state_cycle);

                // enable report gathering
                a->setReport(report);

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
                                              size);
            
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
