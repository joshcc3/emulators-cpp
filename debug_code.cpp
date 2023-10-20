#include <syncstream>
#include <iostream>
#include <vector>
#include <fstream>
#include "gameboy/debug_utils.h"
#include <thread>
#include <fcntl.h>
#include <cassert>
#include <mutex>
#include <shared_mutex>
#include "errno.h"

using namespace std;

mutex m;

long long int clockL = 0;
long long int regL = 0;

atomic<bool> done(false);


/*
 * fetch_add
 * load_store
 * compare_exchange_strong
 * atomic_exchange
 */

template<int>
void spin();

template<int>
void cpu();

template<int>
void ad();

template<>
void spin<1>() {
//    osyncstream(cout) << sched_getcpu() << endl;
    while (!done.load(memory_order_acquire)) {
        {
            lock_guard<mutex> lg(m);
            ++clockL;
            regL = clockL;
        }

    }
}

template<>
void cpu<1>() {
//    osyncstream(cout) << sched_getcpu() << endl;
    long long int prev;
    long long int current;
    {
        lock_guard lg(m);
        prev = clockL;
        current = prev;
    }

    while (!done.load(memory_order_relaxed)) {
        while (prev == current) {
            lock_guard<mutex> lg(m);
            current = clockL;
            regL = current;
        }
        prev = current;
    }

}

template<>
void ad<1>() {
//    osyncstream(cout) << sched_getcpu() << endl;
    while (!done.load(memory_order_relaxed)) {
        lock_guard lg(m);
        long long int x = regL;
        long long int c = clockL;
        if (x != c && x != c - 1) {
            cerr << x << " " << c << endl;
            assert(false);
        }
    }
}

shared_mutex sm;


template<>
void spin<2>() {
//    osyncstream(cout) << sched_getcpu() << endl;
    while (!done.load(memory_order_acquire)) {
        {
            unique_lock lg(sm);
            ++clockL;
            regL = clockL;
        }

    }
}

template<>
void cpu<2>() {
//    osyncstream(cout) << sched_getcpu() << endl;
    long long int prev;
    long long int current;
    {
        shared_lock lg(sm);
        prev = clockL;
        current = prev;
    }

    while (!done.load(memory_order_relaxed)) {
        while (prev == current) {
            unique_lock lg(sm);
            current = clockL;
            regL = current;
        }
        prev = current;
    }

}

template<>
void ad<2>() {
//    osyncstream(cout) << sched_getcpu() << endl;
    while (!done.load(memory_order_relaxed)) {
        unique_lock lg(sm);
        long long int x = regL;
        long long int c = clockL;
        if (x != c && x != c - 1) {
            cerr << x << " " << c << endl;
            assert(false);
        }
    }
}


atomic_flag spinLock = ATOMIC_FLAG_INIT;


// without contention goes up to probable 81782805 ops per second or approx 5.5 instructions per loop
// with betwee 1 and 5 * 10^8
template<>
void spin<3>() {
//    osyncstream(cout) << sched_getcpu() << endl;
    while (!done.load(memory_order_acquire)) {
        {
            while (spinLock.test_and_set(memory_order_acquire));
            ++clockL;
            regL = clockL;
            spinLock.clear(memory_order_release);
        }

    }
}

template<>
void cpu<3>() {
//    osyncstream(cout) << sched_getcpu() << endl;
    long long int prev;
    long long int current;
    {
        while (spinLock.test_and_set(memory_order_acquire));
        prev = clockL;
        current = prev;
        spinLock.clear(memory_order_release);
    }

    while (!done.load(memory_order_relaxed)) {
        while (prev == current) {
            while (spinLock.test_and_set(memory_order_acquire));
            current = clockL;
            regL = current;
            spinLock.clear(memory_order_release);
        }
        prev = current;
    }

}

template<>
void ad<3>() {
//    osyncstream(cout) << sched_getcpu() << endl;
    while (!done.load(memory_order_relaxed)) {
        while (spinLock.test_and_set(memory_order_acquire));
        long long int x = regL;
        long long int c = clockL;
        spinLock.clear(memory_order_release);
        if (x != c && x != c - 1) {
            cerr << x << " " << c << endl;
            assert(false);
        }
    }
}


atomic<uint64_t> unit;

#define CLOCKL(a, m) (a.load(m) >> 32)
/*
 * using memory order relaxed for both operations: 8 * 10^8 ops per second.
 * using memory order seq cst for both operations there is a hit, it goes down to 7.5 * 10^8 operations per second.
 *
 * Uncontended using just a single long for the atomic increment of both - 1.5*10^9 (<3 instructions per loop).
 */
