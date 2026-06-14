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
    if (!s.empty() && s.back() == '\r') s.pop_back();

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

      if (lineVec.size() < 3) continue;

      fromState = lineVec.front();
      toState = lineVec.back();
      for(int i = 1; i < static_cast<int>(lineVec.size())-1; ++i) {
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

  // Input section (read as string)
  std::ostringstream string;
  string << in.rdbuf();
  std::string input = string.str();

  // Remove trailing newline characters from input
  if (!input.empty() && input.back() == '\n') input.pop_back();
  if (!input.empty() && input.back() == '\r') input.pop_back();

  // SMM Algorithm
  std::string currentState = initialState;
  std::string currentToken = "";

  for (size_t i = 0; i < input.length();) {
    char c = input[i];
    auto it = transitions.find({currentState, c});

    if (it != transitions.end()) {
      currentState = it->second;
      currentToken += c;
      ++i;
    } else {
        if (acceptingStates.count(currentState) && !currentToken.empty()) {
              // Valid token found! Output lexeme.
              std::cout << currentToken << "\n";
              
              // Reset DFA to scan the next token
              currentState = initialState;
              currentToken = "";
              
              // Do NOT increment 'i' here. We must re-evaluate the character 
          } else {
              // Stuck in a non-accepting state, or invalid character at the start.
              std::cerr << "ERROR: Invalid token or in non-accepting state" << std::endl;
              return 1;
          }
      }
  }

  // Check for any remaining token after processing input
  if (!currentToken.empty()) {
    if (acceptingStates.count(currentState)) {
      std::cout << currentToken << std::endl;
    } else {
      std::cerr << "ERROR: reached EOF in Non-accepting state" << std::endl;
      return 1;
    }
  }
}
