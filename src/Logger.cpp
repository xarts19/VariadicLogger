#include "VariadicLogger/Logger.h"

#include "VariadicLogger/Event.h"

#include <thread>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <map>
#include <atomic>
#include <queue>
#include <list>

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


vl::LogManager* vl::LogManager::self_ = nullptr;


namespace vl
{
    typedef std::shared_ptr<std::ostream> ostream_sptr;


    namespace d_
    {
        struct Work
        {
            Work(std::string&& m, bool cout, bool cerr, const std::vector<ostream_sptr>& ss)
                : use_cout(cout)
                , use_cerr(cerr)
                , streams(ss)
                , msg(std::move(m))
            { }

            bool use_cout;
            bool use_cerr;
            std::vector<ostream_sptr> streams;
            std::string msg;
        };
    }


    struct Logger::Impl
    {
        Impl(const std::string& name) :
            name         (name),
            streams      (),
            mutex        (new std::mutex),
            cout_level   (nologging),
            cerr_level   (nologging),
            streams_level(nologging),
            options      (usual)
        { }

        std::string                    name;
        std::vector<ostream_sptr>      streams;
        std::shared_ptr<std::mutex>    mutex;
        LogLevel                       cout_level;
        LogLevel                       cerr_level;
        LogLevel                       streams_level;
        unsigned int                   options;  // LogOpts flags
    };


    struct LogManager::Impl
    {
        typedef std::queue<d_::Work, std::list<d_::Work>> queue_type;

        std::mutex lock_;
        std::map<std::string, vl::Logger> loggers_;
        std::atomic<bool> is_running_;
        std::thread writer_thread_;
        queue_type msg_queue_;
        vl::Event new_msgs_event_;
    };
}


vl::LogManager::LogManager()
{
    if (self_ != nullptr)
        throw std::runtime_error("LogManager already created");

    self_ = this;
    d->is_running_.store(true);
    d->writer_thread_ = std::thread(&vl::LogManager::writer_loop, this);
}


vl::LogManager::~LogManager()
{
    d->is_running_.store(false);

    if (d->writer_thread_.joinable())
        d->writer_thread_.join();

    self_ = nullptr;
}


void vl::d_::queue_work(d_::Work&& work)
{
    if (!LogManager::self_)
    {
        throw std::runtime_error("Trying to log messages without valid LogManager");
    }

    std::lock_guard<std::mutex> lock(LogManager::self_->d->lock_);
    LogManager::self_->d->msg_queue_.push(std::move(work));
    LogManager::self_->d->new_msgs_event_.signal();
}


void vl::LogManager::writer_loop()
{
    Impl::queue_type tmp_queue_;

    while (d->is_running_.load())
    {
        d->new_msgs_event_.wait();

        {
            std::lock_guard<std::mutex> lock(d->lock_);
            tmp_queue_.swap(d->msg_queue_);
            d->new_msgs_event_.reset();
        }

        while (!tmp_queue_.empty())
        {
            d_::Work& work = tmp_queue_.front();

            if (work.use_cout)
            {
                std::cout << work.msg;
                std::cout.flush();
            }
            if (work.use_cerr)
            {
                std::cerr << work.msg;
                std::cerr.flush();
            }
            for (ostream_sptr& stream : work.streams)
            {
                *stream << work.msg;
                stream->flush();
            }

            tmp_queue_.pop();
        }
    }
}


vl::Logger vl::get_logger(const std::string& name)
{
    if (!LogManager::self_)
    {
        throw std::runtime_error("Trying to get logger without valid LogManager");
    }

    std::lock_guard<std::mutex> lock(LogManager::self_->d->lock_);

    auto it = LogManager::self_->d->loggers_.find(name);
    if (it != LogManager::self_->d->loggers_.end())
        return it->second;

    auto pair = LogManager::self_->d->loggers_.insert(std::make_pair(name, Logger::cout(name)));
    return pair.first->second;
}


void vl::set_logger(const Logger& logger)
{
    if (!LogManager::self_)
    {
        throw std::runtime_error("Trying to set logger without valid LogManager");
    }

    std::lock_guard<std::mutex> lock(LogManager::self_->d->lock_);

    auto it = LogManager::self_->d->loggers_.find(logger.name());
    if (it != LogManager::self_->d->loggers_.end())
    {
        it->second = logger;
    }
    else
    {
        LogManager::self_->d->loggers_.insert(std::make_pair(logger.name(), logger));
    }
}


std::string createTimestamp()
{
    char buf[80];

    auto now = std::chrono::system_clock::now();
    time_t datetime = std::chrono::system_clock::to_time_t(now);
    struct tm timeinfo = *localtime(&datetime);
    strftime(buf, sizeof(buf), TIMESTAMP_FORMAT, &timeinfo);

    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;
    std::stringstream ss;
    ss << "[" << std::setfill('0') << std::setw(3) << millis << "]";
    strcat(buf, ss.str().c_str());

    return buf;
}


