#include "include/RegParser.h"

bool RegParser::parse()
{
    do
    {
        Re element = parseElement();
        if (element.type == ETK)
            return false;
        regex.push_back(element);
    } while (!isEof());
    return true;
}

Re RegParser::makeRe(RegType type, char *ccl, bool isNegative)
{
    Re current;
    current.type = type;
    current.ccl = ccl;
    current.isNegative = isNegative;
    current.quantifier = NONE;
    return current;
}

bool RegParser::match_current(const char *c, const std::vector<Re> &regex, int idx)
{
    const Re *current = &regex[idx];
    switch (current->type)
    {
    // case ALT:
    // {
    //     for (const auto &alt : current->alternatives)
    //     {
    //         if (match_from_position(&c, alt, 0))
    //             return true;
    //     }
    //     return false;
    // }
    case DIGIT:
    {
        return isdigit(*c);
    }
    case ALPHANUM:
    {
        return isalnum(*c) || *c == '_';
    }
    case SINGLE_CHAR:
    {
        return *c == *current->ccl || *current->ccl == '.';
    }
    case LIST:
    {
        const char *p = current->ccl;
        bool found = false;
        while (*p != '\0')
        {
            if (*c == *p)
            {
                found = true;
                break;
            }
            ++p;
        }
        return current->isNegative ? !found : found;
    }
    case START:
        return true;
    case END:
        return *c == '\0';
    default:
    {
        return false;
    }
    }
}

bool RegParser::match_from_position(const char **start_pos, const std::vector<Re> &regex, int idx)
{
    const char *c = *start_pos;
    int rIdx = idx;
    int pattern_length = regex.size();

    while (rIdx < pattern_length && *c != '\0')
    {
        const Re &current = regex[rIdx];
        bool matched = false;
        switch (current.quantifier)
        {
        case PLUS:
            if (current.type == ALT)
                return match_alt_one_or_more(&c, regex, rIdx);
            else
                return match_one_or_more(&c, regex, rIdx);
        case MARK:
            if (current.type == ALT)
                return match_alt_zero_or_one(&c, regex, rIdx);
            else
                return match_zero_or_one(&c, regex, rIdx);
        case NONE:
        default:
            if (current.type == ALT)
            {
                for (const auto &alt : current.alternatives)
                {
                    const char *temp_c = c;
                    if (match_from_position(&temp_c, alt, 0))
                    {
                        c = temp_c;
                        matched = true;
                        // ++rIdx;
                        break;
                    }
                }
                if (!matched)
                {
                    *start_pos = c;
                    return false;
                }
            }
            else
            {
                if (RegParser::match_current(c, regex, rIdx))
                {
                    ++c;
                    // ++rIdx;
                    matched = true;
                }
                else
                {
                    *start_pos = c;
                    return false;
                }
            }
            break;
        }
        if (matched)
            ++rIdx;
    }

    *start_pos = c;
    return (rIdx >= pattern_length) || (rIdx < pattern_length && regex[rIdx].type == END && *c == '\0');
}

bool RegParser::match_one_or_more(const char **c, const std::vector<Re> &regex, int idx)
{
    const Re &current = regex[idx];

    const char *t = *c;
    // TODO: assert(ccl is valid) for DIGIT and ALPHANUM
    // char target_char = *current.ccl;

    if (!match_current(t, regex, idx))
        return false;
    ++t;

    while (*t != '\0' && match_current(t, regex, idx))
    {
        ++t;
    }

    do
    {
        const char *test_pos = t;
        if (match_from_position(&test_pos, regex, idx + 1))
        {
            *c = test_pos;
            return true;
        }
        --t;
    } while (t > *c);
    return false;
}

bool RegParser::match_alt_one_or_more(const char **c, const std::vector<Re> &regex, int idx)
{
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
        if (match_from_position(&t, regex, idx + 1))
        {
            *c = t;
            return true;
        }
    }
    return false;
}

bool RegParser::match_zero_or_one(const char **c, const std::vector<Re> &regex, int idx)
{
    const Re &current = regex[idx];

    if (match_current(*c, regex, idx))
    {
        const char *temp_pos = *c + 1;
        if (match_from_position(&temp_pos, regex, idx + 1))
        {
            *c = temp_pos;
            return true;
        }
    }
    return match_from_position(c, regex, idx + 1);
}

bool RegParser::match_alt_zero_or_one(const char **c, const std::vector<Re> &regex, int idx)
{
    const Re &altGp = regex[idx];

    for (const auto &alt : altGp.alternatives)
    {
        const char *t = *c;
        if (match_from_position(&t, alt, 0))
        {
            if (match_from_position(&t, regex, idx + 1))
            {
                *c = t;
                return true;
            }
        }
    }
    return match_from_position(c, regex, idx + 1);
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
            return makeRe(ALPHANUM);
        else if (match('d'))
            return makeRe(DIGIT);
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
    return current;
}

Re RegParser::parseGroup()
{
    Re altGp = makeRe(ALT);
    std::vector<Re> current_sequence;
    bool isClosed = false;

    do
    {
        if (match('|'))
        {
            altGp.alternatives.push_back(current_sequence);
            current_sequence.clear();
        }
        else if (match(')'))
        {
            altGp.alternatives.push_back(current_sequence);
            isClosed = true;
            break;
        }
        else
        {
            Re element = parseElement();
            if (element.type == ETK)
                return makeRe(ETK);
            current_sequence.push_back(element);
        }
    } while (!isEof());

    if (!isClosed)
        return makeRe(ETK);

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
