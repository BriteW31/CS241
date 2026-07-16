#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <cctype>
#include <map>
#include <stdexcept>

using namespace std;

struct Node {
    string rule;                
    string lhs;                 
    vector<string> rhs;         
    vector<Node*> children;     
    string type = "";           

    ~Node() {
        for (auto child : children) {
            delete child;
        }
    }
};

map<string, vector<string>> procSignatures; 
map<string, string> currentSymTable;        
string currentProc;

// Function to recursively parse and rebuild the tree from standard input
Node* parseTree() {
    string line;
    if (!getline(cin, line)) return nullptr;
    
    // Strip trailing carriage return if present (Fixes DOS-style ending issues on Marmoset)
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }

    Node* n = new Node();
    n->rule = line;
    
    istringstream iss(line);
    iss >> n->lhs;
    string token;
    while (iss >> token) {
        n->rhs.push_back(token);
    }

    // Terminals in WLP4 are entirely uppercase. Non-terminals are lowercase.
    bool isTerminal = true;
    for (char c : n->lhs) {
        if (islower(c)) {
            isTerminal = false;
            break;
        }
    }

    if (!isTerminal) {
        if (n->rhs.size() == 1 && n->rhs[0] == ".EMPTY") {
            // Empty derivation, 0 children
        } else {
            for (size_t i = 0; i < n->rhs.size(); i++) {
                n->children.push_back(parseTree());
            }
        }
    }
    
    return n;
}

// Helpers for DCL nodes
string getDclType(Node* dcl) {
    return (dcl->children[0]->rhs.size() == 2) ? "long*" : "long";
}

string getDclName(Node* dcl) {
    return dcl->children[1]->rhs[0];
}

// Bottom-up recursive function to extract argument types from an arglist node
vector<string> extractArgTypes(Node* arglist) {
    vector<string> types;
    types.push_back(arglist->children[0]->type); // expr's type
    if (arglist->rhs.size() == 3) {              // expr COMMA arglist
        vector<string> rest = extractArgTypes(arglist->children[2]);
        types.insert(types.end(), rest.begin(), rest.end());
    }
    return types;
}

