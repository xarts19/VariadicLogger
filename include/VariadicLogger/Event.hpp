/*
 *  Copyright (c) 2013, Vitalii Turinskyi
 *  All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>

namespace vl
{
    class Event
    {
    public:
        Event(bool autoreset = false)
            : lock_()
            , cond_()
            , is_signaled_(false)
            , autoreset_(autoreset)
        {}

        bool is_signaled() { return is_signaled_; }

        void signal()
        {
            if (is_signaled_)
                return;

            std::lock_guard<std::mutex> lk(lock_);

            if (!autoreset_)
                is_signaled_ = true;

            cond_.notify_all();
        }

        void reset()
        {
            is_signaled_ = false;
        }

        // wait until event is signaled
        void wait()
        {
            std::unique_lock<std::mutex> lk(lock_);

            if (is_signaled_)
                return;

            cond_.wait(lk, [&]{ return is_signaled_ == true; });
        }
        
        // return true if event is signaled and false on timeout
        template <typename Rel, typename Period>
        bool wait_for(const std::chrono::duration<Rel, Period>& timeout)
        {
            std::unique_lock<std::mutex> lk(lock_);

            if (is_signaled_)
                return true;

            return cond_.wait_for(lk, timeout, [&]{ return is_signaled_ == true; });
        }

    private:
        Event(const Event&);
        Event& operator=(const Event&);

        std::mutex lock_;
        std::condition_variable cond_;
        std::atomic<bool> is_signaled_;
        bool autoreset_;
    };
}
