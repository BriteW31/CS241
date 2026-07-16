#include <iostream>
#include <vector>
#include <string>
#include <deque>
#include <sstream>

using namespace std;

struct Rule {
    string lhs;
    vector<string> rhs;
};

int main() {
    string line;
    vector<Rule> rules;
    deque<string> inputLines;
    vector<string> outputLines;

    // read the CFG rules until ".INPUT" is encountered
    while (getline(cin, line) && line != ".INPUT") {
        if (line == ".CFG" || line.empty()) {
            continue;
        }

        istringstream iss(line);
        Rule rule;
        iss >> rule.lhs;
        string symbol;

        while (iss >> symbol) {
            if (symbol != ".EMPTY") {
                rule.rhs.push_back(symbol);
            }
        }
        rules.push_back(rule);
    }

    // read input lines until ".ACTIONS" is encountered
    while (getline(cin, line) && line != ".ACTIONS") {
        if (line.empty()) continue;

        istringstream iss(line);
        string token;

        while (iss >> token) {
            inputLines.push_back(token);
        }
    }

    // process possible commands until ".END" is encountered, with linear time restraints
    while (getline(cin, line) && line != ".END") {
        if (line.empty()) continue;

        istringstream iss(line);
        string command;
        iss >> command;
        
        if (command == "print") {
            for (const string& s: outputLines) {
                cout << s << " ";
            }
            cout << ".";
            for (const string& s: inputLines) {
                cout << " " << s;
            }
            cout << endl;
        } else if (command == "shift") {
            if (!inputLines.empty()) {
                outputLines.push_back(inputLines.front());
                inputLines.pop_front();
            }
        } else if (command == "reduce") {
            int ruleIndex;
            iss >> ruleIndex;

            const Rule& rule = rules[ruleIndex];
            int rhsSize = rule.rhs.size();

            for (int i = 0; i < rhsSize; i++) {
                outputLines.pop_back();
            }

            outputLines.push_back(rule.lhs);
        }
    }

    return 0;
}
