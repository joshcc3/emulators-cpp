#include <iostream>
#include <vector>
#include <fstream>
#include "gameboy/debug_utils.h"
#include <thread>
#include <fcntl.h>
#include "errno.h"

using namespace std;

#define SZ 1000000

volatile bool barrier = false;

template<int ix>
void
f(tuple<atomic<int> &, atomic<int> &, atomic<int> &, atomic<int> &> &t,
  vector<tuple<int, long, int, int, int, int, int, int>> &v) {
    while (!barrier);
    int vs[4] = {get<0>(t), get<1>(t), get<2>(t), get<3>(t)};
    for (int i = 0; i < SZ; ++i) {
        vs[0] = get<0>(t);
        vs[1] = get<1>(t);
        vs[2] = get<2>(t);
        vs[3] = get<3>(t);
        auto time = duration_cast<chrono::nanoseconds>(chrono::system_clock::now().time_since_epoch()).count();
        ++(get<ix>(t));
        ++vs[ix];
        v.emplace_back(ix, time, i, sched_getcpu(), vs[0], vs[1], vs[2], vs[3]);
    }
}


struct MyBuf {
    static constexpr int capacity = 0x10000000;

    char *buffer;
    char *it;
    size_t sz;

    MyBuf() : buffer(new char[capacity]), sz{0}, it{buffer} {
    }

    inline void append(const char *a, size_t s) {
        int offs = s % 8;
        char *iter = it;
        for (int j = 0; j < offs; ++j) {
            *(iter++) = *(a++);
        }
        const auto *aL = reinterpret_cast<const long long int *>(a);
        long long int *iL = reinterpret_cast<long long int *>(iter);
        for (int j = offs; j < s; j += sizeof(long long int)) {
            *(iL++) = *(aL++);
        }
        it += s;
        sz += s;
    }


    template<typename T>
    inline void append(T l) {
        if (l < 0) {
            *(it++) = '-';
            ++sz;
            l *= -1;
        }
        char *itCp = it;
        int digSz = 1;
        while (l > 9) {
            long r = l % 10;
            l /= 10;
            *(it++) = r + '0';
            ++sz;
            ++digSz;
        }
        *(it++) = l + '0';
        ++sz;
        for (int j = 0; j < digSz / 2; ++j) {
            swap(itCp[j], itCp[digSz - j - 1]);
        }
    }

    inline void append(char c) {
        *(it++) = c;
        ++sz;
    }

    ~MyBuf() {
        delete[] buffer;
        if (sz > capacity) {
            cerr << "Allocated bad" << endl;
            exit(1);
        }
    }
};

void appendToBuf(MyBuf &buf, const string &&it);

#define CLOCK(a) { \
    auto _s = chrono::system_clock::now(); \
    a;             \
    auto _e = chrono::system_clock::now(); \
    timeSpent[i] += elapsed(_s, _e);                      \
}

string &&to_string() {

}

int main() {



    using th = thread;
    using ths = vector<th>;

    atomic<int> a(0);
    atomic<int> b(0);
    atomic<int> c(0);
    atomic<int> d(0);

    vector<tuple<int, long, int, int, int, int, int, int>> v1;
    vector<tuple<int, long, int, int, int, int, int, int>> v2;
    vector<tuple<int, long, int, int, int, int, int, int>> v3;
    vector<tuple<int, long, int, int, int, int, int, int>> v4;

    tuple<atomic<int> &, atomic<int> &, atomic<int> &, atomic<int> &> t(a, b, c, d);

    th t1(f<0>, ref(t), ref(v1));
    th t2(f<1>, ref(t), ref(v2));
    th t3(f<2>, ref(t), ref(v3));
    th t4(f<3>, ref(t), ref(v4));

    barrier = true;

    t1.join();
    t2.join();
    t3.join();
    t4.join();

    cout << "Writing data" << endl;
    auto start = chrono::system_clock::now();

    int fd = open("multi_thread_relaxed_writes.csv", O_WRONLY | O_CREAT);
    if (fd < 0) {
        cerr << "Could not open file" << endl;
        exit(1);
    }

    double timeSpent[5] = {0, 0, 0, 0, 0};
    auto elapsed = [](auto t1, auto t2) {
        return chrono::duration_cast<chrono::nanoseconds>(t2 - t1).count() / 1000000000.0;
    };


    MyBuf buf;

    const string header = "tid,time,iteration,cpu,v1,v2,v3,v4\n";
    buf.append(header.c_str(), header.size());

    for (auto v: {v1, v2, v3, v4}) {
        for (int i = 0; i < SZ; ++i) {
//            string line[15];
            auto e = v[i];

            CLOCK(
                    int i = 0;
                    buf.append(get<0>(e));
            )
            CLOCK(
                    int i = 1;
                    buf.append(',');

            )
            CLOCK(
                    int i = 2;
                    buf.append(get<1>(e));
            )
            buf.append(',');
            buf.append(get<2>(e));
            buf.append(',');
            buf.append(get<3>(e));
            buf.append(',');
            buf.append(get<4>(e));
            buf.append(',');
            buf.append(get<5>(e));
            buf.append(',');
            buf.append(get<6>(e));
            buf.append(',');
            buf.append(get<7>(e));
            buf.append('\n');

        }
    }

    auto dataCp = chrono::system_clock::now();

    int bytesWritten = write(fd, buf.buffer, buf.sz);
    if (bytesWritten != buf.sz) {
        cerr << "Fail" << endl;
        exit(1);
    }

    string errnoTable[100];
    fill(errnoTable, errnoTable + 100, "-");
    errnoTable[EBADF] = "EBADF";
    errnoTable[EINTR] = "EINTR";
    errnoTable[EIO] = "EIO";

    int closeRes = 0;
    if (close(fd)) {
        cerr << "Failed to close file [" << closeRes << "] [" << errnoTable[errno] << "]" << endl;
    }


    auto end = chrono::system_clock::now();

    cout << "Total [" << elapsed(start, end) << "s]" << endl;
    cout << "Data copy [" << elapsed(start, dataCp) << "s]" << endl;
    cout << "Write syscall [" << elapsed(dataCp, end) << "s]" << endl;
    cout << "single constant byte [" << timeSpent[0] << "s]" << endl;
    cout << "single char comma [" << timeSpent[1] << "s]" << endl;
    cout << "long int [" << timeSpent[2] << "s]" << endl;

}

inline void appendToBuf(MyBuf &buf, const string &&s) {
    const char *src = (s.c_str());
    unsigned long strSz = s.size();
    buf.append(src, strSz);
}



