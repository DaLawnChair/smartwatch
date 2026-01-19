/* C++ run-time type info (RTTI) example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <iostream>
#include <memory>
#include <vector>
#include <algorithm>
/*#include <cxxabi.h>*/
#include <chrono>
#include <ctime>
#include <iomanip>
#include <string>
/*#include <stdbool.h>*/
#include <thread>
#include <atomic>
#include <sstream>


/*#include "time_utils.hpp"*/


/*class Clock { */
    /*std::string datetime_{};*/
    /*std::time_t curr_time_{0};*/
    /**/
    /*std::float countdown_{0};*/
    /*std::tm localtime_ = std::chrono::system_clock::to_time_t();*/
    /*std::float update_delay_{0.01};*/
/*};*/
/*std::string Clock::get_current_time();*/
/*std::string Clock::get_datetime(const std::time_t time);*/
/**/
/*std::string Clock::get_current_time(){*/
/*    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();    */
/*    std::time_t currentTime_t = std::chrono::system_clock::to_time_t(now);*/
/**/
/*    localtime_r(&currentTime_t, &localtime_);*/
/**/
/*};*/



/**
 * @brief Convert POSIX timestamp in date
 * @param epoch POSIX timestamp
 * @return Date in `yyyy-MM-dd hh::mm::ss`format
 */
/*std::string get_datetime(const long time) {*/
/*    std::tm tm_struct = *std::gmtime(&time);*/
/**/
/*    std::ostringstream oss; */
/*    oss<< std::put_time(&tm_struct, "%Y-%m-%d %H:%M:%S") << '.' << std::setfill('0') << std::setw(3) << time % 1000;*/
/*    return oss.str();  */
/*};*/
/**/



/* Inside a .cpp file, app_main function must be declared with C linkage */
/*extern "C" void app_main()*/
int main(){
    return 1;
}


