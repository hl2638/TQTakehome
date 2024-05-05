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
    // virtual void print(std::ostream& os = std::cout) {
    //     os << "print() not implemented for this type of message" << std::endl;
    // }
    virtual void process() = 0;
    virtual ~BaseMessage() {}
};


class SystemEventMessage: public BaseMessage {
    uint16_t stock_locate;
    // ignore tracking number 2 bytes, not interested
    uint64_t timestamp; // 6 bytes
    char event_code;

    std::weak_ptr<SystemData> sys_data;

public:
    SystemEventMessage(const std::shared_ptr<SystemData>& sd): sys_data{sd} {}
    void read_from_stream(std::istream& is) override {
        stock_locate = read_big_endian<2>(is);
        skip_bytes(2, is); // skip tracking number
        timestamp = read_big_endian<6>(is);
        is.read(&event_code, 1);
    }

    void process() override {
        std::cout << TimeOfDay(timestamp).to_string() << " "
        << "[DEBUG]System message: Event code = " << event_code << std::endl;
        // TODO handle system events
    }
};

class StockDirectoryMessage: public BaseMessage {
    uint16_t stock_locate;
    // ignore tracking number 2 bytes, not interested
    uint64_t timestamp; // 6 bytes
    std::string stock; // 8 bytes
    // 20 bytes of uninteresting data

    std::weak_ptr<SystemData> sys_data;
public:
    StockDirectoryMessage(const std::shared_ptr<SystemData>& sd): sys_data{sd} {}
    void read_from_stream(std::istream& is) override {
        stock_locate = read_big_endian<2>(is);
        skip_bytes(2, is); // skip tracking number
        timestamp = read_big_endian<6>(is);
        get_stock_8bytes(stock, is);
        skip_bytes(20, is);
    }

