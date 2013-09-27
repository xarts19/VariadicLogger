#include "Logger.h"

#include "StringUtil.h"

#include <thread>
#include <stdexcept>
#include <iostream>
#include <iomanip>

#include <time.h>
#include <assert.h>

#define TIMESTAMP_FORMAT "%Y-%m-%d %H:%M:%S"

#define LL_DEBUG    "Debug"
#define LL_INFO     "Info"
#define LL_WARNING  "Warning"
#define LL_ERROR    "Error"
#define LL_CRITICAL "Critical"

// shut up fopen and localtime security warnings in Visual Studio
#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 4996)
#endif


Ut::LogManager Ut::LogManager::self_;
bool Ut::LogManager::self_valid_ = false;


std::string createTimestamp()
{
    time_t     now      = time(0);
    struct tm  timeinfo = *localtime(&now);
    char       buf[80];
    strftime(buf, sizeof(buf), TIMESTAMP_FORMAT, &timeinfo);
    return buf;
}


Ut::LogLevel Ut::LogLevel_from_str(const std::string& level)
{
    if (level == LL_DEBUG)
        return LL_Debug;
    else if (level == LL_INFO)
        return LL_Info;
    else if (level == LL_WARNING)
        return LL_Warning;
    else if (level == LL_ERROR)
        return LL_Error;
    else if (level == LL_CRITICAL)
        return LL_Critical;
    else
        return LL_NoLogging;
}

const char* getLogLevel(Ut::LogLevel l)
{
    switch (l)
    {
    case Ut::LL_Debug:
        return LL_DEBUG;
    case Ut::LL_Info:
        return LL_INFO;
    case Ut::LL_Warning:
        return LL_WARNING;
    case Ut::LL_Error:
        return LL_ERROR;
    case Ut::LL_Critical:
        return LL_CRITICAL;
    default:
        assert(0);
        return "Unknown";
    }
}


namespace
{
    bool is_set(unsigned int options, Ut::LogOpts opt)
    {
        return (options & opt) == static_cast<unsigned int>(opt);
    }

    void set(unsigned int& options, Ut::LogOpts opt)
    {
        options |= opt;
    }

    void unset(unsigned int& options, Ut::LogOpts opt)
    {
        options &= ~opt;
    }
}



struct Ut::Logger::Impl
{
    Impl(const std::string& name) :
        name        (name),
        stream      (nullptr),
        cout_level  (LL_NoLogging),
        cerr_level  (LL_NoLogging),
        stream_level(LL_NoLogging),
        options     (LO_Default)
    { }

    std::string                    name;
    std::shared_ptr<std::FILE>     stream;
    LogLevel                       cout_level;
    LogLevel                       cerr_level;
    LogLevel                       stream_level;
    unsigned int                   options;  // LogOpts flags
};


Ut::Logger::Logger(const std::string& name)
    : pimpl_(new Impl(name))
{
}

Ut::Logger::~Logger()
{
    delete pimpl_;
}

Ut::Logger::Logger(const Logger& other)
    : pimpl_(new Impl(*other.pimpl_))
{
}

Ut::Logger& Ut::Logger::operator=(Logger other)
{
    swap(other);
    return *this;
}

void Ut::Logger::swap(Logger& other)
{
    std::swap(pimpl_, other.pimpl_);
}


void Ut::Logger::set_cout(LogLevel reporting_level)
{
    pimpl_->cout_level = reporting_level;
}

void Ut::Logger::set_cerr(LogLevel reporting_level)
{
    pimpl_->cerr_level = reporting_level;
}

bool Ut::Logger::set_stream(std::FILE* stream, LogLevel reporting_level)
{
    assert((stream != nullptr && reporting_level != LL_NoLogging) ||
           (stream == nullptr && reporting_level == LL_NoLogging));

    if (stream)
    {
        pimpl_->stream = std::shared_ptr<std::FILE>(stream, std::fclose);
        pimpl_->stream_level = reporting_level;
    }
    else
    {
        pimpl_->stream = nullptr;
        pimpl_->stream_level = LL_NoLogging;
    }

    return true;
}

bool Ut::Logger::set_stream(const std::string& filename, LogLevel reporting_level)
{
    assert((!filename.empty() && reporting_level != LL_NoLogging) ||
           ( filename.empty() && reporting_level == LL_NoLogging));

    if (!filename.empty())
    {
        FILE* file = std::fopen(filename.c_str(), "a+");
        if (file)
            return set_stream(file, reporting_level);
        else
            return false;
    }
    else
    {
        return set_stream(nullptr, reporting_level);
    }
}


void Ut::Logger::set(LogOpts opt)
{
    ::set(pimpl_->options, opt);
}

