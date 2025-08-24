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
    END,
    LIST,
    ETK,
} RegType;

typedef struct
{
    RegType type;
    // char *ch;
    char *ccl;
    bool isNegative;
    bool hasPlus;
} Re;

class RegParser
{
public:
    RegParser(const std::string &pattern) : _pattern(pattern.c_str())
    {
        _begin = _pattern;
        _end = _begin + pattern.size();
    };

    // RegParser(const std::string &input_line, const std::string &pattern) : _input_line(input_line.c_str()), _pattern(pattern.c_str()) {};

    bool parse();
    Re makeRe(RegType type, char *ccl = nullptr, bool isNegative = false);

    static bool match_current(const char *c, const std::vector<Re> &regex, int idx);
    static bool match_from_position(const char *start_pos, const std::vector<Re> &regex, int idx);
    static bool match_one_or_more(const char *c, const std::vector<Re> &regex, int idx);
    std::vector<Re> regex;

private:
    // const char *_input_line;
    const char *_pattern{nullptr};
    const char *_begin{nullptr};
    const char *_end{nullptr};

    // std::string _pattern;

    bool isEof();
    bool consume();
    bool check(char c);
    bool match(char c);

    inline bool at_begin() const { return _pattern == _begin; }
    inline bool at_end() const { return _pattern >= _end; }
    inline size_t offset() const { return static_cast<size_t>(_pattern - _begin); }
};

#endif