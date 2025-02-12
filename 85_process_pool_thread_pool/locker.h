#ifndef LOCKER_H
#define LOCKER_H

#include <semaphore.h>
#include <pthread.h>
#include <iostream>

class sem {
public:
    sem() {
        if (sem_init(&_sem, 0, 0) != 0) {
            throw std::exception();
        }
    }

    ~sem() {
        sem_destroy(&_sem);
    }

    bool wait() {
        //等待信号
        return sem_wait(&_sem) == 0;
    }

    bool post() {
        //释放信号
        return sem_post(&_sem) == 0;
    }

private:
    sem_t _sem;
};

class mutex_lock {
public:
    mutex_lock() {
        if (pthread_mutex_init(&mutex, NULL) != 0) {
            throw std::exception();
        }
    }

    ~mutex_lock() {
        pthread_mutex_destroy(&mutex);
    }

    bool lock() {
        return pthread_mutex_lock(&mutex) == 0;
    }

    bool unlock() {
        return pthread_mutex_unlock(&mutex) == 0;
    }

private:
    pthread_mutex_t mutex;
};

class cond_lock {
public:
    cond_lock() {
        if (pthread_mutex_init(&mutex, NULL) != 0) {
            throw std::exception();
        }
        if (pthread_cond_init(&cond, NULL) != 0) {
            pthread_mutex_destroy(&mutex);
            throw std::exception();
        }
    }

    ~cond_lock() {
        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&cond);
    }

    bool wait() {
        //等待条件变量
        int ret = 0;
        pthread_mutex_lock(&mutex);
        ret = pthread_cond_wait(&cond, &mutex);
        pthread_mutex_unlock(&mutex);
        return ret == 0;
    }

    bool signal() {
        //唤醒等待条件变量的线程
        return pthread_cond_signal(&cond) == 0;
    }

private:
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

#endif //LOCKER_H
