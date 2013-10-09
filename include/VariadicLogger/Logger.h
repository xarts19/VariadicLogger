/*
 *  Copyright (c) 2013, Vitalii Turinskyi
 *  All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include "VariadicLogger/SafeSprintf.h"

#include <iostream>
#include <ostream>
#include <string>
#include <stdexcept>
#include <sstream>
#include <assert.h>


namespace vl
{
    // forward declarations
    namespace d_
    {
        template <typename T>
        class LogWorker;
        struct Work;
        void queue_work(Work&& work);
    }


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


    class delegate;
    class immediate;


    // logging functions should be thread-safe
    // but anything that changes the logger is probably not
    // logger is copyable and copy will share file stream of
    // original logger
    template <typename T>
    class LoggerT
    {
    public:
        explicit LoggerT(const std::string& name);
        ~LoggerT();

        LoggerT(const LoggerT<T>& other);
        LoggerT& operator=(LoggerT<T> other);

        void swap(LoggerT<T>& other);

        std::string name() const;

        // convenience methods

        static LoggerT<T> cout(const std::string& name, LogLevel reporting_level = vl::debug)
        {
            LoggerT<T> l(name);
            l.set_cout(reporting_level);
            return l;
        }

        static LoggerT<T> cerr(const std::string& name, LogLevel reporting_level = vl::debug)
        {
            LoggerT<T> l(name);
            l.set_cerr(reporting_level);
            return l;
        }

        static LoggerT<T> stream(const std::string& name,
                             const std::string& filename,
                             LogLevel reporting_level = vl::debug)
        {
            LoggerT<T> l(name);
            l.add_stream(filename, reporting_level);
            return l;
        }

        static LoggerT<T> stream(const std::string& name,
                             std::ostream* stream,
                             LogLevel reporting_level = vl::debug)
        {
            LoggerT<T> l(name);
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

        d_::LogWorker<T> log(LogLevel level);

        d_::LogWorker<T> debug();
        d_::LogWorker<T> info();
        d_::LogWorker<T> warning();
        d_::LogWorker<T> error();
        d_::LogWorker<T> critical();

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
                std::cerr << "Error while formatting '" + fmt + "': " + ex.what() + "\n";
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
                std::cerr << "Error while formatting '" + fmt + "': " + ex.what() + "\n";
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
                std::cerr << "Error while formatting '" + fmt + "': " + ex.what() + "\n";
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
                std::cerr << "Error while formatting '" + fmt + "': " + ex.what() + "\n";
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
                std::cerr << "Error while formatting '" + fmt + "': " + ex.what() + "\n";
            }
        }
        
#endif

    private:
        friend class d_::LogWorker<T>;

        // work function

        void add_prelude(std::string& out, LogLevel level);
        void add_epilog(std::string& out, LogLevel level);

        void write_to_streams(LogLevel level, std::string&& msg);

        // private data

        struct Impl;
        Impl* pimpl_;
    };


    typedef LoggerT<delegate> Logger;
    typedef LoggerT<immediate> ImLogger;


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


    namespace d_
    {
        template <typename T>
        class LogWorker
        {
        public:
            LogWorker(LoggerT<T>* logger, LogLevel level);
            ~LogWorker();

            LogWorker(LogWorker&& other);

            // printers

            template <typename A>
            LogWorker& operator<<(A arg)
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
            
            LoggerT<T>*        logger_;
            LogLevel           msg_level_;
            std::ostringstream msg_stream_;
            unsigned int       options_;
            bool               quote_;
        };
    }
}
