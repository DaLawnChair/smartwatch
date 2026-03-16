// g++-13 -std=c++2b stopwatch.cpp -o a.out && ./a.out
// error from handling the parsing of the time, can be solved with some of the code I wrote for time_utils for the orderbook
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <string>
using namespace std::string_literals; // for access to s postfix for strings
#include <system_error>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <unordered_map>

#include <stdexcept>
#include <format>

struct UpdateDelays {
    std::time_t SEC = 1 * 1000;
    std::time_t MIN = 60 * 1000;
    std::time_t HOUR = 60*60 * 1000;
};


class Clock { 
private:
    std::string string_curr_time_{};
    std::time_t curr_time_{0};

    std::tm local_time_;
    std::string selected_time_delay_{"sec"};
    std::time_t delay_;
    UpdateDelays update_delays_ = UpdateDelays();

    std::atomic<bool> status_;
    std::thread thread_;
    std::mutex mtx_;
    std::condition_variable cv_;
    std::atomic<bool> exit_status_;


public:
    Clock(std::string selected_time_delay="MIN"s);
    ~Clock() {
        end();
    }
    void set_delay(std::string choice);
    std::time_t get_delay();
 
    void update_clock();
    void start();
    void end();
    void set_current_time(std::string tracking_type); // set time as automatic or by current time

    std::time_t get_current_time();
    std::string get_datetime();
};


Clock::Clock(std::string selected_time_delay): // default arg is MIN
    string_curr_time_{},
    curr_time_{0},
    local_time_{},
    selected_time_delay_{selected_time_delay},
    status_{false},
    exit_status_{false}
{
    set_delay(selected_time_delay_);
    thread_ = std::thread(&Clock::update_clock,this);
};


void Clock::set_delay(std::string choice) {
    {
        std::unique_lock<std::mutex> lock(mtx_);

        if(choice=="MIN") {delay_ = update_delays_.MIN; selected_time_delay_ = choice;}
        else if(choice=="SEC") {delay_ = update_delays_.SEC; selected_time_delay_ = choice;}
        else if(choice=="HOUR") {delay_ = update_delays_.HOUR; selected_time_delay_ = choice;}
        else {throw std::invalid_argument(std::format("{} is not implemented", choice)); }
    }
};

std::time_t Clock::get_current_time() {
    std::unique_lock<std::mutex> lock(mtx_);
    return curr_time_;
};

std::string Clock::get_datetime() {
    std::unique_lock<std::mutex> lock(mtx_);
    return string_curr_time_; 
};

std::time_t Clock::get_delay() {
    std::unique_lock<std::mutex> lock(mtx_);
    return delay_;
};

void Clock::update_clock() {
    while (true) {
        {
            std::unique_lock<std::mutex> lock(mtx_);
            /*std::cout << "waiting...\n";*/
            cv_.wait(lock, [this] { return status_ || exit_status_; });

            if (exit_status_) {
                break;
            }
           
            auto now = std::chrono::system_clock::now();
            curr_time_ = std::chrono::system_clock::to_time_t(now);
            
            // convert to edt time (by default of my region) [][]
            std::chrono::zoned_time zt{std::chrono::current_zone(), now};
            string_curr_time_ = std::format("{:%Y-%m-%d %H:%M:%S}", zt);
            /*std::cout << string_curr_time_ << '\n';*/
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_));
        /*}*/
    }
}

/*void StopWatch::pause_unpause(){*/
/*    {*/
/*        std::lock_guard<std::mutex> lock(mtx_);*/
/*        status_ = !status_;*/
/*    }*/
/*    cv_.notify_one(); */
/*    stopwatch_start_ = std::chrono::steady_clock::now();*/
/*    resumed_time_ = (duration_.count() > resumed_time_.count()) ? duration_ : resumed_time_;*/
/*    duration_ = resumed_time_;*/
/*};*/

void Clock::start(){
    {
       std::lock_guard<std::mutex> lock(mtx_);
       status_ = true;
    }
    cv_.notify_one(); 
};

void Clock::end(){
    {
        std::lock_guard<std::mutex> lock(mtx_);
        status_ = false;
        exit_status_ = true;
    }
    cv_.notify_one();
    std::cout << "notified\n";
    
    if(thread_.joinable()){
        thread_.join();
    }

    std::cout << "Stopwatch object destroyed\n";
};

/* Inside a .cpp file, app_main function must be declared with C linkage */
/*extern "C" void app_main()*/
int main()
{

    Clock clock = Clock();
    clock.start();
 
    int i = 0;
    while(true){
        std::cout << clock.get_datetime()<<'\n';
        std::this_thread::sleep_for(std::chrono::milliseconds(clock.get_delay()+1));
    }
}

