#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include "include/RegParser.h"
#include <filesystem>

bool match_pattern(const std::string &input_line, const std::string &pattern)
{
    try
    {
        RegParser rp(pattern);
        return rp.match(input_line);
    }
    catch (const std::exception &e)
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

bool search_directory(const std::string &path, const std::string &pattern)
{
    bool is_found = false;
    for (auto const &entry : std::filesystem::recursive_directory_iterator(path))
    {
        if (entry.is_regular_file())
        {
            std::ifstream file(entry.path());
            if (!file.is_open())
            {
                std::cerr << "Failed to open file: " << entry.path() << std::endl;
                continue;
            }
            is_found |= any_match(file, pattern, entry.path().string());
        }
    }
    return is_found;
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

    if (flag == "-E")
    {
        std::string pattern = argv[2];
        std::string filename;
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
    else if (flag == "-r")
    {
        if (argc < 4)
        {
            std::cerr << "Expected pattern and directory" << std::endl;
            return 1;
        }
        std::string pattern = argv[3];
        std::string dir = argv[4];
        bool found = search_directory(dir, pattern);
        return found ? 0 : 1;
    }
    else
    {
        std::cerr << "Expected first argument to be '-E'" << std::endl;
        return 1;
    }
}
