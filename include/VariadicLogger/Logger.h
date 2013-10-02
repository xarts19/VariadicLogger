#pragma once

#include "VariadicLogger/SafeSprintf.h"

#include <ostream>
#include <string>
#include <stdexcept>
#include <sstream>
#include <assert.h>

// TODO:
//   * implement coloring (separate implementations for each platform)
//   * add custom timestamp formatting
/*
    Example:
        First, you need to create LogManager in main() function:
            LogManager log_manager;
        Then you can call get_logger() and set_logger() to retrieve and store loggers in log manager.

        LogManager not only provides a storage for your loggers, but also a separate thread
        for writing log messages to streams so to not block execution on long locked writes to disk.

        You can create a logger like this:
            Logger logger("default");
            logger.set_cout();
            logger.add_stream("logfile.log");

        And use it like this:
            logger.log(debug, "{0} {1}", "Hello", "world!");
            logger.debug() << "Hello" << "world";

        You can change logger options like this:
            logger.set(vl::noendl);
            logger.set(vl::nospace);
            logger.unset(vl::nospace);
            logger.reset();

        You can store it and then retrieve it from anywhere:
            set_logger(logger);
            Logger l = get_logger("default");

          * changes to logger are not shared between other instances of the logger, retrieved with get_logger()


*/

namespace vl
{
    // forward declarations
    namespace d_
    {
        class LogWorker;
        struct Work;
        void queue_work(Work&& work);
    }

    class Logger;
    

    // enumerations

    enum LogLevel
    {
        debug       = 0,
        info        = 1,
        warning     = 2,
        error       = 3,
        critical    = 4,
        nologging   = 5
    };

    // return LL_NoLogging on unknown strings
    LogLevel LogLevel_from_str(const std::string& level);

    enum LogOpts
    {
        usual        = 0u,
        noendl       = (1u << 0),
        nologlevel   = (1u << 1),
        notimestamp  = (1u << 2),
        noflush      = (1u << 3),
        nologgername = (1u << 4),
        nothreadid   = (1u << 5),
        nospace      = (1u << 6)
    };


    vl::Logger get_logger(const std::string& name);
    void set_logger(const Logger& logger);


    class LogManager
    {
    public:
        LogManager();
        ~LogManager();

    private:
        friend vl::Logger get_logger(const std::string& name);
        friend void set_logger(const Logger& logger);
        friend void d_::queue_work(d_::Work&& work);

        void writer_loop();

        static LogManager* self_;

        struct Impl;
        Impl* d;
    };


    // logging functions should be thread-safe
    // but anything that changes the logger is probably not
    // logger is copyable and copy will share file stream of
    // original logger
    class Logger
    {
    public:
        explicit Logger(const std::string& name);
        ~Logger();

        Logger(const Logger& other);
        Logger& operator=(Logger other);

        void swap(Logger& other);

        std::string name() const;

        // convenience methods

        static Logger cout(const std::string& name, LogLevel reporting_level = vl::debug)
        {
            Logger l(name);
            l.set_cout(reporting_level);
            return l;
        }

        static Logger cerr(const std::string& name, LogLevel reporting_level = vl::debug)
        {
            Logger l(name);
            l.set_cerr(reporting_level);
            return l;
        }

        static Logger stream(const std::string& name,
                             const std::string& filename,
                             LogLevel reporting_level = vl::debug)
        {
            Logger l(name);
            l.add_stream(filename, reporting_level);
            return l;
        }

        static Logger stream(const std::string& name,
                             std::ostream* stream,
                             LogLevel reporting_level = vl::debug)
        {
            Logger l(name);
            l.add_stream(stream, reporting_level);
            return l;
        }


        // enable certain streams
        // passing LL_NoLogging disables the stream
        // pass nullptr as stream in set_stream when passing LL_NoLogging

        void set_cout(LogLevel reporting_level = vl::debug);
        void set_cerr(LogLevel reporting_level = vl::debug);

        // logger assumes control of FILE object and closes it when
        // all copies of this logger are destroyed or have new stream set
        bool add_stream(std::ostream* stream, LogLevel reporting_level = vl::debug);

        // returns false when failed to open the file
        // uses std::fopen, so use apropriate functions to get error codes
        bool add_stream(const std::string& filename, LogLevel reporting_level = vl::debug);

        void clear_streams();

        // modify logger options

        void set(LogOpts opt);
        void unset(LogOpts opt);
        void reset();


        // returns temporary object for atomic write

        d_::LogWorker log(LogLevel level);

        d_::LogWorker debug();
        d_::LogWorker info();
        d_::LogWorker warning();
        d_::LogWorker error();
        d_::LogWorker critical();

        // type-safe veriadic logging functions

#ifdef VL_VARIADIC_TEMPLATES_SUPPORTED

