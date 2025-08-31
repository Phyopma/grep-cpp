#include "include/RegParser.h"

bool RegParser::parse()
{
    parser_gp_stack.push(&token_list);
    while (!isEof())
    {
        Re element = parseElement();
        if (element.type == ETK)
            return false;
        token_list.regex.push_back(element);
    }
    parser_gp_stack.pop();
    return true;
}

Re RegParser::makeRe(RegType type, char *ccl, bool isNegative)
{
    return {type, ccl, isNegative, NONE, -1, {}};
}

bool RegParser::match_current(const char *c, const std::vector<Re> &regex, int idx)
{
    const Re &current = regex[idx];

    switch (current.type)
    {
    case DIGIT:
        return isdigit(*c);
    case ALPHANUM:
        return isalnum(*c) || *c == '_';
    case SINGLE_CHAR:
        return *c == *current.ccl || *current.ccl == '.';
    case LIST:
        return matchCharacterInList(*c, current);
    case START:
        return true;
    case END:
        return *c == '\0';
    default:
        return false;
    }
}

bool RegParser::matchCharacterInList(char c, const Re &listRe) const
{
    const char *p = listRe.ccl;
    while (*p != '\0')
    {
        if (c == *p)
            return !listRe.isNegative;
        ++p;
    }
    return listRe.isNegative;
}

bool RegParser::match_from_position(const char **start_pos, const TokenList &token_list, int idx, bool is_backtrack)
{
    const char *c = *start_pos;
    const auto &regex = token_list.regex;
    int rIdx = idx;
    int pattern_length = regex.size();

    // TODO:

    // if (!is_backtracking)
    sync_index(const_cast<TokenList *>(&token_list), rIdx);
    if (idx >= pattern_length)
    {
        return true;
    }

    while (rIdx < pattern_length && *c != '\0')
    {
        const Re &current = token_list.regex[rIdx];

        // if (is_backtrack && current.type == BACKREF)
        // {
        //     return true;
        // }

        // std::cout << "before >> " << rIdx << std::endl;
        int consumed = handle_quantified_match(&c, token_list, rIdx);
        if (consumed <= 0)
            return false;
        // std::cout << "after >> consumed " << consumed << " tokens." << std::endl;

        rIdx += consumed;
        // if (!is_backtracking)
        sync_index(const_cast<TokenList *>(&token_list), rIdx);
    }

    *start_pos = c;
    if (rIdx >= pattern_length)
        return true;
    if (*c == '\0')
    {
        return regex[rIdx].type == END || regex[rIdx].quantifier == MARK;
    }
    // return (rIdx >= pattern_length) || (rIdx < pattern_length && regex[rIdx].type == END && *c == '\0');
    return false;
    // return true;
}

bool RegParser::can_match_next_here(const char *start_pos, const TokenList &token_list, int idx, bool is_backtracking)
{
    const char *temp_pos = start_pos;
    bool isMatched = false;
    int restored_index = idx;

    if (is_backtracking && idx >= token_list.regex.size())
    {

        auto *parent_token_list = token_list.parent;
        if (!parent_token_list || gp_stack.find(parent_token_list) == gp_stack.end())
        {
            return true; // Parent is invalid or not tracked, can't continue
        }

        int outer_next_index = gp_stack[parent_token_list] + 1;

        const Re &previous = parent_token_list->regex[outer_next_index - 1];
        if (previous.captured_gp_id >= 0)
        {
            const char *prev_gp_start = group_start[previous.captured_gp_id];
            captures[previous.captured_gp_id] = {prev_gp_start,
                                                 static_cast<size_t>(start_pos - prev_gp_start)};
        }

        while (parent_token_list && outer_next_index >= parent_token_list->regex.size())
        {
            parent_token_list = parent_token_list->parent;
            if (!parent_token_list || gp_stack.find(parent_token_list) == gp_stack.end())
            {
                // break; // Parent is invalid or not tracked
                return true;
            }

            outer_next_index = gp_stack[parent_token_list] + 1;
            const Re &previous = parent_token_list->regex[outer_next_index - 1];
            if (previous.captured_gp_id >= 0)
            {
                const char *prev_gp_start = group_start[previous.captured_gp_id];
                captures[previous.captured_gp_id] = {prev_gp_start,
                                                     static_cast<size_t>(start_pos - prev_gp_start)};
            }
        }
        restored_index = outer_next_index - 1;
        if (parent_token_list->regex[restored_index].quantifier == PLUS)
        {
            isMatched = match_from_position(&temp_pos, *parent_token_list, outer_next_index - 1, is_backtracking);
        }
        else
        {
            isMatched = match_from_position(&temp_pos, *parent_token_list, outer_next_index, is_backtracking);
        }
        sync_index(parent_token_list, restored_index);
        return isMatched;
    }

    isMatched = match_from_position(&temp_pos, token_list, idx, is_backtracking);
    sync_index(const_cast<TokenList *>(&token_list), restored_index);
    return isMatched;
}

