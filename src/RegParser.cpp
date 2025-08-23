#include "include/RegParser.h"

bool RegParser::parse()
{
    do
    {
        if (match('^'))
        {
            regex.push_back(makeRe(START));
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
            regex.push_back(makeRe(SINGLE_CHAR, cstr));
        }

    } while (!isEof());

    return true;
}

Re RegParser::makeRe(RegType type, char *ccl, bool isNegative)
{
    Re current;
    current.type = type;
    current.ccl = ccl;
    current.isNegative = isNegative;
    return current;
}

bool RegParser::match_current(const char *c, std::vector<Re> &regex, int idx)
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
        return *c == *current->ccl;
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
    default:
    {
        return false;
    }
    }
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
