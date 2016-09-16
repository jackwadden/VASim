#!/usr/bin/python

import os, sys, argparse, re

sys.path.append(os.path.dirname(os.path.abspath(sys.argv[0])) + '/pydot/lib/')
from pydotplus import *
from micronap.sdk import *

counters = {
    "roll pulse" : CounterMode.ROLLOVER_PULSE,
    "stop pulse" : CounterMode.STOP_PULSE,
    "stop hold"  : CounterMode.STOP_HOLD
}

booleans = {
    "NOT"  : (BooleanMode.NOT,1),
    "OR"   : (BooleanMode.OR,1),
    "AND"  : (BooleanMode.AND,1),
    "NAND" : (BooleanMode.NAND,1),
    "NOR"  : (BooleanMode.NOR,1),
    "SOP"  : (BooleanMode.SOP2,3),
    "POS"  : (BooleanMode.POS2,3),
    "NSOP" : (BooleanMode.NSOP,3),
    "NPOS" : (BooleanMode.NPOS,3)
}

ports = {
    "in"  : AnmlDefs.COUNT_ONE_PORT,
    "rst" : AnmlDefs.RESET_PORT,
    "t0"  : AnmlDefs.T1_PORT,
    "t1"  : AnmlDefs.T2_PORT,
    "t2"  : AnmlDefs.T3_PORT
}

