//
// heavily inspired by https://www.geeksforgeeks.org/thread-pool-in-cpp/
//

#ifndef GROUP3ENGINE_THREADPOOL_HPP
#define GROUP3ENGINE_THREADPOOL_HPP
#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <condition_variable>
#include <mutex>

class ThreadPool {
public:
    ThreadPool(size_t num_threads= std::thread::hardware_concurrency());
    ~ThreadPool();
    void enqueue(std::function<void(int)> task);
    void wait(); // Wait until all tasks are completed.

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void(int)>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    std::condition_variable finished_condition;
    size_t active_tasks = 0;
    bool stop;
};


#endif //GROUP3ENGINE_THREADPOOL_HPP
