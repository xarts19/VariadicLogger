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


void concurrent_test()
{
    auto logger = Ut::Logger::cout("concurrent");

    std::thread t1(std::bind(concur_test_fnc, logger));
    std::thread t2(std::bind(concur_test_fnc, logger));

    t1.join();
    t2.join();
}


int main(int /*argc*/, char* /*argv*/[])
{
    auto logger = Ut::Logger::cout("default");

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

    concurrent_test();

    // test file logging
    auto file_logger = Ut::Logger::stream("file", "log.log");
    file_logger.debug() << "Test1";
    file_logger.warning() << "Warning!";

    Ut::Logger logger_other = Ut::get_logger("other_default");
    logger_other.log(Ut::LL_Debug, "Message {0}", 1);

    // TODO: test output for all common types

    return 0;
}
