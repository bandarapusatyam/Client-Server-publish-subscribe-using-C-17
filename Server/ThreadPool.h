#pragma once

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <functional>
#include <memory>

using namespace std;
class ThreadPool
{
public:
    explicit ThreadPool(int size) : thread_pool_size(size), isExit(false)
    {
    }
    void ProcessThreadPool()
    {
        while (!isExit)
        {
            function<void()> task;
            unique_lock<mutex> l(m);
            while (taskQueue.empty())
            {
                cv.wait(l);
            }
            task = taskQueue.front();
            taskQueue.pop();
            cv.notify_all();
            l.unlock();
            task();
        }
    }

    void AddTask(function<void()> fn)
    {
        unique_lock<mutex> l(m);
        taskQueue.push(fn);
        cv.notify_all();
        l.unlock();
    }

    void CreateThreadPool()
    {
        for (int i = 0; i < thread_pool_size; i++)
        {
            th_pool.push_back(move(thread(&ThreadPool::ProcessThreadPool, this)));
        }
    }

    void CloseThreadPool()
    {
        cv.notify_all();
        for (int i = 0; i < thread_pool_size; i++)
        {
            if (th_pool[i].joinable())
            {
                th_pool[i].join();
            }
        }
    }

    ~ThreadPool()
    {
        isExit = true;
        CloseThreadPool();
    }

private:
    //thread pool
    vector<thread> th_pool;
    //To have fast access to the queue,
    // implement custom ringbuffer with locks free and wait free queue
    queue<function<void()>> taskQueue;
    mutex m;
    condition_variable cv;
    int thread_pool_size;
    bool isExit;
};
