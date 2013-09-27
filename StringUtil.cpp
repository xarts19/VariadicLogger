#include "StringUtil.h"

#include <cctype>
#include <algorithm>

std::string Ut::StrUtils::trim(const std::string& str,
                               const std::string& whitespace)
{
    const size_t strBegin = str.find_first_not_of(whitespace);
    if (strBegin == std::string::npos)
        return ""; // no content

    const size_t strEnd = str.find_last_not_of(whitespace);
    const size_t strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}

bool Ut::StrUtils::starts_with(const std::string& str,
                               const std::string& prefix)
{
    if ( str.substr( 0, prefix.size() ) == prefix )
        return true;
    return false;
}

namespace detail_
{
    char char_to_lower(char in)
    {
        return static_cast<char>( std::tolower(in) );
    }
}

std::string Ut::StrUtils::to_lower(std::string str)
{
    std::transform(str.begin(), str.end(), str.begin(), detail_::char_to_lower);
    return str;
}
