/*
 Copyright 2016 Nervana Systems Inc.
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/

#pragma once

#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstdio>
#include <iostream>
#include <chrono>
#include <utility>
#include <algorithm>
#include <pthread.h>

namespace nervana {
    class thread_pool;
}

static void display_sched_attr(int policy, struct sched_param *param)
{
    printf("    policy=%s, priority=%d\n",
            (policy == SCHED_FIFO)  ? "SCHED_FIFO" :
            (policy == SCHED_RR)    ? "SCHED_RR" :
            (policy == SCHED_OTHER) ? "SCHED_OTHER" :
            "???",
            param->sched_priority);
}

/* thread_pool
 *
 * A collection of a constant number of threads implemented
 * using std::thread.  Methods are provided to start, stop and join all
 * N threads simultaneously.
 *
 */
class nervana::thread_pool {
public:
    explicit thread_pool(int count) :
        _count(count),
        _done(false)
    {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
        _stopped = new bool[count];
        for (int i = 0; i < count; i++) {
            _stopped[i] = false;
        }
    }

    virtual ~thread_pool()
    {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
        stop();
        std::cout << __FILE__ << " " << __LINE__ << std::endl;
        for (std::thread* t : _threads) {
            std::cout << __FILE__ << " " << __LINE__ << std::endl;
            t->join();
            std::cout << __FILE__ << " " << __LINE__ << std::endl;
            delete t;
            std::cout << __FILE__ << " " << __LINE__ << std::endl;
        }
        std::cout << __FILE__ << " " << __LINE__ << std::endl;
        delete[] _stopped;
        std::cout << __FILE__ << " " << __LINE__ << std::endl;
    }

    virtual void start()
    {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
        for (int i = 0; i < _count; i++) {
            _threads.push_back(new std::thread(&thread_pool::run, this, i));

            auto thread = _threads.back()->native_handle();
            int policy, s;
            struct sched_param param;
            s = pthread_getschedparam(thread, &policy, &param);
//            pthread_setschedparam(_threads.back().native_handle(), policy, {priority});
            display_sched_attr(policy, &param);
        }
    }

    virtual void stop()
    {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
        _done = true;
    }

    bool stopped()
    {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
        for (int i = 0; i < _count; i++) {
            if (_stopped[i] == false) {
                return false;
            }
        }
        return true;
    }

//    void join()
//    {
//        std::cout << __PRETTY_FUNCTION__ << std::endl;
//        for (auto t : _threads) {
//            t->join();
//        }
//    }

protected:
    virtual void work(int id) = 0;

    virtual void run(int id)
    {
        std::cout << __FILE__ << " " << __LINE__ << std::endl;
        while (_done == false) {
            work(id);
        }
        _stopped[id] = true;
        std::cout << __FILE__ << " " << __LINE__ << std::endl;
    }

protected:
    int                         _count;
    std::vector<std::thread*>   _threads;
    bool                        _done;
    bool*                       _stopped;
};