def generate_anml(graph,network,elements,verbose,blockrows,lastId="",mapping=None):
    for subgraph in graph.get_subgraphs():
        
        if verbose:
            print 'Currently parsing subgraph',subgraph.get_name()
            sys.stdout.flush()
            try:
                os.fsync(sys.stdout.fileno())
            except:
                pass
        
        #get information about this subgraph
        #it should contain info on block and row numbers
        #if it doesn't, we don't care b/c no nodes at this level of the gv
        
        # graphviz escapes backslashes: decode removes those
        # pydot leaves quotes around label -> remove those with [1:-1]
        subgraph_name = subgraph.get_name().split("_")
        subgraph_id = subgraph_name[len(subgraph_name)-1]
        subgraph_label = (subgraph.get("label").decode('unicode_escape'))[1:-1]
        block_row_search = re.search('.*N(\d+).*B\((\d+),(\d+)\):R(\d+)',subgraph_label, flags=re.DOTALL)
        # group(1) is node, group(2) block, group(3) is block, group(4) is row
        # search returns None if not found
        if block_row_search is not None:
            block_id1,block_id2 = (int(block_row_search.group(2)),int(block_row_search.group(3)))
            row_id =  int(block_row_search.group(4))
            node_id = int(block_row_search.group(1))
        
        
        for node in subgraph.get_nodes():
            if verbose:
                print 'Currently parsing',subgraph.get_name(),node.get_name()
                sys.stdout.flush()
                try:
                    os.fsync(sys.stdout.fileno())
                except:
                    pass
            
            #get the node label
            # graphviz escapes backslashes: decode removes those
            # pydot leaves quotes around label -> remove those with [1:-1]
            node_label = (node.get("label").decode('unicode_escape'))[1:-1]
            
            # if no block and row, we are at a top-level netlist
            # these nodes are special elements, so they contain the block and row
            if block_row_search is None:
                block_row_search = re.search('.*N(\d+).*B\((\d+),(\d+)\):R(\d+)',node_label, flags=re.DOTALL)
                block_id1,block_id2 = (int(block_row_search.group(2)),int(block_row_search.group(3)))
                row_id =  int(block_row_search.group(4))
                node_id = int(block_row_search.group(1))
                
            # block and row should be set if we got here
            assert 'row_id' in locals() or 'row_id' in globals()
            assert 'block_id1' in locals() or 'block_id1' in globals()
            assert 'block_id2' in locals() or 'block_id2' in globals()
            
            node_type = node.get("shape")
            
            if mapping is not None:
                pass
            # handling element maps is not yet supported
            #    node_name = mapping.get_name(block,row,)
            else:
                node_name = node.get_name().decode('unicode_escape')
                if blockrows:
                    if node_type == "circle" or node_type == "doublecircle":
                        node_name_id = node_name + '_S' + str(lastId) + '_B'+str(block_id1)+'_'+str(block_id2)+'R'+str(row_id)
                    else:
                        node_name_id = node_name + '_S' + str(subgraph_id) + '_B'+str(block_id1)+'_'+str(block_id2)+'R'+str(row_id)
                else:
                    node_name_id = node_name
            if node_type == "circle":
                
                if (node.get("fillcolor"))[1:-1] == '#008000':
                    elements[node_name] = [network.AddSTE(node_label, anmlId=node_name_id, match=False, startType=AnmlDefs.START_OF_DATA)]
                else:
                    elements[node_name] = [network.AddSTE(node_label, anmlId=node_name_id, match=False)]
            elif node_type == "doublecircle":
                if (node.get("color"))[1:-1] == '#FF00FF':
                    elements[node_name] = [network.AddSTE(node_label, anmlId=node_name_id, match=True, startType=AnmlDefs.START_OF_DATA)]
                else:
                    elements[node_name] = [network.AddSTE(node_label, anmlId=node_name_id, match=True)]
            elif node_type == "record":
                node_label = (node.get("label").decode('unicode_escape'))[1:-1]
                if 'CTR_o' in node_label:
                    #this is a counter
                    #let's look for the count inside the label
                    node_count = int(re.search('count=(.*)}', node_label).group(1))
                    
                    #what kind of counting are we doing?
                    counter_type = re.search('count.*{(.*)\n', node_label).group(1)
                    
                    #does the counter report?
                    report = True if (node.get("color"))[1:-1] == '#FF00FF' else False
                        
                    #add the counter
                    elements[node_name] = [network.AddCounter(node_count, anmlId=node_name_id, match=report, mode=counters[counter_type])]
                elif 'BOOL_o' in node_label:
                    #this is a boolean
                    node_type = re.search('mode=(.*)', node_label).group(1)
                    boolean_mode,num_ports = booleans[node_type]
                    
                    #does the boolean report?
                    report = True if (node.get("color"))[1:-1] == '#FF00FF' else False
                    
                    #add the boolean
                    elements[node_name] = [network.AddBoolean(mode=boolean_mode, anmlId=node_name_id, terminals=num_ports, match=report)]
        
        generate_anml(subgraph,network,elements,verbose,blockrows,lastId=subgraph_id)
        
        for edge in subgraph.get_edges():
            source = edge.get_source().split(':',1)[0]
            dest = edge.get_destination().split(':',1)[0]
            port_split = edge.get_destination().rsplit(':',1)
            port = 0 if len(port_split) == 1 else ports[port_split[1]]
            
            #we need to do something special if going through an OR in a GoT
            if dest[-2:] == 'or':
                if dest in elements:
                    elements[dest].append(elements[source][0])
                else:
                    elements[dest] = [elements[source][0]]
            else:
                for e_source in elements[source]:
                    for e_dest in elements[dest]:
                        network.AddAnmlEdge(e_source,e_dest,port)
            


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("dotfile", help="the dot file to convert")
    parser.add_argument("outfile", help="the destination of the ANML file")
    parser.add_argument("-v","--verbose", help="print progress messages", action="store_true")
    parser.add_argument("-b","--blockrows", help="add block and row information to ANML IDs", action="store_true")
    args = parser.parse_args()
    
    verbose = args.verbose
    blockrows = args.blockrows
    
    if verbose:
        print "reading:", args.dotfile
        sys.stdout.flush()
        try:
            os.fsync(sys.stdout.fileno())
        except:
            pass
    
    if verbose:
        print "exporting to:", args.outfile
        sys.stdout.flush()
        try:
            os.fsync(sys.stdout.fileno())
        except:
            pass
        
    if verbose:
        if blockrows:
            msg = "adding blocks and rows to IDs"
        else:
            msg = "not adding blocks and rows to IDs"
        print msg
        sys.stdout.flush()
        try:
            os.fsync(sys.stdout.fileno())
        except:
            pass
    
    if verbose:
        print "reading dot file"
        sys.stdout.flush()
        try:
            os.fsync(sys.stdout.fileno())
        except:
            pass
    
    # our graphviz object
    dot_graph = graphviz.graph_from_dot_file(args.dotfile)
    
    if verbose:
        print "generating anml file"
        sys.stdout.flush()
        try:
            os.fsync(sys.stdout.fileno())
        except:
            pass
    
    # our anml object
    anml = Anml()
    
    # our anml network
    anmlNet = anml.CreateAutomataNetwork(dot_graph.get_name())   

    generate_anml(dot_graph,anmlNet,dict(),verbose,blockrows)

    
    if verbose:
        print "saving anml file"
        sys.stdout.flush()
        try:
            os.fsync(sys.stdout.fileno())
        except:
            pass
        
    anmlNet.ExportAnml(args.outfile)
    
if __name__ == '__main__':
    main()