/*
 *  Copyright (c) 2013, Turinskyi Vitalii
 *  All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
//#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include "VariadicLogger/Logger.h"

#include <thread>
#include <stdio.h>


void concur_test_fnc(vl::ImLogger logger)
{
    for (int i = 0; i < 40; ++i)
    {
        std::this_thread::sleep_for(std::chrono::microseconds(1));
        logger.log(vl::debug, "{0} {1}{2}", "Hello", "world", "!");
        logger.debug() << "Another" << "hello" << "!!!";
    }
}


TEST_CASE( "Concurrent logging")
{
    std::stringstream* out = new std::stringstream;
    vl::ImLogger logger("concurrent");
    logger.add_stream(out);
    logger.set(vl::notimestamp);
    logger.set(vl::nothreadid);
    logger.set(vl::nologgername);
    logger.set(vl::nologlevel);

    std::thread t1(concur_test_fnc, logger);
    std::thread t2(concur_test_fnc, logger);

    t1.join();
    t2.join();

    // check tha all lines are continuous
    std::string result = out->str();
    size_t index = 0;
    size_t pos = 0;
    while ( (pos = result.find('\n', index)) != result.npos )
    {
        if (result[index] == 'H')
            REQUIRE(result.substr(index, pos - index) == "Hello world!");
        else if (result[index] == 'A')
            REQUIRE(result.substr(index, pos - index) == "Another hello !!! ");
        else
        {
            REQUIRE(false);
            break;
        }

        index = pos + 1;
        if (index >= result.size())
            break;
    }
}


TEST_CASE( "Stream logging" )
{
    vl::ImLogger l("default");

    l.set(vl::notimestamp);
    l.set(vl::nothreadid);

    std::stringstream* output = new std::stringstream;
    l.add_stream(output, vl::debug);

    SECTION ( "logger name" )
    {
        l.debug() << "Hello";
        CHECK(output->str() == "[default] <Debug> Hello \n");
    }

    SECTION ( "log levels" )
    {
        SECTION ( "debug" )
        {
            l.debug() << "Hello";
            CHECK(output->str() == "[default] <Debug> Hello \n");
        }

        SECTION ( "info" )
        {
            l.info() << "Hello";
            CHECK(output->str() == "[default] <Info> Hello \n");
        }

        SECTION ( "warning" )
        {
            l.warning() << "Hello";
            CHECK(output->str() == "[default] <Warning> Hello \n");
        }

        SECTION ( "error" )
        {
            l.error() << "Hello";
            CHECK(output->str() == "[default] <Error> Hello \n");
        }

        SECTION ( "critical" )
        {
            l.critical() << "Hello";
            CHECK(output->str() == "[default] <Critical> Hello \n");
        }
    }

    l.set(vl::nologgername);
    l.set(vl::nologlevel);

    SECTION ( "options" )
    {
        l.set(vl::noendl);
        l.set(vl::nospace);

        l.debug() << "NoSpace" << "and" << "NoEndl";
        l.debug() << "is" << "set";

        CHECK(output->str() == "NoSpaceandNoEndlisset");
    }

    SECTION ( "options reset" )
    {
        l.set(vl::nologgername);
        l.set(vl::nologlevel);
        l.set(vl::noendl);
        l.set(vl::nospace);
        l.reset();

        l.set(vl::notimestamp);
        l.set(vl::nothreadid);

        l.debug() << "NoSpace and NoEndl" << "options" << "were";
        l.debug() << "set" << "to" << "default";

        CHECK(output->str() == "[default] <Debug> NoSpace and NoEndl options were \n[default] <Debug> set to default \n");
    }

    SECTION ( "output" )
    {
        l.debug() << 5 << "times" << 3 << "equals" << 15 << "which is" << std::hex << 0xf << "in hex";
        CHECK(output->str() == "5 times 3 equals 15 which is f in hex \n");
    }

    SECTION ( "variadic output 0" )
    {
        l.log(vl::debug, "Hello world!");
        CHECK(output->str() == "Hello world!\n");
    }

    SECTION ( "variadic output 1" )
    {
        l.log(vl::debug, "Hello {0}", "world!");
        CHECK(output->str() == "Hello world!\n");
    }

    SECTION ( "variadic output 2" )
    {
        l.log(vl::debug, "{0} {1}", "Hello", "world!");
        CHECK(output->str() == "Hello world!\n");
    }

    SECTION ( "variadic output 3" )
    {
        l.log(vl::debug, "{0} {1}{2}", "Hello", "world", "!");
        CHECK(output->str() == "Hello world!\n");
    }

    SECTION ( "variadic output reversed" )
    {
        l.log(vl::debug, "{2} {1}{0}",  "!", "world", "Hello");
        CHECK(output->str() == "Hello world!\n");
    }

    SECTION ( "variadic output repeats" )
    {
        l.log(vl::debug, "{0} {1}! {0}! {0}!",  "No", "way");
        CHECK(output->str() == "No way! No! No!\n");
    }
}


TEST_CASE ( "File logging" )
{
    // clear the file
    const char* log_filename = "variadiclogger_test_log.log";
    {
        std::ofstream f(log_filename);
        f.close();
    }

    auto fl = vl::ImLogger::stream("file", log_filename);

    fl.set(vl::notimestamp);
    fl.set(vl::nothreadid);

    fl.debug() << "Hello!";
    fl.warning() << "World!";

    std::ifstream f(log_filename);
    std::string res( (std::istreambuf_iterator<char>(f)),
                      std::istreambuf_iterator<char>()   );

    CHECK(res == "[file] <Debug> Hello! \n[file] <Warning> World! \n");

    remove(log_filename);
}


TEST_CASE( "deffered logging" )
{
    for (int j = 0; j < 10; ++j)  // repeat 10 times because of undeterministic behaviour
    {
        auto lm = std::unique_ptr<vl::LogManager>(new vl::LogManager);

        vl::Logger l("default");
        l.set(vl::notimestamp);
        l.set(vl::nothreadid);
        l.set(vl::nologgername);
        l.set(vl::nologlevel);
        l.set(vl::noendl);
        l.set(vl::nospace);

        std::stringstream* output = new std::stringstream;
        l.add_stream(output, vl::debug);

        for (int i = 0; i < 10000; ++i)
        {
            l.debug() << "0";
        }

        lm = nullptr;

        REQUIRE(output->str().size() == 10000);
    }
}


TEST_CASE( "safe_sprintf hex formatting")
{
    std::string out;

    vl::safe_sprintf(out, "{0:x}", 42);
    CHECK( out == "2a" );

    out.clear();
    vl::safe_sprintf(out, "{0:X}", 42);
    CHECK( out == "2A" );

    out.clear();
    vl::safe_sprintf(out, "{0:#X}", 42);
    CHECK( out == "0X2A" );
}


TEST_CASE( "safe_sprintf dec formatting")
{
    std::string out;

    vl::safe_sprintf(out, "{0:d}", 42);
    CHECK( out == "42" );
}


TEST_CASE( "safe_sprintf oct formatting")
{
    std::string out;

    vl::safe_sprintf(out, "{0:o}", 42);
    CHECK( out == "52" );

    out.clear();
    vl::safe_sprintf(out, "{0:#o}", 42);
    CHECK( out == "052" );
}


TEST_CASE( "safe_sprintf fill + align")
{
    std::string out;

    out.clear();
    vl::safe_sprintf(out, "{0:5}", 42);
    CHECK( out == "   42" );

    out.clear();
    vl::safe_sprintf(out, "{0:#<5}", 42);
    CHECK( out == "42###" );

    out.clear();
    vl::safe_sprintf(out, "{0:=>5}", 42);
    CHECK( out == "===42" );

    out.clear();
    vl::safe_sprintf(out, "{0:*=5}", -42);
    CHECK( out == "-**42" );

    out.clear();
    vl::safe_sprintf(out, "{0:<5}", 42);
    CHECK( out == "42   " );

    out.clear();
    vl::safe_sprintf(out, "{0:5}", "ab");
    CHECK( out == "ab   " );

    out.clear();
    vl::safe_sprintf(out, "{0:>5}", "ab");
    CHECK( out == "   ab" );

    out.clear();
    vl::safe_sprintf(out, "{0:<5}", "ab");
    CHECK( out == "ab   " );
}


TEST_CASE( "safe_sprintf sign")
{
    std::string out;

    out.clear();
    vl::safe_sprintf(out, "{0} {1}", 42, -42);
    CHECK( out == "42 -42" );

    out.clear();
    vl::safe_sprintf(out, "{0:+} {1:+}", 42, -42);
    CHECK( out == "+42 -42" );

    out.clear();
    vl::safe_sprintf(out, "{0:-} {1:-}", 42, -42);
    CHECK( out == "42 -42" );
}


TEST_CASE( "safe_sprintf float formatting and precision")
{
    std::string out;

    // general

    out.clear();
    vl::safe_sprintf(out, "{0}", 42.0);
    CHECK( out == "42" );

    out.clear();
    vl::safe_sprintf(out, "{0:g}", 42.125);
    CHECK( out == "42.125" );

    out.clear();
    vl::safe_sprintf(out, "{0:g}", 42.123456789);
    CHECK( out == "42.1235" );

    out.clear();
    vl::safe_sprintf(out, "{0:g}", 6.1234567e17);
    CHECK( out == "6.12346e+17" );

    // fixed

    out.clear();
    vl::safe_sprintf(out, "{0:f}", 42.0);
    CHECK( out == "42.000000" );

    out.clear();
    vl::safe_sprintf(out, "{0:f}", 42.125);
    CHECK( out == "42.125000" );

    out.clear();
    vl::safe_sprintf(out, "{0:f}", 42.123456789);
    CHECK( out == "42.123457" );

    out.clear();
    vl::safe_sprintf(out, "{0:f}", 6.1234567e17);
    CHECK( out == "612345670000000000.000000" );

    // scientific

    out.clear();
    vl::safe_sprintf(out, "{0:E}", 42.0);
    CHECK( out == "4.200000E+01" );

    out.clear();
    vl::safe_sprintf(out, "{0:e}", 42.125);
    CHECK( out == "4.212500e+01" );

    out.clear();
    vl::safe_sprintf(out, "{0:e}", 42.123456789);
    CHECK( out == "4.212346e+01" );

    out.clear();
    vl::safe_sprintf(out, "{0:e}", 6.1234567e17);
    CHECK( out == "6.123457e+17" );
}


int main(int argc, char* argv[])
{
    return Catch::Session().run(argc, argv);
}
