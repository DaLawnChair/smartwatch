#include "soc/clk_tree_defs.h"
#include <cstdio>
#include <cstdint>
#include <iostream> 
#include <cmath>

extern "C" {
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
}

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


// ---------- Config ----------

#define GPIO_OUTPUT_IO_0    GPIO_NUM_18
#define GPIO_OUTPUT_IO_1    GPIO_NUM_19
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_IO_0) | (1ULL<<GPIO_OUTPUT_IO_1))

#define ESP_INTR_FLAG_DEFAULT 0

class Vibrator {
private:
    gpio_config_t io_conf_{};
    int delay_;

    std::atomic<bool> status_;
    std::thread thread_;
    std::mutex mtx_;
    std::condition_variable cv_;
    std::atomic<bool> exit_status_;

public:
    Vibrator();
    ~Vibrator(){
        end();
    };

    void set_up_gpio();
    void set_delay(int delay);
    void start();
    void run();
    void pause_unpause();
    void end();
};

Vibrator::Vibrator() :
    status_{false},
    exit_status_{false}
{
    set_up_gpio();
    set_delay(1000);
    thread_ = std::thread(&Vibrator::run,this);
};

void Vibrator::set_up_gpio(){
    //disable interrupt
    io_conf_.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf_.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf_.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    //disable pull-down mode
    io_conf_.pull_down_en = GPIO_PULLDOWN_DISABLE;
    //disable pull-up mode
    io_conf_.pull_up_en = GPIO_PULLUP_DISABLE;
    //configure GPIO with the given settings
    gpio_config(&io_conf_);
};

void Vibrator::set_delay(int delay){
    std::unique_lock<std::mutex> lock(mtx_);
    delay_ = delay;
};

void Vibrator::pause_unpause(){
    std::unique_lock<std::mutex> lock(mtx_);
    status_ = !status_;
    std::cout << "pause upause status " << status_ << '\n'; 
    cv_.notify_one();
};

void Vibrator::run(){
    int cnt=0;
    while(true){
        {
            std::unique_lock<std::mutex> lock(mtx_);
            cv_.wait(lock, [this]{ return status_ || exit_status_;});
            if(exit_status_) {break;}
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_));
        gpio_set_level(GPIO_OUTPUT_IO_0, cnt % 2);
        cnt++;
    }
};

void Vibrator::start(){
    {
       std::lock_guard<std::mutex> lock(mtx_);
       status_ = true;
    }
    cv_.notify_one(); 
};

void Vibrator::end(){
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


extern "C" void app_main(void)
{
    //interrupt of rising edge
    /*io_conf.intr_type = GPIO_INTR_POSEDGE;*/
    //bit mask of the pins, use GPIO4/5 here
    /*io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;*/
    //set as input mode
    /*io_conf.mode = GPIO_MODE_INPUT;*/
    //enable pull-up mode
    /*io_conf.pull_up_en = GPIO_PULLUP_DISABLE;*/
    /*gpio_config(&io_conf);*/

    Vibrator vibrator = Vibrator();
    std::cout << "lol\n";
    vibrator.start();

    /*std::this_thread::sleep_for(std::chrono::milliseconds(5000));*/
    /*std::cout << "end now\n";*/
    /*vibrator.end();*/

    while (1) {

        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        vibrator.pause_unpause();
    }
}
