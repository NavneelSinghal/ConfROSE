#include <rose.h>
#include <iostream>
#include "parsers.hpp"

#ifdef DEBUG

#define debug(args...) {\
    std::string _s = #args;\
    replace(_s.begin(), _s.end(), ',', ' ');\
    std::stringstream _ss(_s);\
    std::istream_iterator <std::string> _it(_ss);\
    err(_it, args);\
}

void err (std::istream_iterator <std::string> it) {}
template <typename T, typename... Args>
void err (std::istream_iterator <std::string> it, T a, Args... args) {
    std::cerr << *it << " = " << a << std::endl;
    err(++it, args...);
}
#endif

//debugging stuff ends

std::map <std::string, std::set <std::string>> securityGraph;
std::map <std::string, std::pair <int, std::string>> securityAssociation;
std::map <std::string, std::map <std::string, std::string>> lca;
std::map <std::string, std::vector <std::string>> functionData;

std::string LCA(std::string a, std::string b) {
    if (lca[a][b] != "") return lca[a][b];
    else return lca[b][a];
}

#if 0

-------------------------- COMMENT TRAVERSAL FOR PARSING SECURITY ANNOTATIONS ---------------------------

#endif

class commentTraversal : public AstSimpleProcessing {
    public:
        virtual void visit (SgNode* n);
};

void commentTraversal::visit (SgNode* n) {

    SgLocatedNode* locatedNode = isSgLocatedNode(n);
    if (locatedNode == NULL) return;
    
    SgFunctionDeclaration* functionDeclaration = isSgFunctionDeclaration(n);
    bool isFunctionDeclaration = (functionDeclaration != NULL);
    
    bool isGlobalDeclaration;
    SgVariableDeclaration* variableDeclaration = isSgVariableDeclaration(n);
    if (variableDeclaration == NULL) {
        isGlobalDeclaration = false;
    }
    else {
        SgScopeStatement* scope = variableDeclaration -> get_scope();
        if (scope -> class_name() == "SgGlobal") {
            isGlobalDeclaration = true;
        }
        else {
            isGlobalDeclaration = false;
        }
    }

    if(!isGlobalDeclaration && !isFunctionDeclaration) return;

    int declarationType = (isGlobalDeclaration) ? GLOBAL_VARIABLE : FUNCTION_DECLARATION;
    //now populate vars - in the case of global stuff just put the variable name into vars, and in the case of function declaration, find the names of the variables
    //now for the addend - pass "-global" for global and the function name for function declaration (can't use "global" as this can be the name of a function).
    
    std::string objectName;
    std::vector <std::string> vars;
    if (declarationType == GLOBAL_VARIABLE) {
        auto varList = variableDeclaration -> get_variables();
        assert(varList.size() == 1);
        vars.push_back((varList[0] -> get_name()).getString());
        objectName = "-global";
    }
    else {
        for (auto &argument : (functionDeclaration -> get_parameterList()) -> get_args()) { //list of args
            vars.push_back((argument -> get_name()).getString());
        }
        objectName = (functionDeclaration -> get_name()).getString();
    }

    AttachedPreprocessingInfoType* comments = locatedNode -> getAttachedPreprocessingInfo();
    
    if (comments != NULL) {
        //note that security annotation needs to be one single comment, which is just before the function declaration
        //for this try parsing every comment which has relative position PreprocessingInfo::before and check if this is successful and take only the first one which is successful
        std::string comment;
        for (auto c : *comments) {
            if (checkAnnotation(c -> getString()) && (c -> getRelativePosition() == PreprocessingInfo::before)) {
                comment = c -> getString();
                break;
            }
        }
        if (comment == "") {
            throw "No security annotations found for this definition.\n";
        }
        auto functionStuff = annotationParser(comment, securityGraph, vars, declarationType, objectName);
        for (auto &x : functionStuff) {
            securityAssociation[x.first] = x.second;
        }
        
        if (objectName != "-global") {
            std::vector <std::string> listOfTypes;
            for (auto &argument : (functionDeclaration -> get_parameterList()) -> get_args()) {
                listOfTypes.push_back(securityAssociation[objectName + "::" + (argument -> get_name()).getString()].second);
            }
            functionData[objectName] = listOfTypes;
            if (objectName != "main") {
                //allow only void functions
                SgType* returnType = functionDeclaration -> get_orig_return_type();
                SgTypeVoid* voidType = isSgTypeVoid(returnType);
                if (voidType == NULL) {
                    std::cout << "Function type is non-void which is not allowed for a non-main function" << std::endl;
                    throw "Non-void function";
                }
            }
        }
    }
    else {
        throw "No security annotations found for this definition.\n";
    }
}

