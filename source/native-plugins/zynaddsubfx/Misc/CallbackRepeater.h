#pragma once
#include <functional>
#include <ctime>

struct CallbackRepeater
{
    typedef std::function<void(void)> cb_t ;

    //Call interval in seconds and callback
    CallbackRepeater(int interval, cb_t cb_);

    //Time Check
    void tick(void);

    std::time_t last;
    std::time_t dt;
    cb_t        cb;
};
