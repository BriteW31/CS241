#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <map>

const std::string ALPHABET    = ".ALPHABET";
const std::string STATES      = ".STATES";
const std::string TRANSITIONS = ".TRANSITIONS";
const std::string INPUT       = ".INPUT";
const std::string EMPTY       = ".EMPTY";

bool isChar(std::string s) {
  return s.length() == 1;
}
bool isRange(std::string s) {
  return s.length() == 3 && s[1] == '-';
}

// Locations in the program that you should modify to store the
// DFA information have been marked with four-slash comments:
//// (Four-slash comment)
int main() {
  std::istream& in = std::cin;
  std::string s;
  std::set<char> alphabet;
  std::string initialState;
  std::set<std::string> acceptingStates;
  std::map<std::pair<std::string, char>, std::string> transitions;

  std::getline(in, s); // Alphabet section (skip header)
  // Read characters or ranges separated by whitespace
  while(in >> s) {
    if (s == STATES) { 
      break; 
    } else {
      if (isChar(s)) {
        alphabet.insert(s[0]);
      } else if (isRange(s)) {
        for(char c = s[0]; c <= s[2]; ++c) {
          alphabet.insert(c);
        }
      } 
    }
  }

  std::getline(in, s); // States section (skip header)
  // Read states separated by whitespace
  while(in >> s) {
    if (s == TRANSITIONS) { 
      break; 
    } else {
      static bool initial = true;
      bool accepting = false;
      if (s.back() == '!' && !isChar(s)) {
        accepting = true;
        s.pop_back();
      }
      //// Variable 's' contains the name of a state
      if (initial) {
        initialState = s;
        initial = false;
      }
      if (accepting) {
        acceptingStates.insert(s);
      }
    }
  }

  std::getline(in, s); // Transitions section (skip header)
  // Read transitions line-by-line
  while(std::getline(in, s)) {
    if (s == INPUT) { 
      // Note: Since we're reading line by line, once we encounter the
      // input header, we will already be on the line after the header
      break; 
    } else {
      std::string fromState, symbols, toState;
      std::istringstream line(s);
      std::vector<std::string> lineVec;
      while(line >> s) {
        lineVec.push_back(s);
      }
      fromState = lineVec.front();
      toState = lineVec.back();
      for(int i = 1; i < lineVec.size()-1; ++i) {
        std::string s = lineVec[i];
        if (isChar(s)) {
          symbols += s;
        } else if (isRange(s)) {
          for(char c = s[0]; c <= s[2]; ++c) {
            symbols += c;
          }
        }
      }
      for ( char c : symbols ) {
        transitions[{fromState, c}] = toState;
      }
    }
  }

  // Input section (already skipped header)
  while (in >> s) {
    std::string currentState = initialState;
    bool valid = true;

    if (s == EMPTY) {
        if (acceptingStates.count(currentState)) {
            std::cout << s << " true" << std::endl;
        } else {
            std::cout << s << " false" << std::endl;
        }
    } else {
        for (char c : s) {
            auto it = transitions.find({currentState, c});
            if (it != transitions.end()) {
                currentState = it->second;
            } else {
                valid = false;
                break;
            }
        }

        if (valid && acceptingStates.count(currentState)) {
            std::cout << s << " true" << std::endl;
        } else {
            std::cout << s << " false" << std::endl;
        }
    }
  }
}
