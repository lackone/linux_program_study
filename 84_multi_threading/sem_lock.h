#ifndef SEM_LOCK_H
#define SEM_LOCK_H

#include <semaphore.h>

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

#endif //SEM_LOCK_H
