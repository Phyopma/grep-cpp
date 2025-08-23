#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include "include/RegParser.h"

// #define DEBUG
// #define USE_TEST

bool match_pattern(const std::string &input_line, const std::string &pattern)
{
    RegParser rp(pattern);
    if (rp.parse())
    {
        int pattern_length = rp.regex.size();
        const char *c = input_line.c_str();

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
            default:
            {
                std::cout << "ETK" << std::endl;
                break;
            }
            }
        }
#endif
        return false;
    }
    else
    {
        throw std::runtime_error("Unhandled pattern " + pattern);
    }
    // if (pattern.length() == 1)
    // {
    //     return input_line.find(pattern) != std::string::npos;
    // }
    // else if (pattern == "\\d")
    // {
    //     bool found = false;
    //     for (const char c : input_line)
    //     {
    //         found |= isdigit(c);
    //         if (found)
    //             break;
    //     }
    //     return found;
    // }
    // else if (pattern == "\\w")
    // {
    //     bool found = false;
    //     for (const char c : input_line)
    //     {
    //         found |= (isalnum(c) || (c == '_'));
    //         if (found)
    //             break;
    //     }
    //     return found;
    // }
    // else if (!pattern.empty() && pattern.front() == '[' && pattern.back() == ']')
    // {
    //     bool isNegative = pattern.at(1) == '^';
    //     std::string white_list = isNegative ? pattern.substr(2, pattern.size() - 3) : pattern.substr(1, pattern.size() - 2);

    //     bool found = false;
    //     if (!isNegative)
    //     {
    //         for (const char c : white_list)
    //         {
    //             found |= input_line.find(c) != std::string::npos;
    //             if (found)
    //                 break;
    //         }
    //     }
    //     else
    //     {
    //         // std::cout << "Negative" << std::endl;
    //         for (const char c : white_list)
    //         {
    //             // std::cout << c << ",  ";
    //             found |= input_line.find(c) == std::string::npos;
    //             if (found)
    //                 break;
    //         }
    //     }
    //     return found;
    // }
    // else
    // {
    //     throw std::runtime_error("Unhandled pattern " + pattern);
    // }
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
