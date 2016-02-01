#include "CallbackRepeater.h"
CallbackRepeater::CallbackRepeater(int interval, cb_t cb_)
    :last(time(0)), dt(interval), cb(cb_)
{}

void CallbackRepeater::tick(void)
{
    auto now = time(0);
    if(now-last > dt && dt >= 0) {
        cb();
        last = now;
    }
}
