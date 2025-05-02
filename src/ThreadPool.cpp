//
// Created by thomas on 02/05/25.
//

#include "ThreadPool.hpp"

void ThreadPool::enqueue(std::function<void(int)> task)
{
    {
        // Lock the queue mutex
        std::unique_lock<std::mutex> lock(queue_mutex);
        // Push the task into the queue
        tasks.push(std::move(task));
    }
    // Notify one of the waiting threads
    condition.notify_one();
}

ThreadPool::ThreadPool(size_t num_threads)
{
    // Creating worker threads
    for (size_t i = 0; i < num_threads; ++i) {
        workers.emplace_back([this, i] {
            while (true) {
                std::function<void(int)> task;
                // The reason for putting the below code
                // here is to unlock the queue before
                // executing the task so that other
                // threads can perform enqueue tasks
                {
                    // Locking the queue so that data
                    // can be shared safely
                    std::unique_lock<std::mutex> lock(queue_mutex);

                    // Waiting until there is a task to
                    // execute or the pool is stopped
                    condition.wait(lock, [this] {
                        return !tasks.empty() || stop;
                    });

                    // exit the thread in case the pool
                    // is stopped and there are no tasks
                    if (stop && tasks.empty()) {
                        return;
                    }
                    // Get the next task from the queue
                    task = std::move(tasks.front());
                    tasks.pop();
                    active_tasks++;
                }

                task(i);
                {
                    std::unique_lock<std::mutex> lock(queue_mutex);
                    --active_tasks;
                    // Notify wait() in case the queue is empty
                    finished_condition.notify_all();
                }
            }
        });
    }

}

ThreadPool::~ThreadPool()
{
    {
        // Lock the queue to update the stop flag safely
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }

    // Notify all threads
    condition.notify_all();

    // Joining all worker threads to ensure they have
    // completed their tasks
    for (auto& thread : workers) {
        thread.join();
    }

}
void ThreadPool::wait() {
    std::unique_lock<std::mutex> lock(queue_mutex);
    // Wait until both the queue is empty and no active tasks are running
    finished_condition.wait(lock, [this] {
        return tasks.empty() && (active_tasks == 0);
    });
}