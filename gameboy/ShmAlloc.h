//
// Created by jc on 09/10/23.
//

#ifndef AUDIO_TEST_SHMALLOC_H
#define AUDIO_TEST_SHMALLOC_H

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/vector.hpp>

#include <cstdlib>
#include <iostream>
#include <vector>
#include <chrono>


template<typename T, int Sz = 65536>
struct ShmAlloc : std::allocator<T> {

    /*
     * declare rebind, no state, pointer and reference types must match T*,
     */

    typedef T value_type;
    typedef T *pointer;
    typedef const T *const_pointer;
    typedef T &reference;
    typedef const T &const_reference;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;

    static T *const base_addr;
    static difference_type watermark;

    ShmAlloc() {
    }

    ShmAlloc &operator=(ShmAlloc &) = delete;

    inline pointer address(reference t) {
        return &t;
    }

    inline const_pointer address(const_reference t) const {
        return &t;
    }

    inline pointer allocate(size_type n, const_pointer = 0) const {
        std::cout << "Allocate [" << n << "]" << std::endl;
        pointer addr = base_addr + watermark;
        watermark += n * sizeof(T);
        if (addr >= base_addr + Sz) {
            return nullptr;
        }
        return addr;
    }

    inline void deallocate(pointer t, size_type sz) {
        std::cout << "Dealloc [" << sz << "]" << std::endl;
    }

    inline size_type max_size() const {
        return static_cast<size_type>(-1) / sizeof(T);
    }

    template<typename U>
    struct rebind {
        typedef ShmAlloc<U, Sz> other;
    };
};


template<typename T, int Sz>
T *intialiseShm() {
    using namespace boost::interprocess;
    static shared_memory_object shm(open_only, "A", read_write);
    shm.truncate(Sz);
    static mapped_region region(shm, read_write, 0, Sz);
    return reinterpret_cast<T *>(region.get_address());
}

template<typename T, int Sz>
T *const ShmAlloc<T, Sz>::base_addr = intialiseShm<T, Sz>();
template<typename T, int Sz>
ShmAlloc<T, Sz>::difference_type ShmAlloc<T, Sz>::watermark = 0;



#endif //AUDIO_TEST_SHMALLOC_H
