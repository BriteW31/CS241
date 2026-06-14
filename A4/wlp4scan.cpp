#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>

using namespace std;

enum State {
    START, IDSTATE, 
    ZEROSTATE, NUMSTATE, 
    LPARENSTATE, RPARENSTATE, 
    LBRACESTATE, RBRACESTATE, 
    LBRACKSTATE, RBRACKSTATE, 
    COMMASTATE, SEMISTATE, 
    PLUSSTATE, MINUSSTATE, STARSTATE, PCTSTATE, AMPSTATE, 
    SLASHSTATE, COMMENTSTATE, 
    BECOMESSTATE, EQSTATE, 
    NOTSTATE, NESTATE, 
    LTSTATE, LESTATE, 
    GTSTATE, GESTATE, 
    WHITESPACESTATE
};

bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool isNum(char c) {
    return (c >= '0' && c <= '9');
}

bool isAlnum(char c) {
    return isAlpha(c) || isNum(c);
}

bool isAccepting (State s) {
    return s != START && s != NOTSTATE;
}

State transition(State current, char ch) {
    switch (current) {
        case START:
            if (isAlpha(ch)) return IDSTATE;
            if (ch == '0') return ZEROSTATE;
            if (ch >= '1' && ch <= '9') return NUMSTATE;
            if (ch == '(') return LPARENSTATE;
            if (ch == ')') return RPARENSTATE;
            if (ch == '{') return LBRACESTATE;
            if (ch == '}') return RBRACESTATE;
            if (ch == '[') return LBRACKSTATE;
            if (ch == ']') return RBRACKSTATE;
            if (ch == ',') return COMMASTATE;
            if (ch == ';') return SEMISTATE;
            if (ch == '+') return PLUSSTATE;
            if (ch == '-') return MINUSSTATE;
            if (ch == '*') return STARSTATE;
            if (ch == '%') return PCTSTATE;
            if (ch == '&') return AMPSTATE;
            if (ch == '/') return SLASHSTATE;
            if (ch == '=') return BECOMESSTATE;
            if (ch == '!') return NOTSTATE;
            if (ch == '<') return LTSTATE;
            if (ch == '>') return GTSTATE;
            if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') return WHITESPACESTATE;
            break;
        case IDSTATE:
            if (isAlnum(ch)) return IDSTATE;
            break;
        case NUMSTATE:
            if (isNum(ch)) return NUMSTATE;
            break;
        case ZEROSTATE:
            break; // leading 0 cannot be followed by more digits in a single token
        case SLASHSTATE:
            if (ch == '/') return COMMENTSTATE;
            break;
        case COMMENTSTATE:
            if (ch != '\n' && ch != '\r') return COMMENTSTATE;
            break;
        case BECOMESSTATE:
            if (ch == '=') return EQSTATE;
            break;
        case NOTSTATE:
            if (ch == '=') return NESTATE;
            break;
        case LTSTATE:
            if (ch == '=') return LESTATE;
            break;
        case GTSTATE:
            if (ch == '=') return GESTATE;
            break;
        case WHITESPACESTATE:
            if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') return WHITESPACESTATE;
            break;
        default:
        // remaining states do not transition to any other states, so break and return invalid state
            break;
    }
    // invalid transition
    return (State)-1;
}

string getTokenType(State s, const string& lexeme) {
    if (s == IDSTATE) {
        // WLP4 Keywords
        if (lexeme == "return") return "RETURN";
        if (lexeme == "if") return "IF";
        if (lexeme == "else") return "ELSE";
        if (lexeme == "while") return "WHILE";
        if (lexeme == "println") return "PRINTLN";
        if (lexeme == "wain") return "WAIN";
        if (lexeme == "long") return "LONG";
        if (lexeme == "new") return "NEW";
        if (lexeme == "delete") return "DELETE";
        if (lexeme == "NULL") return "NULL";
        return "ID";
    }
    if (s == ZEROSTATE || s == NUMSTATE) return "NUM";
    if (s == LPARENSTATE) return "LPAREN";
    if (s == RPARENSTATE) return "RPAREN";
    if (s == LBRACESTATE) return "LBRACE";
    if (s == RBRACESTATE) return "RBRACE";
    if (s == LBRACKSTATE) return "LBRACK";
    if (s == RBRACKSTATE) return "RBRACK";
    if (s == COMMASTATE) return "COMMA";
    if (s == SEMISTATE) return "SEMI";
    if (s == PLUSSTATE) return "PLUS";
    if (s == MINUSSTATE) return "MINUS";
    if (s == STARSTATE) return "STAR";
    if (s == PCTSTATE) return "PCT";
    if (s == AMPSTATE) return "AMP";
    if (s == SLASHSTATE) return "SLASH";
    if (s == BECOMESSTATE) return "BECOMES";
    if (s == EQSTATE) return "EQ";
    if (s == NESTATE) return "NE";
    if (s == LTSTATE) return "LT";
    if (s == LESTATE) return "LE";
    if (s == GTSTATE) return "GT";
    if (s == GESTATE) return "GE";
    
    // if none of the above, then return nothing
    return "";
}

// note that this main function is taken exactly from A2P5's asm.cpp, with necessary modifications to fit WLP4 standards and token definitions
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
                // Ignore WHITESPACE and COMMENT tokens
                if (currentState != WHITESPACESTATE && currentState != COMMENTSTATE) {
                    if (currentState == NUMSTATE || currentState == ZEROSTATE) {
                        try {
                            std::stoll(currentToken);
                        } catch (...) {
                            cerr << "ERROR: Number out of range\n";
                            return 1;
                        }
                    }

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
            if (currentState != WHITESPACESTATE && currentState != COMMENTSTATE) {

                if (currentState == NUMSTATE || currentState == ZEROSTATE) {
                        try {
                            std::stoll(currentToken);
                        } catch (...) {
                            cerr << "ERROR: Number out of range\n";
                            return 1;
                        }
                }

                cout << getTokenType(currentState, currentToken) << " " << currentToken << "\n";
                }
            }
        } else {
            cerr << "ERROR\n";
            return 1;
        }

    return 0;
}
