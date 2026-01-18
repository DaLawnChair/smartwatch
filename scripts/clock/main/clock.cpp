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
class StopWatch { 
public:

    // stopwatch settings
    std::chrono::steady_clock::time_point stopwatch_start_;
    std::atomic<bool> status_;
    std::chrono::milliseconds duration_;
    std::chrono::milliseconds continued_time_; // after a reset
    std::thread thread_; 

    StopWatch();
    void update_stopwatch();
    void pause_unpause();
    void start();

    void restart();

    std::string format_time();

    ~StopWatch() {
        if(thread_.joinable()){
            status_ = false;
            thread_.join();
        }
        std::cout << "Stopwatch object destroyed\n";
    }
};
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



StopWatch::StopWatch():
    stopwatch_start_{},
    status_{false},
    duration_{},
    continued_time_{}
{};

void StopWatch::update_stopwatch(){
    while(true){
        if(status_){
            std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
            auto res = now - stopwatch_start_ + continued_time_;
            duration_ = std::chrono::duration_cast<std::chrono::milliseconds>(res);
        }
    }
};

void StopWatch::pause_unpause(){
    stopwatch_start_ = std::chrono::steady_clock::now();
    status_ = !status_;
    continued_time_ = (duration_.count() > continued_time_.count()) ? duration_ : continued_time_;
    duration_ = continued_time_;
};

void StopWatch::start(){
    status_ = true;
    stopwatch_start_ = std::chrono::steady_clock::now();
    thread_ = std::thread(&StopWatch::update_stopwatch,this);
};


void StopWatch::restart(){
    status_ = true;
    duration_ = std::chrono::milliseconds(0);
    continued_time_ = std::chrono::milliseconds(0);
    stopwatch_start_ = std::chrono::steady_clock::now();
    thread_ = std::thread(&StopWatch::update_stopwatch,this);
};

std::string StopWatch::format_time(){
    std::ostringstream oss;
    auto time = duration_;

    auto hours = std::chrono::duration_cast<std::chrono::hours>(time);
    time -= hours;
    
    auto min = std::chrono::duration_cast<std::chrono::minutes>(time);
    time -= min;

    auto sec = std::chrono::duration_cast<std::chrono::seconds>(time);
    time -= sec;
    
    if(hours.count()>0) { oss << hours.count() << ':';}
    if(min.count()>0) { oss << min.count() << ':';}
    oss << sec.count() << '.' << std::setfill('0') << std::setw(3) << time.count() % 1000;

    return oss.str();

};


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
int main()
{

    StopWatch stopwatch = StopWatch();

    /*std::cout << "Start stopwatch: current time: " << stopwatch.stopwatch_start_ << "\n";*/
    
    /*stopwatch.update_stopwatch();*/
    stopwatch.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::cout << "duration: " << stopwatch.duration_.count() << "\n";   
    std::cout << "continued_time_: " << stopwatch.continued_time_.count() << "\n";   

    stopwatch.pause_unpause();
    stopwatch.pause_unpause();

    std::cout << "after pause:\n";
    std::cout << "duration: " << stopwatch.duration_.count() << "\n";   
    std::cout << "continued_time_: " << stopwatch.continued_time_.count() << "\n";
    std::cout << stopwatch.duration_.count() << "\n";

    int i = 0;

    while(true){
        std::cout << "status_:" << stopwatch.status_ << 
            "Continued_time_: "  << stopwatch.continued_time_.count() <<
            "current duration: " << stopwatch.duration_.count() << "\n";
        std::cout << "hr time: " << stopwatch.format_time() << "\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        
        /*i+=200;*/
        /*if(i%1000==0){*/
        /*    std::cout << "pause for 5s\n";*/
        /*    stopwatch.pause_unpause();*/
        /*    std::this_thread::sleep_for(std::chrono::milliseconds(5000));*/
        /*    std::cout << "status_:" << stopwatch.status_ << '\n';*/
        /*    stopwatch.pause_unpause();*/
        /*    stopwatch.restart();*/
        /*}*/
    }

}


