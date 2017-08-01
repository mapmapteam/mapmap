/*
 * Toonloop
 *
 * Copyright (c) 2010 Alexandre Quessy <alexandre@quessy.net>
 * Copyright (c) 2010 Tristan Matthews <le.businessman@gmail.com>
 *
 * Toonloop is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Toonloop is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the gnu general public license
 * along with Toonloop.  If not, see <http://www.gnu.org/licenses/>.
 */

// Written by Anthony Williams, 2008
// Public domain based on his comment:
// "Yes, you can just copy the code presented here and use it for whatever you 
// like. There won't be any licensing issues. I'm glad you find it helpful."
// Reference:
// http://www.justsoftwaresolutions.co.uk/threading/implementing-a-thread-safe-queue-using-condition-variables.html

#ifndef _CONCURRENT_QUEUE_H_
#define _CONCURRENT_QUEUE_H_

#include <queue>
#include <QMutex>
#include <QWaitCondition>

namespace mmp {

template<typename Data>
class ConcurrentQueue
{
    private:
        std::queue<Data> queue_;
        QMutex mutex_;
        QWaitCondition condition_;
    public:
        ConcurrentQueue() : queue_(), mutex_(), condition_()
    {}

        void push(Data const& data)
        {
            QMutexLocker locker(&mutex_);
            queue_.push(data);
            condition_.wakeOne();
        }

        bool empty() const
        {
            QMutexLocker locker(&mutex_);
            return queue_.empty();
        }

        bool try_pop(Data& popped_value)
        {
            QMutexLocker locker(&mutex_);
            if (queue_.empty())
            {
                return false;
            }

            popped_value = queue_.front();
            queue_.pop();
            return true;
        }

        void wait_and_pop(Data& popped_value)
        {
            QMutexLocker locker(&mutex_);
            while (queue_.empty())
            {
                condition_.wait(&mutex_);
            }

            popped_value = queue_.front();
            queue_.pop();
        }
};

}

#endif // _CONCURRENT_QUEUE_H_
