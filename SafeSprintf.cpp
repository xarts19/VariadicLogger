#include "SafeSprintf.h"

#include <stdexcept>
#include <assert.h>


Ut::d_::Split Ut::d_::split_format(const std::string& fmt)
{
    Split result;
    size_t start = 0;
    size_t end = 0;
    size_t pos = 0;

    for (;;)
    {
        start = fmt.find('{', pos);

        if (start == fmt.npos)
        {
            if (pos != fmt.npos)
                result.push_back(Substring(SubstrText, fmt.substr(pos)));

            pos = fmt.npos;

            break;
        }
        else
        {
            if (start + 1 < fmt.size() && fmt[start + 1] == '{')
            {
                result.push_back(Substring(SubstrText, fmt.substr(pos, start - pos + 2)));
                pos = start + 2;
                continue;
            }

            if (start != pos)
                result.push_back(Substring(SubstrText, fmt.substr(pos, start - pos)));

            pos = start;
        }

        // loop until we find a closing curly brace
        for (;;)
        {
            // ok if we go out of bounds, because it will return npos anyway
            end = fmt.find('}', pos + 1);

            if (end == fmt.npos)
            {
                throw std::runtime_error("Error in format string: no closing curly brace.");
            }
            else
            {
                if (end + 1 < fmt.size() && fmt[end + 1] == '}')
                {
                    pos = end + 2;
                    continue;
                }

                result.push_back(Substring(SubstrAnchor, fmt.substr(start + 1, end - start - 1)));
                pos = end + 1;
                break;
            }
        }
    }

    return result;
}


void Ut::d_::join(std::string& out, const Split& split)
{
    for (const Substring& chunk : split)
    {
        if (chunk.type == SubstrText)
            out.append(chunk.content);
        else
        {
            out.push_back('{');
            out.append(chunk.content);
            out.push_back('}');
        }
    }
}


bool Ut::d_::has_index(const std::string& substr, int index)
{
    size_t pos = substr.find_first_of(":");

    if ((pos != substr.npos && pos < 1) || (pos == substr.npos && substr.size() < 1))
        throw std::runtime_error("No position marker provided");

    return index == stoi(substr.substr(0, pos));
}


void Ut::d_::modify_stream(std::ostringstream& oss, const std::string format)
{
    if (format.find_first_of("xX") != format.npos)
        oss << std::hex;
}

