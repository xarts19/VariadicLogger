/*
 *  Copyright (c) 2013, Turinskyi Vitalii
 *  All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#include "VariadicLogger/Event.h"

#include <assert.h>

vl::Event::Event(bool autoreset)
    : lock_()
    , cond_()
    , is_signaled_(false)
    , autoreset_(autoreset)
{
}

void vl::Event::signal()
{
    if (is_signaled_)
        return;

    if (!autoreset_)
        is_signaled_ = true;

    cond_.notify_all();
}

void vl::Event::reset()
{
    is_signaled_ = false;
}

bool vl::Event::wait(int timeout_ms)
{
    assert( timeout_ms >= -1 && "Incorrect timeout value" );
    
    std::unique_lock<std::mutex> lk(lock_);

    if (timeout_ms != -1)
    {
        bool res = cond_.wait_for(lk, std::chrono::milliseconds(timeout_ms), [&]{ return is_signaled_ == true; });
        return res;  // res is true if event was signaled before timeout
    }
    else
    {
        cond_.wait(lk, [&]{ return is_signaled_ == true; });
        return true;
    }
}
