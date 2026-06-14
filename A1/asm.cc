#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <map>
#include <cstdint>
#include <stdexcept>


void formatError(const std::string & message);

/** For a given instruction, returns the machine code for that instruction.
 *
 * @param[out] word The machine code for the instruction
 * @param instruction The name of the instruction
 * @param one The value of the first parameter
 * @param two The value of the second parameter
 * @param three The value of the third parameter
 */

bool checkReg(int reg) {
    return reg >= 0 && reg <= 31;
}

bool checkImm(int val, int bits) {
    int min = -(1 << (bits - 1));
    int max = (1 << (bits - 1)) - 1;
    return val >= min && val <= max;
}

bool compileLine(uint32_t &          word,
                 const std::string & instruction,
                 int            one,
                 int            two,
                 int            three)
{
    word = 0;
    
    // 1. Add: 10001011001 mmmmm 011000 nnnnn ddddd
    if (instruction == "add") {
        if (!checkReg(one) || !checkReg(two) || !checkReg(three)) {
            formatError("Register index out of bounds");
            return false;
        } 
        word = 0x8B206000 | (three << 16) | (two << 5) | one;
    }
    // 2. Sub: 11001011001 mmmmm 011000 nnnnn ddddd
    else if (instruction == "sub") {
        if (!checkReg(one) || !checkReg(two) || !checkReg(three)) {
            formatError("Register index out of bounds");
            return false;
        }
        word = 0xCB206000 | (three << 16) | (two << 5) | one;
    }
    // 3. Mul: 10011011000 mmmmm 011111 nnnnn ddddd
    else if (instruction == "mul") {
        if (!checkReg(one) || !checkReg(two) || !checkReg(three)) {
            formatError("Register index out of bounds");
            return false;
        }
        word = 0x9B007C00 | (three << 16) | (two << 5) | one;
    }
    // 4. Smulh: 10011011010 mmmmm 011111 nnnnn ddddd
    else if (instruction == "smulh") {
        if (!checkReg(one) || !checkReg(two) || !checkReg(three)) {
            formatError("Register index out of bounds");
            return false;
        }
        word = 0x9B407C00 | (three << 16) | (two << 5) | one;
    }
    // 5. Umulh: 10011011110 mmmmm 011111 nnnnn ddddd
    else if (instruction == "umulh") {
        if (!checkReg(one) || !checkReg(two) || !checkReg(three)) {
            formatError("Register index out of bounds");
            return false;
        }
        word = 0x9BC07C00 | (three << 16) | (two << 5) | one;
    }
    // 6. Sdiv: 10011010110 mmmmm 000011 nnnnn ddddd
    else if (instruction == "sdiv") {
        if (!checkReg(one) || !checkReg(two) || !checkReg(three)) {
            formatError("Register index out of bounds");
            return false;
        }
        word = 0x9AC00C00 | (three << 16) | (two << 5) | one;
    }
    // 7. Udiv: 10011010110 mmmmm 000010 nnnnn ddddd
    else if (instruction == "udiv") {
        if (!checkReg(one) || !checkReg(two) || !checkReg(three)) {
            formatError("Register index out of bounds");
            return false;
        }
        word = 0x9AC00800 | (three << 16) | (two << 5) | one;
    }
    // 8. Cmp: 11101011001 mmmmm 011000 nnnnn 11111
    else if (instruction == "cmp") {
        if (!checkReg(one) || !checkReg(two)) {
            formatError("Register index out of bounds");
            return false;
        }
        word = 0xEB206000 | (two << 16) | (one << 5) | 31;
    }
    // 9. Br: 11010110000 11111 000000 nnnnn 00000
    else if (instruction == "br") {
        if (!checkReg(one)) {
            formatError("Register index out of bounds");
            return false;
        }
        word = 0xD61F0000 | (one << 5); // 0xD61F matches 11010110 000 11111
    }
    // 10. Blr: 11010110001 11111 000000 nnnnn 00000
    else if (instruction == "blr") {
        if (!checkReg(one)) {
            formatError("Register index out of bounds");
            return false;
        }
        word = 0xD63F0000 | (one << 5);
    }
    // 11. Ldur: 11111000010 iiiiiiii 00 nnnnn ddddd
    else if (instruction == "ldur") {
        if (!checkReg(one) || !checkReg(two) || !checkImm(three, 9)) {
            formatError("Immediate value out of bounds");
            return false;
        }
        word = 0xF8400000 | ((static_cast<uint32_t>(three) & 0x1FF) << 12) | (two << 5) | one;
    }
    // 12. Stur: 11111000000 iiiiiiii 00 nnnnn ddddd
    else if (instruction == "stur") {
        if (!checkReg(one) || !checkReg(two) || !checkImm(three, 9)) {
            formatError("Immediate value out of bounds");
            return false;
        }
        word = 0xF8000000 | ((static_cast<uint32_t>(three) & 0x1FF) << 12) | (two << 5) | one;
    }
    // 13. Ldr: 01011000 iiiiiiiiiiiiiiiiiii iii ddddd
    else if (instruction == "ldr") {
        if (!checkReg(one)) {
            formatError("Immediate value out of bounds");
            return false;
        }
        if (two % 4 != 0) {
            formatError("LDR offset must be a multiple of 4");
            return false;
        }
        
        // Convert byte offset to word offset
        int word_offset = two / 4;
        
        if (!checkImm(word_offset, 19)) {
            formatError("Immediate value out of bounds");
            return false;
        }
        word = 0x58000000 | ((static_cast<uint32_t>(word_offset) & 0x7FFFF) << 5) | one;
    }
    // 14. B: 000101 iiiiiiiiiiiiiiiiiiiiiiiiii
    else if (instruction == "b") {
        if (one % 4 != 0) {
            formatError("Branch offset must be a multiple of 4");
            return false;
        }
        
        // Convert byte offset to word offset
        int word_offset = one / 4;
        
        if (!checkImm(word_offset, 26)) {
            formatError("Immediate value out of bounds");
            return false;
        }
        word = 0x14000000 | (static_cast<uint32_t>(word_offset) & 0x3FFFFFF);
    }

    return true;
}

