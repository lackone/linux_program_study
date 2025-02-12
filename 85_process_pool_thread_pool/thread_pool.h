#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <list>
#include "locker.h"

template<typename T>
class thread_pool {
public:
    //线程池中线程数量，请求队列中最多允许的，等待处理的请求数量
    thread_pool(int thread_num = 8, int max_request = 10000) : _thread_num(thread_num),
                                                               _max_request(max_request),
                                                               _stop(false),
                                                               _threads(NULL) {
        if (thread_num <= 0 || max_request <= 0) {
            throw std::exception();
        }

        _threads = new pthread_t[thread_num];
        if (!_threads) {
            throw std::exception();
        }

        //创建线程，并设置脱离线程
        for (int i = 0; i < thread_num; i++) {
            printf("create thread %d\n", i);
            if (pthread_create(&_threads[i], NULL, worker, this) != 0) {
                delete[] _threads;
                throw std::exception();
            }
            if (pthread_detach(_threads[i])) {
                delete[] _threads;
                throw std::exception();
            }
        }
    }

    ~thread_pool() {
        delete[] _threads;
        _stop = true;
    }

    //往请求队列中添加任务
    bool append(T *request) {
        _queue_lock.lock();
        //判断是否超过最大请求数量
        if (_queue.size() > _max_request) {
            _queue_lock.unlock();
            return false;
        }
        _queue.push_back(request);
        _queue_lock.unlock();
        _queue_stat.post(); //唤醒线程，队列中有数据到来
        return true;
    }

private:
    //工作线程运行的函数
    static void *worker(void *arg) {
        thread_pool *pool = (thread_pool *) arg;
        pool->run();
        return pool;
    }

    void run() {
        while (!_stop) {
            _queue_stat.wait();
            _queue_lock.lock();
            if (_queue.empty()) {
                _queue_lock.unlock();
                continue;
            }
            T *request = _queue.front();
            _queue.pop_front();
            _queue_lock.unlock();
            if (!request) {
                continue;
            }
            request->process();
        }
    }

private:
    int _thread_num; //线程池中线程数量
    int _max_request; //队列中允许的最大请求数
    pthread_t *_threads; //线程池的数组，大小为_thread_num
    std::list<T *> _queue; //请求队列
    mutex_lock _queue_lock; //请求队列的互斥锁
    sem _queue_stat; //是否有任务需要处理
    bool _stop; //是否结束线程
};


#endif //THREAD_POOL_H