#if 0

------------------------- TYPE CHECKING TRAVERSAL ----------------------


   plan of action to do type checking - 
   
   first check the kind of the expression 

STATIC:
    - integer constant : type is public
    - variable - type is as given in the mapping from "global::variablename" to security level
    - argument - no security check - type is as specified in the mapping from "functionname::variablename" to security level
    - address (&x) - no security check - type is public
    - struct {e1, e2, ..., en} - find types of all elements and then find lca of all
    - struct extraction s.e - same security type as struct
    - e1 op e2 - find lca of the levels of e1 and e2
    - typecast - no security check
RUNTIME:
    - *e - evaluate e and finds the level corresponding to the memory location e, and assigns it this type 

    if this is not an expression, it can be a command or a program

    commands don't have type, they only have security checks associated to them
    so for static checks only, we can do this - since we have no runtime checks to insert,
    we can just typecheck everything statically

    commands - 
STATIC:
    - sequential check (inherent due to traversal)
    - assignment of variable - if static type of variable is higher or equal to that of the expression, only then is this valid (otherwise throw security error)
    - if and while - for static stuff, just check if the variable type for the boolean expression is public
    - calling a procedure - for static stuff, make sure that the argument types are at least as strict as the corresponding variables we pass here: this should be done using a case analysis of scopes and a simple lca check. also note that for this, you'll need to keep a set of a list of procedure arguments somewhere, in order, so that the security types can be checked statically.
RUNTIME:
    - assigning data whose type is statically determined to a pointer - insert corresponding runtime check as in the notes
    - assigning data whose type is determined at runtime to a variable - insert corresponding runtime check as in the notes
    - assigning data whose type is determined at runtime to a pointer - here we will need theta_3, so for that implement a parser for the security level comments just before this statement wherever this is needed

    program - 
STATIC: 
    - need to do nothing as all the static checks are done at compile-time and we don't have any runtime checks to propagate
RUNTIME:
    - need to take care of the runtime checks that come from each of the possibilities


RUNTIME GENERAL STUFF:
    when implementing the runtime checks as calls to functions (basically you would make the attribute a struct consisting of a type and a list of runtime type-checks), you would need to ensure that the security type-checks are copied up everywhere.


#endif 

class Type {
    public:
        std::string type;
        std::vector <Type> TypeVector;
        Type () : type("") {};
        Type (std::string s) : type(s) {};
        Type (std::string s, std::vector<Type> v) : type(s) {
            for (auto t : v) {
                TypeVector.push_back(t);
            }
        };
        Type (const Type& t) : type(t.type) {
            for (auto tt : t.TypeVector) {
                TypeVector.push_back(tt);
            }
        };
};

class Scope {
    public:
        std::string functionName;
        Scope () : functionName("") {};
        Scope (std::string s) : functionName(s) {};
        Scope (const Scope& s) : functionName(s.functionName) {};
};

class typeCheckingTraversal : public AstTopDownBottomUpProcessing<Scope, Type> {
    public:
        virtual Type evaluateSynthesizedAttribute(SgNode* n, Scope scope, SynthesizedAttributesList childAttributes);
        virtual Scope evaluateInheritedAttribute(SgNode* n, Scope scope = std::string("-global"));
};

Scope typeCheckingTraversal::evaluateInheritedAttribute(SgNode* n, Scope scope) {
    SgFunctionDeclaration* func = isSgFunctionDeclaration(n);
    if (func != NULL) {
        return Scope((func -> get_name()).getString());
    }
    return scope;
}

