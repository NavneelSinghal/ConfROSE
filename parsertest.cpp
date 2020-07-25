#include "parsers.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    
    assert(argc == 2);
    std::ifstream securityFile;
    securityFile.open(argv[1]);
    std::map <std::string, std::set <std::string>> graph;
    try {
        graph = securityFileParser(securityFile);
    }
    catch (char const* a) {
        std::cout << std::string(a) << std::endl;
    }
    for(auto x : graph) {
        std::cout << x.first << "\n";
        for(auto y : x.second) {
            std::cout << y << " ";
        }
        std::cout << "\n";
    }
    securityFile.close();
    

    std::cout << "Enter comment\n";
    int n = 1;
    std::string s;
    bool found = false;
    for (int i = 0; i < n; i++) {
        std::getline(std::cin, s);
        if(checkAnnotation(s)) {
            found = true;
            break;
        }
    }
    if(!found) {
        std::cout << "No annotations found.\n";
        return 0;
    }

    std::cout << "Enter function name\n";
    std::string funcname;
    std::cin >> funcname;
    std::cout << "Enter number of arguments\n";
    std::cin >> n; //number of arguments
    std::vector <std::string> args(n);
    for (auto &x : args) std::cin >> x;
    try {
        auto ans = annotationParser(s, graph, args, 1, funcname);
        for (auto &x : ans) {
            std::cout << x.first << " " << x.second.first << " " << x.second.second << std::endl;
        }
    }
    catch (char const* a) {
        std::cout << std::string(a) << std::endl;
    }
}
