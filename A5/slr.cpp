#include <iostream>
#include <vector>
#include <string>
#include <deque>
#include <sstream>
#include <map>

using namespace std;

struct Rule {
    string lhs;
    vector<string> rhs;
};

// print current state
void printProgress (const vector<string>& symbols, const deque<string>& inputLines) {
    for (const string& s: symbols) {
        cout << s << " ";
    }
    cout << ".";
    for (const string& s: inputLines) {
        cout << " " << s;
    }
    cout << endl;
}

int main() {
    string line;
    vector<Rule> rules;
    deque<string> inputLines;
    vector<string> symbolStack;
    vector<int> stateStack;

    // store DFA transitions and reductions
    map<pair<int, string>, int> transitions;
    map<pair<int, string>, int> reductions;

    // 1 --> cfg, 2 --> input, 3 --> transitions, 4 --> reductions
    int sections = 0;

    // read input sections
    while (getline(cin, line) && line != ".END") {
        if (line == ".CFG") {
            sections = 1;
            continue;
        } 
        if (line == ".INPUT") {
            sections = 2;
            continue;
        } 
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
        } else if (sections == 2) {
            string token;
            while (iss >> token) {
                inputLines.push_back(token);
            }
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

    // initialize states
    stateStack.push_back(0);
    int shiftedCount = 0;


    // print state
    printProgress(symbolStack, inputLines);

    // execute SLR instructions
    while (true) {
        int s = stateStack.back();
        string a;
        if  (inputLines.empty()) {
            a =  "";
        } else {
            a = inputLines.front();
        }

        bool isReduce = false;
        int ruleIndex = -1;

        // reduction check
        if (reductions.count({s, a})) {
            isReduce = true;
            ruleIndex = reductions[{s, a}];
        }

        if (isReduce) {
            const Rule& rule = rules[ruleIndex];
            int k = rule.rhs.size();

            for (int i = 0; i < k; i++) {
                symbolStack.pop_back();
                stateStack.pop_back();
            }

            // push LHS to symbol stack
            symbolStack.push_back(rule.lhs);
            int topState = stateStack.back();

            // if rule is at 0, end loop
            if (ruleIndex == 0) {
                printProgress(symbolStack, inputLines);
                break;
            }

            // find transition for the LHS
            if (transitions.count({topState, rule.lhs})) {
                stateStack.push_back(transitions[{topState, rule.lhs}]);
            } else {
                cerr << "ERROR at " << shiftedCount + 1 << endl;
                return 1;
            }

            printProgress(symbolStack, inputLines);

        } else {
            // shift
            if (!a.empty() && transitions.count({s, a})) {
                int newState = transitions[{s, a}];
                
                symbolStack.push_back(a);
                inputLines.pop_front();
                stateStack.push_back(newState);
                shiftedCount++;

                printProgress(symbolStack, inputLines);
            } else {
                // error if both shift and reduce not possible
                cerr << "ERROR at " << shiftedCount + 1 << endl;
                return 1;
            }
        }
    }

    return 0;
}
