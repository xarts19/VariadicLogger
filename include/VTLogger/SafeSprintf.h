#pragma once

/*
 * Exports "Ut::safe_sprintf" function.
 *
 * Formatting (moddeled after Python's str.format() function)
 * ==========================================================
 *  * Format string specifiec a template for resulting string, where substring of the form "{X:f}" will be
 *    substituted by provided arguments.
 *  * Doubled occurences of curly braces will be replaced by single occurence, e. g. "{{" will become "{" and
 *    "}}" will become "}" without affecting arguments substitution (use this to insert curly braces).
 *  * X : argument number for substitution starting from 0 for first provided argument
 *  * f : format specifier
 *
 *  Format mini-language
 *  --------------------
 *    format_spec ::=  [[fill]align][sign][#][0][width][,][.precision][type]
 *    fill        ::=  <a character other than '{' or '}'>
 *    align       ::=  "<" | ">" | "=" | "^"
 *    sign        ::=  "+" | "-" | " "
 *    width       ::=  integer
 *    precision   ::=  integer
 *    type        ::=  "b" | "c" | "d" | "e" | "E" | "f" | "F" | "g" | "G" | "n" | "o" | "s" | "x" | "X" | "%"
 *
 *  For more info, see http://docs.python.org/3.3/library/string.html#formatspec
 *
 */

#include <string>
#include <sstream>
#include <vector>
#include <type_traits>

namespace Ut
{
    namespace d_
    {
        enum SubstrType
        {
            SubstrAnchor,
            SubstrText
        };

        enum ValueType
        {
            VT_Integral,
            VT_Floating,
            VT_Other
        };

        struct Substring
        {
            template <typename T>
            Substring(SubstrType t, T&& c)
                : type(t)
                , content(std::forward<T>(c))
            { }

            Substring(const Substring& other)
                : type(other.type)
                , content(other.content)
            { }

            Substring(Substring&& other)
                : content()
            {
                swap(*this, other);
            }

            Substring& operator=(Substring rhs)
            {
                swap(*this, rhs);
                return *this;
            }

            friend void swap(Substring& fst, Substring& snd)
            {
                using std::swap;
                swap(fst.type, snd.type);
                swap(fst.content, snd.content);
            }

            SubstrType type;
            std::string content;
        };

        typedef std::vector<Substring> Split;

        Split split_format(const std::string& fmt);
        void join(std::string& out, const Split& split);
        bool has_index(const std::string& substr, int index);
        void modify_stream(std::ostringstream& oss, const std::string& format, ValueType type);

        template <typename A>
        Substring format_argument(const std::string& substr, A&& arg)
        {
            size_t pos = substr.find_first_of(":");
            std::string format;

            if (pos != substr.npos)
                format = substr.substr(pos + 1);

            std::ostringstream oss;
            ValueType type = VT_Other;

            if (std::is_integral<A>::value)
                type = VT_Integral;
            else if (std::is_floating_point<A>::value)
                type = VT_Floating;

            modify_stream(oss, format, type);
            oss << arg;
            return Substring(SubstrText, oss.str());
        }

// No variadic templates in Visual Studio 2012
#ifndef _MSC_VER

        // base case for variadic template handling recursion
        inline void safe_sprintf_worker(int /*index*/, Split& /*fmt*/) {}

        template <typename A, typename... Args>
        void safe_sprintf_worker(int index, Split& fmt, A&& arg, Args&&... args)
        {
            for (Substring& substr : fmt)
            {
                if (substr.type == SubstrAnchor && has_index(substr.content, index))
                    substr = format_argument(substr.content, std::forward<A>(arg));
            }
            safe_sprintf_worker(index + 1, fmt, std::forward<Args>(args)...);
        }

#else  // limit to 3 arguments

        template <typename A0>
        void safe_sprintf_worker(int index, Split& fmt, A0&& arg0)
        {
            for (Substring& substr : fmt)
            {
                if (substr.type == SubstrAnchor && has_index(substr.content, index))
                    substr = format_argument(substr.content, std::forward<A0>(arg0));
            }
        }

        template <typename A0, typename A1>
        void safe_sprintf_worker(int index, Split& fmt, A0&& arg0, A1&& arg1)
        {
            for (Substring& substr : fmt)
            {
                if (substr.type == SubstrAnchor && has_index(substr.content, index))
                    substr = format_argument(substr.content, std::forward<A0>(arg0));
            }
            safe_sprintf_worker(index + 1, fmt, std::forward<A1>(arg1));
        }

        template <typename A0, typename A1, typename A2>
        void safe_sprintf_worker(int index, Split& fmt, A0&& arg0, A1&& arg1, A2&& arg2)
        {
            for (Substring& substr : fmt)
            {
                if (substr.type == SubstrAnchor && has_index(substr.content, index))
                    substr = format_argument(substr.content, std::forward<A0>(arg0));
            }
            safe_sprintf_worker(index + 1, fmt, std::forward<A1>(arg1), std::forward<A2>(arg2));
        }

#endif
    }

    /*
     * Appends string formatted according to [fmt] and specified arguments [args] to [out] string.
     */
// No variadic templates in Visual Studio 2012
#ifndef _MSC_VER

    template <typename... Args>
    void safe_sprintf(std::string& out, const std::string& fmt, Args&&... args)
    {
        auto split = d_::split_format(fmt);
        d_::safe_sprintf_worker(0, split, std::forward<Args>(args)...);
        d_::join(out, split);
    }

    // Version returning formatted string
    template <typename... Args>
    std::string safe_sprintf_ret(const std::string& fmt, Args&&... args)
    {
        auto split = d_::split_format(fmt);
        d_::safe_sprintf_worker(0, split, std::forward<Args>(args)...);
        std::string out;
        d_::join(out, split);
        return out;
    }

#else  // limit to 3 arguments

    template <typename A0>
    void safe_sprintf(std::string& out, const std::string& fmt, A0&& arg0)
    {
        auto split = d_::split_format(fmt);
        d_::safe_sprintf_worker(0, split, std::forward<A0>(arg0));
        d_::join(out, split);
    }

    template <typename A0, typename A1>
    void safe_sprintf(std::string& out, const std::string& fmt, A0&& arg0, A1&& arg1)
    {
        auto split = d_::split_format(fmt);
        d_::safe_sprintf_worker(0, split, std::forward<A0>(arg0), std::forward<A1>(arg1));
        d_::join(out, split);
    }

    template <typename A0, typename A1, typename A2>
    void safe_sprintf(std::string& out, const std::string& fmt, A0&& arg0, A1&& arg1, A2&& arg2)
    {
        auto split = d_::split_format(fmt);
        d_::safe_sprintf_worker(0, split, std::forward<A0>(arg0), std::forward<A1>(arg1), std::forward<A2>(arg2));
        d_::join(out, split);
    }

#endif
}