/** Prints an error to stderr with an "ERROR: " prefix, and newline suffix.
 *
 * @param message The error to print
 */
void formatError(const std::string & message)
{
    std::cerr << "ERROR: " << message << std::endl;
}

/** Matches a line of ARM assembly, potentially with comments */
std::regex ARM_LINE_PATTERN(
    "^\\s*([a-z]+)\\s+(x\\d+|0x[0-9a-fA-F]*|-?\\d+|xzr)\\s*(?:(?:(?:,\\s*(x\\d+|0x[0-9a-fA-F]*|-?\\d+|xzr))?(?:,\\s*(x\\d+|0x[0-9a-fA-F]*|-?\\d+|xzr))?)|(?:,\\s*\\[\\s*(x\\d+|xzr)\\s*,\\s*(0x[0-9a-fA-F]*|-?\\d+)\\s*\\]))\\s*(?:\\/\\/.*)?$"
);

/** Recognizes an empty line (or an empty line with a comment) */
std::regex EMPTY_LINE("^\\s*(//.*)?$");

/** Maps the instruction name to the parameter type.  The value must be a 3 character string, 'r'
 *  represents a register, 'i' represents an immediate, 'z' represents a register where 0 is allowed, and ' ' represents no value */
const std::map<std::string, std::string> INSTRUCTION_PARAMETER_PATTERN = {
    {"add",   "rrz"},
    {"sub",   "rrz"},
    {"mul",   "rrz"},
    {"smulh", "rrz"},
    {"umulh", "rrz"},
    {"sdiv",  "rrz"},
    {"udiv",  "rrz"},
    {"cmp",   "rz "},
    {"br",    "r  "},
    {"blr",   "r  "},
    {"ldur",  "rri"},
    {"stur",  "rri"},
    {"ldr",   "ri "},
    {"b",     "i  "}
};


/** Convert a string representation of an immediate value to a signed 32-bit integer. Accounts for negatives.
 * If the string starts with "0x", it is interpreted as an unsigned hexadecimal value.
 *
 * The function name is read as "string to uint32".
 *
 * @param s The string to parse
 * @return The uint32_t representation of the string
 */
int readImm(const std::string & s)
{
    if(s.starts_with("0x"))
    {
        return std::stoi(s.substr(2), nullptr, 16);
    }
    return std::stoi(s);
}

/** Convert a string representation of a register name to the register number. If "xzr" (the zero register)
 * is read, it returns 31. Otherwise, it returns the register number directly. 
 *
 * The function name is read as "string to uint32".
 *
 * @param s The string to parse
 * @return The uint32_t representation of the string
 */
uint32_t readReg(bool zeroable, const std::string & s)
{
    if (s == "xzr") {
        if (zeroable) {
            return 31;
        } else {
            throw std::runtime_error("Register 'xzr' is not allowed in a non-'xm' position");
        }
    }

    if(!s.starts_with("x"))
    {
        throw std::runtime_error("Invalid register value '" + s + "'");
    }
    int ret = std::stoi(s.substr(1), nullptr, 10);
    if (ret > 30) {
        throw std::runtime_error("Register value '" + s + "' is too large");
    }
    if (ret < 0) {
        throw std::runtime_error("Register value '" + s + "' is negative");
    }
    return ret;
}


