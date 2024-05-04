#include <iostream>
#include "utils.h"


int main()
{
    unsigned char sixbytes[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};
    std::cout << read_big_endian<1>(sixbytes) << std::endl;
    std::cout << read_big_endian<2>(sixbytes) << std::endl;
    std::cout << read_big_endian<3>(sixbytes) << std::endl;
    std::cout << read_big_endian<4>(sixbytes) << std::endl;
    std::cout << read_big_endian<5>(sixbytes) << std::endl;
    std::cout << read_big_endian<6>(sixbytes) << std::endl;
    return 0;
}