int RegParser::handle_quantified_match(const char **c, const TokenList &token_list, int idx)
{
    const auto &regex = token_list.regex;
    const Re &current = regex[idx];

    switch (current.quantifier)
    {
    case PLUS:
        return handle_plus_quantifier(c, token_list, idx) ? 1 : 0;
    case MARK:
        return handle_question_quantifier(c, token_list, idx) ? 1 : 0;
    case NONE:
    default:
        return handle_no_quantifier(c, token_list, idx) ? 1 : 0;
    }
}

bool RegParser::handle_plus_quantifier(const char **c, const TokenList &token_list, int idx)
{
    const auto &regex = token_list.regex;
    const Re &current = regex[idx];

    if (current.type == ALT)
    {
        return match_alt_one_or_more(c, token_list, idx);
    }
    else
    {
        return match_one_or_more(c, token_list, idx);
    }
}

bool RegParser::handle_question_quantifier(const char **c, const TokenList &token_list, int idx)
{
    const auto &regex = token_list.regex;
    const Re &current = regex[idx];

    if (current.type == ALT)
    {
        return match_alt_zero_or_one(c, token_list, idx);
    }
    else
    {
        return match_zero_or_one(c, token_list, idx);
    }
}

bool RegParser::handle_no_quantifier(const char **c, const TokenList &token_list, int idx)
{
    const auto &regex = token_list.regex;
    const Re &current = regex[idx];

    if (current.type == ALT)
    {
        return handle_alternation(c, token_list, idx);
    }
    else
    {
        return handle_single_match(c, token_list, idx);
    }
}

bool RegParser::handle_alternation(const char **c, const TokenList &token_list, int idx)
{
    const auto &regex = token_list.regex;
    const Re &current = regex[idx];
    group_start[current.captured_gp_id] = *c;

    for (const auto &alt : current.alternatives)
    {
        const char *temp_c = *c;
        if (match_from_position(&temp_c, alt, 0))
        {

            if (current.captured_gp_id >= 0)
            {
                captures[current.captured_gp_id] = {*c, static_cast<size_t>(temp_c - *c)};
#ifdef DEBUG
                std::cout << "Captured group " << current.captured_gp_id << ": ";
                for (size_t i = 0; i < captures[current.captured_gp_id].length; ++i)
                    std::cout << captures[current.captured_gp_id].start[i];
                std::cout << std::endl;
#endif
            }
            *c = temp_c;
            return true;
        }
    }
    return false;
}

