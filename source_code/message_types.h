#ifndef MESSAGE_TYPES_H
#define MESSAGE_TYPES_H
#include <cstdint>
#include <iomanip>
#include <stddef.h>
#include <unordered_map>
#include <memory>
#include "trade_types.h"
#include "system_data.h"
#include "utils.h"

static constexpr int PRICE_DIVIDER_4DIGITS = 10000;

static inline void get_price_4digits(float& price, std::istream& is) {
    uint32_t price_buf = read_big_endian<4>(is);
    price = float(price_buf) / PRICE_DIVIDER_4DIGITS;
}

static inline void get_stock_8bytes(std::string& stock, std::istream& is) {
    char stock_buf[8];
    is.read(stock_buf, 8);
    stock.assign(stock_buf, 8);
}

/* We could actually ignore the side of orders for the purpose of VWAP but anyways */
static inline void get_buy_sell_side(BuySellSide& side, std::istream& is) {
    char buy_sell_buf;
    is.read(&buy_sell_buf, 1);
    if (buy_sell_buf == 'B') {
        side = kBuy;
    } else if (buy_sell_buf == 'S') {
        side = kSell;
    } else {
        side = kUnknown;
    }
}

static inline void get_printable(bool& printable, std::istream& is) {
    char printable_buf;
    is.read(&printable_buf, 1);
    printable = (printable_buf == 'Y');
}

static inline void skip_bytes(size_t n_bytes, std::istream& is) {
    is.ignore(n_bytes);
}

// All message types skip Message Type field (1 byte)

class BaseMessage {
public:
    virtual void read_from_stream(std::istream& is) = 0;
    virtual void process(SystemData& sd) = 0;
    virtual ~BaseMessage() {}
};


class SystemEventMessage: public BaseMessage {
    uint16_t stock_locate;
    // ignore tracking number 2 bytes, not interested
    uint64_t timestamp; // 6 bytes
    char event_code;

public:
    void read_from_stream(std::istream& is) override {
        stock_locate = read_big_endian<2>(is);
        skip_bytes(2, is); // skip tracking number
        timestamp = read_big_endian<6>(is);
        is.read(&event_code, 1);
    }

    void process(SystemData& sd) override {
        sd.update_timestamp(timestamp);

        switch(event_code) {
            case 'Q': {
                sd.market_open();
                break;
            }
            case 'M': {
                sd.market_close();
                break;
            }
            default: {
                break;
            }
        }
    }
};

class StockDirectoryMessage: public BaseMessage {
    uint16_t stock_locate;
    // ignore tracking number 2 bytes, not interested
    uint64_t timestamp; // 6 bytes
    std::string stock; // 8 bytes
    // 20 bytes of uninteresting data

public:
    void read_from_stream(std::istream& is) override {
        stock_locate = read_big_endian<2>(is);
        skip_bytes(2, is); // skip tracking number
        timestamp = read_big_endian<6>(is);
        get_stock_8bytes(stock, is);
        skip_bytes(20, is);
    }

    void process(SystemData& sd) override {
        sd.update_timestamp(timestamp);
        sd.add_stock_record(stock_locate, stock);
    }

};

class AddOrderMessage: public BaseMessage {
    uint16_t stock_locate;
    // ignore tracking number 2 bytes, not interested
    uint64_t timestamp; // 6 bytes
    uint64_t order_reference_number;
    BuySellSide side = kUnknown; // read as 'B' or 'S'
    uint32_t shares;
    std::string stock;
    float price; // read as 4 bytes unsigned int, last 4 digits are after decimal

public:

    void read_from_stream(std::istream& is) override {
        stock_locate = read_big_endian<2>(is);
        skip_bytes(2, is); // skip tracking number
        timestamp = read_big_endian<6>(is);
        order_reference_number = read_big_endian<8>(is);
        get_buy_sell_side(side, is);
        shares = read_big_endian<4>(is);
        get_stock_8bytes(stock, is);
        get_price_4digits(price, is);

    }
    