/** Compiles one line of assembly and send the binary to standard out.  If the assembly is invalid,
 *  print an error to stderr and return false.  Assumes that the assembly does not have a trailing
 *  comment.
 *
 * @param line The line to parse
 * @return True if the line is valid assembly and was output to stdout, false otherwise
 */
bool parseLine(const std::string & line)
{
    std::smatch matches;
    if(!std::regex_search(line, matches, ARM_LINE_PATTERN))
    {
        formatError((std::stringstream() << "Unable to parse line: \"" << line << "\"").str());
        return false;
    }

    std::string instruction = matches[1];

    uint32_t parameters[3] = {0, 0, 0};

    auto pattern = INSTRUCTION_PARAMETER_PATTERN.find(instruction);
    if (pattern == INSTRUCTION_PARAMETER_PATTERN.end()) {
        formatError((std::stringstream() << "'" << instruction << "' is not a known instruction").str());
        return false;
    }

    std::string argmatches[3] = {
        matches[2],
        matches[3].matched ? matches[3] : matches[5],
        matches[4].matched ? matches[4] : matches[6],
    };
    uint32_t index = 0;
    try {
        for (char c : pattern->second) {
            if (c == 'r') {
                if (argmatches[index] == "") {
                    throw std::runtime_error("Instruction '" + instruction + "' is missing a register value");
                }
                parameters[index] = readReg(false, argmatches[index]);
            } else if (c == 'z') {
                if (argmatches[index] == "") {
                   throw std::runtime_error("Instruction '" + instruction + "' is missing a zeroable register value");
                }
                parameters[index] = readReg(true, argmatches[index]);
            } else if (c == 'i') {
                if (argmatches[index] == "") {
                    throw std::runtime_error("Instruction '" + instruction + "' is missing an immediate value");
                }
                parameters[index] = readImm(argmatches[index]);
            } else { // Extra args
                if (argmatches[index] != "") {
                    throw std::runtime_error("Instruction '" + instruction + "' has extraneous arguments");
                }
            }
            index++;
        }
    } catch (std::runtime_error& s) {
        formatError(s.what());
        return false;
    } catch (std::invalid_argument& arg){
        formatError(arg.what());
        return false;
    }

    uint32_t binary = 0;
    bool compiled = compileLine(binary,
                                instruction,
                                parameters[0],
                                parameters[1],
                                parameters[2]);
    if(compiled)
    {
        // Output of the binary in little-endian order
        std::cout << (char)((binary >> 0) & 0xFF)
                  << (char)((binary >> 8) & 0xFF)
                  << (char)((binary >> 16) & 0xFF)
                  << (char)((binary >> 24) & 0xFF);
        return true;
    }
    else
    {
        // compileLine (should have) printed an error.  We don't have to print one here.
        return false;
    }
}

/** Entrypoint for the assembler.  The first parameter (optional) is a mips assembly file to
 *  read.  If no parameter is specified, read assembly from stdin.  Prints machine code to stdout.
 *  If invalid assembly is found, prints an error to stderr, stops reading assembly, and return a
 *  non-0 value.
 *
 * If the file is not found, print an error and returns a non-0 value.
 *
 * @return 0 on success, non-0 on error
 */
int main(int argc, char * argv[])
{
    if(argc > 2)
    {
        std::cerr << "Usage:" << std::endl
                  << "\tasm [$FILE]" << std::endl
                  << std::endl
                  << "If $FILE is unspecified or if $FILE is `-`, read the assembly from standard "
                  << "in. Otherwise, read the assembly from $FILE." << std::endl;
        return 1;
    }

    std::ifstream fp;
    std::istream &in =
        (argc > 1 && std::string(argv[1]) != "-")
      ? [&]() -> std::istream& {
            fp.open(argv[1]);
            return fp;
        }()
      : std::cin;

    if(!fp && argc > 1)
    {
        formatError((std::stringstream() << "file '" << argv[1] << "' not found!").str());
        return 1;
    }

    while(!in.eof())
    {
        std::string line;
        std::getline( in, line );

        // Filter out any comments
        uint32_t startComment = line.find(";");
        if(startComment != std::string::npos)
        {
            line = line.substr(0, line.find(";"));
        }

        std::smatch matches;
        if(std::regex_search(line, matches, EMPTY_LINE))
        {
            continue;
        }

        if(!parseLine(line))
        {
            return 1;
        }
    }

    return 0;
}
