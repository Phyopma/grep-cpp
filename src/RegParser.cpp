#include "include/RegParser.h"

bool RegParser::parse()
{
    do
    {
        if (match('^'))
        {
            regex.push_back(makeRe(START));
        }
        else if (match('$'))
        {
            regex.push_back(makeRe(END));
        }
        else if (match('\\'))
        {
            // consume(); // consume '\'
            if (match('w'))
                regex.push_back(makeRe(ALPHANUM));
            else if (match('d'))
                regex.push_back(makeRe(DIGIT));
        }
        else if (match('['))
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
                regex.push_back(makeRe(ETK));
                return false;
            }

            char *cstr = new char[buffer.size() + 1];
            std::copy(buffer.begin(), buffer.end(), cstr);
            cstr[buffer.size()] = '\0';

            current.ccl = cstr;
            regex.push_back(current);
        }
        else
        {
            char *cstr = new char[2];
            cstr[0] = *_pattern;
            cstr[1] = '\0';
            consume();

            Re current = makeRe(SINGLE_CHAR, cstr);
            if (!isEof() && match('+'))
                current.quantifier = PLUS;
            else if (!isEof() && match('?'))
                current.quantifier = MARK;
            regex.push_back(current);
        }

    } while (!isEof());

    return true;
}

Re RegParser::makeRe(RegType type, char *ccl, bool isNegative, Quantifier quant)
{
    Re current;
    current.type = type;
    current.ccl = ccl;
    current.isNegative = isNegative;
    current.quantifier = quant;
    return current;
}

bool RegParser::match_current(const char *c, const std::vector<Re> &regex, int idx)
{
    const Re *current = &regex[idx];
    switch (current->type)
    {
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

bool RegParser::match_from_position(const char *start_pos, const std::vector<Re> &regex, int idx)
{
    const char *c = start_pos;
    int rIdx = idx;
    int pattern_length = regex.size();

    while (rIdx < pattern_length && *c != '\0')
    {
        const Re &current = regex[rIdx];
        switch (current.quantifier)
        {
        case PLUS:
            return match_one_or_more(c, regex, rIdx);
        case MARK:
            return match_zero_or_one(c, regex, rIdx);
        case NONE:
        default:
            if (RegParser::match_current(c, regex, rIdx))
            {
                ++c;
                ++rIdx;
            }
            else
                return false;
            break;
        }
    }
    return (rIdx >= pattern_length) || (rIdx < pattern_length && regex[rIdx].type == END && *c == '\0');
}

bool RegParser::match_one_or_more(const char *c, const std::vector<Re> &regex, int idx)
{
    const char *t = c;
    char target_char = *regex[idx].ccl;

    while (*t != '\0' && (*t == target_char || target_char == '.'))
    {
        ++t;
    }

    if (t == c)
        return false;

    do
    {
        if (match_from_position(t, regex, idx + 1))
            return true;
        --t;
    } while (t > c);
    return false;
}

bool RegParser::match_zero_or_one(const char *c, const std::vector<Re> &regex, int idx)
{
    if (match_current(c, regex, idx))
    {
        if (match_from_position(c + 1, regex, idx + 1))
            return true;
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
