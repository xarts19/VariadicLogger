/*
 *  Copyright (c) 2013, Turinskyi Vitalii
 *  All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include <mutex>
#include <condition_variable>
#include <atomic>

namespace vl
{
    class Event
    {
    public:
        Event(bool autoreset = false);

        void signal();
        void reset();
        
        // return true if event is signaled and false on timeout
        // if timeout == -1, waits infinitely
        bool wait(int timeout_ms = -1);

        bool is_signaled() { return is_signaled_; }

    private:
        Event(const Event&);
        Event& operator=(const Event&);

        std::mutex lock_;
        std::condition_variable cond_;
        std::atomic<bool> is_signaled_;
        bool autoreset_;
    };
}
