#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <string>
#include <sstream>

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

class StopWatch { 
public:

    // stopwatch settings
    std::chrono::steady_clock::time_point stopwatch_start_;
    std::atomic<bool> status_;
    std::chrono::milliseconds duration_;
    std::chrono::milliseconds continued_time_; // after a reset
    std::thread thread_;
    std::mutex mtx_;
    std::condition_variable cv_;
    std::atomic<bool> exit_status_;


    StopWatch();
    void update_stopwatch();
    void pause_unpause();
    void start();

    void restart();
    void end();

    std::string format_time();

    ~StopWatch() {
        end();
    }
};

StopWatch::StopWatch():
    stopwatch_start_{},
    status_{false},
    duration_{},
    continued_time_{},
    exit_status_{false}
{
    thread_ = std::thread(&StopWatch::update_stopwatch,this);

};

void StopWatch::update_stopwatch(){
    while(true){
        {
            std::unique_lock<std::mutex> lock(mtx_);
            cv_.wait(lock, [this]{ return status_==true || exit_status_==true;});
            if(exit_status_==true) {break;}
        }
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        auto res = now - stopwatch_start_ + continued_time_;
        duration_ = std::chrono::duration_cast<std::chrono::milliseconds>(res);
            
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
};

void StopWatch::pause_unpause(){
    {
        std::lock_guard<std::mutex> lock(mtx_);
        status_ = !status_;
    }
    cv_.notify_one(); 
 
    stopwatch_start_ = std::chrono::steady_clock::now();

    continued_time_ = (duration_.count() > continued_time_.count()) ? duration_ : continued_time_;
    duration_ = continued_time_;
};

void StopWatch::start(){
    {
       std::lock_guard<std::mutex> lock(mtx_);
       status_ = true;
    }
    cv_.notify_one(); 
    stopwatch_start_ = std::chrono::steady_clock::now();
};


void StopWatch::restart(){
    {
       std::lock_guard<std::mutex> lock(mtx_);
       status_ = true;
    }
    cv_.notify_one(); 
    duration_ = std::chrono::milliseconds(0);
    continued_time_ = std::chrono::milliseconds(0);
    stopwatch_start_ = std::chrono::steady_clock::now();
};


void StopWatch::end(){
    {
        std::lock_guard<std::mutex> lock(mtx_);
        status_ = false;
        exit_status_ = true;
        std::cout << "got lock\n";
    }
    cv_.notify_one();
    std::cout << "notified\n";
    
    if(thread_.joinable()){
        thread_.join();
        std::cout << "joined\n";
    }

    std::cout << "Stopwatch object destroyed\n";
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

/* Inside a .cpp file, app_main function must be declared with C linkage */
/*extern "C" void app_main()*/
int main()
{

    StopWatch stopwatch = StopWatch();
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
    int step = 20;
    while(true){
        std::cout << "status_:" << stopwatch.status_ << 
            "Continued_time_: "  << stopwatch.continued_time_.count() <<
            "current duration: " << stopwatch.duration_.count() << "\n";
        std::cout << "hr time: " << stopwatch.format_time() << "\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(step));
        
        i+=step;
        if(i%10000==0){
            std::cout << "pause for 10s\n";
            stopwatch.pause_unpause();
            std::this_thread::sleep_for(std::chrono::milliseconds(5000));
            std::cout << "status_:" << stopwatch.status_ << '\n';
            stopwatch.pause_unpause();
        }
        // stop after 30 seconds
        if(stopwatch.duration_.count()>30000){
            break;
        }
    }
}


