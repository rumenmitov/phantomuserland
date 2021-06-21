import re
import subprocess

def generate_cflow_command(file_name, gcc_command):

    cc_options = re.findall(r"-I\S*|-include \S*|-D\S*|-imacros \S*|-nostdinc", gcc_command)
#     print(cc_options)

    # file_name = gcc_command.split(" ")[-1]
    # print(file_name)

    cc_command = "gcc -E " + " ".join(cc_options)
    # print(cc_command)
    
    cflow_command = 'cflow -i +_s --brief --cpp="{}" {}'.format(cc_command, file_name)
    
    return cflow_command
    
# print(generate_cflow_command(sample_gcc))

def run_cflow(cflow_cmd):
    res = subprocess.run(cflow_cmd, stdout=subprocess.PIPE, shell=True, check=True)
    return res.stdout


def parse_from_build_log(build_log_path):
    text = None

    with open("build_kernel.log", "r") as f:
        text = f.read()
    matches = re.findall(r"\!source_file\{(\S*)\}.*?\n(.*?)\n", text)

    files_to_cflow_commands = {}

    for m in matches:
        file_name = m[0]
        gcc_command = m[1]
        cflow_command = generate_cflow_command(file_name, gcc_command)

        if file_name in files_to_cflow_commands.keys():
            print("Warning: duplicating files: {}".format(file_name))
        else:
            files_to_cflow_commands[file_name] = cflow_command


    cflow_results = {}

    for file_name, cmd in files_to_cflow_commands.items():
        cflow_results[file_name] = run_cflow(cmd)

    return cflow_results

# Graph internals

from queue import Queue

class FuncNode:
    def __init__(self, name, sign, location):
        self.name = name
        self.sign = sign
        self.location = location
    
    def compare(self, other):
        if not isinstance(other, FuncNode):
            return False
        else:
            return self.name == other.name and self.sign == other.sign and self.location == other.location
    
    def is_complete(self):
        return (self.sign is not None) and (self.location is not None)
    

class Index:
    
    def __init__(self):
        self.store = {}
        self.relations = {}
        self.reverse_relations = {}
    
    def update_store(self, new_node):
        # If new absent info about node, update
        # otherwise return or give warning
        
        if (not new_node.name in self.store.keys()):
            self.store[new_node.name] = new_node
            
            # also update graphs with empty nodes
            self.relations[new_node.name] = set()
            self.reverse_relations[new_node.name] = set()
            
        else:
            found_node = self.store[new_node.name]
            if (new_node.is_complete() and found_node.is_complete()):
                if not found_node.compare(new_node):
                    print("WARNING : duplicating def of {}".format(new_node.name))
                    print("          old: {} at {}".format(found_node.sign, found_node.location))
                    print("          new: {} at {}".format(new_node.sign, new_node.location))
            if (new_node.is_complete() and not self.store[new_node.name].is_complete):
                self.store[new_node.name].sign = new_node.sign
                self.store[new_node.name].location = new_node.location
    
    def get_callers(self, name):
        return list(self.reverse_relations[name])
    
    def get_callees(self, name):
        return list(self.relations[name])
    
    def get_store(self):
        return self.store
    
    def _bfs(self, graph, start_nodes, term_condition, ignore_nodes):
        
        visited = set()
        
        subgraph = {}
        
        to_visit = Queue()
        
        for node in start_nodes:
            to_visit.put(node)
            visited.add(node)
        
        while (not to_visit.empty()):
            curr = to_visit.get()
            
            subgraph[curr] = set()
            
            if term_condition(curr):
                continue
            
            for next_func in graph[curr]:
                
                if next_func in ignore_nodes:
                    continue
                
                subgraph[curr].add(next_func)
                
                if not next_func in visited:
                    to_visit.put(next_func)
                    visited.add(next_func)
        
        # Just sanity check
        
        assert(len(subgraph.keys()) == len(visited))
        
        return subgraph
            
    
    def get_subgraph(self, starting_nodes, term_condition, ignore_nodes):
        return self._bfs(self.relations, starting_nodes, term_condition, ignore_nodes)
    
    def add(self, from_name, to_node):
        # from_name = from_node.name
        to_name = to_node.name
        
        # self.update_store(from_node)
        self.update_store(to_node)
        
        # caller->calee
        if (from_name in self.relations.keys()):
            self.relations[from_name].add(to_name)
        else:
            self.relations[from_name] = set([to_name])
        
        # calee->caller
        if (to_name in self.reverse_relations.keys()):
            self.reverse_relations[to_name].add(from_name)
        else:
            self.reverse_relations[to_name] = set([from_name])
            
    
    def debug_print(self):
        print("Store: ")
        for k, v in self.store.items():
            print("- [{}] {} {} {}".format(k, v.name, v.sign, v.location))
        print("Relations: ")
        for k, v in self.relations.items():
            print("- {}".format(k))
            for to_name in v:
                print("    - {}".format(to_name))


def parse_cflow_output(index, res_text):
    
    stack = []
    ident = 0
    delim = "    "
    prev_name = None
    
    for l in res_text.split("\n"):
        curr = l
        curr_ident = 0
        while (curr.startswith(delim)):
            curr_ident += 1
            curr = curr[len(delim):]
        
        func_name = None
        func_sig = None
        func_location = None
        
        curr = curr.strip()
        res = re.findall(r"^(\S+) \<(.*?) at (.*?)\>|^(\S+)", curr)
        if len(res) != 1:
            # print("Warning, skipping iteration with input '{}'".format(curr))
            continue
            
        res = res[0]
        if res[3]:
                # print("SYM: {}".format(res[3]))
                func_name = res[3]
        else:
                # print("DEC: {}, {}, {}".format(res[0], res[1], res[2]))
                func_name = res[0]
                func_sig = res[1]
                func_location = res[2]

        
        new_node = FuncNode(func_name, func_sig, func_location)
        
        if curr_ident > ident:
            stack.append(prev_name)
            ident = curr_ident
        
        if curr_ident < ident:
            stack.pop()
            ident = curr_ident
        
        if len(stack) > 0:
            index.add(stack[-1], new_node)
        else:
            index.update_store(new_node)
            
        prev_name = func_name
            
# Misc functions

def _dfs(graph, visited, start_node, depth):
    res = set([(depth, start_node)])
    
    if depth == 0:
        return res
    
    if start_node in graph.keys():
        for v in graph[start_node]:
            if not v in visited:
                visited.add(v)
                res.update(_dfs(graph, visited, v, depth - 1))
    
    return res

def get_neighbours(graph, start_node, depth):
    visited = set()
    res = {}
    raw_res = _dfs(graph, visited, start_node, depth)
    
    for curr_d, node in raw_res:
        curr_d = depth - curr_d
        if not curr_d in res.keys():
            res[curr_d] = set()
        res[curr_d].add(node)
    
    return res
        
