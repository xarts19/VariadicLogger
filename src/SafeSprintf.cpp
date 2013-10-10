/*
 *  Copyright (c) 2013, Vitalii Turinskyi
 *  All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#include "VariadicLogger/SafeSprintf.h"

#include <stdexcept>
#include <algorithm>
#include <assert.h>
#include <ctype.h>
#include <iomanip>


vl::d_::SubstrType vl::d_::Substring::type() const
{
    return type_;
}


bool vl::d_::Substring::has_string() const
{
    return has_string_;
}


const std::string& vl::d_::Substring::content() const
{
    assert(has_string_);
    return content_;
}


const char* vl::d_::Substring::str() const
{
    assert(!has_string_);
    return begin_;
}


int vl::d_::Substring::size() const
{
    assert(!has_string_);
    return size_;
}


const char* vl::d_::find_in_str(const char* b, int size, char what)
{
    return std::find(b, b + size, what);
}


vl::d_::Split vl::d_::split_format(const std::string& fmt)
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
                result.push_back(Substring(SubstrText, &fmt[pos], static_cast<int>(fmt.size() - pos)));

            pos = fmt.npos;

            break;
        }
        else
        {
            if (start + 1 < fmt.size() && fmt[start + 1] == '{')
            {
                result.push_back(Substring(SubstrText, &fmt[pos], static_cast<int>(start - pos + 2)));
                pos = start + 2;
                continue;
            }

            if (start != pos)
                result.push_back(Substring(SubstrText, &fmt[pos], static_cast<int>(start - pos)));

            pos = start;
        }

        // loop until we find a closing curly brace
        for (;;)
        {
            // ok if we go out of bounds, because it will return npos anyway
            end = fmt.find('}', pos + 1);

            if (end == fmt.npos)
            {
                assert(false && "Error in format string: no closing curly brace.");
                throw vl::format_error("Error in format string: no closing curly brace.");
            }
            else
            {
                if (end + 1 < fmt.size() && fmt[end + 1] == '}')
                {
                    pos = end + 2;
                    continue;
                }

                result.push_back(Substring(SubstrAnchor, &fmt[start + 1], static_cast<int>(end - start - 1)));
                pos = end + 1;
                break;
            }
        }
    }

    return result;
}


void vl::d_::join(std::string& out, const Split& split)
{
    for (const Substring& chunk : split)
    {
        if (chunk.type() == SubstrText)
        {
            if (chunk.has_string())
                out.append(chunk.content());
            else
                out.append(chunk.str(), chunk.size());
        }
        else
        {
            out.push_back('{');

            if (chunk.has_string())
                out.append(chunk.content());
            else
                out.append(chunk.str(), chunk.size());

            out.push_back('}');
        }
    }
}


bool vl::d_::has_index(const char* substr, int substr_size, int index)
{
    const char* pos = find_in_str(substr, substr_size, ':');

    if ((pos != substr + substr_size && pos == substr) || (pos == substr + substr_size && substr_size < 1))
    {
        assert(false && "No position marker provided");
        throw vl::format_error("No position marker provided");
    }

    try
    {
        return index == std::stoi(std::string(substr, pos - substr));
    }
    catch (const std::logic_error& ex)
    {
        assert(false && "Error in position marker");
        throw vl::format_error(std::string("Error in position marker: ") + ex.what());
    }
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

        explicit Format(vl::d_::ValueType vtype)
            : fill(' ')
            , align(A_Left)
            , sign(S_Negative)
            , alternate_form(false)
            , width(0)
            , use_thousand_sep(false)
            , precision(-1)
            , type('s')
        {
            switch (vtype)
            {
            case vl::d_::VT_Integral:
                type = 'd';
                align = A_Right;
                break;
            case vl::d_::VT_Floating:
                type = 'g';
                align = A_Right;
                break;
            case vl::d_::VT_Other:
                // already set
                break;
            default:
                assert(false && "Bad value type");
                throw vl::format_error("Bad value type");
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
                assert(false && "Bad align chracter");
                throw vl::format_error(std::string("Bad align chracter: ") + ch);
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
                assert(false && "Bad sign chracter");
                throw vl::format_error(std::string("Bad sign chracter: ") + ch);
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

    Format parse_format(const char* format, int format_size, vl::d_::ValueType type)
    {
        /*
         * format_spec ::=  [[fill]align][sign][#][0][width][,][.precision][type]
         * fill        ::=  <a character other than '{' or '}'>
         * align       ::=  "<" | ">" | "=" | "^"
         * sign        ::=  "+" | "-" | " "
         * width       ::=  integer
         * precision   ::=  integer
         * type        ::=  "b" | "d" | "e" | "E" | "f" | "F" | "g" | "G" | "o" | "s" | "x" | "X" | "%"
         */

        char aligns[] = "><=^";
        char signs[] = "+- ";
        char types[] = "sbdoxXeEfFgG%";
        char int_types[] = "bdoxX";
        char float_types[] = "eEfFgG%";
        char other_types[] = "s";
        int index = 0;
        Format f(type);

        if (index == format_size)
            return f;

        // [[fill]align]

        if (index+1 < format_size && char_in_set(format[index+1], aligns))
        {
            f.align = Format::align_from_char(format[index+1]);
            f.fill = format[index];
            index += 2;
        }
        else if (char_in_set(format[index], aligns))
        {
            f.align = Format::align_from_char(format[index]);
            ++index;
        }

        if (index == format_size)
            return f;

        // [sign]

        if (char_in_set(format[index], signs))
        {
            f.sign = Format::sign_from_char(format[index]);
            ++index;
        }

        if (index == format_size)
            return f;

        // [#]

        if (format[index] == '#')
        {
            f.alternate_form = true;
            ++index;
        }

        if (index == format_size)
            return f;

        // [0]
        if (format[index] == '0')
        {
            f.fill = '0';
            f.align = A_SignAware;
            ++index;
        }

        if (index == format_size)
            return f;

        // [width]
        if (isdigit(format[index]))
        {
            int count = 1;
            while (isdigit(format[index + count]))
                ++count;
            f.width = std::stoi(std::string(&format[index], count));
            index += count;
        }

        if (index == format_size)
            return f;

        // [,]
        if (format[index] == ',')
        {
            f.use_thousand_sep = true;
            ++index;
        }

        if (index == format_size)
            return f;

        // [.precision]
        if (format[index] == '.')
        {
            if (!isdigit(format[index+1]))
            {
                assert(false && "Precision not specified after '.'");
                throw vl::format_error("Precision not specified after '.'");
            }

            if (type == vl::d_::VT_Integral)
            {
                assert(false && "Precision is not allowed for integral types");
                throw vl::format_error("Precision is not allowed for integral types");
            }

            ++index;  // skip the dot
            int count = 1;  // we already checked that first character after the dot is digit

            while (isdigit(format[index + count]))
                ++count;

            f.precision = std::stoi(std::string(&format[index], count));
            index += count;
        }

        if (index == format_size)
            return f;

        // [type]
        if (char_in_set(format[index], types))
        {
            f.type = format[index];

            if (type == vl::d_::VT_Other && !char_in_set(f.type, other_types))
            {
                assert(false && "Incorrect format for non-number type");
                throw vl::format_error("Incorrect format for non-number type");
            }

            if (type == vl::d_::VT_Integral && !char_in_set(f.type, int_types))
            {
                assert(false && "Incorrect format for integral type");
                throw vl::format_error("Incorrect format for integral type");
            }

            if (type == vl::d_::VT_Floating && !char_in_set(f.type, float_types))
            {
                assert(false && "Incorrect format for floating type");
                throw vl::format_error("Incorrect format for floating type");
            }

            ++index;
        }

        if (index != format_size)
        {
            assert(false && "Unknown symbols in format specifier");
            throw vl::format_error("Unknown symbols in format specifier");
        }

        return f;
    }
}

