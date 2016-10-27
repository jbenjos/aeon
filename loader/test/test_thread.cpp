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

#include <vector>
#include <string>
#include <sstream>
#include <random>

#include "gtest/gtest.h"
#include "thread_pool.hpp"

using namespace std;
using namespace nervana;

TEST(thread, basic_function)
{
    class test_pool : public thread_pool
    {
    public:
        test_pool(int count) :
            thread_pool{count},
            tick(0)
        {
        }
        void work(int id) override
        {
            tick++;
        }

        size_t tick;
    };


    test_pool tp1{1};

    tp1.start();
    usleep(10000);
    tp1.stop();

    EXPECT_LT(0, tp1.tick);
}
