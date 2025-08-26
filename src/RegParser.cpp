#include "include/RegParser.h"

bool RegParser::parse()
{
    while (!isEof())
    {
        Re element = parseElement();
        if (element.type == ETK)
            return false;
        regex.push_back(element);
    }
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

bool RegParser::match_from_position(const char **start_pos, const std::vector<Re> &regex, int idx)
{
    const char *c = *start_pos;
    int rIdx = idx;
    int pattern_length = regex.size();

    while (rIdx < pattern_length)
    {
        const Re &current = regex[rIdx];

        int consumed = handle_quantified_match(&c, regex, rIdx);
        if (consumed <= 0)
            return false;

        rIdx += consumed;
    }

    *start_pos = c;
    // return (rIdx >= pattern_length) || (rIdx < pattern_length && regex[rIdx].type == END && *c == '\0');
    return true;
}

int RegParser::handle_quantified_match(const char **c, const std::vector<Re> &regex, int idx)
{
    const Re &current = regex[idx];

    switch (current.quantifier)
    {
    case PLUS:
        return handle_plus_quantifier(c, regex, idx) ? regex.size() - idx : 0;
    case MARK:
        return handle_question_quantifier(c, regex, idx) ? regex.size() - idx : 0;
    case NONE:
    default:
        return handle_no_quantifier(c, regex, idx) ? 1 : 0;
    }
}

bool RegParser::handle_plus_quantifier(const char **c, const std::vector<Re> &regex, int idx)
{
    const Re &current = regex[idx];

    if (current.type == ALT)
    {
        return match_alt_one_or_more(c, regex, idx);
    }
    else
    {
        return match_one_or_more(c, regex, idx);
    }
}

bool RegParser::handle_question_quantifier(const char **c, const std::vector<Re> &regex, int idx)
{
    const Re &current = regex[idx];

    if (current.type == ALT)
    {
        return match_alt_zero_or_one(c, regex, idx);
    }
    else
    {
        return match_zero_or_one(c, regex, idx);
    }
}

bool RegParser::handle_no_quantifier(const char **c, const std::vector<Re> &regex, int idx)
{
    const Re &current = regex[idx];

    if (current.type == ALT)
    {
        return handle_alternation(c, regex, idx);
    }
    else
    {
        return handle_single_match(c, regex, idx);
    }
}

bool RegParser::handle_alternation(const char **c, const std::vector<Re> &regex, int idx)
{
    const Re &current = regex[idx];

    for (const auto &alt : current.alternatives)
    {
        const char *temp_c = *c;
        if (match_from_position(&temp_c, alt, 0))
        {

            if (current.captured_gp_id >= 0)
            {
                captures[current.captured_gp_id] = {*c, static_cast<size_t>(temp_c - *c)};

                // for (size_t i = 0; i < captures[current.captured_gp_id].length; ++i)
                //     std::cout << captures[current.captured_gp_id].start[i];
                // std::cout << std::endl;
            }
            *c = temp_c;
            return true;
        }
    }
    return false;
}

bool RegParser::handle_single_match(const char **c, const std::vector<Re> &regex, int idx)
{
    const Re &current = regex[idx];

    if (current.type == BACKREF)
    {
        int gp_id = current.captured_gp_id;

        if (captures.find(gp_id) == captures.end())
            return false;

        const CaptureGroup &cap = captures[gp_id];
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

bool RegParser::match_one_or_more(const char **c, const std::vector<Re> &regex, int idx)
{
    const Re &current = regex[idx];

    const char *t = *c;

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
        if (match_from_position(&t, regex, idx + 1))
        {

            if (altGp.captured_gp_id >= 0)
            {
                const char *last_match_start = (num_matches > 0) ? match_ends[num_matches - 1] : *c;
                captures[altGp.captured_gp_id] = {last_match_start,
                                                  static_cast<size_t>(match_ends[num_matches] - last_match_start)};
            }
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
            // TODO: population to map
            const char *temp_t = t;

            if (match_from_position(&t, regex, idx + 1))
            {
                if (altGp.captured_gp_id >= 0)
                {
                    captures[altGp.captured_gp_id] = {*c, static_cast<size_t>(temp_t - *c)};
                }
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
    Re altGp = makeRe(ALT);
    altGp.captured_gp_id = next_capture_id++;
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