void vl::d_::modify_stream(std::ostringstream& oss, const char* format, int format_size, ValueType type)
{
    Format f = parse_format(format, format_size, type);

    switch (f.type)
    {
    // Integer presentation types:
    case 'X':
        oss << std::uppercase;
    case 'x':
        oss << std::hex;
        break;

    case 'o':
        oss << std::oct;
        break;

    case 'd':
        oss << std::dec;
        break;

    case 'b':
        // not implemented
        break;

    // Floating point presentation types:
    case 'E':
        oss << std::uppercase;
    case 'e':
        oss << std::scientific;
        break;

    case 'F':
        oss << std::uppercase;
    case 'f':
        oss << std::fixed;
        break;

    case 'G':
    case 'g':
        // default
        break;

    case '%':
        // not impelemented
        break;

    // String and default presentation types
    case 's':
    default:
        break;
    }

    switch (f.sign)
    {
    case S_Both:
        oss << std::showpos;
        break;

    case S_Space:
        // not implemented
        break;

    case S_Negative:
    default:
        // default
        break;
    }

    if (f.alternate_form)
    {
        if (type == VT_Integral)
            oss << std::showbase;
        else if (type == VT_Floating)
            oss << std::showpoint;
    }

    if (f.width != 0)
    {
        oss << std::setw(f.width);

        // only has meaning if width is specified
        oss << std::setfill(f.fill);

        switch (f.align)
        {
        case A_Left:
            oss << std::left;
            break;

        case A_Right:
            oss << std::right;
            break;

        case A_Center:
            // not implemented
            break;

        case A_SignAware:
            oss << std::internal;
            break;

        default:
            assert(0 && "impossible");
            break;
        }
    }

    if (f.precision != -1)
        oss << std::setprecision(f.precision);
}