template<>
void spin<4>() {
    assert(unit.is_always_lock_free);
//    osyncstream(cout) << sched_getcpu() << endl;
    while (!done.load(memory_order_acquire)) {
        {
            unit.fetch_add(0x100000001, memory_order_relaxed);
        }

    }
    cout << "Actual ops: [" << CLOCKL(unit, memory_order_relaxed)/5 << "]" << endl;
}

template<>
void cpu<4>() {
//    osyncstream(cout) << sched_getcpu() << endl;
    long long int prev;
    long long int current;
    {
        u64 v = unit.load(memory_order_relaxed);
        prev = v >> 32;
        current = prev;
    }

    while (!done.load(memory_order_relaxed)) {
        while (prev == current) {
            current = CLOCKL(unit, memory_order_relaxed);
        }
        prev = current;
    }

}

template<>
void ad<4>() {
//    osyncstream(cout) << sched_getcpu() << endl;
    while (!done.load(memory_order_relaxed)) {
        u64 v = unit.load(memory_order_relaxed);
        long long int x = v & 0xFFFFFFFF;
        long long int c = v >> 32;
        if (x != c && x != c - 1) {
            cerr << x << " " << c << endl;
            assert(false);
        }
    }
}

// if I put my data on one cache line, can I guarentee atomicity?
// like each processor will always see the upto date latest version of the data.
// then do I even need to synchronize on the two variables? can they both be relaxed

//atomic<int> clockLF(0);
//atomic<int> lock(0);
//int regLF1(0);
//int regLF2(0);
//
//template<>
//void spin<4>() {
//    int y;
//    atomic<int&> x(y);
//
////    osyncstream(cout) << sched_getcpu() << endl;
//
//    while(!done.load(memory_order_relaxed)) {
//        // is fetch add slower than a load with a store?
//        clockLF.fetch_add(1, memory_order_acq_rel);
//    }
//
//}
//
//template<>
//void cpu<4>() {
//
//    int prev;
//    int current;
////    osyncstream(cout) << sched_getcpu() << endl;
//
//    while(!done.load(memory_order_relaxed)) {
//        while(prev == current) {
//            int expected = 0;
//            while(!lock.compare_exchange_strong(expected, 1, memory_order_acquire)) {
//                expected = 0;
//            }
//            current = clockLF2;
//            regLFF2 = current;
//            lock.store(0, memory_order_release);
//        }
//        prev = current;
//    }
//}
//
//template<>
//void ad<4>() {
////    osyncstream(cout) << sched_getcpu() << endl;
//    while(!done.load(memory_order_relaxed)) {
//        int expected = 0;
//        while(!lock.compare_exchange_strong(expected, 1, memory_order_acquire)) {
//            expected = 0;
//        }
//        int x = regLFF2;
//        int c = clockLF2;
//        lock.store(0, memory_order_release);
//        if(x != c && x != c - 1) {
//            cerr << "Reg: " << x << " " << "; Clock: " << c << endl;
//            assert(false);
//        }
//    }
//}



/*
 * Ops/s: 4164770
Ops/s: 8904858
Ops/s: 13072515
[1] Avg Ops: 8.71405e+06
Ops/s: 2405756
Ops/s: 4575252
Ops/s: 7357265
[2] Avg Ops: 4.77942e+06
Ops/s: 8283071
Ops/s: 11789284
Ops/s: 19719199
[3] Avg Ops: 1.32639e+07
 */
int main() {

    {
        regL = 0;
        constexpr int type = 4;
        double avgOps = 0;
        for (int i = 0; i < 24; ++i) {

            unit.store(0, memory_order_relaxed);
            done.store(false, memory_order_release);
            thread spinT{spin<type>};
            thread cpuT{cpu<type>};
            thread adT{ad<type>};

            constexpr int SLEEP_TIME_S = 5;

            sleep(5);
            done.store(true, memory_order_acq_rel);

            spinT.join();
            cpuT.join();
            adT.join();

            // lock version: 4859407

            int clockL = CLOCKL(unit, memory_order_relaxed);
            avgOps += (double(clockL) / SLEEP_TIME_S);
            cout << "Ops/s: " << (clockL / SLEEP_TIME_S) << endl;

        }

        cout << "[" << type << "] Avg Ops: " << avgOps / 3 << endl;
    }

}