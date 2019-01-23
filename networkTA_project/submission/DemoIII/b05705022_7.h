#ifndef SAFE_QUEUE_H
#define SAFE_QUEUE_H

#include <mutex>
#include <queue>
using namespace std;

template <typename T>
class SafeQueue {
private:
    queue<T> _queue;
    mutex m_mutex;

public:
    bool empty() {
        unique_lock<mutex> lock(m_mutex);
        return _queue.empty();
    }

    int size() {
        unique_lock<mutex> lock(m_mutex);
        return _queue.size();
    }

    void enqueue(T& t) {
        unique_lock<mutex> lock(m_mutex);
        _queue.push(t);
    }

    bool dequeue(T& t) {
        unique_lock<mutex> lock(m_mutex);

        if (_queue.empty()) return false;
        t = move(_queue.front());

        _queue.pop();
        return true;
    }
};

#endif