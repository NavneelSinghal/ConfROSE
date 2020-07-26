#ifndef _PARSERS
#define _PARSERS

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <set>
#include <functional>
#include <cassert>
#include <utility>
#include <vector>
#include <algorithm>
#include <cmath>
#include <iterator>

#define FUNCTION_DECLARATION 1
#define GLOBAL_VARIABLE 2

//debugging stuff follows

#define debug(args...) {\
    std::string _s = #args;\
    replace(_s.begin(), _s.end(), ',', ' ');\
    std::stringstream _ss(_s);\
    std::istream_iterator <std::string> _it(_ss);\
    err(_it, args);\
}

//debugging stuff ends

bool checkAnnotation (const std::string &comment);
std::map <std::string, std::pair<int, std::string>> annotationParser(const std::string &comment, const std::map <std::string, std::set <std::string>> &graph, const std::vector <std::string> &vars, const int typeOfStatement, const std::string &statementName);

std::pair <std::map <std::string, std::map <std::string, std::string>>, std::map <std::string, std::set <std::string>>> securityFileParser(std::ifstream & securityFile);

#endif
