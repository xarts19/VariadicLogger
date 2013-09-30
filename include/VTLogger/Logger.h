#pragma once

#include "VTLogger/SafeSprintf.h"

#include <ostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <stdexcept>
#include <sstream>

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

    enum LogLevel
    {
        LL_Debug       = 0,
        LL_Info        = 1,
        LL_Warning     = 2,
        LL_Error       = 3,
        LL_Critical    = 4,
        LL_NoLogging   = 5
    };

    // return LL_NoLogging on unknown strings
    LogLevel LogLevel_from_str(const std::string& level);

    enum LogOpts
    {
        LO_Default     = 0u,
        LO_NoEndl      = (1u << 0),
        LO_NoLogLevel  = (1u << 1),
        LO_NoTimestamp = (1u << 2),
        LO_NoFlush     = (1u << 3),
        LO_NoLoggerName= (1u << 4),
        LO_NoThreadId  = (1u << 5),
        LO_NoSpace     = (1u << 6)
    };


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

        static Logger cout(const std::string& name, LogLevel reporting_level = LL_Debug)
        {
            Logger l(name);
            l.set_cout(reporting_level);
            return l;
        }

        static Logger cerr(const std::string& name, LogLevel reporting_level = LL_Debug)
        {
            Logger l(name);
            l.set_cerr(reporting_level);
            return l;
        }

        static Logger stream(const std::string& name,
                             const std::string& filename,
                             LogLevel reporting_level = LL_Debug)
        {
            Logger l(name);
            l.set_stream(filename, reporting_level);
            return l;
        }

        static Logger stream(const std::string& name,
                             std::FILE* stream,
                             LogLevel reporting_level = LL_Debug)
        {
            Logger l(name);
            l.set_stream(stream, reporting_level);
            return l;
        }


        // enable certain streams
        // passing LL_NoLogging disables the stream
        // pass nullptr as stream in set_stream when passing LL_NoLogging

        void set_cout(LogLevel reporting_level = LL_Debug);
        void set_cerr(LogLevel reporting_level = LL_Debug);

        // logger assumes control of FILE object and closes it when
        // all copies of this logger are destroyed or have new stream set
        bool set_stream(std::FILE* stream, LogLevel reporting_level = LL_Debug);

        // returns false when failed to open the file
        // uses std::fopen, so use apropriate functions to get error codes
        bool set_stream(const std::string& filename, LogLevel reporting_level = LL_Debug);


        // modify logger options

        void set(LogOpts opt);
        void unset(LogOpts opt);
        void reset();


        // returns temporary object for atomic write

        detail_::LogWorker log(LogLevel level);

        detail_::LogWorker debug();
        detail_::LogWorker info();
        detail_::LogWorker warning();
        detail_::LogWorker error();
        detail_::LogWorker critical();

        // type-safe veriadic logging functions

#ifndef _MSC_VER

        template <typename... Args>
        void log(LogLevel level, const std::string& fmt, Args&&... args)
        {
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
            log(LL_Debug, fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void info(const std::string& fmt, Args&&... args)
        {
            log(LL_Info, fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void warning(const std::string& fmt, Args&&... args)
        {
            log(LL_Warning, fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void error(const std::string& fmt, Args&&... args)
        {
            log(LL_Error, fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void critical(const std::string& fmt, Args&&... args)
        {
            log(LL_Critical, fmt, std::forward<Args>(args)...);
        }

#else  // limit to 3 arguments

        template <typename A0>
        void log(LogLevel level, const std::string& fmt, A0&& arg0)
        {
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
        void log(LogLevel level, const std::string& fmt, A0&& arg0, A1&& arg1)
        {
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
        void log(LogLevel level, const std::string& fmt, A0&& arg0, A1&& arg1, A2&& arg2)
        {
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

        void add_prelude(std::string& out, LogLevel level);
        void add_epilog(std::string& out, LogLevel level);
        void write_to_streams(LogLevel level, const std::string& msg);

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

            friend void Ut::quote(LogWorker& log_worker);
            
            Logger*            logger_;
            LogLevel           msg_level_;
            std::ostringstream msg_stream_;
            unsigned int       options_;
            bool               quote_;
        };
    }
}
