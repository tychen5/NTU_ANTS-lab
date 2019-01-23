#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>
#include "SafeQueue.h"
using namespace std;

class ThreadPool {
private:
    class ThreadWorker {
    private:
        int             _id;
        ThreadPool*     _pool;

    public:
        ThreadWorker(ThreadPool* pool, const int id): _pool(pool), _id(id) {}

        void operator()() {
            function<void()> func;
            bool dequeued;
            while (!_pool->_shutdown) {
                {
                    unique_lock<mutex> lock(_pool->_conditional_mutex);
                    if (_pool->_queue.empty()) {
                        _pool->_conditional_lock.wait(lock);
                    }
                    dequeued = _pool->_queue.dequeue(func);
                }
                if (dequeued) func();
            }
        }
    };

    bool _shutdown;
    SafeQueue<function<void()>>    _queue;
    vector<thread>                 _threads;
    mutex                          _conditional_mutex;
    condition_variable             _conditional_lock;

public:
    ThreadPool(const int n_threads): 
        _threads(vector<thread>(n_threads)), _shutdown(false) {}

    void init() {
        for (int i = 0; i < _threads.size(); ++i) {
            _threads[i] = thread(ThreadWorker(this, i));
        }
    }

    // Waits until threads finish their current task and shutdowns the pool
    void shutdown() {
        _shutdown = true;
        _conditional_lock.notify_all();

        for (int i = 0; i < _threads.size(); ++i) {
            if (_threads[i].joinable()) {
                _threads[i].join();
            }
        }
    }

    // Submit a function to be executed asynchronously by the pool
    template <typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> future<decltype(f(args...))> {
        function<decltype(f(args...))()> func = bind(forward<F>(f), forward<Args>(args)...);
        auto task_ptr = make_shared<packaged_task<decltype(f(args...))()>>(func);
        function<void()> wrapper_func = [task_ptr]() {
            (*task_ptr)();
        };

        _queue.enqueue(wrapper_func);
        _conditional_lock.notify_one();
        return task_ptr->get_future();
    }
};

#endif