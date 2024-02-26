// --------------------------------------------------------------------------
// |              _    _ _______     .----.      _____         _____        |
// |         /\  | |  | |__   __|  .  ____ .    / ____|  /\   |  __ \       |
// |        /  \ | |  | |  | |    .  / __ \ .  | (___   /  \  | |__) |      |
// |       / /\ \| |  | |  | |   .  / / / / v   \___ \ / /\ \ |  _  /       |
// |      / /__\ \ |__| |  | |   . / /_/ /  .   ____) / /__\ \| | \ \       |
// |     /________\____/   |_|   ^ \____/  .   |_____/________\_|  \_\      |
// |                              . _ _  .                                  |
// --------------------------------------------------------------------------
//
// All Rights Reserved.
// Any use of this source code is subject to a license agreement with the
// AUTOSAR development cooperation.
// More information is available at www.autosar.org.
//
// Disclaimer
//
// This work (specification and/or software implementation) and the material
// contained in it, as released by AUTOSAR, is for the purpose of information
// only. AUTOSAR and the companies that have contributed to it shall not be
// liable for any use of the work.
//
// The material contained in this work is protected by copyright and other
// types of intellectual property rights. The commercial exploitation of the
// material contained in this work requires a license to such intellectual
// property rights.
//
// This work may be utilized or reproduced without any modification, in any
// form or by any means, for informational purposes only. For any other
// purpose, no part of the work may be utilized or reproduced, in any form
// or by any means, without permission in writing from the publisher.
//
// The work has been developed for automotive applications only. It has
// neither been developed, nor tested for non-automotive applications.
//
// The word AUTOSAR and the AUTOSAR logo are registered trademarks.
// --------------------------------------------------------------------------

#include "common/timer.h"
#include <utility>
#include <random>

