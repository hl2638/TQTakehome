#ifndef UTILS_H
#define UTILS_H
#include <cstdint>
#include <chrono>
#include <stddef.h>

template <size_t size>
uint64_t read_big_endian(std::istream& is) {
    uint64_t result = 0;
    for (size_t i = 0; i < size; ++i) {
        uint8_t byte;
        is.read(reinterpret_cast<char*>(&byte), sizeof(byte));
        result = (result << 8) + byte;
    }
    return result;
}

inline int get_hour_by_timestamp(uint64_t timestamp) {
    return (timestamp / (1000000000 * 60 * 60)) % 24;
}

class TimeOfDay {
private:
    std::chrono::nanoseconds timestamp_;

public:
    TimeOfDay(const std::chrono::nanoseconds timestamp) : timestamp_(timestamp) {}
    TimeOfDay(const uint64_t timestamp) : timestamp_(timestamp) {}
    TimeOfDay(int hours, int minutes, int seconds, int nanoseconds) {
        timestamp_ = std::chrono::hours(hours) + std::chrono::minutes(minutes) +
                    std::chrono::seconds(seconds) + std::chrono::nanoseconds(nanoseconds);
    }

    std::string to_string() const {
        auto hours = std::chrono::duration_cast<std::chrono::hours>(timestamp_);
        auto minutes = std::chrono::duration_cast<std::chrono::minutes>(timestamp_ - hours);
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(timestamp_ - hours - minutes);
        auto nanoseconds = timestamp_ - hours - minutes - seconds;

        std::stringstream ss;
        ss << std::setw(2) << std::setfill('0') << hours.count() << ":" 
           << std::setw(2) << std::setfill('0') << minutes.count() << ":" 
           << std::setw(2) << std::setfill('0') << seconds.count() << "."
           << std::setw(9) << std::setfill('0') << nanoseconds.count()
        //    << "(" << timestamp_.count() << ")"
           ;
        return ss.str();
    }
};

#endif // UTILS_H