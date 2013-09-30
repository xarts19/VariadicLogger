#include "VTLogger/SafeSprintf.h"

#include <stdexcept>
#include <assert.h>
#include <ctype.h>


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


namespace
{


    enum Align
    {
        A_Left,   // default for text
        A_Right,  // default for numers
        A_Center,
        A_SignAware    // e. g. +0000123
    };

    enum Sign
    {
        S_Both,
        S_Negative,   // Default
        S_Space       // Space for positive, minus for negative
    };

    struct Format
    {
        char fill;
        Align align;
        Sign sign;
        bool alternate_form;
        int width;
        bool use_thousand_sep;
        int precision;
        char type;

        explicit Format(Ut::d_::ValueType type)
            : fill(' ')
            , align(A_Left)
            , sign(S_Negative)
            , alternate_form(false)
            , width(0)
            , use_thousand_sep(false)
            , precision(-1)
            , type('\0')
        {
            switch (type)
            {
            case Ut::d_::VT_Integral:
            case Ut::d_::VT_Floating:
                fill = '0';
                align = A_Right;
                break;
            case Ut::d_::VT_Other:
                // already set
                break;
            default:
                assert(0 && "impossible");
            }
        }

        static Align align_from_char(char ch)
        {
            switch (ch)
            {
            case '>':
                return A_Right;
            case '<':
                return A_Left;
            case '=':
                return A_SignAware;
            case '^':
                return A_Center;
            default:
                assert(0 && "impossible");
            };
        }

        static Sign sign_from_char(char ch)
        {
            switch (ch)
            {
            case '+':
                return S_Both;
            case '-':
                return S_Negative;
            case ' ':
                return S_Space;
            default:
                assert(0 && "impossible");
            };
        }
    };

    template <int N>
    bool char_in_set(char ch, const char (&set)[N])
    {
        for (int i = 0; i < N-1; ++i)
            if (ch == set[i])
                return true;
        return false;
    }

    Format parse_format(const std::string& format, Ut::d_::ValueType type)
    {
        /*
         * format_spec ::=  [[fill]align][sign][#][0][width][,][.precision][type]
         * fill        ::=  <a character other than '{' or '}'>
         * align       ::=  "<" | ">" | "=" | "^"
         * sign        ::=  "+" | "-" | " "
         * width       ::=  integer
         * precision   ::=  integer
         * type        ::=  "b" | "c" | "d" | "e" | "E" | "f" | "F" | "g" | "G" | "n" | "o" | "s" | "x" | "X" | "%"
         */

        char aligns[] = "><=^";
        char signs[] = "+- ";
        char types[] = "bcdeEfFgGnosxX%";
        int index = 0;
        Format f(type);

        if (index == static_cast<int>(format.size()))
            return f;

        // [[fill]align]

        if (char_in_set(format[index], aligns))
        {
            f.align = Format::align_from_char(format[index]);
            ++index;
        }
        else if (char_in_set(format[index+1], aligns))
        {
            f.align = Format::align_from_char(format[index+1]);
            f.fill = format[index];
            index += 2;
        }

        if (index == static_cast<int>(format.size()))
            return f;

        // [sign]

        if (char_in_set(format[index], signs))
        {
            f.sign = Format::sign_from_char(format[index]);
            ++index;
        }

        if (index == static_cast<int>(format.size()))
            return f;

        // [#]

        if (format[index] == '#')
        {
            f.alternate_form = true;
            ++index;
        }

        if (index == static_cast<int>(format.size()))
            return f;

        // [0]
        if (format[index] == '0')
        {
            f.fill = '0';
            f.align = A_SignAware;
            ++index;
        }

        if (index == static_cast<int>(format.size()))
            return f;

        // [width]
        if (isdigit(format[index]))
        {
            int count = 1;
            while (isdigit(format[index + count]))
                ++count;
            f.width = std::stoi(format.substr(index, count));
            index += count;
        }

        if (index == static_cast<int>(format.size()))
            return f;

        // [,]
        if (format[index] == ',')
        {
            f.use_thousand_sep = true;
            ++index;
        }

        if (index == static_cast<int>(format.size()))
            return f;

        // [.precision]
        if (format[index] == '.')
        {
            if (!isdigit(format[index+1]))
                throw std::runtime_error("Precision not specified after '.'");

            if (type == Ut::d_::VT_Integral)
                throw std::runtime_error("Precision is not allowed for integral types");

            ++index;  // skip the dot
            int count = 1;  // we already checked that first character after the dot is digit
            while (isdigit(format[index + count]))
                ++count;
            f.precision = std::stoi(format.substr(index, count));
            index += count;
        }

        if (index == static_cast<int>(format.size()))
            return f;

        // [type]
        if (char_in_set(format[index], types))
        {
            f.type = format[index];
            ++index;
        }

        if (index != static_cast<int>(format.size()))
            throw std::runtime_error("Error in format specifier: " + format);

        return f;
    }
}

void Ut::d_::modify_stream(std::ostringstream& oss, const std::string& format, ValueType type)
{
    Format f = parse_format(format, type);

    if (f.type == 'x' || f.type == 'X')
    {
        oss << std::hex;
        if (f.type == 'X')
            oss << std::uppercase;
    }


}

