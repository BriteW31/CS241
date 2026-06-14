#include <iostream>
#include <sstream>
#include <string>

using namespace std;

// Define all possible states for the DFA
enum State {
    START,
    DOT, DOTID,
    ID, LABEL,
    ZERO, ZERO_X, HEXINT,
    MINUS, INT_STATE,
    COMMA_STATE, LBRACK_STATE, RBRACK_STATE,
    WHITESPACE,
    NEWLINE
};

// Check if the current state is a valid accepting state
bool isAccepting(State s) {
    return s == DOTID || s == ID || s == LABEL || s == ZERO ||
           s == HEXINT || s == INT_STATE || s == COMMA_STATE ||
           s == LBRACK_STATE || s == RBRACK_STATE || s == WHITESPACE || s == NEWLINE;
}

// Character classification helpers
bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool isNum(char c) {
    return (c >= '0' && c <= '9');
}

bool isAlnum(char c) {
    return isAlpha(c) || isNum(c);
}

bool isHex(char c) {
    return isNum(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

// DFA transition logic
State transition(State current, char c) {
    switch(current) {
        case START:
            if (c == '.') return DOT;
            if (isAlpha(c)) return ID;
            if (c == '0') return ZERO;
            if (c >= '1' && c <= '9') return INT_STATE;
            if (c == '-') return MINUS;
            if (c == ',') return COMMA_STATE;
            if (c == '[') return LBRACK_STATE;
            if (c == ']') return RBRACK_STATE;
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') return WHITESPACE;
            break;
        case DOT:
            if (isAlnum(c)) return DOTID;
            break;
        case DOTID:
            if (isAlnum(c)) return DOTID;
            break;
        case ID:
            if (isAlnum(c)) return ID;
            if (c == ':') return LABEL;
            break;
        case LABEL:
            break; // No transitions out of a label
        case ZERO:
            if (c == 'x') return ZERO_X;
            break; // 0 can't transition to any other numbers
        case ZERO_X:
            if (isHex(c)) return HEXINT;
            break;
        case HEXINT:
            if (isHex(c)) return HEXINT;
            break;
        case MINUS:
            if (c >= '1' && c <= '9') return INT_STATE;
            break;
        case INT_STATE:
            if (isNum(c)) return INT_STATE;
            break;
        case COMMA_STATE:
        case LBRACK_STATE:
        case RBRACK_STATE:
            break;
        case WHITESPACE:
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') return WHITESPACE;
            break;
        case NEWLINE:
            break;
    }
    // If no valid transition, return an invalid state
    return (State)-1;
}

// Post-classification to handle REG and ZREG taking precedence over ID
string getTokenType(State s, const string& lexeme) {
    if (s == ID) {
        if (lexeme == "xzr") return "ZREG";
        
        // Check if it matches 'x' followed by 1 or 2 digits
        if (lexeme.length() >= 2 && lexeme.length() <= 3 && lexeme[0] == 'x') {
            bool isReg = true;
            for (size_t i = 1; i < lexeme.length(); ++i) {
                if (!isNum(lexeme[i])) { 
                    isReg = false; 
                    break; 
                }
            }
            if (isReg) return "REG";
        }
        return "ID";
    }
    
    if (s == DOTID) return "DOTID";
    if (s == LABEL) return "LABEL";
    if (s == ZERO || s == INT_STATE) return "INT";
    if (s == HEXINT) return "HEXINT";
    if (s == COMMA_STATE) return "COMMA";
    if (s == LBRACK_STATE) return "LBRACK";
    if (s == RBRACK_STATE) return "RBRACK";
    if (s == NEWLINE) return "NEWLINE";
    
    return "";
}

int main() {
    // Read the entire stdin stream into a single string
    ostringstream ss;
    ss << cin.rdbuf();
    string input = ss.str();

    State currentState = START;
    string currentToken = "";

    // SMM Algorithm Core
    for (size_t i = 0; i < input.length(); /* increment happens in loop */) {
        char c = input[i];
        State nextState = transition(currentState, c);

        if (nextState != (State)-1) {
            // Valid transition, consume character
            currentState = nextState;
            currentToken += c;
            ++i;
        } else {
            // DFA is stuck, check if we landed in an accepting state
            if (isAccepting(currentState)) {
                // Ignore WHITESPACE tokens
                if (currentState != WHITESPACE) {
                    cout << getTokenType(currentState, currentToken) << " " << currentToken << "\n";
                }
                
                // Reset to scan the next token
                currentState = START;
                currentToken = "";
                
                // NOTE: We do NOT increment 'i' here. We re-evaluate the stuck character.
            } else {
                // Stuck in a non-accepting state
                cerr << "ERROR\n";
                return 1;
            }
        }
    }

    // Process any lingering token once End-Of-File is reached
    if (!currentToken.empty()) {
        if (isAccepting(currentState)) {
            if (currentState != WHITESPACE) {
                cout << getTokenType(currentState, currentToken) << " " << currentToken << "\n";
            }
        } else {
            cerr << "ERROR\n";
            return 1;
        }
    }

    return 0;
}
