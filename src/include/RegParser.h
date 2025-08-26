#ifndef REG_PARSER
#define REG_PARSER

#include <vector>
#include <unordered_map>
#include <string>
#include <cctype>
#include <iostream>

typedef enum
{
    NONE,
    PLUS,
    STAR,
    MARK
} Quantifier;

typedef enum
{
    SINGLE_CHAR,
    DIGIT,
    ALPHANUM,
    START,
    END,
    LIST,
    ALT,
    BACKREF,
    ETK,
} RegType;

struct Re;
struct Re
{
    RegType type;
    // char *ch;
    char *ccl;
    bool isNegative;
    Quantifier quantifier;
    int captured_gp_id;
    std::vector<std::vector<Re>> alternatives;
};

typedef struct
{
    const char *start;
    size_t length;
} CaptureGroup;

static int next_capture_id = 1; // will change back to member later

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

    bool match_current(const char *c, const std::vector<Re> &regex, int idx);
    bool match_from_position(const char **start_pos, const std::vector<Re> &regex, int idx);
    bool match_one_or_more(const char **c, const std::vector<Re> &regex, int idx);
    bool match_alt_one_or_more(const char **c, const std::vector<Re> &regex, int idx);
    bool match_zero_or_one(const char **c, const std::vector<Re> &regex, int idx);
    bool match_alt_zero_or_one(const char **c, const std::vector<Re> &regex, int idx);
    std::vector<Re> regex;

private:
    // const char *_input_line;
    const char *_pattern{nullptr};
    const char *_begin{nullptr};
    const char *_end{nullptr};

    std::unordered_map<int, CaptureGroup> captures;

    // std::string _pattern;

    bool isEof();
    bool consume();
    bool check(char c);
    bool match(char c);

    Re parseElement();
    Re parseCharacterClass();
    Re parseGroup();
    void applyQuantifiers(Re &element);

    inline bool at_begin() const { return _pattern == _begin; }
    inline bool at_end() const { return _pattern >= _end; }
    inline size_t offset() const { return static_cast<size_t>(_pattern - _begin); }

    // New helper methods for refactored match_from_position
    int handle_quantified_match(const char **c, const std::vector<Re> &regex, int idx);
    bool handle_plus_quantifier(const char **c, const std::vector<Re> &regex, int idx);
    bool handle_question_quantifier(const char **c, const std::vector<Re> &regex, int idx);
    bool handle_no_quantifier(const char **c, const std::vector<Re> &regex, int idx);
    bool handle_alternation(const char **c, const std::vector<Re> &regex, int idx);
    bool handle_single_match(const char **c, const std::vector<Re> &regex, int idx);

    bool matchCharacterInList(char c, const Re &ListRe) const;
};

#endif