    void process(SystemData& sd) override {
        sd.update_timestamp(timestamp);
        Order order{
            .stock_locate = stock_locate, 
            .side = side, 
            .shares = shares, 
            .price = price, 
            .order_reference_number = order_reference_number
            };
        sd.add_order(std::move(order));

    }

};

class AddOrderMPIDAttributionMessage: public BaseMessage {
    uint16_t stock_locate;
    // ignore tracking number 2 bytes, not interested
    uint64_t timestamp; // 6 bytes
    uint64_t order_reference_number;
    BuySellSide side = kUnknown; // read as 'B' or 'S'
    uint32_t shares;
    std::string stock;
    float price; // read as 4 bytes unsigned int, last 4 digits are after decimal
    // attribution 4 bytes, not interested
    

public:
    void read_from_stream(std::istream& is) override {
        stock_locate = read_big_endian<2>(is);
        skip_bytes(2, is); // skip tracking number
        timestamp = read_big_endian<6>(is);
        order_reference_number = read_big_endian<8>(is);
        get_buy_sell_side(side, is);
        shares = read_big_endian<4>(is);
        get_stock_8bytes(stock, is);
        get_price_4digits(price, is);
        skip_bytes(4, is);
    }
    
    void process(SystemData& sd) override {
        sd.update_timestamp(timestamp);
        Order order{
            .stock_locate = stock_locate, 
            .side = side, 
            .shares = shares, 
            .price = price, 
            .order_reference_number = order_reference_number
            };
        sd.add_order(std::move(order));
    }
    
};

class OrderExecutedMessage: public BaseMessage {
    uint16_t stock_locate;
    // ignore tracking number 2 bytes, not interested
    uint64_t timestamp; // 6 bytes
    uint64_t order_reference_number;
    uint32_t executed_shares;
    uint64_t match_number;

public:
    void read_from_stream(std::istream& is) override {
        stock_locate = read_big_endian<2>(is);
        skip_bytes(2, is); // skip tracking number
        timestamp = read_big_endian<6>(is);
        order_reference_number = read_big_endian<8>(is);
        executed_shares = read_big_endian<4>(is);
        match_number = read_big_endian<8>(is);
    }

    void process(SystemData& sd) override {
        sd.update_timestamp(timestamp);

        Order order; 
        sd.get_order_by_reference_number(order_reference_number, order);
        Trade trade{
            .stock_locate = stock_locate,
            .shares = executed_shares,
            .price = order.price,
            .match_number = match_number
        };

        sd.add_trade(std::move(trade));

    }
   
};

class OrderExecutedWithPriceMessage: public BaseMessage {
    uint16_t stock_locate;
    // ignore tracking number 2 bytes, not interested
    uint64_t timestamp; // 6 bytes
    uint64_t order_reference_number;
    uint32_t executed_shares;
    uint64_t match_number;
    bool printable; // read as Y or N
    float execution_price; // read as 4 bytes unsigned int, last 4 digits are after decimal
    
public:
    void read_from_stream(std::istream& is) override {
        stock_locate = read_big_endian<2>(is);
        skip_bytes(2, is); // skip tracking number
        timestamp = read_big_endian<6>(is);
        order_reference_number = read_big_endian<8>(is);
        executed_shares = read_big_endian<4>(is);
        match_number = read_big_endian<8>(is);
        get_printable(printable, is);
        get_price_4digits(execution_price, is);
    }

   void process(SystemData& sd) override {
        sd.update_timestamp(timestamp);

        // Do not calculate into VWAP if printable is "N"
        if (!printable) {
            return;
        }

        Trade trade{
            .stock_locate = stock_locate,
            .shares = executed_shares,
            .price = execution_price,
            .match_number = match_number
        };

        sd.add_trade(std::move(trade));
        
    }
};


/* 
    For the purpose of VWAP, not interested in order cancel and delete 
        since they don't modify price
    Also not checking if an order is still valid
        assuming data is correct 
        (trades that are erratic will be announced in trade break messages)
*/

