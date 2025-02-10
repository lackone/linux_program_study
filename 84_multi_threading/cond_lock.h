#ifndef COND_LOCK_H
#define COND_LOCK_H

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

#endif //COND_LOCK_H
