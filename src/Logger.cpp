#include "VTLogger/Logger.h"

#include <thread>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <fstream>

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


namespace Ut
{
    class LogManager
    {
    private:
        friend Ut::Logger get_logger(const std::string& name);
        friend void set_logger(const std::string& name, const Logger& logger);

        LogManager();
        ~LogManager();

        std::map<std::string, Ut::Logger> loggers_;

        static LogManager self_;
        static bool self_valid_;

        std::mutex lock_;
    };
}


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


Ut::ll::LogLevel Ut::LogLevel_from_str(const std::string& level)
{
    if (level == LL_DEBUG)
        return ll::debug;
    else if (level == LL_INFO)
        return ll::info;
    else if (level == LL_WARNING)
        return ll::warning;
    else if (level == LL_ERROR)
        return ll::error;
    else if (level == LL_CRITICAL)
        return ll::critical;
    else
        return ll::nologging;
}

const char* getLogLevel(Ut::ll::LogLevel l)
{
    switch (l)
    {
    case Ut::ll::debug:
        return LL_DEBUG;
    case Ut::ll::info:
        return LL_INFO;
    case Ut::ll::warning:
        return LL_WARNING;
    case Ut::ll::error:
        return LL_ERROR;
    case Ut::ll::critical:
        return LL_CRITICAL;
    default:
        assert(0);
        return "Unknown";
    }
}


namespace
{
    bool is_set(unsigned int options, Ut::lo::LogOpts opt)
    {
        return (options & opt) == static_cast<unsigned int>(opt);
    }

    void set(unsigned int& options, Ut::lo::LogOpts opt)
    {
        options |= opt;
    }

    void unset(unsigned int& options, Ut::lo::LogOpts opt)
    {
        options &= ~opt;
    }
}



struct Ut::Logger::Impl
{
    Impl(const std::string& name) :
        name         (name),
        streams      (),
        mutex        (new std::mutex),
        cout_level   (ll::nologging),
        cerr_level   (ll::nologging),
        streams_level(ll::nologging),
        options      (lo::usual)
    { }

    // reference counting is atomic, so no need to lock in copy constructors
    typedef std::shared_ptr<std::ostream> ostream_sptr;
    typedef std::shared_ptr<std::mutex> mutex_sptr;

