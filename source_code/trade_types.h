#ifndef TRADE_TYPES_H
#define TRADE_TYPES_H
#include <cstdint>
#include <unordered_map>

enum BuySellSide {
    kUnknown = -1,
    kBuy = 0,
    kSell = 1
};

struct Order {
    uint16_t stock_locate;
    BuySellSide side;
    uint64_t shares;
    float price;
    uint64_t order_reference_number;
};

struct Trade {
    uint16_t stock_locate;
    uint64_t shares;
    float price;
    uint64_t match_number;
    // Don't care about side for the purpose of VWAP
};


#endif // TRADE_TYPES_H