class OrderReplaceMessage: public BaseMessage {
    uint16_t stock_locate;
    // ignore tracking number 2 bytes, not interested
    uint64_t timestamp;
    uint64_t original_order_reference_number;
    uint64_t new_order_reference_number;
    uint32_t shares;
    float price; // read as 4 bytes unsigned int, last 4 digits are after decimal
    

public:
    void read_from_stream(std::istream& is) override {
        stock_locate = read_big_endian<2>(is);
        skip_bytes(2, is); // skip tracking number
        timestamp = read_big_endian<6>(is);
        original_order_reference_number = read_big_endian<8>(is);
        new_order_reference_number = read_big_endian<8>(is);
        shares = read_big_endian<4>(is);
        get_price_4digits(price, is);
    }

    void process(SystemData& sd) override {
        sd.update_timestamp(timestamp);

        sd.replace_order(
            original_order_reference_number, new_order_reference_number,
            shares, price);
    }
};

class TradeMessage: public BaseMessage {
    uint16_t stock_locate;
    // ignore tracking number 2 bytes, not interested
    uint64_t timestamp;
    // Ignore order_reference_number 8 bytes, 
    // and side 1 byte as they are deprecated
    uint32_t shares;
    std::string stock;
    float price; // read as 4 bytes unsigned int, last 4 digits are after decimal
    uint64_t match_number;

public:
    void read_from_stream(std::istream& is) override {
        stock_locate = read_big_endian<2>(is);
        skip_bytes(2, is); // skip tracking number
        timestamp = read_big_endian<6>(is);
        skip_bytes(8 + 1, is); // skip order_reference_number and side, as they are deprecated
        shares = read_big_endian<4>(is);
        get_stock_8bytes(stock, is);
        get_price_4digits(price, is);
        match_number = read_big_endian<8>(is);
    }

    void process(SystemData& sd) override {
        sd.update_timestamp(timestamp);

        Trade trade{
            .stock_locate = stock_locate,
            .shares = shares,
            .price = price,
            .match_number = match_number
        };

        sd.add_trade(std::move(trade));
    }
   
};

class CrossTradeMessage: public BaseMessage {
    uint16_t stock_locate;
    // ignore tracking number 2 bytes, not interested
    uint64_t timestamp;
    uint64_t shares;
    std::string stock;
    float cross_price; // read as 4 bytes unsigned int, last 4 digits are after decimal
    uint64_t match_number;
    // Ignore cross type 1 byte - not interested.
    
public:
    void read_from_stream(std::istream& is) override {
        stock_locate = read_big_endian<2>(is);
        skip_bytes(2, is); // skip tracking number
        timestamp = read_big_endian<6>(is);
        shares = read_big_endian<8>(is);
        get_stock_8bytes(stock, is);
        get_price_4digits(cross_price, is);
        match_number = read_big_endian<8>(is);
        skip_bytes(1, is); // skip cross type 1 byte.
    }

    void process(SystemData& sd) override {
        sd.update_timestamp(timestamp);

        Trade trade{
            .stock_locate = stock_locate,
            .shares = shares,
            .price = cross_price,
            .match_number = match_number
        };

        sd.add_trade(std::move(trade));
    }
};


class BrokenTradeMessage: public BaseMessage {
    uint16_t stock_locate;
    // ignore tracking number 2 bytes, not interested
    uint64_t timestamp; // 6 bytes
    uint64_t match_number;
    
public:
    void read_from_stream(std::istream& is) override {
        stock_locate = read_big_endian<2>(is);
        skip_bytes(2, is); // skip tracking number
        timestamp = read_big_endian<6>(is);
        match_number = read_big_endian<8>(is);
    }

    void process(SystemData& sd) override {
        sd.update_timestamp(timestamp);
        
        sd.cancel_trade(match_number);
    }
   
};


#endif // MESSAGE_TYPES_H