Type typeCheckingTraversal::evaluateSynthesizedAttribute(SgNode* n, Scope scope, SynthesizedAttributesList childAttributes) {
    //find what type of a node this is
    
    //expression
    
    //integer literal
    SgIntVal* intValue = isSgIntVal(n);
    if (intValue != NULL) {
        //std::cout << "integer constant value is public" << std::endl;
        return Type("-static::public");
    }

    //variable or argument
    //SgInitializedName* varSymbol = isSgInitializedName(n);
    SgVarRefExp* varSymbol = isSgVarRefExp(n);
    if (varSymbol != NULL) {
        //global variable
        //now check whether this was declared globally or not
        std::string rawName = (((varSymbol -> get_symbol()) -> get_name()).getString());
        std::string candidate = scope.functionName + "::" + rawName;
        if(securityAssociation.find(candidate) != securityAssociation.end()) {
            std::cout << "this is a function argument with type " << securityAssociation[candidate].second << std::endl; 
            return Type(securityAssociation[candidate].second);
        }
        else {
            std::string t = securityAssociation["-global::" + rawName].second;
            std::cout << "this is a global variable with type " << t << std::endl;
            //if (t == "") {
            //    throw "Dynamic security levels are not currently supported";
            //}
            //commenting this out because this causes an issue in the struct extraction otherwise
            return Type(t);
        }
    }

    //address (&x)
    SgAddressOfOp* addressOf = isSgAddressOfOp(n);
    if (addressOf != NULL) {
        std::cout << "this is an application of the address of operator" << std::endl;
        return Type("-static::public");
    }

    //struct definition or any list of expressions (also used in function calls)
    SgExprListExp* initList = isSgExprListExp(n);
    if (initList != NULL) {
        std::cout << "this is an initializer list used for initialising the struct" << std::endl;
        std::string s = "public";

        for (auto type : childAttributes) {
            assert(type.type.size() >= 9 && (type.type.substr(0, 9) == "-static::"));
            s = LCA(s, type.type.substr(9));
        }
        std::cout << "the type of this initializer list is " << "-static::" + s << std::endl;
        return Type("-static::" + s, childAttributes);
    }

    //struct extraction
    SgDotExp* dotOp = isSgDotExp(n);
    if (dotOp != NULL) {
        std::cout << "this is a struct access with type " << childAttributes[0].type << std::endl;
        return childAttributes[0];
    }

    //e1 op e2
    //DISALLOWING +=, -=, *=, /=, &=, ^=, |=, --, ++

    SgBitAndOp* bitAndOp = isSgBitAndOp(n);
    SgBitOrOp* bitOrOp = isSgBitOrOp(n);
    SgDivideOp* divideOp = isSgDivideOp(n);
    SgEqualityOp* equalityOp = isSgEqualityOp(n);
    SgGreaterOrEqualOp* geOp = isSgGreaterOrEqualOp(n);
    SgLessOrEqualOp* leOp = isSgLessOrEqualOp(n);
    SgGreaterThanOp* gtOp = isSgGreaterThanOp(n);
    SgLessThanOp* ltOp = isSgLessThanOp(n);
    SgLshiftOp* lsOp = isSgLshiftOp(n);
    SgRshiftOp* rsOp = isSgRshiftOp(n);
    SgMultiplyOp* multiplyOp = isSgMultiplyOp(n);
    SgModOp* modOp = isSgModOp(n);
    SgNotEqualOp* neOp = isSgNotEqualOp(n);
    SgOrOp* orOp = isSgOrOp(n);
    SgSubtractOp* subtractOp = isSgSubtractOp(n);
    SgAddOp* addOp = isSgAddOp(n);
    SgAndOp* andOp = isSgAndOp(n);

    //op e1 - for unary operations which don't change e1
    //DISALLOWING *e

    SgMinusOp* minusOp = isSgMinusOp(n);
    SgNotOp* notOp = isSgNotOp(n);

    if (
            bitOrOp != NULL || bitAndOp != NULL || 
            divideOp != NULL || multiplyOp != NULL || modOp != NULL || subtractOp != NULL || addOp != NULL || 
            equalityOp != NULL || geOp != NULL || leOp != NULL || gtOp != NULL || ltOp != NULL || neOp != NULL ||
            lsOp != NULL || rsOp != NULL ||  orOp != NULL || andOp != NULL ||
            minusOp != NULL || notOp != NULL
        ) {
        std::cout << "this is a binary/unary operator" << std::endl;
        std::string s = "public";
        for (auto type : childAttributes) {
            assert(type.type.size() >= 9 && (type.type.substr(0, 9) == "-static::"));
            s = LCA(s, type.type.substr(9));
        }
        std::cout << "the type of the result of the operator is " << "-static::" + s << std::endl;
        return Type("-static::" + s);
    }

    //typecast
    //handled by the default return of childAttributes[0]

    //assignment
    //for static typechecking, only handling assignment to global variables and no pointers, and the rhs must be statically typed
    
    SgAssignOp* assignment = isSgAssignOp(n);
    if (assignment != NULL) {
        std::cout << "This is a variable assignment" << std::endl;
        auto typeLeft = childAttributes[0];
        auto typeRight = childAttributes[1];
        std::cout << "Left variable type is " << typeLeft.type << std::endl;
        std::cout << "Right expr type is " << typeRight.type << std::endl;
        assert(typeLeft.type.size() >= 9);
        assert(typeRight.type.size() >= 9);
        assert(typeLeft.type.substr(0, 9) == "-static::");
        assert(typeRight.type.substr(0, 9) == "-static::");
        std::string leftSecurityLevel = typeLeft.type.substr(9);
        std::string rightSecurityLevel = typeRight.type.substr(9);
        if (LCA(leftSecurityLevel, rightSecurityLevel) != leftSecurityLevel) {
            throw "Invalid assignment - security types are incompatible";
        }
        return typeLeft;
    }

    //if
    
    SgIfStmt* ifStatement = isSgIfStmt(n);
    if (ifStatement != NULL) {
        std::cout << "This is an if statement" << std::endl;

        if (childAttributes[0].type != "-static::public") {
            throw "Can't branch on private data";
        }
        return Type("-static::public");
    }

    //while

    SgWhileStmt* whileStatement = isSgWhileStmt(n);
    if (whileStatement != NULL) {
        std::cout << "This is a while statement" << std::endl;

        if (childAttributes[0].type != "-static::public") {
            throw "Can't branch on private data";
        }
        return Type("-static::public");
    }
   
    //function call which is not a function pointer
    SgFunctionCallExp* functionCall = isSgFunctionCallExp(n);
    if (functionCall != NULL) {
        SgFunctionDeclaration* functionDeclaration = functionCall -> getAssociatedFunctionDeclaration();
        std::cout << "This is a function call" << std::endl;
        for (auto t : childAttributes) {
            std::cout << t.type << std::endl;
            for (auto s : t.TypeVector) {
                std::cout << "\t" + s.type << std::endl;
            }
        }
        auto attributes = childAttributes[1].TypeVector; //list of types in the initializer list (which is the argument list)
        int index = 0;
        auto functionTypeList = functionData[(functionDeclaration -> get_name()).getString()];
        for (auto &type : attributes) {
            std::string targetType = functionTypeList[index];
            assert(targetType.substr(0, 9) == "-static::");
            assert(type.type.substr(0, 9) == "-static::");
            auto s = targetType.substr(9);
            auto t = type.type.substr(9);
            std::cout << "Target type is " << s << std::endl;
            std::cout << "Argument type is " << t << std::endl;
            if (s != LCA(s, t)) {
                throw "Invalid assignment - security types are incompatible";
            }
            ++index;
        }
    }

    //default case just to propagate the child types up the tree for cases we do not care about
    return childAttributes[0];
}