void synthesizeTypes(Node* n) {
    if (!n) return;
    
    // Post-order traversal: process children first
    for (auto c : n->children) {
        synthesizeTypes(c);
    }

    // Terminals
    if (n->lhs == "NUM") {
        n->type = "long";
    } else if (n->lhs == "NULL") {
        n->type = "long*";
    } 
    
    // Non-Terminals
    else if (n->lhs == "lvalue") {
        if (n->rhs[0] == "ID") {
            string name = n->children[0]->rhs[0];
            if (!currentSymTable.count(name)) {
                if (procSignatures.count(name)) throw runtime_error("Cannot use procedure as variable");
                throw runtime_error("Undeclared variable");
            }
            n->type = currentSymTable[name];
            n->children[0]->type = n->type; // Annotate terminal ID
        } else if (n->rhs[0] == "STAR") { 
            if (n->children[1]->type != "long*") throw runtime_error("Dereference of non-pointer");
            n->type = "long";
        } else if (n->rhs[0] == "LPAREN") { 
            n->type = n->children[1]->type;
        }
    }
    else if (n->lhs == "factor") {
        if (n->rhs[0] == "ID" && n->rhs.size() == 1) {
            string name = n->children[0]->rhs[0];
            if (!currentSymTable.count(name)) {
                if (procSignatures.count(name)) throw runtime_error("Cannot use procedure as variable");
                throw runtime_error("Undeclared variable");
            }
            n->type = currentSymTable[name];
            n->children[0]->type = n->type;
        } else if (n->rhs[0] == "ID" && n->rhs.size() == 3) {
            // ID LPAREN RPAREN
            string name = n->children[0]->rhs[0];
            if (!procSignatures.count(name)) throw runtime_error("Undeclared procedure");
            if (procSignatures[name].size() != 0) throw runtime_error("Argument count mismatch");
            n->type = "long";
        } else if (n->rhs[0] == "ID" && n->rhs.size() == 4) {
            // ID LPAREN arglist RPAREN
            string name = n->children[0]->rhs[0];
            if (!procSignatures.count(name)) throw runtime_error("Undeclared procedure");
            vector<string> args = extractArgTypes(n->children[2]);
            if (args != procSignatures[name]) throw runtime_error("Argument type mismatch");
            n->type = "long";
        } else if (n->rhs[0] == "NUM") {
            n->type = "long";
            n->children[0]->type = "long";
        } else if (n->rhs[0] == "NULL") {
            n->type = "long*";
            n->children[0]->type = "long*";
        } else if (n->rhs[0] == "LPAREN") {
            n->type = n->children[1]->type;     
        } else if (n->rhs[0] == "AMP") { 
            if (n->children[1]->type != "long") throw runtime_error("Address-of applied to non-long");
            n->type = "long*";
        } else if (n->rhs[0] == "STAR") { 
            if (n->children[1]->type != "long*") throw runtime_error("Dereference of non-pointer");
            n->type = "long";
        } else if (n->rhs[0] == "NEW") { 
            if (n->children[3]->type != "long") throw runtime_error("NEW size must be of type long");
            n->type = "long*";
        } else if (n->rhs[0] == "GETCHAR") {
            n->type = "long";
        }
    } 
    else if (n->lhs == "term") {
        if (n->rhs.size() == 1) { 
            n->type = n->children[0]->type;         
        } else {
            if (n->children[0]->type != "long" || n->children[2]->type != "long") {
                throw runtime_error("Invalid operands for STAR/SLASH/PCT (pointers not allowed)");
            }
            n->type = "long";
        }
    } 
    else if (n->lhs == "expr") {
        if (n->rhs.size() == 1) { 
            n->type = n->children[0]->type;         
        } else {
            // PLUS or MINUS, followed by two expr children
            string op = n->rhs[1]; 
            string type1 = n->children[0]->type;
            string type2 = n->children[2]->type;
            
            if (op == "PLUS") {
                if (type1 == "long" && type2 == "long") n->type = "long";
                else if (type1 == "long*" && type2 == "long") n->type = "long*";
                else if (type1 == "long" && type2 == "long*") n->type = "long*";
                else throw runtime_error("Invalid operands for PLUS");
            } else if (op == "MINUS") {
                if (type1 == "long" && type2 == "long") n->type = "long";
                else if (type1 == "long*" && type2 == "long") n->type = "long*";
                else if (type1 == "long*" && type2 == "long*") n->type = "long";
                else throw runtime_error("Invalid operands for MINUS");
            }
        }
    }
    else if (n->lhs == "test") {
        string type1 = n->children[0]->type;
        string type2 = n->children[2]->type;
        
        // Pointers and longs can be compared, but only to identical types.
        if (type1 != type2) {
            throw runtime_error("Test expressions type mismatch");
        }
    }
    else if (n->lhs == "statement") {
        if (n->rhs[0] == "lvalue") {
            if (n->children[0]->type != n->children[2]->type) {
                throw runtime_error("Assignment type mismatch");
            }
        } else if (n->rhs[0] == "PRINTLN") {
            if (n->children[2]->type != "long") {
                throw runtime_error("PRINTLN requires long");
            }
        } else if (n->rhs[0] == "PUTCHAR") {
            if (n->children[2]->type != "long") {
                throw runtime_error("PUTCHAR requires long");
            }
        } else if (n->rhs[0] == "DELETE") {
            if (n->children[3]->type != "long*") {
                throw runtime_error("DELETE requires long*");
            }
        }
    }
}

// Process all requirements
void processDcls(Node* dclsNode) {
    if (dclsNode->rhs.size() == 1 && dclsNode->rhs[0] == ".EMPTY") return;
    
    processDcls(dclsNode->children[0]);
    
    Node* dcl = dclsNode->children[1];
    string type = getDclType(dcl);
    string name = getDclName(dcl);
    
    if (currentSymTable.count(name)) throw runtime_error("Duplicate local var");
    
    currentSymTable[name] = type;
    // Annotate terminal ID
    dcl->children[1]->type = type; 
    
    string initType = dclsNode->children[3]->lhs;
    string valType = (initType == "NUM") ? "long" : "long*";
    // Annotate NUM/NULL terminal
    dclsNode->children[3]->type = valType; 
    
    if (type != valType) throw runtime_error("Invalid initialization");
}

