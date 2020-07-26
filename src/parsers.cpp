#include "parsers.hpp"

void err (std::istream_iterator <std::string> it) {}
template <typename T, typename... Args>
void err (std::istream_iterator <std::string> it, T a, Args... args) {
    std::cerr << *it << " = " << a << std::endl;
    err(++it, args...);
}

bool checkAnnotation (const std::string &comment) {
    
    //remove the //
    std::string commentContent = comment.substr(2);
    
    //tokenize
    std::istringstream s(commentContent);
    std::string firstWord;
    
    if (s >> firstWord) {
        if (firstWord.substr(0, std::min(9, (int) firstWord.size())) == "@security") return true;
    }
    
    return false;
}

std::map <std::string, std::pair<int, std::string>> annotationParser (const std::string &comment, const std::map <std::string, std::set <std::string>> &graph, const std::vector <std::string> &vars, const int typeOfStatement, const std::string &statementName) {

    std::string commentContent = comment.substr(2);
    std::map <std::string, std::pair<int, std::string>> result;

    int processedAnnotations = 0;
    int position = 0;

    while (position < (int) commentContent.size() && commentContent[position] != '(') {
        position++;
    }

    //ensure that there is no stray text after @security
    std::string target = "@security";
    int index = 0;
    int targetIndex = 0;
    for (int i = 0; i < position; i++) {
        if (targetIndex < (int) target.size() && target[targetIndex] == commentContent[i]) {
            targetIndex++;
        }
        else if (!isspace(commentContent[i])) {
            throw "Security annotation syntax error - stray text after @security.\n";
        }
    }
    
    while (position < (int) commentContent.size()) {
        
        processedAnnotations++;
        
        int oldPosition = position;
        while (position < (int) commentContent.size() && commentContent[position] != ')') {
            position++;
        }
        if (position == (int) commentContent.size()) {
            throw "Security annotation syntax error - possible issues with pairs.\n";
        }

        std::string annotation = commentContent.substr(oldPosition + 1, position - oldPosition - 1);

        position++;
        while (position < (int) commentContent.size() && commentContent[position] != '(') {
            if (!isspace(commentContent[position])) {
                throw "Security annotation syntax error - stray text between annotations.\n";
            }
            position++;
        }

        int countComma = std::count(annotation.begin(), annotation.end(), ',');
        int countLparen = std::count(annotation.begin(), annotation.end(), '(');

        if (countComma != 1 || countLparen != 0) {
            throw "Security annotation syntax error - possible issues with pairs.\n";
        }

        //replace , with spaces and ensure correct , placement
        bool foundSpace = false;
        bool startedOthers = false;
        bool startedOthers2 = false;
        for (auto &c : annotation) {
            if (c == ',') {
                if (!startedOthers2) c = ' ';
                else {
                    throw "Security annotation syntax error - possible issues with comma placement.\n";
                }
            }
            else if (isspace(c) && startedOthers) {
                foundSpace = true;
            }
            else if (!isspace(c)) {
                if (!foundSpace) startedOthers = true;
                else startedOthers2 = true;
            }
        }
        
        std::istringstream tokens(annotation);
        std::string variable, option, dependency;
        
        if (!(tokens >> variable)) {
            throw "Security annotation syntax error - too few elements.\n";
        }
        if (!(tokens >> option)) {
            throw "Security annotation syntax error - too few elements.\n";
        }
        if (!(tokens >> dependency)) {
            throw "Security annotation syntax error - too few elements.\n";
        }
        std::string remaining;
        if (tokens >> remaining) {
            throw "Security annotation syntax error - too many elements.\n";
        }

        //now analysis as usual
        
        //find the variable in vars
        bool foundDependency = false;
        bool foundVariable = false;
        for (auto &v : vars) {
            if (v == variable) {
                foundVariable = true;
                break;
            }
        }
        if (!foundVariable) {
            throw "Variable not found in the list of function arguments.\n";
        }

        //now work with security levels
        int dependencyType = 0;
        
        if (option == "-s") {
            if (graph.count(dependency) == 0) {
                throw "Security level not found.\n";
            }
            dependencyType = 1;
        }
        else if (option == "-a" && typeOfStatement == FUNCTION_DECLARATION) {

            //note that here we do NOT allow self-dependency (like (name, -a name))
            //we are making sure that the security level of a function argument variable should only depend on variables (function arguments) to the left of it, however since we search the whole list to the left of the variable, the security levels of the arguments can be specified in any order, and not necessarily in the order they appear in the function signature.
            
            for (int i = 0; vars[i] != variable; i++) {
                if (vars[i] == dependency) {
                    foundDependency = true;
                    break;
                }
            }
            if (!foundDependency) {
                throw "Security level variable not found in the list of function arguments to the left of the corresponding function argument.\n";
            }
            dependencyType = 2;
        }
        else {
            throw "Security annotation syntax error.\n";
        }
        
        //now we prepend variable name with (statementName + "::") and then store it as a key into the map with the integer part of the pair being dependencyType, and the string part of the pair being the dependency variable/security level name
        
        std::string newVariableName = statementName + "::" + variable;
        
        if (result.count(newVariableName) != 0) {
            throw "Security level specification done multiple times for the same variable.\n";
        }
        else {
            dependency = ((dependencyType == 1) ? "-static" : statementName) + "::" + dependency;
            result[newVariableName] = make_pair(dependencyType, dependency);
        }
    }
    
    if (processedAnnotations != vars.size()) {
        throw "Security annotations incompletely specified.\n";
    }
    
    return result;
    
}

//auxiliary function that checks whether in a given DAG, every pair of elements has a join or not

