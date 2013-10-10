/*
 *  Copyright (c) 2013, Vitalii Turinskyi
 *  All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

// No variadic templates before Visual Studio 2013
#if (!defined(_MSC_VER) || _MSC_VER >= 1800)
    #define VL_VARIADIC_TEMPLATES_SUPPORTED
#endif

#include <string>
#include <sstream>
#include <vector>
#include <type_traits>
#include <stdexcept>
#include <assert.h>

namespace vl
{
    class format_error : public std::runtime_error
    {
    public:
        explicit format_error(const char* msg) : std::runtime_error(msg) {}
        explicit format_error(const std::string& msg) : std::runtime_error(msg) {}
    };

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
                : type_(t)
                , has_string_(true)
                , content_(std::forward<T>(c))
            { }

            Substring(SubstrType t, const char* str, int s)
                : type_(t)
                , has_string_(false)
                , begin_(str)
                , size_(s)
            { }

            Substring(const Substring& other)
                : type_(other.type_)
                , has_string_(other.has_string_)
                , content_(other.content_)
                , begin_(other.begin_)
                , size_(other.size_)
            { }

            Substring(Substring&& other)
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
                swap(fst.type_, snd.type_);
                swap(fst.has_string_, snd.has_string_);
                swap(fst.content_, snd.content_);
                swap(fst.begin_, snd.begin_);
                swap(fst.size_, snd.size_);
            }

            SubstrType type() const;
            bool has_string() const;
            const std::string& content() const;
            const char* str() const;
            int size() const;

        private:
            SubstrType type_;
            bool has_string_;
            std::string content_;
            const char* begin_;
            int size_;
        };

        typedef std::vector<Substring> Split;

        const char* find_in_str(const char* b, int size, char what);
        Split split_format(const std::string& fmt);
        void join(std::string& out, const Split& split);
        bool has_index(const char* substr, int substr_size, int index);
        void modify_stream(std::ostringstream& oss, const char* format, int format_size, ValueType type);

        template <typename A>
        Substring format_argument(const char* format, int format_size, A&& arg)
        {
            const char* pos = find_in_str(format, format_size, ':');
            const char* format_proper = "";
            int format_proper_size = 0;

            if (pos != format + format_size)
            {
                format_proper = pos + 1;
                format_proper_size = format - format_proper + format_size;
            }

            std::ostringstream oss;
            ValueType type = VT_Other;

            if (std::is_integral<A>::value)
                type = VT_Integral;
            else if (std::is_floating_point<A>::value)
                type = VT_Floating;

            modify_stream(oss, format_proper, format_proper_size, type);
            oss << arg;
            return Substring(SubstrText, oss.str());
        }

#ifdef VL_VARIADIC_TEMPLATES_SUPPORTED

        // base case for variadic template handling recursion
        inline void safe_sprintf_worker(int /*index*/, Split& /*fmt*/) {}

        template <typename A, typename... Args>
        void safe_sprintf_worker(int index, Split& fmt, A&& arg, Args&&... args)
        {
            for (Substring& substr : fmt)
            {
                if (substr.type() == SubstrAnchor)
                {
                    assert(!substr.has_string());
                    if (has_index(substr.str(), substr.size(), index))
                        substr = format_argument(substr.str(), substr.size(), std::forward<A>(arg));
                }
            }
            safe_sprintf_worker(index + 1, fmt, std::forward<Args>(args)...);
        }

#else  // limit to 3 arguments

        template <typename A0>
        void safe_sprintf_worker(int index, Split& fmt, A0&& arg0)
        {
            for (Substring& substr : fmt)
            {
                if (substr.type() == SubstrAnchor)
                {
                    assert(!substr.has_string());
                    if (has_index(substr.str(), substr.size(), index))
                        substr = format_argument(substr.str(), substr.size(), std::forward<A0>(arg0));
                }
            }
        }

        template <typename A0, typename A1>
        void safe_sprintf_worker(int index, Split& fmt, A0&& arg0, A1&& arg1)
        {
            for (Substring& substr : fmt)
            {
                if (substr.type() == SubstrAnchor)
                {
                    assert(!substr.has_string());
                    if (has_index(substr.str(), substr.size(), index))
                        substr = format_argument(substr.str(), substr.size(), std::forward<A0>(arg0));
                }
            }
            safe_sprintf_worker(index + 1, fmt, std::forward<A1>(arg1));
        }

        template <typename A0, typename A1, typename A2>
        void safe_sprintf_worker(int index, Split& fmt, A0&& arg0, A1&& arg1, A2&& arg2)
        {
            for (Substring& substr : fmt)
            {
                if (substr.type() == SubstrAnchor)
                {
                    assert(!substr.has_string());
                    if (has_index(substr.str(), substr.size(), index))
                        substr = format_argument(substr.str(), substr.size(), std::forward<A0>(arg0));
                }
            }
            safe_sprintf_worker(index + 1, fmt, std::forward<A1>(arg1), std::forward<A2>(arg2));
        }

#endif
    }

#ifdef VL_VARIADIC_TEMPLATES_SUPPORTED

    /*
     * Appends string formatted according to [fmt] and specified arguments [args] to [out] string.
     */
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

    inline void safe_sprintf(std::string& out, const std::string& fmt)
    {
        out.append(fmt);
    }

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