int main (int argc, char* argv[]) {

    ROSE_INITIALIZE;

    argc--;
    char* securityFileName = argv[1];
    std::swap(argv[0], argv[1]);
    argv++;

    std::ifstream securityFile;
    securityFile.open(securityFileName);
    try {
        std::tie(lca, securityGraph) = securityFileParser(securityFile);
        for (auto x : lca) {
            for (auto w : x.second) {
                if (w.second != "") {
                    lca[w.first][x.first] = w.second;
                }
            }
        }
    }
    catch (char const* a) {
        std::cout << std::string(a) << std::endl;
        return 0;
    }
    securityFile.close();


    SgProject* project = frontend(argc, argv);

    ROSE_ASSERT(project != NULL);

    commentTraversal visitor_1;
    try {
        visitor_1.traverseInputFiles(project, preorder);
    }
    catch (char const* a) {
        std::cout << std::string (a) << std::endl;
        return 0;
    }

    for (auto &x : securityAssociation) {
        std::cout << x.first << " has security type " << x.second.first << " and attached variable/security level " << x.second.second << std::endl;
    }

    typeCheckingTraversal typeChecker;
    try {
        typeChecker.traverseInputFiles(project, Scope("-global"));
    }
    catch (char const* a) {
        std::cout << std::string (a) << std::endl;
        return 0;
    }

    return backend(project);
}
