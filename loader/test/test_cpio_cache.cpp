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

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "gtest/gtest.h"
#include "cpio.hpp"
#include "buffer_in.hpp"

#define private public

using namespace std;
using namespace nervana;

cv::Mat generate_test_image(int rows, int cols, int embedded_id)
{
    cv::Mat image{rows, cols, CV_8UC3};
    uint8_t* p = image.data;
    for(int row=0; row<rows; row++) {
        for(int col=0; col<cols; col++) {
            *p++ = uint8_t(embedded_id >> 16);
            *p++ = uint8_t(embedded_id >>  8);
            *p++ = uint8_t(embedded_id >>  0);
        }
    }
    return image;
}

int read_embedded_id(const cv::Mat& image)
{
    uint8_t* p = image.data;
    int id;
    id  = int(*p++ << 16);
    id |= int(*p++ <<  8);
    id |= int(*p++ <<  0);
    return id;
}

TEST(cpio_cache, basic)
{
    for(int i=0; i<70000; i++)
    {
        cv::Mat image = generate_test_image(10, 10, i);
        int id = read_embedded_id(image);
        cout << "i=" << i << ", id=" << id << endl;
    }
}