std::map <std::string, std::map <std::string, std::string>> checkJoin (const std::map <std::string, std::set <std::string>> &graph, const std::string start, const std::string finish) {

    std::map <std::string, int> visited;

    std::vector <std::string> topologicalSort;

    std::function <void(std::string)> dfs = [&] (const std::string v) -> void {
        if (!visited[v]) {
            visited[v] = 1;
            try { //in case graph.at(v) does not exist, which is basically when v is private
                for (auto x : graph.at(v)) {
                    dfs(x);
                }
            }
            catch (std::out_of_range) {
                ;
            }
            topologicalSort.push_back(v);
        }
    };

    dfs(start);

    //now topologicalSort is the opposite order of a topologically sorted array of vertices

    std::map <std::string, std::map <std::string, std::string>> join;
    
    for (auto &x : topologicalSort) {
        join[x][x] = x;
    }

    int n = topologicalSort.size();

    
    bool works = true;
    for (int i = 1; i < n; i++) {

        std::string atI = topologicalSort[i];

        for (int j = 0; j < i; j++) {

            std::string currentJoin = finish;
            std::string atJ = topologicalSort[j];
            
            for (auto x : graph.at(atI)) {

                std::string parentJoin = join[atJ][x];
                
                //if (parentJoin == "") parentJoin = join[x][atJ];
                
                assert(parentJoin != "");
                
                if (join[parentJoin][currentJoin] == currentJoin) {// || join[currentJoin][parentJoin] == currentJoin) {
                    currentJoin = parentJoin;
                }


            }
            
            bool verify = true;
            
            for (auto x : graph.at(atI)) {
                
                std::string parentJoin = join[atJ][x];

                //if (parentJoin == "") parentJoin = join[x][atJ];
                
                assert(parentJoin != "");

                if (join[parentJoin][currentJoin] != parentJoin) {// && join[currentJoin][parentJoin] != parentJoin) {
                
                    verify = false;
                    break;
                
                }
            }
            
            if (verify) {
                join[atI][atJ] = currentJoin;
                join[atJ][atI] = currentJoin; //added later
            }
            else {
                works = false;
                break;
            }
        }
        if (!works) {
            break;
        }


    }

    if (!works) {
        throw "This directed acyclic graph is not a lattice.\n";
    }

    return join;

}


//assumes security file is a list of (child, parent) pairs separated by a space 
//public and private are to be mentioned in the file itself, constructing a lattice needs to be done by the programmer
//we simply parse and check the validity of the the lattice - first by checking that it is a dag, then lattice
//we expect the programmer to give as input the hasse diagram of the lattice, which is the covering relation of the poset formed by the lattice
//and not just any relation that fully describes the lattice 

std::pair<std::map <std::string, std::map <std::string, std::string>>, std::map <std::string, std::set <std::string>>> securityFileParser (std::ifstream &securityFile) {

    std::map <std::string, std::set<std::string>> parents, children;
    std::set <std::string> securityLevels;

    std::string child, parent;
    
    while (securityFile >> child) {

        if (!(securityFile >> parent)) {
            throw "Parent not found for some level in the security description file.\n";
        }

#if 0
        //not really necessary
        //check that parent and child both do not start with numbers
        assert(!isdigit(parent[0]));
        assert(!isdigit(child[0]));
#endif 

        //check that public is not the parent of any level
        if (parent == "public") {
            throw "Public can not be the parent of any level.\n";
        }

        //check that private is not the child of any level
        if (child == "private") {
            throw "Private can not be the child of any level.\n";
        }
        
        //check for self-loops
        if (parent == child) {
            throw "Self loop found in the security description file.\n";
        }

        //check for multiple edges
        if (parents[child].find(parent) != parents[child].end()) {
            throw "Multiple edges found in the security description file.\n";
        }
        
        parents[child].insert(parent);
        children[parent].insert(child);
        securityLevels.insert(child);
        securityLevels.insert(parent);
    }

    //check for cycles
    std::map <std::string, int> colour;
    std::function<bool(std::string)> isCyclic = [&] (std::string v) -> bool {
        colour[v] = 1;
        for (auto w : parents[v]) {
            if (colour[w] == 1) {
                return true;
            }
            if (colour[w] == 0) {
                if (isCyclic(w)) return true;
            }
        }
        colour[v] = 2;
        return false;
    };

    for (auto &x : securityLevels) {
        if (colour[x] == 0) {
            if (isCyclic(x)) {
                throw "Security levels form a cycle in the security description file.\n";
            }
        }
    }

    for (auto &x : parents) {
        if (x.second.empty()) {
            if (x.first != "private") {
                throw "private and at least one security level do not have a join.\n";
            }
        }
    }

    for (auto &x : children) {
        if (x.second.empty()) {
            if (x.first != "public") {
                throw "public and at least one security level do not have a meet.\n";
            }
        }
    }

    long long int edges = 0;
    long long int vertices = 0;
    for(auto &x : parents) {
        edges += x.second.size();
        vertices ++;
    }


    //notice that the next check does not say that the specification is a covering relation, however it just checks that there are not too many edges, in the sense that any covering relation has at most these many edges, and thus for any security lattice, a minimal specification would always pass through this check.

    if (edges >= ceil(pow(2*vertices + 10, 1.5) / 3.0)) { //error analysis needed, but should be okay, since a^1.5 = a sqrt(a)
        throw "This directed acyclic graph has many more edges than required for a sufficient specification of the lattice.\n";
    }
    //checks whether any two elements have a join
    auto returnValue = checkJoin(parents, "public", "private");
    //checks whether any two elements have a meet
    checkJoin(children, "private", "public");

    //return the graph
    return {returnValue, parents};

}

