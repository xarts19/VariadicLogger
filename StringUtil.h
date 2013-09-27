#pragma once

#include <string>

namespace Ut
{
    namespace StrUtils
    {
        // delimiters is a string of delimiting characters of length 1
        // do not include empty strings between consecutive delimiters
        template <typename Container>
        void split(const std::string& str,
                   Container*         result,
                   const std::string& delimiters = " ")
        {
            size_t current;
            size_t next = static_cast<size_t>(-1);
            do
            {
                next = str.find_first_not_of( delimiters, next + 1 );
                if (next == std::string::npos) break;
                next -= 1;

                current = next + 1;
                next = str.find_first_of( delimiters, current );
                result->push_back( str.substr( current, next - current ) );
            }
            while (next != std::string::npos);
        }

        std::string trim(const std::string& str,
                         const std::string& whitespace = " ");
        
        bool starts_with(const std::string& str,
                         const std::string& prefix);

        std::string to_lower(std::string str);
    }
}