    std::string                    name;
    std::vector<ostream_sptr>      streams;
    mutex_sptr                     mutex;
    ll::LogLevel                   cout_level;
    ll::LogLevel                   cerr_level;
    ll::LogLevel                   streams_level;
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


void Ut::Logger::set_cout(ll::LogLevel reporting_level)
{
    pimpl_->cout_level = reporting_level;
}

void Ut::Logger::set_cerr(ll::LogLevel reporting_level)
{
    pimpl_->cerr_level = reporting_level;
}

bool Ut::Logger::add_stream(std::ostream* stream, ll::LogLevel reporting_level)
{
    assert(stream != nullptr && reporting_level != ll::nologging);
    pimpl_->streams.push_back(std::shared_ptr<std::ostream>(stream));
    pimpl_->streams_level = reporting_level;
    return true;
}

bool Ut::Logger::add_stream(const std::string& filename, ll::LogLevel reporting_level)
{
    assert(!filename.empty() && reporting_level != ll::nologging);

    if (!filename.empty())
    {
        std::ofstream* file = new std::ofstream(filename.c_str(), std::ios_base::app);

        if (file && *file && file->is_open())
        {
            return add_stream(file, reporting_level);
        }
        else
        {
            if (file)
                delete file;

            return false;
        }
    }
    else
    {
        return add_stream(nullptr, reporting_level);
    }
}


void Ut::Logger::clear_streams()
{
    pimpl_->streams.clear();
}


void Ut::Logger::set(lo::LogOpts opt)
{
    ::set(pimpl_->options, opt);
}

void Ut::Logger::unset(lo::LogOpts opt)
{
    ::unset(pimpl_->options, opt);
}

void Ut::Logger::reset()
{
    pimpl_->options = lo::usual;
}


Ut::detail_::LogWorker Ut::Logger::log(ll::LogLevel level)
{
    assert(level != ll::nologging);
    return detail_::LogWorker(this, level);
}
Ut::detail_::LogWorker Ut::Logger::debug()   { return log(ll::debug); }
Ut::detail_::LogWorker Ut::Logger::info()    { return log(ll::info); }
Ut::detail_::LogWorker Ut::Logger::warning() { return log(ll::warning); }
Ut::detail_::LogWorker Ut::Logger::error()   { return log(ll::error); }
Ut::detail_::LogWorker Ut::Logger::critical(){ return log(ll::critical); }


void Ut::Logger::add_prelude(std::string& out, ll::LogLevel level)
{
    if (!is_set(pimpl_->options, lo::notimestamp))
        safe_sprintf(out, "{0} ", createTimestamp());
    if (!is_set(pimpl_->options, lo::nologgername))
        safe_sprintf(out, "[{0}] ", pimpl_->name);
    if (!is_set(pimpl_->options, lo::nothreadid))
        safe_sprintf(out, "0x{0:x} ", std::this_thread::get_id());
    if (!is_set(pimpl_->options, lo::nologlevel))
        safe_sprintf(out, "<{0}> ", getLogLevel(level));
}


void Ut::Logger::add_epilog(std::string& out, ll::LogLevel /*level*/)
{
    if (!is_set(pimpl_->options, lo::noendl))
        out.push_back('\n');
}


void Ut::Logger::write_to_streams(ll::LogLevel level, const std::string& msg)
{
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
    if (level >= pimpl_->streams_level && !pimpl_->streams.empty())
    {
        std::lock_guard<std::mutex> l(*pimpl_->mutex);

        for (Impl::ostream_sptr& stream : pimpl_->streams)
        {
            *stream << msg;
            stream->flush();
        }
    }
}


void Ut::quote(detail_::LogWorker& log_worker)
{
    log_worker.quote_ = true;
}


Ut::Logger Ut::get_logger(const std::string& name)
{
    if (!LogManager::self_valid_)
    {
        throw std::runtime_error("Trying to get logger after LogManager destruction");
    }

    std::lock_guard<std::mutex> lock(LogManager::self_.lock_);

    auto it = LogManager::self_.loggers_.find(name);
    if (it != LogManager::self_.loggers_.end())
        return it->second;

    auto pair = LogManager::self_.loggers_.insert(std::make_pair(name, Logger::cout(name)));
    return pair.first->second;
}


void Ut::set_logger(const std::string& name, const Logger& logger)
{
    if (!LogManager::self_valid_)
    {
        throw std::runtime_error("Trying to get logger after LogManager destruction");
    }

    std::lock_guard<std::mutex> lock(LogManager::self_.lock_);

    auto it = LogManager::self_.loggers_.find(name);
    if (it != LogManager::self_.loggers_.end())
    {
        it->second = logger;
    }
    else
    {
        LogManager::self_.loggers_.insert(std::make_pair(name, logger));
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


Ut::detail_::LogWorker::LogWorker(Logger* logger, ll::LogLevel level)
    : logger_(logger)
    , msg_level_(level)
    , msg_stream_()
    , options_(logger_->pimpl_->options)
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
    , options_(other.options_)
    , quote_(other.quote_)
{
    // FIXME: use msg_stream_ move constructor to move it
    // as soon as gcc supports it
    msg_stream_ << other.msg_stream_.rdbuf();
    other.logger_ = nullptr;
}


void Ut::detail_::LogWorker::optionally_add_space()
{
    if (!(options_ & lo::nospace))
        msg_stream_ << " ";
}


#ifdef _MSC_VER
    #pragma warning(pop)
#endif
