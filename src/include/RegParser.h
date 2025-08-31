#ifndef REG_PARSER
#define REG_PARSER

#include <vector>
#include <unordered_map>
#include <string>
#include <cstring>
#include <cctype>
#include <iostream>
#include <stack>
#include <memory>

// #define DEBUG

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

struct TokenList;

struct Re
{
    RegType type = ETK;
    std::string ccl;
    bool isNegative = false;
    Quantifier quantifier = NONE;
    int captured_gp_id = -1;
    std::vector<TokenList> alternatives;
};

struct TokenList
{
    TokenList *parent = nullptr;
    std::vector<Re> regex;
};

struct CaptureGroup
{
    const char *start = nullptr;
    size_t length = 0;
};

class RegParser
{
public:
    explicit RegParser(const std::string &pattern) : _pattern(pattern.c_str()), _begin(pattern.c_str()), _end(_begin + pattern.size())
    {
        gp_stack[&token_list] = 0;
    };

    // disable copy and assignment (pointer conflict)
    RegParser(const RegParser &) = delete;
    RegParser &operator=(const RegParser &) = delete;

    // Move constructor and assignment
    RegParser(RegParser &&) = default;
    RegParser &operator=(RegParser &&) = default;

    bool parse();
    bool match(const std::string &input_line);

    TokenList token_list;
    std::unordered_map<TokenList *, int> gp_stack;

private:
    const char *_pattern{nullptr};
    const char *_begin{nullptr};
    const char *_end{nullptr};
    int next_capture_id = 1;

    // parsing state
    std::stack<TokenList *> parser_gp_stack;

    // runtime state for matching
    std::unordered_map<int, CaptureGroup> captures;
    std::unordered_map<int, const char *> group_start;

    // Parsing methods
    bool isEof() const { return _pattern >= _end; }
    bool consume();
    bool check(char c) const { return !isEof() && *_pattern == c; }
    bool match(char c);

    Re parseElement();
    Re parseCharacterClass();
    Re parseGroup();
    void applyQuantifiers(Re &element);

    Re makeRe(RegType type, const std::string &ccl = "", bool isNegative = false);

    // matching methods
    void sync_index(TokenList *tl, int idx) { gp_stack[tl] = idx; }

    bool match_current(const char *c, const std::vector<Re> &regex, int idx);
    bool match_from_position(const char **start_pos, const TokenList &token_list, int idx, bool is_backtracking = false);
    bool can_match_next_here(const char *start_pos, const TokenList &token_list, int idx, bool is_backtracking = false);
    bool matchCharacterInList(char c, const Re &ListRe) const;

    bool match_one_or_more(const char **c, const TokenList &token_list, int idx);
    bool match_alt_one_or_more(const char **c, const TokenList &token_list, int idx);
    bool match_zero_or_one(const char **c, const TokenList &token_list, int idx);
    bool match_alt_zero_or_one(const char **c, const TokenList &token_list, int idx);

    // quantifier handlers
    int handle_quantified_match(const char **c, const TokenList &token_list, int idx);
    bool handle_plus_quantifier(const char **c, const TokenList &token_list, int idx);
    bool handle_question_quantifier(const char **c, const TokenList &token_list, int idx);
    bool handle_no_quantifier(const char **c, const TokenList &token_list, int idx);
    bool handle_alternation(const char **c, const TokenList &token_list, int idx);
    bool handle_single_match(const char **c, const TokenList &token_list, int idx);

    // utility
    inline bool at_begin() const { return _pattern == _begin; }
    inline bool at_end() const { return _pattern >= _end; }
    inline size_t offset() const { return static_cast<size_t>(_pattern - _begin); }
};

#endif