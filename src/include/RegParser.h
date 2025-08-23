#ifndef REG_PARSER
#define REG_PARSER

#include <vector>
#include <string>
#include <cctype>
#include <iostream>

typedef enum
{
    SINGLE_CHAR,
    DIGIT,
    ALPHANUM,
    START,
    LIST,
    ETK,
} RegType;

typedef struct
{
    RegType type;
    // char *ch;
    char *ccl;
    bool isNegative;
} Re;

class RegParser
{
public:
    RegParser(const std::string &input_line, const std::string &pattern) : _input_line(input_line.c_str()), _pattern(pattern.c_str()) {};

    bool parse();
    Re makeRe(RegType type, char *ccl = nullptr, bool isNegative = false);

    static bool match_current(const char *c, std::vector<Re> &regex, int idx);
    std::vector<Re> regex;

private:
    const char *_input_line;
    const char *_pattern;

    // std::string _pattern;

    bool isEof();
    bool consume();
    bool check(char c);
    bool match(char c);
};

#endif