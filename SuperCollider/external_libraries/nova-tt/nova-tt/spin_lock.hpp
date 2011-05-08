//  spin_lock class
//  Copyright (C) 2010 Tim Blechmann
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; see the file COPYING.  If not, write to
//  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
//  Boston, MA 02111-1307, USA.

/** \file spin_lock.hpp */

#ifndef NOVA_TT_SPIN_LOCK_HPP
#define NOVA_TT_SPIN_LOCK_HPP

#include <boost/noncopyable.hpp>
#include <boost/atomic.hpp>

namespace nova
{

/** spinlock, implements the Lockable concept
 */
class spin_lock:
    public boost::noncopyable
{
    typedef enum {locked_state,
                  unlocked_state} lock_state;
    boost::atomic<lock_state> state;

public:
    struct scoped_lock
    {
        scoped_lock(spin_lock & sl):
            sl_(sl)
        {
            sl_.lock();
        }

        ~scoped_lock(void)
        {
            sl_.unlock();
        }

        spin_lock & sl_;
    };

    spin_lock(void):
        state(unlocked_state)
    {}

    ~spin_lock(void)
    {
        assert (state == unlocked_state);
    }

    void lock(void)
    {
        while(!try_lock())
            ;
    }

    bool try_lock(void)
    {
        return state.exchange(locked_state, boost::memory_order_acquire) == unlocked_state;
    }

    void unlock(void)
    {
        assert(state.load(boost::memory_order_relaxed) == locked_state);
        state.store(unlocked_state, boost::memory_order_release);
    }
};

} /* namespace nova */

#endif /* NOVA_TT_SPIN_LOCK_HPP */
