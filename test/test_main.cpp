#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "VTLogger/Logger.h"

#include <thread>


void concur_test_fnc(Ut::Logger logger)
{
    for (int i = 0; i < 40; ++i)
    {
        std::this_thread::sleep_for(std::chrono::microseconds(1));
        logger.log(Ut::LL_Debug, "{0} {1} {2}", "This", "is", "spam");
        logger.debug() << "This" << "is" << "more" << "spam";
    }
}


int concurrent_test()
{
    auto logger = Ut::Logger::cout("concurrent");

    std::thread t1(concur_test_fnc, logger);
    std::thread t2(concur_test_fnc, logger);

    t1.join();
    t2.join();

    return 0;
}


int general_test()
{
    auto logger = Ut::Logger::cout("default");

    logger.log(Ut::LL_Debug, "{0:_>+#15,.3F}", 42.3);
    logger.log(Ut::LL_Debug, "{0:*<+#15,X}", 42);

    logger.debug() << "Hello" << 55 << "done";
    logger.debug() << std::hex << 11 << "done";

    logger.debug() << "smth" << "is" << 25 << "times" << std::hex << 12 << "wrong";
    logger.debug() << "";
    logger.log(Ut::LL_Debug) << "DEBUG!!!";
    logger.log(Ut::LL_Info) << "Info!!!";
    logger.log(Ut::LL_Warning) << "WARNING!!!";
    logger.log(Ut::LL_Error) << "ERROR!!!";
    logger.log(Ut::LL_Critical) << "CRITICAL!!!";
    logger.info() << "Info!!!";
    logger.warning() << "WARNING!!!";
    logger.error() << "ERROR!!!";
    logger.critical() << "CRITICAL!!!";

    logger.debug() << "NoSpace" << "is" << "not" << "set";

    logger.set(Ut::LO_NoEndl);
    logger.set(Ut::LO_NoSpace);

    logger.debug() << "NoSpace" << "and" << "NoEndl";
    logger.debug() << "is" << "set";

    logger.reset();
    logger.debug() << "Ok." << "Options" << "are" << "set" << "to" << "default";

    // test file logging
    auto file_logger = Ut::Logger::stream("file", "log.log");
    file_logger.debug() << "Test1";
    file_logger.warning() << "Warning!";

    Ut::Logger logger_other = Ut::get_logger("other_default");
    logger_other.log(Ut::LL_Debug, "Message {0}", 1);

    return 0;
}


TEST_CASE( "Concurrent logging")
{
    REQUIRE( concurrent_test() == 0 );
}


TEST_CASE( "General logging")
{
    REQUIRE( general_test() == 0 );
}


TEST_CASE( "Logging" )
{
    std::stringstream output;

    std::streambuf* coutbuf = std::cout.rdbuf();
    std::cout.rdbuf(output.rdbuf());

    Ut::Logger l = Ut::Logger::cout("default");
    l.set(Ut::LO_NoTimestamp);
    l.set(Ut::LO_NoThreadId);

    SECTION ( "simple" )
    {
        l.debug() << "Hello" << 55 << "done";
        CHECK(output.str() == "[default] <Debug> Hello 55 done");
    }

    std::cout.rdbuf(coutbuf);
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

