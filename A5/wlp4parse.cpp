#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include "wlp4data.h"

using namespace std;

struct Rule {
    string lhs;
    vector<string> rhs;
};

struct TreeNode {
    string value;
    vector<TreeNode*> children;

    ~TreeNode() {
        for (auto child: children) {
            delete child;
        }
    }
};

// print the tree from left-right traversal
void printTree(TreeNode* node) {
    if (!node) return;
    cout << node->value << endl;
    for (TreeNode* child: node->children) {
        printTree(child);
    }
}

int main() {
    istringstream issLine(WLP4_COMBINED);
    string line;
    vector<Rule> rules;
    map<pair<int, string>, int> transitions;
    map<pair<int, string>, int> reductions;
    int sections = 0;

    // read instructions
    while (getline(issLine, line) && line != ".END") {
        if (line == ".CFG") {
            sections = 1;
            continue;
        } 
        if (line == ".INPUT") continue;
        if (line == ".TRANSITIONS") {
            sections = 3;
            continue;
        } 
        if (line == ".REDUCTIONS") {
            sections = 4;
            continue;
        }
        if (line.empty()) continue;

        istringstream iss(line);

        if (sections == 1) {
            Rule rule;
            iss >> rule.lhs;
            string symbol;

            while (iss >> symbol) {
                if (symbol != ".EMPTY") {
                    rule.rhs.push_back(symbol);
                }
            }
            rules.push_back(rule);
        } else if (sections == 3) {
            int state;
            string symbol;
            int nextState;
            iss >> state >> symbol >> nextState;
            transitions[{state, symbol}] = nextState;
        } else if (sections == 4) {
            int state;
            string symbol;
            int ruleIndex;
            iss >> state >> ruleIndex;

            if (iss >> symbol) {
                if (symbol == ".ACCEPT") {
                    reductions[{state, ""}] = ruleIndex;
                } else {
                    reductions[{state, symbol}] = ruleIndex;
                }
            } else {
                reductions[{state, ""}] = ruleIndex;
            }
        }
    }

    // read output from stdin
    vector<pair<string, string>> inputLines;
    inputLines.push_back({"BOF", "BOF"});

    string type, lexeme;
    while (cin >> type >> lexeme) {
        inputLines.push_back({type, lexeme});
    }

    inputLines.push_back({"EOF", "EOF"});

    // initialize parsing
    vector<TreeNode*> symbolStack;
    vector<int> stateStack;
    stateStack.push_back(0);
    int inputIndex = 0;

    // parsing loop to build tree
    while (true) {
        int s = stateStack.back();
        bool doReduce = false;
        int ruleIndex = -1;

        string a;
        if (inputIndex < inputLines.size()) {
            a = inputLines[inputIndex].first;
        } else {
            a = "";
        }

        if (reductions.count({s, a})) {
            doReduce = true;
            ruleIndex = reductions[{s, a}];
        }

        if (doReduce) {
            // reduction found
            const Rule& rule = rules[ruleIndex];
            int k = rule.rhs.size();

            string ruleStr = rule.lhs;
            if (k == 0) {
                ruleStr += " .EMPTY";
            } else {
                for (const string& sym: rule.rhs) {
                    ruleStr += " " + sym;
                }
            }

            TreeNode* inNode = new TreeNode{ruleStr, {}};
            inNode->children.resize(k);

            for (int i = k - 1; i >= 0; i--) {
                inNode->children[i] = symbolStack.back();
                symbolStack.pop_back();
                stateStack.pop_back();
            }

            symbolStack.push_back(inNode);
            int topState = stateStack.back();

            if (ruleIndex == 0) {
                break;
            }

            if (transitions.count({topState, rule.lhs})) {
                stateStack.push_back(transitions[{topState, rule.lhs}]);
            } else {
                cerr << "ERROR at " << ((inputIndex == 0) ? 1 : inputIndex) << endl;
                for (TreeNode* node : symbolStack) {
                    delete node;
                }
                return 1;
            }
        } else {
            // shift found
            if (!a.empty() && transitions.count({s, a})) {
                int newState = transitions[{s, a}];
                
                TreeNode* leaf = new TreeNode{inputLines[inputIndex].first + " " + inputLines[inputIndex].second, {}};
                symbolStack.push_back(leaf);
                
                inputIndex++;
                stateStack.push_back(newState);
            } else {
                cerr << "ERROR at " << ((inputIndex == 0) ? 1 : inputIndex) << endl;
                for (TreeNode* node : symbolStack) {
                    delete node;
                }
                return 1;
            }
        }
    }

    // output, clean memory, return
    printTree(symbolStack.back());
    delete symbolStack.back();
    return 0;
}