void Ut::Logger::unset(LogOpts opt)
{
    ::unset(pimpl_->options, opt);
}

void Ut::Logger::reset()
{
    pimpl_->options = LO_Default;
}


Ut::detail_::LogWorker Ut::Logger::log(LogLevel level)
{
    return detail_::LogWorker(this, level);
}
Ut::detail_::LogWorker Ut::Logger::debug()   { return log(LL_Debug); }
Ut::detail_::LogWorker Ut::Logger::info()    { return log(LL_Info); }
Ut::detail_::LogWorker Ut::Logger::warning() { return log(LL_Warning); }
Ut::detail_::LogWorker Ut::Logger::error()   { return log(LL_Error); }
Ut::detail_::LogWorker Ut::Logger::critical(){ return log(LL_Critical); }


void Ut::Logger::add_prelude(std::string& out, LogLevel level)
{
    if (!is_set(pimpl_->options, LO_NoTimestamp))
        safe_sprintf(out, "{0} ", createTimestamp());
    if (!is_set(pimpl_->options, LO_NoLoggerName))
        safe_sprintf(out, "[{0}] ", pimpl_->name);
    if (!is_set(pimpl_->options, LO_NoThreadId))
        safe_sprintf(out, "0x{0:X} ", std::this_thread::get_id());
    if (!is_set(pimpl_->options, LO_NoLogLevel))
        safe_sprintf(out, "<{0}> ", getLogLevel(level));
}


void Ut::Logger::add_epilog(std::string& out, LogLevel /*level*/)
{
    if (!is_set(pimpl_->options, LO_NoEndl))
        out.push_back('\n');
}


void Ut::Logger::write_to_streams(LogLevel level, const std::string& msg)
{
    // fprintf is thread-safe on line level

    if (level >= pimpl_->cout_level)
    {
        fprintf(stdout, "%s", msg.c_str());
        fflush(stdout);
    }
    if (level >= pimpl_->cerr_level)
    {
        fprintf(stderr, "%s", msg.c_str());
        fflush(stderr);
    }
    if (level >= pimpl_->stream_level)
    {
        fprintf(pimpl_->stream.get(), "%s", msg.c_str());
        fflush(pimpl_->stream.get());
    }
}


void Ut::quote(detail_::LogWorker& log_worker)
{
    log_worker.quote_ = true;
}


Ut::Logger& Ut::get_logger(const std::string& name, const std::string& copy_from)
{
    if (!LogManager::self_valid_)
    {
        throw std::runtime_error("Trying to get logger after LogManager destruction");
    }

    auto it = LogManager::self_.loggers_.find(name);
    if (it != LogManager::self_.loggers_.end())
        return it->second;

    if (!copy_from.empty())
    {
        auto it = LogManager::self_.loggers_.find(copy_from);
        if (it != LogManager::self_.loggers_.end())
        {
            auto pair = LogManager::self_.loggers_.insert(std::make_pair(name, it->second));
            Ut::Logger& logger = pair.first->second;
            logger.pimpl_->name = name;
            return pair.first->second;
        }
        else
        {
            auto pair = LogManager::self_.loggers_.insert(std::make_pair(name, Logger::cout(name)));
            Ut::Logger& logger = pair.first->second;
            logger.warning() << "No logger with name" << copy_from << "to copy config from";
            return logger;
        }
    }
    else
    {
        auto pair = LogManager::self_.loggers_.insert(std::make_pair(name, Logger::cout(name)));
        return pair.first->second;
    }
}


Ut::LogManager::LogManager()
{
    self_valid_ = true;
}


Ut::LogManager::~LogManager()
{
    self_valid_ = false;
}



Ut::detail_::LogWorker::LogWorker(Logger* logger, LogLevel level)
    : logger_(logger)
    , msg_level_(level)
    , msg_stream_()
    , quote_(false)
{
    std::string out;
    logger_->add_prelude(out, level);
    msg_stream_ << out;
}

Ut::detail_::LogWorker::~LogWorker()
{
    assert(logger_);
    std::string out(msg_stream_.str());
    logger_->add_epilog(out, msg_level_);
    logger_->write_to_streams(msg_level_, out);
}

Ut::detail_::LogWorker::LogWorker(LogWorker&& other)
    : logger_(other.logger_)
    , msg_level_(other.msg_level_)
    , msg_stream_()
    , options_(other.msg_level_)
    , quote_(other.quote_)
{
    // FIXME: use msg_stream_ move constructor to move it
    // as soon as gcc supports it
    msg_stream_ << other.msg_stream_.rdbuf();
    other.logger_ = nullptr;
}

#ifdef _MSC_VER
    #pragma warning(pop)
#endif
