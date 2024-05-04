#include <cstdint>
template <size_t size>
uint64_t read_big_endian(const unsigned char *buff) {
    return (read_big_endian<size-1>(buff) << 8) + buff[size-1];
}

template <>
uint64_t read_big_endian<1>(const unsigned char *buff) {
    return buff[0];
}