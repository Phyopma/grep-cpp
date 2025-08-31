#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include "include/RegParser.h"

// #define DEBUG
// #define USE_TEST

#ifdef DEBUG

void printQuantifier(const Re &re)
{
    switch (re.quantifier)
    {
    case PLUS:
    {
        std::cout << "+";
        break;
    }
    case MARK:
    {
        std::cout << "?";
        break;
    }
    case STAR:
    {
        std::cout << "*";
        break;
    }
    default:
        break;
    }
}

void printDebug(const std::vector<Re> &reList)
{
    for (const auto &tmp : reList)
    {
        switch (tmp.type)
        {
        case DIGIT:
        {
            std::cout << "DIGIT" << " >> ";
            printQuantifier(tmp);
            std::cout << std::endl;
            break;
        }
        case ALPHANUM:
        {
            std::cout << "ALPHANUM" << " >> ";
            printQuantifier(tmp);
            std::cout << std::endl;
            break;
        }
        case SINGLE_CHAR:
        {
            std::cout << "SINGLE CHAR" << " >> " << tmp.ccl;
            printQuantifier(tmp);
            std::cout << std::endl;
            break;
        }
        case BACKREF:
        {
            std::cout << "BACKREF" << " >> ";
            std::cout << tmp.captured_gp_id << std::endl;
            break;
        }
        case ALT:
        {
            std::cout << "ALT" << " >> " << std::endl;
            std::cout << "(" << std::endl;
            size_t l = tmp.alternatives.size();
            for (size_t i = 0; i < l; ++i)
            {
                printDebug(tmp.alternatives[i].regex);
                if (i < l - 1)
                    std::cout << "|" << std::endl;
            }
            std::cout << ")";
            printQuantifier(tmp);
            std::cout << std::endl;
            break;
        }
        case LIST:
        {
            std::cout << (tmp.isNegative ? "NEGATIVE " : "POSITIVE ") << "LIST" << " >> " << tmp.ccl;
            printQuantifier(tmp);
            std::cout << std::endl;
            break;
        }
        case START:
        {
            std::cout << "START" << std::endl;
            break;
        }
        case END:
        {
            std::cout << "END" << std::endl;
            break;
        }
        default:
        {
            std::cout << "ETK" << std::endl;
            break;
        }
        }
    }
}
#endif

bool match_pattern(const std::string &input_line, const std::string &pattern)
{
    RegParser rp(pattern);

    if (rp.parse())
    {
        // rp.parser_gp_stack.push(&rp.token_list);
        const auto &regex = rp.token_list.regex;
#ifdef DEBUG
        printDebug(regex);
#endif
        int pattern_length = regex.size();
        const char *c = input_line.c_str();

        bool hasStartAnchor = !regex.empty() && regex[0].type == START;

        if (hasStartAnchor)
        {
            rp.sync_index(&rp.token_list, 1);
            return rp.match_from_position(&c, rp.token_list, 1);
        }
        else
        {
            while (*c != '\0')
            {
                rp.sync_index(&rp.token_list, 0);
                if (rp.match_from_position(&c, rp.token_list, 0))
                {
                    return true;
                }
                ++c;
            }

            return false;
        }
    }
    else
    {
        throw std::runtime_error("Unhandled pattern " + pattern);
    }
}

bool any_match(std::istream &in, const std::string &pattern, const std::string &filename = "")
{
    std::string line;
    bool isMatched = false;
    while (std::getline(in, line))
    {
        if (match_pattern(line, pattern))
        {
            isMatched = true;
            if (!filename.empty())
                std::cout << filename << ":";
            std::cout << line << std::endl;
        }
    }
    return isMatched;
}

int main(int argc, char *argv[])
{
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    // You can use print statements as follows for debugging, they'll be visible when running tests.
    // std::cerr << "Logs from your program will appear here" << std::endl;

    if (argc < 3)
    {
        std::cerr << "Expected two arguments" << std::endl;
        return 1;
    }

    std::string flag = argv[1];
    std::string pattern = argv[2];
    std::string filename;

    if (flag != "-E")
    {
        std::cerr << "Expected first argument to be '-E'" << std::endl;
        return 1;
    }

    try
    {
        if (argc >= 4)
        {
            bool is_found = false;
            for (int i = 3; i < argc; ++i)
            {
                filename = argv[i];
                if (!filename.empty())
                {
                    std::ifstream file(filename);
                    if (!file.is_open())
                    {
                        std::cerr << "Failed to open file: " << filename << std::endl;
                        return 1;
                    }
                    is_found |= any_match(file, pattern, argc > 4 ? filename : "");
                }
            }
            return !is_found;
        }
        else
        {
            return any_match(std::cin, pattern) ? 0 : 1;
        }
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
