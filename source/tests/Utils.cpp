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
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#include "CarlaUtils.hpp"

#include "CarlaBackendUtils.hpp"
#include "CarlaBridgeUtils.hpp"
// #include "CarlaDssiUtils.hpp"
#include "CarlaJuceUtils.hpp"
#include "CarlaLadspaUtils.hpp"
// #include "CarlaLibUtils.hpp"
// #include "CarlaLv2Utils.hpp"
#include "CarlaOscUtils.hpp"
// #include "CarlaPipeUtils.hpp"
// #include "CarlaShmUtils.hpp"
// #include "CarlaStateUtils.hpp"
// #include "CarlaVstUtils.hpp"

// #include "CarlaLibCounter.hpp"
// #include "CarlaLogThread.hpp"
// #include "CarlaMutex.hpp"
// #include "CarlaRingBuffer.hpp"
// #include "CarlaString.hpp"
// #include "CarlaThread.hpp"
// #include "List.hpp"
// #include "Lv2AtomQueue.hpp"
// #include "RtList.hpp"

// #include "JucePluginWindow.hpp"

struct MyStruct {
    char pad[100];
    int i;
    double d;
    void* ptr;
};

class MyLeakCheckedClass
{
public:
    MyLeakCheckedClass() {}
    ~MyLeakCheckedClass() {}
private:
    char pad[100];
    int i;
    double d;
    void* ptr;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MyLeakCheckedClass)
};

#if 0
class MyThread : public CarlaThread
{
public:
    MyThread(CarlaMutex* const m)
        : CarlaThread("myThread"),
          fMu(m)
    {
    }

protected:
    void run() override
    {
        carla_stderr("Thread started");

        carla_stderr("Thread lock");
        fMu->lock();

        carla_stderr("Thread sleeping");
        carla_sleep(3);

        carla_stderr("Thread unlock");
        fMu->unlock();

        carla_stderr("Thread sleep-waiting for stop");

        while (! shouldExit())
            carla_sleep(1);

        carla_stderr("Thread finished");

        return;

        vstPluginCanDo(nullptr, "something");
    }

private:
    CarlaMutex* const fMu;
};
#endif

int main()
{
    // misc functions
    bool2str(false);
    bool2str(true);
    pass();

    // string print functions
    carla_debug("DEBUG");
    carla_stdout("STDOUT");
    carla_stderr("STDERR");
    carla_stderr2("STDERR2");

    // carla_*sleep
    carla_sleep(1);
    carla_msleep(1);

    // carla_setenv
    carla_setenv("THIS", "THAT");
    assert(std::strcmp(std::getenv("THIS"), "THAT") == 0);

    // carla_strdup
    const char* const str1(carla_strdup("string1"));
    const char* const strF(carla_strdup_free(strdup("stringFree")));
    delete[] str1;
    delete[] strF;

    {
        struct TestStruct {
            const char* str1;
            const char* str2;
            const char* str3;
            const char* str4;

            TestStruct()
                : str1(carla_strdup("str1")),
                  str2(carla_strdup("str2")),
                  str3(nullptr),
                  str4(carla_strdup("str4")) {}

            ~TestStruct()
            {
                if (str1 != nullptr)
                {
                    delete[] str1;
                    str1 = nullptr;
                }

                if (str2 != nullptr)
                {
                    delete[] str2;
                    str2 = nullptr;
                }

                if (str3 != nullptr)
                {
                    delete[] str3;
                    str3 = nullptr;
                }

                if (str4 != nullptr)
                {
                    delete[] str4;
                    str4 = nullptr;
                }
            }
        };

        TestStruct a, b, c;
    }

    // math/memory functions
    {
        assert(carla_min<int32_t>(0, -5, 8) == 8);
        assert(carla_max<int32_t>(0, -5, 8) == 0);
        assert(carla_fixValue<float>(0.0f, 1.0f, 1.1f) == 1.0f);

        int v1 = 6;
        int v2 = 8;
        const int v3 = 9;
        assert(v1 == 6 && v2 == 8 && v3 == 9);
        carla_copy<int>(&v1, &v2, 1);
        assert(v1 == 8 && v2 == 8 && v3 == 9);
        carla_copy<int>(&v2, &v3, 1);
        assert(v1 == 8 && v2 == 9 && v3 == 9);

        float fl[5];
        carla_fill(fl, 5, 1.11f);
        assert(fl[0] == 1.11f);
        assert(fl[1] == 1.11f);
        assert(fl[2] == 1.11f);
        assert(fl[3] == 1.11f);
        assert(fl[4] == 1.11f);

        carla_add(fl, fl, 5);
        assert(fl[0] == 1.11f*2);
        assert(fl[1] == 1.11f*2);
        assert(fl[2] == 1.11f*2);
        assert(fl[3] == 1.11f*2);
        assert(fl[4] == 1.11f*2);

        carla_add(fl, fl, 4);
        assert(fl[0] == 1.11f*4);
        assert(fl[1] == 1.11f*4);
        assert(fl[2] == 1.11f*4);
        assert(fl[3] == 1.11f*4);
        assert(fl[4] == 1.11f*2);

        carla_add(fl, fl, 3);
        assert(fl[0] == 1.11f*8);
        assert(fl[1] == 1.11f*8);
        assert(fl[2] == 1.11f*8);
        assert(fl[3] == 1.11f*4);
        assert(fl[4] == 1.11f*2);

        carla_add(fl, fl, 2);
        assert(fl[0] == 1.11f*16);
        assert(fl[1] == 1.11f*16);
        assert(fl[2] == 1.11f*8);
        assert(fl[3] == 1.11f*4);
        assert(fl[4] == 1.11f*2);

        carla_add(fl, fl, 1);
        assert(fl[0] == 1.11f*32);
        assert(fl[1] == 1.11f*16);
        assert(fl[2] == 1.11f*8);
        assert(fl[3] == 1.11f*4);
        assert(fl[4] == 1.11f*2);

        char ch[500];
        carla_zeroChar(ch, 500);
        for (int i=0; i<500; ++i)
            assert(ch[i] == '\0');
    }

    {
        MyStruct a, b, c[2], d[2];
        carla_zeroMem(&a, sizeof(MyStruct));
        carla_zeroStruct<MyStruct>(b);
        carla_zeroStruct<MyStruct>(c, 2);
        carla_copyStruct<MyStruct>(b, a);
        carla_copyStruct<MyStruct>(d, c, 2);
    }

    // Leak check
    {
        MyLeakCheckedClass a;
        MyLeakCheckedClass* const b(new MyLeakCheckedClass);
        delete b;
    }

#if 0
    // Mutex
    {
        CarlaMutex m;
        assert(! m.wasTryLockCalled());

        assert(m.tryLock());
        assert(m.wasTryLockCalled());
        assert(! m.wasTryLockCalled()); // once

        m.unlock();
        m.lock();
        assert(! m.wasTryLockCalled());

        { CarlaMutex::ScopedUnlocker su(m); }
        assert(! m.wasTryLockCalled());

        m.unlock();

        { CarlaMutex::ScopedLocker sl(m); }
    }

    // String
    {
        CarlaString a, b(2), c("haha"), d((uint)0x999, true), e(0.7f), f(0.9), g('u');
        assert(g == "u");
    }

    // Thread
    {
        CarlaMutex m;
        MyThread t(&m);
        carla_stdout("Thread init started");
        t.start();
        carla_stdout("Thread init finished, lock waiting...");
        m.lock();
        carla_stdout("Thread lock wait done");
        m.unlock();
        t.stop(-1);
    }
#endif

    return 0;
}