vl::LogLevel vl::LogLevel_from_str(const std::string& level)
{
    if (level == LL_DEBUG)
        return debug;
    else if (level == LL_INFO)
        return info;
    else if (level == LL_WARNING)
        return warning;
    else if (level == LL_ERROR)
        return error;
    else if (level == LL_CRITICAL)
        return critical;
    else
        return nologging;
}

const char* getLogLevel(vl::LogLevel l)
{
    switch (l)
    {
    case vl::debug:
        return LL_DEBUG;
    case vl::info:
        return LL_INFO;
    case vl::warning:
        return LL_WARNING;
    case vl::error:
        return LL_ERROR;
    case vl::critical:
        return LL_CRITICAL;
    default:
        assert(0);
        return "Unknown";
    }
}


namespace
{
    bool is_set(unsigned int options, vl::LogOpts opt)
    {
        return (options & opt) == static_cast<unsigned int>(opt);
    }

    void set(unsigned int& options, vl::LogOpts opt)
    {
        options |= opt;
    }

    void unset(unsigned int& options, vl::LogOpts opt)
    {
        options &= ~opt;
    }
}


vl::Logger::Logger(const std::string& name)
    : pimpl_(new Impl(name))
{
}

vl::Logger::~Logger()
{
    delete pimpl_;
}

vl::Logger::Logger(const Logger& other)
    : pimpl_(new Impl(*other.pimpl_))
{
}

vl::Logger& vl::Logger::operator=(Logger other)
{
    swap(other);
    return *this;
}

void vl::Logger::swap(Logger& other)
{
    std::swap(pimpl_, other.pimpl_);
}


std::string vl::Logger::name() const
{
    return pimpl_->name;
}


void vl::Logger::set_cout(LogLevel reporting_level)
{
    pimpl_->cout_level = reporting_level;
}

void vl::Logger::set_cerr(LogLevel reporting_level)
{
    pimpl_->cerr_level = reporting_level;
}

bool vl::Logger::add_stream(std::ostream* stream, LogLevel reporting_level)
{
    assert(stream != nullptr && reporting_level != nologging);
    pimpl_->streams.push_back(std::shared_ptr<std::ostream>(stream));
    pimpl_->streams_level = reporting_level;
    return true;
}

bool vl::Logger::add_stream(const std::string& filename, LogLevel reporting_level)
{
    assert(!filename.empty() && reporting_level != nologging);

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


void vl::Logger::clear_streams()
{
    pimpl_->streams.clear();
}


void vl::Logger::set(LogOpts opt)
{
    ::set(pimpl_->options, opt);
}

void vl::Logger::unset(LogOpts opt)
{
    ::unset(pimpl_->options, opt);
}

void vl::Logger::reset()
{
    pimpl_->options = usual;
}


vl::d_::LogWorker vl::Logger::log(LogLevel level)
{
    assert(level != nologging);
    return d_::LogWorker(this, level);
}
vl::d_::LogWorker vl::Logger::debug()   { return log(vl::debug); }
vl::d_::LogWorker vl::Logger::info()    { return log(vl::info); }
vl::d_::LogWorker vl::Logger::warning() { return log(vl::warning); }
vl::d_::LogWorker vl::Logger::error()   { return log(vl::error); }
vl::d_::LogWorker vl::Logger::critical(){ return log(vl::critical); }


void vl::Logger::add_prelude(std::string& out, LogLevel level)
{
    if (!is_set(pimpl_->options, notimestamp))
        safe_sprintf(out, "{0} ", createTimestamp());
    if (!is_set(pimpl_->options, nologgername))
        safe_sprintf(out, "[{0}] ", pimpl_->name);
    if (!is_set(pimpl_->options, nothreadid))
        safe_sprintf(out, "0x{0:x} ", std::this_thread::get_id());
    if (!is_set(pimpl_->options, nologlevel))
        safe_sprintf(out, "<{0}> ", getLogLevel(level));
}


void vl::Logger::add_epilog(std::string& out, LogLevel /*level*/)
{
    if (!is_set(pimpl_->options, noendl))
        out.push_back('\n');
}


void vl::Logger::write_to_streams(LogLevel level, std::string&& msg)
{
    d_::queue_work(d_::Work(std::move(msg),
                            level >= pimpl_->cout_level,
                            level >= pimpl_->cerr_level,
                            (level >= pimpl_->streams_level && !pimpl_->streams.empty()
                                ? pimpl_->streams
                                : std::vector<ostream_sptr>())));
}


void vl::quote(d_::LogWorker& log_worker)
{
    log_worker.quote_ = true;
}


vl::d_::LogWorker::LogWorker(Logger* logger, LogLevel level)
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


vl::d_::LogWorker::~LogWorker()
{
    assert(logger_);
    std::string msg(msg_stream_.str());
    logger_->add_epilog(msg, msg_level_);
    logger_->write_to_streams(msg_level_, std::move(msg));
}


vl::d_::LogWorker::LogWorker(LogWorker&& other)
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


void vl::d_::LogWorker::optionally_add_space()
{
    if (!(options_ & nospace))
        msg_stream_ << " ";
}


#ifdef _MSC_VER
    #pragma warning(pop)
#endif
