#include <queue>
#include <pthread.h>
#include <vector>
using namespace std;

class SocketQueue
{
  private:
    queue<int *> client_sock_queue;
    pthread_mutex_t mutex_queue = PTHREAD_MUTEX_INITIALIZER;

  public:
    SocketQueue(){};
    ~SocketQueue(){};
    int *pop()
    {
        pthread_mutex_lock(&mutex_queue);
        int *s = client_sock_queue.front();
        client_sock_queue.pop();
        pthread_mutex_unlock(&mutex_queue);
        return s;
    };
    void push(int *t)
    {
        pthread_mutex_lock(&mutex_queue);
        client_sock_queue.push(t);
        pthread_mutex_unlock(&mutex_queue);
    }
    size_t size()
    {
        pthread_mutex_lock(&mutex_queue);
        size_t s = client_sock_queue.size();
        pthread_mutex_unlock(&mutex_queue);
        return s;
    }
};