bool RegParser::handle_single_match(const char **c, const TokenList &token_list, int idx)
{
    const auto &regex = token_list.regex;
    const Re &current = regex[idx];

    if (current.type == BACKREF)
    {
        int gp_id = current.captured_gp_id;

        if (captures.find(gp_id) == captures.end())
            return false;

        const CaptureGroup &cap = captures[gp_id];

        // Check if we have enough characters remaining in the input
        if (strlen(*c) < cap.length)
            return false;

        for (size_t i = 0; i < cap.length; ++i)
        {
            if ((*c)[i] != cap.start[i])
                return false;
        }
        *c += cap.length;
        return true;
    }
    if (match_current(*c, regex, idx))
    {
        if (!(regex[idx].type == START || regex[idx].type == END))
        {
            ++(*c);
        }
        return true;
    }
    return false;
}

bool RegParser::match_one_or_more(const char **c, const TokenList &token_list, int idx)
{
    const auto &regex = token_list.regex;
    const Re &current = regex[idx];

    const char *t = *c;

    if (!match_current(t, regex, idx))
        return false;
    ++t;

    const char *temp_pos = t;
    while (*t != '\0' && match_current(t, regex, idx))
    {
        ++t;
    }

    if (temp_pos == t)
    {
        *c = t;
        return true;
    }

    while (t > *c)
    {
        const char *test_pos = t;
        if (can_match_next_here(test_pos, token_list, idx + 1, true))
        {
            *c = test_pos;
            return true;
        }
        --t;
    }
    return false;
}

bool RegParser::match_alt_one_or_more(const char **c, const TokenList &token_list, int idx)
{
    const auto &regex = token_list.regex;
    const Re &altGp = regex[idx];
    std::vector<const char *> match_ends;
    const char *pos = *c;

    // match as far as it could go (greedy)
    while (*pos != '\0')
    {
        // const char *match_start = pos;
        bool found_match = false;

        for (const auto &alt : altGp.alternatives)
        {
            const char *temp_pos = pos;
            if (match_from_position(&temp_pos, alt, 0))
            {
                if (temp_pos == pos)
                {
                    continue;
                }
                // return false;
                match_ends.push_back(temp_pos); // store end pos
                pos = temp_pos;
                found_match = true;
                break;
            }
        }

        if (!found_match)
            break;
    }

    if (match_ends.empty())
        return false;

    // backtracking

    for (int num_matches = match_ends.size() - 1; num_matches >= 0; num_matches--)
    {
        const char *t = match_ends[num_matches];
        if (can_match_next_here(t, token_list, idx + 1, true))
        {

            if (altGp.captured_gp_id >= 0)
            {
                const char *last_match_start = (num_matches > 0) ? match_ends[num_matches - 1] : *c;
                captures[altGp.captured_gp_id] = {last_match_start,
                                                  static_cast<size_t>(match_ends[num_matches] - last_match_start)};
#ifdef DEBUG
                std::cout << "Captured group " << altGp.captured_gp_id << ": ";
                for (size_t i = 0; i < captures[altGp.captured_gp_id].length; ++i)
                    std::cout << captures[altGp.captured_gp_id].start[i];
                std::cout << std::endl;
#endif
            }
            *c = t;
            return true;
        }
    }
    return false;
}

bool RegParser::match_zero_or_one(const char **c, const TokenList &token_list, int idx)
{
    const auto &regex = token_list.regex;
    const Re &current = regex[idx];

    if (match_current(*c, regex, idx))
    {
        const char *temp_pos = *c + 1;
        if (can_match_next_here(temp_pos, token_list, idx + 1, true))
        {
            *c = temp_pos;
            return true;
        }
    }

    // Try matching without the character (zero occurrences)
    return true;
    // return can_match_next_here(*c, token_list, idx + 1);
}

