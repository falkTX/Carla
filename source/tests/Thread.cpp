/*
 * Carla Tests
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#include "carla_thread.hpp"

class MyThread : public CarlaThread
{
public:
    MyThread(bool wait)
        : blockWait(wait)
    {
    }

protected:
    void run()
    {
        printf("RUN(%i)\n", blockWait);

        if (blockWait)
        {
            for (int i=0; i < 100; i++)
            {
                carla_msleep(50);
                printf("RUN(%i) - BLOCKING\n", blockWait);
            }
        }

        printf("RUN(%i) - FINISHED\n", blockWait);
    }

private:
    bool blockWait;
};

int main()
{
    MyThread t1(false);
    MyThread t2(true);

    MyThread t3(true);
    MyThread t4(true);
    MyThread t5(false);

    t1.start();
    t2.start();
    //t3.start();

    //t3.waitForStarted();
    //t3.stop();

    t1.waitForStarted();
    t2.waitForStarted();

    printf("THREADS STARTED\n");

    // test if threds keep working
    carla_sleep(1);

    printf("THREAD1 STOPPING...\n");

    if (t1.isRunning() && ! t1.stop(500))
    {
       printf("THREAD1 FAILED, TERMINATE\n");
       t1.terminate();
    }

    printf("THREAD2 STOPPING...\n");

    if (t2.isRunning() && ! t2.stop(500))
    {
       printf("THREAD2 FAILED, TERMINATE\n");
       t2.terminate();
    }

    return 0;
}
