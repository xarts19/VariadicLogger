#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "VTLogger/Logger.h"

#include <thread>
#include <stdio.h>


void concur_test_fnc(Ut::Logger logger)
{
    for (int i = 0; i < 40; ++i)
    {
        std::this_thread::sleep_for(std::chrono::microseconds(1));
        logger.log(Ut::ll::debug, "{0} {1}{2}", "Hello", "world", "!");
        logger.debug() << "Another" << "hello" << "!!!";
    }
}


TEST_CASE( "Concurrent logging")
{
    std::stringstream* out = new std::stringstream;
    Ut::Logger logger("concurrent");
    logger.add_stream(out);
    logger.set(Ut::lo::notimestamp);
    logger.set(Ut::lo::nothreadid);
    logger.set(Ut::lo::nologgername);
    logger.set(Ut::lo::nologlevel);

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
    Ut::Logger l("default");

    l.set(Ut::lo::notimestamp);
    l.set(Ut::lo::nothreadid);

    std::stringstream* output = new std::stringstream;
    l.add_stream(output, Ut::ll::debug);

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

    l.set(Ut::lo::nologgername);
    l.set(Ut::lo::nologlevel);

    SECTION ( "options" )
    {
        l.set(Ut::lo::noendl);
        l.set(Ut::lo::nospace);

        l.debug() << "NoSpace" << "and" << "NoEndl";
        l.debug() << "is" << "set";

        CHECK(output->str() == "NoSpaceandNoEndlisset");
    }

    SECTION ( "options reset" )
    {
        l.set(Ut::lo::nologgername);
        l.set(Ut::lo::nologlevel);
        l.set(Ut::lo::noendl);
        l.set(Ut::lo::nospace);
        l.reset();

        l.set(Ut::lo::notimestamp);
        l.set(Ut::lo::nothreadid);

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
        l.log(Ut::ll::debug, "Hello world!");
        CHECK(output->str() == "Hello world!\n");
    }

    SECTION ( "variadic output 1" )
    {
        l.log(Ut::ll::debug, "Hello {0}", "world!");
        CHECK(output->str() == "Hello world!\n");
    }

    SECTION ( "variadic output 2" )
    {
        l.log(Ut::ll::debug, "{0} {1}", "Hello", "world!");
        CHECK(output->str() == "Hello world!\n");
    }

    SECTION ( "variadic output 3" )
    {
        l.log(Ut::ll::debug, "{0} {1}{2}", "Hello", "world", "!");
        CHECK(output->str() == "Hello world!\n");
    }

    SECTION ( "variadic output reversed" )
    {
        l.log(Ut::ll::debug, "{2} {1}{0}",  "!", "world", "Hello");
        CHECK(output->str() == "Hello world!\n");
    }
}


TEST_CASE ( "File logging" )
{
    // clear the file
    {
        std::ofstream f("vtlogger_test_log.log");
        f.close();
    }

    auto fl = Ut::Logger::stream("file", "vtlogger_test_log.log");

    fl.set(Ut::lo::notimestamp);
    fl.set(Ut::lo::nothreadid);

    fl.debug() << "Hello!";
    fl.warning() << "World!";

    std::ifstream f("vtlogger_test_log.log");
    std::string res( (std::istreambuf_iterator<char>(f)),
                      std::istreambuf_iterator<char>()   );

    CHECK(res == "[file] <Debug> Hello! \n[file] <Warning> World! \n");

    remove("vtlogger_test_log.log");
}


TEST_CASE( "safe_sprintf hex formatting")
{
    std::string out;

    Ut::safe_sprintf(out, "{0:x}", 42);
    CHECK( out == "2a" );

    out.clear();
    Ut::safe_sprintf(out, "{0:X}", 42);
    CHECK( out == "2A" );

    out.clear();
    Ut::safe_sprintf(out, "{0:#X}", 42);
    CHECK( out == "0X2A" );
}


TEST_CASE( "safe_sprintf dec formatting")
{
    std::string out;

    Ut::safe_sprintf(out, "{0:d}", 42);
    CHECK( out == "42" );
}


TEST_CASE( "safe_sprintf oct formatting")
{
    std::string out;

    Ut::safe_sprintf(out, "{0:o}", 42);
    CHECK( out == "52" );

    out.clear();
    Ut::safe_sprintf(out, "{0:#o}", 42);
    CHECK( out == "052" );
}

