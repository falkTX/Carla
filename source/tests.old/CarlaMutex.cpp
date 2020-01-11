/*
 * CarlaMutex Test
 */

#include <pthread.h>

class CarlaMutex
{
public:
    CarlaMutex()
    {
        pthread_mutex_init(&fMutex, NULL);
    }

    ~CarlaMutex()
    {
        pthread_mutex_destroy(&fMutex);
    }

    void lock()
    {
        pthread_mutex_lock(&fMutex);
    }

    bool tryLock()
    {
        return (pthread_mutex_trylock(&fMutex) == 0);
    }

    void unlock()
    {
        pthread_mutex_unlock(&fMutex);
    }

private:
    pthread_mutex_t fMutex;
};

int main()
{
    CarlaMutex m;
    m.tryLock();
    m.unlock();

    return 0;
}