namespace common{

Timer::Timer(timer_handler a_timer_handler)
    : timer_handler_{std::move(a_timer_handler)}
    , mode_{Timer::Mode::kOneshot}{
    thread_ = std::thread(&Timer::run, this);
}

Timer::~Timer() noexcept{
    if(!joined_.load()){
        stop_and_join_run_thread();
    }
}

void Timer::start_once(const Timer::Clock::duration timeout, const boost::any& user_data){
    const std::unique_lock<std::mutex> lock(mutex_);
    mode_ = kOneshot;
    next_expiry_point_ = Clock::now() + timeout;
    user_data_ = user_data;
    running_ = true;
    condition_variable_.notify_all();
}

void Timer::start_periodic_immediate(const Timer::Clock::duration period, const boost::any& user_data){
    const std::unique_lock<std::mutex> lock(mutex_);
    mode_ = kPeriodic;
    next_expiry_point_ = Clock::now();
    period_ = period;
    user_data_ = user_data;
    running_ = true;
    condition_variable_.notify_all();
}

void Timer::start_count_periodic_immediate(const Timer::Clock::duration period, const uint16_t &count,const boost::any& user_data){
    const std::unique_lock<std::mutex> lock(mutex_);
    mode_ = kCountPeriodic;
    next_expiry_point_ = Clock::now();
    period_ = period;
    user_data_ = user_data;
    running_ = true;
    max_count_ = count;
    condition_variable_.notify_all();
}

void Timer::start_periodic_delayed(const Timer::Clock::duration period, const boost::any& user_data){
    const std::unique_lock<std::mutex> lock(mutex_);
    mode_ = kPeriodic;
    next_expiry_point_ = Clock::now() + period;
    period_ = period;
    user_data_ = user_data;
    running_ = true;
    condition_variable_.notify_all();
}

void Timer::start_periodic_delayed(const Timer::Clock::duration period, const uint64_t u_period_count, const boost::any& user_data){
    const std::unique_lock<std::mutex> lock(mutex_);
    mode_ = kPeriodic;
    next_expiry_point_ = Clock::now() + period;
    period_ = period;
    u_start_value_ = u_period_count;
    u_period_count_ = u_period_count;
    user_data_ = user_data;
    running_ = true;
    condition_variable_.notify_all();
}

void Timer::start_periodic_delayed(const Clock::duration period, const Clock::duration delay, const boost::any& user_data){
    const std::unique_lock<std::mutex> lock(mutex_);
    mode_ = kPeriodic;
    next_expiry_point_ = Clock::now() + delay;
    period_ = period;
    user_data_ = user_data;
    running_ = true;
    condition_variable_.notify_all();
}

std::chrono::steady_clock::duration Timer::get_time_remaining() const {
    const std::lock_guard<std::mutex> lock(mutex_);
    return next_expiry_point_ - Clock::now();
}

void Timer::run(){
    std::unique_lock<std::mutex> lock(mutex_);
    while (!exit_requested_.load()) {
        // Block until timer expires or timer is started again
        if (running_.load()) {
            (void)condition_variable_.wait_until(lock, next_expiry_point_);
        } else {
            condition_variable_.wait(lock);
        }

        if (running_.load()) {
            if (next_expiry_point_ <= Clock::now()) {
                // Unlock the mutex during a potentially long-running operation
                lock.unlock();
                timer_handler_(user_data_);
                lock.lock();
                // Determine if we have to set the timer again
                if (kOneshot == mode_) {
                    running_ = false;
                } else if (kCountPeriodic == mode_) {
                    static uint32_t count{0U};
                    if (++count < max_count_) {
                        next_expiry_point_ += period_;
                    } else {
                        count = 0U;
                        running_ = false;
                    }
                }else {
                    next_expiry_point_ = Clock::now() + period_;
                }
            }
        }
    }
}

bool Timer::is_running() const {
    const std::lock_guard<std::mutex> lock(mutex_);
    return running_;
}

bool Timer::is_periodic() const noexcept {
    const std::lock_guard<std::mutex> lock(mutex_);
    return kPeriodic == mode_;
}

void Timer::stop() noexcept {
    const std::lock_guard<std::mutex> lock(mutex_);
    running_ = false;
}

void Timer::stop_and_join_run_thread(){
    joined_ = true;
    exit_requested_ = true;
    stop();
    // Lift the thread out of its waiting state
    condition_variable_.notify_all();
    if(thread_.joinable()){
        thread_.join();
    }
}

std::chrono::steady_clock::time_point Timer::get_next_expiry_point() const {
    const std::lock_guard<std::mutex> lock(mutex_);
    return next_expiry_point_;
}

Timer::Timer(Timer::timer_handler a_timer_handler, Timer::Mode timer_mode)
        : timer_handler_{std::move(a_timer_handler)},
          mode_{timer_mode} {
    if (mode_ == kBackOff){
        thread_ = std::thread(&Timer::run_backoff, this);
    } else {
        thread_ = std::thread(&Timer::run, this);
    }
}

void Timer::run_backoff() {
    std::unique_lock<std::mutex> lock(mutex_);
    while (!exit_requested_.load()) {
        // Block until timer expires or timer is started again
        if (running_.load()) {
            (void)condition_variable_.wait_until(lock, next_expiry_point_);
        } else {
            condition_variable_.wait(lock);
        }

        if (running_.load()) {
            if (next_expiry_point_ <= Clock::now()) {
                // Unlock the mutex during a potentially long-running operation
                lock.unlock();
                timer_handler_(user_data_);
                lock.lock();
                // Determine if we have to set the timer again
                if (kOneshot == mode_) {
                    running_ = false;
                } else if (kCountPeriodic == mode_) {
                    static uint32_t count{0U};
                    if (++count < max_count_) {
                        next_expiry_point_ += period_;
                    } else {
                        count = 0U;
                        running_ = false;
                    }
                }else {
                    u_period_count_ += u_period_count_;
                    // backoff algorithm
                    std::random_device rd;
                    std::mt19937 mt(rd());
                    std::uniform_int_distribution<uint64_t> dis(u_start_value_, u_period_count_);
                    uint64_t u_random_value = dis(mt);
                    auto timer_interval = std::chrono::milliseconds(u_random_value);
                    Clock::duration timer_period{timer_interval};
                    next_expiry_point_ = Clock::now() + timer_period;
                }
            }
        }
    }
}
} /* namespace common */