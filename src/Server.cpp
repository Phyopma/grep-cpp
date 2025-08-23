#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include "include/RegParser.h"

#define DEBUG
// #define USE_TEST

bool match_pattern(const std::string &input_line, const std::string &pattern)
{
    RegParser rp(input_line, pattern);

    if (rp.parse())
    {
#ifdef DEBUG
        for (const auto &tmp : rp.regex)
        {
            switch (tmp.type)
            {
            case DIGIT:
            {
                std::cout << "DIGIT" << std::endl;
                break;
            }
            case ALPHANUM:
            {
                std::cout << "ALPHANUM" << std::endl;
                break;
            }
            case SINGLE_CHAR:
            {
                std::cout << "SINGLE CHAR" << " >> " << tmp.ccl << std::endl;
                break;
            }
            case LIST:
            {
                std::cout << (tmp.isNegative ? "NEGATIVE " : "POSITIVE ") << "LIST" << " >> " << tmp.ccl << std::endl;
                break;
            }
            case START:
            {
                std::cout << "START" << std::endl;
                break;
            }
            default:
            {
                std::cout << "ETK" << std::endl;
                break;
            }
            }
        }
#endif
        int pattern_length = rp.regex.size();
        const char *c = input_line.c_str();

        bool hasStartAnchor = !rp.regex.empty() && rp.regex[0].type == START;

        if (hasStartAnchor)
        {
            int rIdx = 1;
            const char *start = c;
            while (rIdx < pattern_length && *c != '\0' && RegParser::match_current(c, rp.regex, rIdx))
            {
                ++c;
                ++rIdx;
            }
            return rIdx == pattern_length;
        }
        else
        {

            while (*c != '\0')
            {
                const char *start = c;

                int rIdx = 0;

                while (rIdx < pattern_length && *c != '\0' && RegParser::match_current(c, rp.regex, rIdx))
                {
                    ++c;
                    ++rIdx;
                }
                if (rIdx == pattern_length)
                    return true;
                c = start + 1;
            }
            return false;
        }
    }
    else
    {
        throw std::runtime_error("Unhandled pattern " + pattern);
    }
}

int main(int argc, char *argv[])
{
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    // You can use print statements as follows for debugging, they'll be visible when running tests.
    std::cerr << "Logs from your program will appear here" << std::endl;

    if (argc != 3)
    {
        std::cerr << "Expected two arguments" << std::endl;
        return 1;
    }

    std::string flag = argv[1];
    std::string pattern = argv[2];

    if (flag != "-E")
    {
        std::cerr << "Expected first argument to be '-E'" << std::endl;
        return 1;
    }

    // Uncomment this block to pass the first stage
    //
    std::string input_line;
#ifdef USE_TEST
    std::ifstream file("test_input.txt");
    if (file.is_open())
    {
        std::getline(file, input_line);
        std::cerr << "Read from file: '" << input_line << "'" << std::endl;
    }
#else
    std::getline(std::cin, input_line);
#endif

    try
    {
        if (match_pattern(input_line, pattern))
        {
            return 0;
        }
        else
        {
            return 1;
        }
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
