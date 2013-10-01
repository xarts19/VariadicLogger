#pragma once

#include "VTLogger/SafeSprintf.h"

#include <ostream>
#include <map>
#include <string>
#include <stdexcept>
#include <sstream>
#include <assert.h>

// Hint: single logger is thread safe with respect to simultanious writing messages to the stream,
// but not thread safe when mutating logger parameters

// TODO:
//   * implement coloring (separate implementations for each platform)
//   * add custom timestamp formatting
/*

*/

namespace Ut
{
    // forward declarations
    namespace detail_ { class LogWorker; }
    class Logger;
    

    // enumerations

    namespace ll
    {
        enum LogLevel
        {
            debug       = 0,
            info        = 1,
            warning     = 2,
            error       = 3,
            critical    = 4,
            nologging   = 5
        };
    }

    // return LL_NoLogging on unknown strings
    ll::LogLevel LogLevel_from_str(const std::string& level);

    namespace lo
    {
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
    }


    Ut::Logger get_logger(const std::string& name);
    void set_logger(const std::string& name, const Logger& logger);


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

        // convenience methods

        static Logger cout(const std::string& name, ll::LogLevel reporting_level = ll::debug)
        {
            Logger l(name);
            l.set_cout(reporting_level);
            return l;
        }

        static Logger cerr(const std::string& name, ll::LogLevel reporting_level = ll::debug)
        {
            Logger l(name);
            l.set_cerr(reporting_level);
            return l;
        }

        static Logger stream(const std::string& name,
                             const std::string& filename,
                             ll::LogLevel reporting_level = ll::debug)
        {
            Logger l(name);
            l.add_stream(filename, reporting_level);
            return l;
        }

        static Logger stream(const std::string& name,
                             std::ostream* stream,
                             ll::LogLevel reporting_level = ll::debug)
        {
            Logger l(name);
            l.add_stream(stream, reporting_level);
            return l;
        }


        // enable certain streams
        // passing LL_NoLogging disables the stream
        // pass nullptr as stream in set_stream when passing LL_NoLogging

        void set_cout(ll::LogLevel reporting_level = ll::debug);
        void set_cerr(ll::LogLevel reporting_level = ll::debug);

        // logger assumes control of FILE object and closes it when
        // all copies of this logger are destroyed or have new stream set
        bool add_stream(std::ostream* stream, ll::LogLevel reporting_level = ll::debug);

        // returns false when failed to open the file
        // uses std::fopen, so use apropriate functions to get error codes
        bool add_stream(const std::string& filename, ll::LogLevel reporting_level = ll::debug);

        void clear_streams();

        // modify logger options

        void set(lo::LogOpts opt);
        void unset(lo::LogOpts opt);
        void reset();


        // returns temporary object for atomic write

        detail_::LogWorker log(ll::LogLevel level);

        detail_::LogWorker debug();
        detail_::LogWorker info();
        detail_::LogWorker warning();
        detail_::LogWorker error();
        detail_::LogWorker critical();

        // type-safe veriadic logging functions

// No variadic templates in Visual Studio 2012
#ifndef _MSC_VER

        template <typename... Args>
        void log(ll::LogLevel level, const std::string& fmt, Args&&... args)
        {
            assert(level != ll::nologging);
            try
            {
                std::string msg;
                add_prelude(msg, level);
                safe_sprintf(msg, fmt, std::forward<Args>(args)...);
                add_epilog(msg, level);
                write_to_streams(level, msg);
            }
            catch (const std::exception& ex)
            {
                throw std::runtime_error("Error while formatting '" + fmt + "': " + ex.what());
            }
        }

        template <typename... Args>
        void debug(const std::string& fmt, Args&&... args)
        {
            log(ll::debug, fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void info(const std::string& fmt, Args&&... args)
        {
            log(ll::info, fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void warning(const std::string& fmt, Args&&... args)
        {
            log(ll::warning, fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void error(const std::string& fmt, Args&&... args)
        {
            log(ll::error, fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void critical(const std::string& fmt, Args&&... args)
        {
            log(ll::critical, fmt, std::forward<Args>(args)...);
        }

#else  // limit to 3 arguments

        template <typename A0>
        void log(ll::LogLevel level, const std::string& fmt, A0&& arg0)
        {
            assert(level != ll::nologging);
            try
            {
                std::string msg;
                add_prelude(msg, level);
                safe_sprintf(msg, fmt, std::forward<A0>(arg0));
                add_epilog(msg, level);
                write_to_streams(level, msg);
            }
            catch (const std::exception& ex)
            {
                throw std::runtime_error("Error while formatting '" + fmt + "': " + ex.what());
            }
        }

        template <typename A0, typename A1>
        void log(ll::LogLevel level, const std::string& fmt, A0&& arg0, A1&& arg1)
        {
            assert(level != ll::nologging);
            try
            {
                std::string msg;
                add_prelude(msg, level);
                safe_sprintf(msg, fmt, std::forward<A0>(arg0), std::forward<A1>(arg1));
                add_epilog(msg, level);
                write_to_streams(level, msg);
            }
            catch (const std::exception& ex)
            {
                throw std::runtime_error("Error while formatting '" + fmt + "': " + ex.what());
            }
        }

        template <typename A0, typename A1, typename A2>
        void log(ll::LogLevel level, const std::string& fmt, A0&& arg0, A1&& arg1, A2&& arg2)
        {
            assert(level != ll::nologging);
            try
            {
                std::string msg;
                add_prelude(msg, level);
                safe_sprintf(msg, fmt, std::forward<A0>(arg0), std::forward<A1>(arg1), std::forward<A2>(arg2));
                add_epilog(msg, level);
                write_to_streams(level, msg);
            }
            catch (const std::exception& ex)
            {
                throw std::runtime_error("Error while formatting '" + fmt + "': " + ex.what());
            }
        }
        
#endif

    private:
        friend class detail_::LogWorker;

        // work function

        void add_prelude(std::string& out, ll::LogLevel level);
        void add_epilog(std::string& out, ll::LogLevel level);
        void write_to_streams(ll::LogLevel level, const std::string& msg);

        // private data

        struct Impl;
        Impl* pimpl_;
    };


    // stream manipulator to surround next argument with quotes
    void quote(detail_::LogWorker& log_worker);
    inline const char* yes_no(bool flag) { return (flag ? "yes" : "no"); }


    namespace detail_
    {
        class LogWorker
        {
        public:
            LogWorker(Logger* logger, ll::LogLevel level);
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

            friend void Ut::quote(LogWorker& log_worker);
            
            Logger*            logger_;
            ll::LogLevel       msg_level_;
            std::ostringstream msg_stream_;
            unsigned int       options_;
            bool               quote_;
        };
    }
}
