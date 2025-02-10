#ifndef MUTEX_LOCK_H
#define MUTEX_LOCK_H

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

#endif //MUTEX_LOCK_H
