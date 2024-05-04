#include <cstdint>
#include <stddef.h>
#pragma pack(push,1)
struct SystemEventMessage {
    // Skip Message Type 1 byte
    // Skip: stock locate:tracking number, 4 bytes
    uint64_t timestamp; // 6 bytes, nanoseconds since bod
    char event_code;
};

struct StockDirectoryMessage {
    // Skip Message Type 1 byte
    uint16_t stock_locate; // 2 bytes
    // Skip tracking number - 2 bytes
    uint64_t timestamp; // 6 bytes, nanoseconds since bod
    char stock[8];
    // Skip byte 19:38, 20 bytes
};

struct AddOrderNoMPIDAttributionMessage {
    // Skip Message Type 1 byte
    uint16_t stock_locate; // 2 bytes
    // Skip tracking number - 2 bytes
    uint64_t timestamp; // 6 bytes, nanoseconds since bod
    uint64_t order_reference_number; // 8 bytes
    char buy_sell_indicator; // 'B' or 'S'
    uint32_t shares; // 4 bytes
    char stock[8];
    float price; // read as 4 bytes unsigned int, last 4 digits are after decimal
};

struct AddOrderMPIDAttributionMessage {
    // Skip Message Type 1 byte
    uint16_t stock_locate; // 2 bytes
    // Skip tracking number - 2 bytes
    uint64_t timestamp; // 6 bytes, nanoseconds since bod
    uint64_t order_reference_number; // 8 bytes
    char buy_sell_indicator; // 'B' or 'S'
    uint32_t shares; // 4 bytes
    char stock[8];
    float price; // read as 4 bytes unsigned int, last 4 digits are after decimal
    // Skip 4 bytes
};

struct OrderExecutedMessage {
    // Skip Message Type 1 byte
    uint16_t stock_locate; // 2 bytes
    // Skip tracking number - 2 bytes
    uint64_t timestamp; // 6 bytes, nanoseconds since bod
    uint64_t order_reference_number; // 8 bytes
    uint32_t executed_shares; // 4 bytes
};

#pragma pack(pop)

constexpr size_t StockTradingActionMsgSize = 25;
constexpr size_t RegSHOShortSalePriceTestRestrictedIndicatorMsgSize = 20;
constexpr size_t MarketParticipantPositionMsgSize = 26;
constexpr size_t MWCBDeclineLevelMessageMsgSize = 35;
constexpr size_t MWCBStatusMessageMsgSize = 12;
constexpr size_t QuotingPeriodUpdateMessageMsgSize = 28;
constexpr size_t LULDAuctionCollarMsgSize = 35;
constexpr size_t OperationalHaltMsgSize = 21;





