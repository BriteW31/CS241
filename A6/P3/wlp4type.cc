#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <cctype>
#include <map>
#include <stdexcept>

using namespace std;

// Structure to represent a node in the parse tree
struct Node {
    // The exact line read from the input
    string rule;                
    // The left-hand side or the terminal kind
    string lhs;                 
    // The right-hand side symbols or the terminal lexeme
    vector<string> rhs;         
    // Pointers to the child subtrees
    vector<Node*> children;     
    // The synthesized type annotation (e.g., "long" or "long*")
    string type = "";           

    ~Node() {
        for (auto child : children) {
            delete child;
        }
    }
};

// Function to recursively parse and rebuild the tree from standard input
Node* parseTree() {
    string line;
    if (!getline(cin, line)) return nullptr;
    
    // Strip trailing carriage return if present (Fixes DOS-style ending issues)
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

    // If it's a non-terminal, build children based on the right-hand side length
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

// Global symbol table mapping variable names to their type ("long" or "long*")
map<string, string> symTable;

// Recursively processes body declarations to build the symbol table and verify types
void processDcls(Node* dclsNode) {
    if (dclsNode->children.empty()) return;
    
    // Evaluate left-recursive child first
    // dcls -> dcls dcl BECOMES NUM SEMI  |  dcls dcl BECOMES NULL SEMI
    processDcls(dclsNode->children[0]);
    
    // Process current declaration
    Node* dcl = dclsNode->children[1];
    string type = (dcl->children[0]->rhs.size() == 2) ? "long*" : "long";
    string name = dcl->children[1]->rhs[0];
    
    // Semantic Check: Duplicate variable name in wain
    if (symTable.count(name)) {
        throw runtime_error("Duplicate variable name");
    }
    symTable[name] = type;
    
    // Semantic Check: Declaration type must match initialization value type
    string initType = dclsNode->children[3]->lhs;
    if (initType == "NUM" && type != "long") {
        throw runtime_error("Type mismatch: NUM initialization assigned to long*");
    }
    if (initType == "NULL" && type != "long*") {
        throw runtime_error("Type mismatch: NULL initialization assigned to long");
    }
}

// Inspects the components of the 'main' procedure to bootstrap the Symbol Table
void buildSymTable(Node* mainNode) {
    // 1st Parameter of wain (dcl is at index 3)
    Node* dcl1 = mainNode->children[3];
    string type1 = (dcl1->children[0]->rhs.size() == 2) ? "long*" : "long";
    string name1 = dcl1->children[1]->rhs[0];
    symTable[name1] = type1;

    // 2nd Parameter of wain (dcl is at index 5)
    Node* dcl2 = mainNode->children[5];
    string type2 = (dcl2->children[0]->rhs.size() == 2) ? "long*" : "long";
    string name2 = dcl2->children[1]->rhs[0];
    
    // Semantic Check: Parameters cannot have duplicate names
    if (symTable.count(name2)) {
        throw runtime_error("Duplicate variable name between parameters");
    }
    symTable[name2] = type2;
    
    // Semantic Check: The second parameter of wain must be long
    if (type2 != "long") {
        throw runtime_error("Second parameter of wain is not long type");
    }

    // Process local declarations (dcls is at index 8)
    Node* dcls = mainNode->children[8];
    processDcls(dcls);
}

// Function to recursively perform type synthesis on the tree expressions
void synthesizeTypes(Node* n) {
    if (!n) return;
    
    // Post-order traversal: process children first
    for (auto c : n->children) {
        synthesizeTypes(c);
    }

    if (n->lhs == "NUM") {
        n->type = "long";
    } else if (n->lhs == "NULL") {
        n->type = "long*";
    } else if (n->lhs == "ID") {
        string name = n->rhs[0];
        // Semantic Check: Undeclared variable used
        if (symTable.find(name) != symTable.end()) {
            n->type = symTable[name];
        } else {
            throw runtime_error("Undeclared variable used: " + name);
        }
    } 
    else if (n->lhs == "lvalue") {
        if (n->rhs[0] == "ID") {
            n->type = n->children[0]->type;
        } else if (n->rhs[0] == "STAR") { 
            // lvalue -> STAR factor
            if (n->children[1]->type != "long*") throw runtime_error("Dereference of non-pointer");
            n->type = "long";
        } else if (n->rhs[0] == "LPAREN") { 
            // lvalue -> LPAREN lvalue RPAREN
            n->type = n->children[1]->type;
        }
    }
    else if (n->lhs == "factor") {
        if (n->rhs.size() == 1 && (n->rhs[0] == "NUM" || n->rhs[0] == "NULL" || n->rhs[0] == "ID")) {
            n->type = n->children[0]->type;      
        } else if (n->rhs[0] == "LPAREN") {
            n->type = n->children[1]->type;     
        } else if (n->rhs[0] == "AMP") { 
            // factor -> AMP lvalue
            if (n->children[1]->type != "long") throw runtime_error("Address-of applied to non-long");
            n->type = "long*";
        } else if (n->rhs[0] == "STAR") { 
            // factor -> STAR factor
            if (n->children[1]->type != "long*") throw runtime_error("Dereference of non-pointer");
            n->type = "long";
        } else if (n->rhs[0] == "NEW") { 
            // factor -> NEW LONG LBRACK expr RBRACK
            if (n->children[3]->type != "long") throw runtime_error("NEW size must be of type long");
            n->type = "long*";
        }
    } 
    else if (n->lhs == "term") {
        if (n->rhs.size() == 1) { 
            // term -> factor
            n->type = n->children[0]->type;         
        } else {
            // term -> term STAR/SLASH/PCT factor
            if (n->children[0]->type != "long" || n->children[2]->type != "long") {
                throw runtime_error("Invalid operands for STAR/SLASH/PCT (pointers not allowed)");
            }
            n->type = "long";
        }
    } 
    else if (n->lhs == "expr") {
        if (n->rhs.size() == 1) { 
            // expr -> term
            n->type = n->children[0]->type;         
        } else {
            string op = n->rhs[1]; // PLUS or MINUS
            string type1 = n->children[0]->type;
            string type2 = n->children[2]->type;
            
            if (op == "PLUS") {
                if (type1 == "long" && type2 == "long") n->type = "long";
                else if (type1 == "long*" && type2 == "long") n->type = "long*";
                else if (type1 == "long" && type2 == "long*") n->type = "long*";
                else throw runtime_error("Invalid operands for PLUS (e.g. adding two pointers)");
            } else if (op == "MINUS") {
                if (type1 == "long" && type2 == "long") n->type = "long";
                else if (type1 == "long*" && type2 == "long") n->type = "long*";
                else if (type1 == "long*" && type2 == "long*") n->type = "long";
                else throw runtime_error("Invalid operands for MINUS");
            }
        }
    }
}

// Function to output the pre-order traversal with type annotations
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
    // Clear global symbol table to ensure no contamination between batch tests
    symTable.clear();

    Node* root = parseTree();
    if (!root) return 0;
    
    Node* mainNode = nullptr;
    if (root->lhs == "start") {
        mainNode = root->children[1]->children[0]; 
    } else if (root->lhs == "procedures") {
        mainNode = root->children[0];
    } else if (root->lhs == "main") {
        mainNode = root;
    }

    if (mainNode) {
        try {
            // Pass 1: Extract declarations and validate scopes/types
            buildSymTable(mainNode);
            
            // Pass 2: Map variables and synthesize types bottom-up
            synthesizeTypes(root);
            
            // Semantic Check: The return expression (index 11) of wain must be long type
            Node* exprNode = mainNode->children[11];
            if (exprNode->type != "long") {
                throw runtime_error("Return expression of wain is not long type");
            }
        } catch (const exception& e) {
            // On hitting any of the documented runtime_errors, abort.
            cerr << "ERROR\n";
            delete root;
            return 1;
        }
    }

    // Print the valid .wlp4ti file
    printTree(root);
    
    delete root;
    return 0;
}