        template <typename... Args>
        void log(LogLevel level, const std::string& fmt, Args&&... args)
        {
            assert(level != nologging);
            try
            {
                std::string msg;
                add_prelude(msg, level);
                safe_sprintf(msg, fmt, std::forward<Args>(args)...);
                add_epilog(msg, level);
                write_to_streams(level, std::move(msg));
            }
            catch (const std::exception& ex)
            {
                throw std::runtime_error("Error while formatting '" + fmt + "': " + ex.what());
            }
        }

        template <typename... Args>
        void debug(const std::string& fmt, Args&&... args)
        {
            log(debug, fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void info(const std::string& fmt, Args&&... args)
        {
            log(info, fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void warning(const std::string& fmt, Args&&... args)
        {
            log(warning, fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void error(const std::string& fmt, Args&&... args)
        {
            log(error, fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void critical(const std::string& fmt, Args&&... args)
        {
            log(critical, fmt, std::forward<Args>(args)...);
        }

#else  // limit to 3 arguments

        inline void log(LogLevel level, const std::string& fmt)
        {
            assert(level != nologging);
            try
            {
                std::string msg;
                add_prelude(msg, level);
                safe_sprintf(msg, fmt);
                add_epilog(msg, level);
                write_to_streams(level, std::move(msg));
            }
            catch (const std::exception& ex)
            {
                throw std::runtime_error("Error while formatting '" + fmt + "': " + ex.what());
            }
        }

        template <typename A0>
        void log(LogLevel level, const std::string& fmt, A0&& arg0)
        {
            assert(level != nologging);
            try
            {
                std::string msg;
                add_prelude(msg, level);
                safe_sprintf(msg, fmt, std::forward<A0>(arg0));
                add_epilog(msg, level);
                write_to_streams(level, std::move(msg));
            }
            catch (const std::exception& ex)
            {
                throw std::runtime_error("Error while formatting '" + fmt + "': " + ex.what());
            }
        }

        template <typename A0, typename A1>
        void log(LogLevel level, const std::string& fmt, A0&& arg0, A1&& arg1)
        {
            assert(level != nologging);
            try
            {
                std::string msg;
                add_prelude(msg, level);
                safe_sprintf(msg, fmt, std::forward<A0>(arg0), std::forward<A1>(arg1));
                add_epilog(msg, level);
                write_to_streams(level, std::move(msg));
            }
            catch (const std::exception& ex)
            {
                throw std::runtime_error("Error while formatting '" + fmt + "': " + ex.what());
            }
        }

        template <typename A0, typename A1, typename A2>
        void log(LogLevel level, const std::string& fmt, A0&& arg0, A1&& arg1, A2&& arg2)
        {
            assert(level != nologging);
            try
            {
                std::string msg;
                add_prelude(msg, level);
                safe_sprintf(msg, fmt, std::forward<A0>(arg0), std::forward<A1>(arg1), std::forward<A2>(arg2));
                add_epilog(msg, level);
                write_to_streams(level, std::move(msg));
            }
            catch (const std::exception& ex)
            {
                throw std::runtime_error("Error while formatting '" + fmt + "': " + ex.what());
            }
        }
        
#endif

    private:
        friend class d_::LogWorker;

        // work function

        void add_prelude(std::string& out, LogLevel level);
        void add_epilog(std::string& out, LogLevel level);
        void write_to_streams(LogLevel level, std::string&& msg);

        // private data

        struct Impl;
        Impl* pimpl_;
    };


    // stream manipulator to surround next argument with quotes
    void quote(d_::LogWorker& log_worker);
    inline const char* yes_no(bool flag) { return (flag ? "yes" : "no"); }


    namespace d_
    {
        class LogWorker
        {
        public:
            LogWorker(Logger* logger, LogLevel level);
            ~LogWorker();

            LogWorker(LogWorker&& other);

            // printers

            template <typename T>
            LogWorker& operator<<(T arg)
            {
                if (quote_)
                    msg_stream_ << '"';

                msg_stream_ << arg;

                if (quote_)
                {
                    msg_stream_ << '"';
                    quote_ = false;
                }

                optionally_add_space();

                return *this;
            }

            LogWorker& operator<<(std::ostream& (*manip)(std::ostream&))
            {
                manip(msg_stream_);
                return *this;
            }

            LogWorker& operator<<(std::ios_base& (*manip)(std::ios_base&))
            {
                manip(msg_stream_);
                return *this;
            }

            LogWorker& operator<<(void (*manip)(LogWorker&))
            {
                manip(*this);
                return *this;
            }

        private:
            // deleted
            LogWorker& operator=(const LogWorker&);

            void optionally_add_space();

            friend void vl::quote(LogWorker& log_worker);
            
            Logger*            logger_;
            LogLevel           msg_level_;
            std::ostringstream msg_stream_;
            unsigned int       options_;
            bool               quote_;
        };
    }
}