void processParamList(Node* paramlist, vector<string>& paramTypes) {
    Node* dcl = paramlist->children[0];
    string type = getDclType(dcl);
    string name = getDclName(dcl);
    
    if (currentSymTable.count(name)) throw runtime_error("Duplicate param");
    if (procSignatures.count(name)) throw runtime_error("Param shadows proc");
    
    currentSymTable[name] = type;
    dcl->children[1]->type = type;
    paramTypes.push_back(type);
    
    if (paramlist->rhs.size() == 3) {
        processParamList(paramlist->children[2], paramTypes);
    }
}

void processProcedure(Node* procNode) {
    // current procedure ID
    currentProc = procNode->children[1]->rhs[0]; 
    if (procSignatures.count(currentProc)) throw runtime_error("Duplicate procedure name");
    
    currentSymTable.clear();
    vector<string> paramTypes;
    
    Node* params = procNode->children[3];
    if (params->rhs.size() == 1 && params->rhs[0] != ".EMPTY") {
        processParamList(params->children[0], paramTypes);
    }
    
    procSignatures[currentProc] = paramTypes;
    
    processDcls(procNode->children[6]);

    // Recursively type-check the entire statements body
    synthesizeTypes(procNode->children[7]);

    // Return expr
    synthesizeTypes(procNode->children[9]); 
    if (procNode->children[9]->type != "long") throw runtime_error("Procedure return type not long");
}

void processWain(Node* wainNode) {
    currentProc = "wain";
    if (procSignatures.count("wain")) throw runtime_error("Duplicate wain");
    
    currentSymTable.clear();
    vector<string> paramTypes;
    
    Node* dcl1 = wainNode->children[3];
    Node* dcl2 = wainNode->children[5];
    
    string t1 = getDclType(dcl1);
    string n1 = getDclName(dcl1);
    string t2 = getDclType(dcl2);
    string n2 = getDclName(dcl2);
    
    if (n1 == n2) throw runtime_error("Duplicate param in wain");
    if (t2 != "long") throw runtime_error("Wain second param must be long");
    if (procSignatures.count(n1) || procSignatures.count(n2)) throw runtime_error("Param shadows proc");
    
    currentSymTable[n1] = t1;
    dcl1->children[1]->type = t1;
    paramTypes.push_back(t1);
    
    currentSymTable[n2] = t2;
    dcl2->children[1]->type = t2;
    paramTypes.push_back(t2);
    
    procSignatures[currentProc] = paramTypes;
    
    processDcls(wainNode->children[8]);

    //Recursively type-check the entire statements body
    synthesizeTypes(wainNode->children[9]);

    // Return expr
    synthesizeTypes(wainNode->children[11]); 
    if (wainNode->children[11]->type != "long") throw runtime_error("Wain return type not long");
}

void processProcedures(Node* procs) {
    if (procs->rhs.size() == 2) {
        // Analyze the procedure locally
        processProcedure(procs->children[0]); 
        // Descend linearly
        processProcedures(procs->children[1]); 
    } else {
        processWain(procs->children[0]);
    }
}

void printTree(Node* n) {
    if (!n) return;
    
    cout << n->rule;
    if (n->type != "") cout << " : " << n->type;
    cout << "\n";
    
    for (auto c : n->children) {
        printTree(c);
    }
}

int main() {
    procSignatures.clear();
    currentSymTable.clear();

    Node* root = parseTree();
    if (!root) return 0;

    Node* mainNode = nullptr;
    if (root->lhs == "start") {
        mainNode = root->children[1]; 
    } else if (root->lhs == "procedures") {
        mainNode = root;
    } 

    if (mainNode) {
        try {
            processProcedures(mainNode);
        } catch (const exception& e) {
            cerr << "ERROR\n";
            delete root;
            return 1;
        }
    }

    printTree(root);
    delete root;
    return 0;
}
