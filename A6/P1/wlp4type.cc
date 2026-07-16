#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <cctype>

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

// Function to recursively perform type synthesis on the tree
void synthesizeTypes(Node* n) {
    if (!n) return;
    
    // Post-order traversal: process children first
    for (auto c : n->children) {
        synthesizeTypes(c);
    }

    // Annotate nodes depending on the grammar rules provided
    if (n->lhs == "factor") {
        if (n->rhs[0] == "NUM") {
            n->type = "long";
            n->children[0]->type = "long";      
        } else if (n->rhs[0] == "NULL") {
            n->type = "long*";
            n->children[0]->type = "long*";     
        } else if (n->rhs[0] == "LPAREN") {
            n->type = n->children[1]->type;     
        }
    } else if (n->lhs == "term") {
        n->type = n->children[0]->type;         
    } else if (n->lhs == "expr") {
        n->type = n->children[0]->type;         
    } else if (n->lhs == "dcl") {
        // children[0] is the type, children[1] is the ID
        if (n->children[0]->rhs.size() == 2) {  
            n->children[1]->type = "long*";     
        } else {
            n->children[1]->type = "long";
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
    Node* root = parseTree();
    if (!root) return 0;
    
    synthesizeTypes(root);

    // Locate the 'main' procedure node
    Node* mainNode = nullptr;
    if (root->lhs == "procedures") {
        mainNode = root->children[0];
    } else if (root->lhs == "main") {
        mainNode = root;
    }

    if (mainNode) {
        // In the restricted grammar, 'dcl' components are strictly at indices 3 and 5,
        // and the return 'expr' is at index 11.
        Node* dcl1 = mainNode->children[3];
        Node* dcl2 = mainNode->children[5];
        Node* exprNode = mainNode->children[11];

        // The ID terminal is the second child (index 1) of the 'dcl' node
        Node* id1 = dcl1->children[1];
        Node* id2 = dcl2->children[1];

        // lexeme of ID1, ID2
        string name1 = id1->rhs[0]; 
        string name2 = id2->rhs[0]; 

        // Semantic Check: The two parameters of wain have the same name.
        // Semantic Check: The second parameter of wain is not long type.
        // Semantic Check: The return expression of wain is not long type.
        if (name1 == name2 || id2->type != "long" || exprNode->type != "long") {
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