    void process() override {
        auto sys_data_ptr = sys_data.lock();
        assert(sys_data_ptr && "sys_data expired");
        sys_data_ptr->add_stock_record(stock_locate, stock);
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
    
    std::weak_ptr<SystemData> sys_data;

public:
    AddOrderMessage(const std::shared_ptr<SystemData>& sd): sys_data{sd} {}

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
    
    void process() override {
        // std::cout << "Processing order: " << order_reference_number << std::endl;
        Order order{.stock_locate = stock_locate, .side = side, .shares = shares, .price = price, .order_reference_number = order_reference_number};
        auto sys_data_ptr = sys_data.lock();
        assert(sys_data_ptr && "sys_data expired");
        sys_data_ptr->add_order(std::move(order));

        // TEST
        Order fetched_order;
        assert(sys_data_ptr->get_order_by_reference_number(order_reference_number, fetched_order) && fetched_order.order_reference_number == order_reference_number);
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
    
    std::weak_ptr<SystemData> sys_data;

public:
    AddOrderMPIDAttributionMessage(const std::shared_ptr<SystemData>& sd): sys_data{sd} {}
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
    
    void process() override {
        Order order{.stock_locate = stock_locate, .side = side, .shares = shares, .price = price, .order_reference_number = order_reference_number};
        auto sys_data_ptr = sys_data.lock();
        assert(sys_data_ptr && "sys_data expired");
        sys_data_ptr->add_order(std::move(order));
    }
    
};

// TODO check ?? Same match_number should only be calculated once. Except for broken message.
class OrderExecutedMessage: public BaseMessage {
    uint16_t stock_locate;
    // ignore tracking number 2 bytes, not interested
    uint64_t timestamp; // 6 bytes
    uint64_t order_reference_number;
    uint32_t executed_shares;
    uint64_t match_number;
    
    std::weak_ptr<SystemData> sys_data;

public:
    OrderExecutedMessage(const std::shared_ptr<SystemData>& sd): sys_data{sd} {}
    void read_from_stream(std::istream& is) override {
        stock_locate = read_big_endian<2>(is);
        skip_bytes(2, is); // skip tracking number
        timestamp = read_big_endian<6>(is);
        order_reference_number = read_big_endian<8>(is);
        executed_shares = read_big_endian<4>(is);
        match_number = read_big_endian<8>(is);
    }

    void process() override {
        auto sys_data_ptr = sys_data.lock();
        assert(sys_data_ptr && "sys_data expired");
        Order order; 
        assert(sys_data_ptr->get_order_by_reference_number(order_reference_number, order) && "Order not found");
        // std::cout << TimeOfDay(timestamp).to_string() << " "
        // << "[DEBUG]Order Executed: match_number = " << match_number
        // << ", stock_locate = " << stock_locate 
        // << ", executed_shares = " << executed_shares
        // << ", order_reference_number = " << order_reference_number
        // << ", side: " << order.side << ", price: " << order.price
        // << std::endl;
        Trade trade{
            .stock_locate = stock_locate,
            .shares = executed_shares,
            .price = order.price,
            .match_number = match_number
        };

        sys_data_ptr->add_trade(std::move(trade));

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
    
    std::weak_ptr<SystemData> sys_data;
    
public:
    OrderExecutedWithPriceMessage(const std::shared_ptr<SystemData>& sd): sys_data{sd} {}
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

   void process() override {
        // Do not calculate into VWAP if printable is "N"

        // std::cout << TimeOfDay(timestamp).to_string() << " "
        // << "[DEBUG]Order Executed With Price: match_number = " << match_number
        // << ", stock_locate = " << stock_locate 
        // << ", executed_shares = " << executed_shares
        // << ", order_reference_number = " << order_reference_number
        // << ", price: " << execution_price
        // << std::endl;

        if (!printable) {
            // std::cout << "[DEBUG]Not printable. Skipped" << std::endl;
            return;
        }

        auto sys_data_ptr = sys_data.lock();
        assert(sys_data_ptr && "sys_data expired");
        Trade trade{
            .stock_locate = stock_locate,
            .shares = executed_shares,
            .price = execution_price,
            .match_number = match_number
        };

        sys_data_ptr->add_trade(std::move(trade));
        
    }
};


/* 
    For the purpose of VWAP, not interested in order cancel and delete 
        since they don't modify price
    Also not checking if an order is still valid
        assuming data is correct 
        (trades that are erratic will be announced in trade break messages)
*/

// class OrderCancelMessage: public BaseMessage {
//     uint16_t stock_locate;
//     // ignore tracking number 2 bytes, not interested
//     uint64_t timestamp;
//     uint64_t order_reference_number;
//     uint32_t cancelled_shares;
    
//     std::weak_ptr<SystemData> sys_data;

// public:
//     OrderCancelMessage(const std::shared_ptr<SystemData>& sd): sys_data{sd} {}
//     void read_from_stream(std::istream& is) override {
//         stock_locate = read_big_endian<2>(is);
//         skip_bytes(2, is); // skip tracking number
//         timestamp = read_big_endian<6>(is);
//         order_reference_number = read_big_endian<8>(is);
//         cancelled_shares = read_big_endian<4>(is);
//     }

//     void process() override {
//         auto sys_data_ptr = sys_data.lock();
//         assert(sys_data_ptr && "sys_data expired");
//         sys_data_ptr->cancel_order_part(order_reference_number, cancelled_shares);
//     }
// };

// class OrderDeleteMessage: public BaseMessage {
//     uint16_t stock_locate;
//     // ignore tracking number 2 bytes, not interested
//     uint64_t timestamp;
//     uint64_t order_reference_number;
    
//     std::weak_ptr<SystemData> sys_data;

// public:
//     OrderDeleteMessage(const std::shared_ptr<SystemData>& sd): sys_data{sd} {}
//     void read_from_stream(std::istream& is) override {
//         stock_locate = read_big_endian<2>(is);
//         skip_bytes(2, is); // skip tracking number
//         timestamp = read_big_endian<6>(is);
//         order_reference_number = read_big_endian<8>(is);
//     }
//     void process() override {
//         auto sys_data_ptr = sys_data.lock();
//         assert(sys_data_ptr && "sys_data expired");
//         sys_data_ptr->delete_order(order_reference_number);
//     }
// };

class OrderReplaceMessage: public BaseMessage {
    uint16_t stock_locate;
    // ignore tracking number 2 bytes, not interested
    uint64_t timestamp;
    uint64_t original_order_reference_number;
    uint64_t new_order_reference_number;
    uint32_t shares;
    float price; // read as 4 bytes unsigned int, last 4 digits are after decimal
    
    std::weak_ptr<SystemData> sys_data;

public:
    OrderReplaceMessage(const std::shared_ptr<SystemData>& sd): sys_data{sd} {}
    void read_from_stream(std::istream& is) override {
        stock_locate = read_big_endian<2>(is);
        skip_bytes(2, is); // skip tracking number
        timestamp = read_big_endian<6>(is);
        original_order_reference_number = read_big_endian<8>(is);
        new_order_reference_number = read_big_endian<8>(is);
        shares = read_big_endian<4>(is);
        get_price_4digits(price, is);
    }

    void process() override {
        // std::cout << "[DEBUG]Replace order: " << std::endl;
        auto sys_data_ptr = sys_data.lock();
        assert(sys_data_ptr && "sys_data expired");
        Order old_order;
        // TEST
        assert(sys_data_ptr->get_order_by_reference_number(original_order_reference_number, old_order) && "Old order not found");
        assert(old_order.stock_locate == stock_locate && "Stock locates do not match for Order Replace");
        // std::cout << TimeOfDay(timestamp).to_string() << " "
        // << "[DEBUG]Old order:" << "reference_number = " << old_order.order_reference_number
        // << ", stock_locate = " << old_order.stock_locate
        // << ", side = " << old_order.side
        // << ", shares = " << old_order.shares
        // << ", price = " << old_order.price
        // << std::endl;

        sys_data_ptr->replace_order(
            original_order_reference_number, new_order_reference_number,
            shares, price);
        Order new_order;

        // TEST
        assert(sys_data_ptr->get_order_by_reference_number(new_order_reference_number, new_order) && "New order not found");
        assert(old_order.stock_locate == new_order.stock_locate
        && old_order.side == new_order.side
        && new_order.order_reference_number == new_order_reference_number
        && new_order.shares == shares
        && new_order.price == price);

        // std::cout << "[DEBUG]New order:" << "reference_number = " << new_order.order_reference_number
        // << ", stock_locate = " << new_order.stock_locate
        // << ", side = " << new_order.side
        // << ", shares = " << new_order.shares
        // << ", price = " << new_order.price
        // << std::endl;

        // TEST
        assert(!sys_data_ptr->get_order_by_reference_number(original_order_reference_number, old_order) && "Old order not deleted");
        
        
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
    
    std::weak_ptr<SystemData> sys_data;

public:
    TradeMessage(const std::shared_ptr<SystemData>& sd): sys_data{sd} {}
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

    void process() override {
        auto sys_data_ptr = sys_data.lock();
        assert(sys_data_ptr && "sys_data expired");

        // TEST
        std::string fetched_symbol;
        assert(sys_data_ptr->get_symbol_by_locate(stock_locate, fetched_symbol) && fetched_symbol == stock);

        // std::cout << TimeOfDay(timestamp).to_string() << " "
        // << "[DEBUG]Trade: match_number = " << match_number
        // << ", stock_locate = " << stock_locate 
        // << ", shares = " << shares
        // << ", price: " << price
        // << std::endl;

        Trade trade{
            .stock_locate = stock_locate,
            .shares = shares,
            .price = price,
            .match_number = match_number
        };

        sys_data_ptr->add_trade(std::move(trade));

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
    
    std::weak_ptr<SystemData> sys_data;
    
public:
    CrossTradeMessage(const std::shared_ptr<SystemData>& sd): sys_data{sd} {}
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

    void process() override {
        auto sys_data_ptr = sys_data.lock();
        assert(sys_data_ptr && "sys_data expired");

        // TEST
        std::string fetched_symbol;
        assert(sys_data_ptr->get_symbol_by_locate(stock_locate, fetched_symbol) && fetched_symbol == stock);

        // std::cout << TimeOfDay(timestamp).to_string() << " "
        // << "[DEBUG]Cross Trade: match_number = " << match_number
        // << ", stock_locate = " << stock_locate 
        // << ", shares = " << shares
        // << ", price: " << cross_price
        // << std::endl;

        Trade trade{
            .stock_locate = stock_locate,
            .shares = shares,
            .price = cross_price,
            .match_number = match_number
        };

        sys_data_ptr->add_trade(std::move(trade));

    }
};


class BrokenTradeMessage: public BaseMessage {
    uint16_t stock_locate;
    // ignore tracking number 2 bytes, not interested
    uint64_t timestamp; // 6 bytes
    uint64_t match_number;
    
    std::weak_ptr<SystemData> sys_data;
    
public:
    void read_from_stream(std::istream& is) override {
        stock_locate = read_big_endian<2>(is);
        skip_bytes(2, is); // skip tracking number
        timestamp = read_big_endian<6>(is);
        match_number = read_big_endian<8>(is);
    }
   
};


#endif // MESSAGE_TYPES_H