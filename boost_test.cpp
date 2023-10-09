#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <cstring>
#include <cstdlib>
#include <string>
#include <iostream>

int main(int argc, char *argv[]) {
    using namespace boost::interprocess;

    //Remove shared memory on construction and destruction
    struct shm_remove {
        shm_remove() { shared_memory_object::remove("A"); }

        ~shm_remove() { shared_memory_object::remove("A"); }
    } remover;

    //Create a shared memory object.
    shared_memory_object shm(create_only, "A", read_write);

    //Set size
    shm.truncate(3000);

    //Map the whole shared memory in this process
    mapped_region region(shm, read_write);

    //Write all the memory to 1
    int *base_addr = static_cast<int*>(region.get_address());
    std::cout << "Created shared memory launching child process" << std::endl;
    //Launch child process
    std::string s(argv[0]);
    s += " child ";
    while(true) {
        int offs;
        int value;
        std::cin >> offs >> value;
        base_addr[offs] = value;
    }

    return 0;
}