bool RegParser::match_alt_zero_or_one(const char **c, const TokenList &token_list, int idx)
{
    const auto &regex = token_list.regex;
    const Re &altGp = regex[idx];

    for (const auto &alt : altGp.alternatives)
    {
        const char *t = *c;
        if (match_from_position(&t, alt, 0))
        {
            const char *temp_t = t;

            if (can_match_next_here(t, token_list, idx + 1, true))
            {
                if (altGp.captured_gp_id >= 0)
                {
                    captures[altGp.captured_gp_id] = {*c, static_cast<size_t>(temp_t - *c)};
#ifdef DEBUG
                    std::cout << "Captured group " << altGp.captured_gp_id << ": ";
                    for (size_t i = 0; i < captures[altGp.captured_gp_id].length; ++i)
                        std::cout << captures[altGp.captured_gp_id].start[i];
                    std::cout << std::endl;
#endif
                }
                *c = t;

                return true;
            }
        }
    }
    return true;
    // return can_match_next_here(*c, token_list, idx + 1);
}

bool RegParser::isEof()
{
    return *_pattern == '\0';
}

bool RegParser::consume()
{
    if (!isEof())
    {
        ++_pattern;
        return true;
    }
    return false;
}

bool RegParser::check(char c)
{
    return *_pattern == c;
}

bool RegParser::match(char c)
{
    if (check(c))
        return consume();
    return false;
}

Re RegParser::parseElement()
{
    if (match('^'))
    {
        return makeRe(START);
    }
    else if (match('$'))
        return makeRe(END);
    else if (match('\\'))
    {
        if (match('w'))
        {

            Re current = makeRe(ALPHANUM);
            applyQuantifiers(current);
            return current;
        }
        else if (match('d'))
        {
            Re current = makeRe(DIGIT);
            applyQuantifiers(current);
            return current;
        }
        else if (isdigit(*_pattern))
        {
            int gp_num = *_pattern - '0';
            consume();

            Re current = makeRe(BACKREF);
            current.captured_gp_id = gp_num;
            applyQuantifiers(current);
            return current;
        }
        else
        {
            consume();
            return makeRe(ETK);
        }
    }
    else if (match('['))
    {
        return parseCharacterClass();
    }
    else if (match('('))
    {
        return parseGroup();
    }
    else
    {
        // parse single char
        char *cstr = new char[2];
        cstr[0] = *_pattern;
        cstr[1] = '\0';
        consume();

        Re current = makeRe(SINGLE_CHAR, cstr);
        applyQuantifiers(current);
        return current;
    }
}

Re RegParser::parseCharacterClass()
{
    Re current = makeRe(LIST);
    std::string buffer;

    current.isNegative = match('^');
    while (!isEof() && !check(']'))
    {
        buffer.push_back(*_pattern);
        consume();
    }

    if (!match(']'))
    {
        consume();
        return makeRe(ETK);
    }

    char *cstr = new char[buffer.size() + 1];
    std::copy(buffer.begin(), buffer.end(), cstr);
    cstr[buffer.size()] = '\0';
    current.ccl = cstr;
    applyQuantifiers(current);
    return current;
}

Re RegParser::parseGroup()
{
    TokenList *parent = parser_gp_stack.top();
    TokenList current_token_list = {parent, std::vector<Re>()};
    parser_gp_stack.push(&current_token_list);

    Re altGp = makeRe(ALT);
    altGp.captured_gp_id = next_capture_id++;
    bool isClosed = false;

    do
    {
        if (match('|'))
        {
            altGp.alternatives.push_back(current_token_list);
            current_token_list.regex.clear();
        }
        else if (match(')'))
        {
            parser_gp_stack.pop();
            altGp.alternatives.push_back(current_token_list);
            isClosed = true;
            break;
        }
        else
        {
            Re element = parseElement();
            if (element.type == ETK)
                return makeRe(ETK);
            current_token_list.regex.push_back(element);
        }
    } while (!isEof());

    if (!isClosed)
    {
        parser_gp_stack.pop();
        return makeRe(ETK);
    }

    applyQuantifiers(altGp);
    return altGp;
}

void RegParser::applyQuantifiers(Re &element)
{
    if (!isEof() && match('+'))
        element.quantifier = PLUS;
    else if (!isEof() && match('?'))
        element.quantifier = MARK;
}
