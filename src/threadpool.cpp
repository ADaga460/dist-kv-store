#include "../include/threadpool.h"
#include <iostream>

ThreadPool::ThreadPool(size_t num_threads) : stop_(false) {
    std::cout << "[THREADPOOL] Creating pool with " << num_threads << " threads" << std::endl;
    
    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back(&ThreadPool::workerThread, this);
    }
}

ThreadPool::~ThreadPool() {
    std::cout << "[THREADPOOL] Shutting down..." << std::endl;
    
    stop_ = true;
    condition_.notify_all();
    
    for (std::thread& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    std::cout << "[THREADPOOL] All threads stopped" << std::endl;
}

void ThreadPool::submit(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        tasks_.push(std::move(task));
    }
    condition_.notify_one();
}

void ThreadPool::workerThread() {
    std::cout << "[THREADPOOL] Worker " << std::this_thread::get_id() << " started" << std::endl;
    
    while (!stop_) {
        std::function<void()> task;
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            
            condition_.wait(lock, [this] { 
                return stop_ || !tasks_.empty(); 
            });
            
            if (stop_ && tasks_.empty()) {
                break;
            }
            
            if (!tasks_.empty()) {
                task = std::move(tasks_.front());
                tasks_.pop();
            }
        }
        
        if (task) {
            task();
        }
    }
    
    std::cout << "[THREADPOOL] Worker " << std::this_thread::get_id() << " exiting" << std::endl;
}
