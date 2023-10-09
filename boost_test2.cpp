#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <cstring>
#include <cstdlib>
#include <string>
#include <iostream>

int main(int argc, char *argv[]) {
    using namespace boost::interprocess;

    //Open already created shared memory object.
    shared_memory_object shm(open_only, "A", read_only);

    //Map the whole shared memory in this process
    mapped_region region(shm, read_only);

    //Check that memory was initialized to 1
    int *base_addr = static_cast<int *>(region.get_address());

    while(true) {
        int offs;
        std::cin >> offs;
        std::cout << offs << ": " << base_addr[offs] << std::endl;
    }


    return 0;

}
