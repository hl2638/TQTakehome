#ifndef SYSTEM_DATA_H
#define SYSTEM_DATA_H
#include <cstdint>
#include <cassert>
#include <string>
#include <unordered_map>
#include <iostream>
#include "trade_types.h"

class SecurityStats {
    uint16_t stock_locate;
    uint64_t traded_shares;
    float total_traded_value;
    // TODO add shared ptrs to orders and trades?
public:
    SecurityStats(const uint16_t locate):
    stock_locate{locate}, traded_shares{0}, total_traded_value{0} {}
    void handle_trade(const Trade& trade) {
        // std::cout << "[DEBUG]Stock_locate: " << std::endl;
        // std::cout << "shares: " << traded_shares << " -> ";
        traded_shares += trade.shares;
        // std::cout << traded_shares << ", ";
        // std::cout << "traded value: " << total_traded_value << " -> ";
        total_traded_value += trade.price * trade.shares;
        // std::cout << total_traded_value << std::endl;
    }
};

class SystemData {
    std::unordered_map<uint16_t, std::string> locate_to_symbol_map;
    std::unordered_map<std::string, uint16_t> symbol_to_locate_map;
    std::unordered_map<uint16_t, SecurityStats> locate_to_sec_stats_map;
    std::unordered_map<uint64_t, Order> order_map; // key = order reference number
    std::unordered_map<uint64_t, Trade> trade_map; // key = match number

    void handle_trade_(const Trade& trade) {
        // Insert entry if stock does not exist 
        auto [it, emplaced] = locate_to_sec_stats_map.emplace(trade.stock_locate, trade.stock_locate);
        it->second.handle_trade(trade);
    }

public:
    void add_stock_record(uint16_t locate, const std::string& symbol) {
        auto found_locate = locate_to_symbol_map.find(locate);
        if (found_locate == locate_to_symbol_map.end()) {
            locate_to_symbol_map.emplace(locate, symbol);
        } else {
            assert(found_locate->second == symbol && "Symbol collision for one locate" );
        }
        
        auto found_symbol = symbol_to_locate_map.find(symbol);
        if (found_symbol == symbol_to_locate_map.end()) {
            symbol_to_locate_map.emplace(symbol, locate);
        } else {
            assert(found_symbol->second == locate && "Locate collision for one symbol" );
        }
    }

    bool get_symbol_by_locate(const uint16_t locate, std::string& symbol) {
        auto found_locate = locate_to_symbol_map.find(locate);
        if (found_locate == locate_to_symbol_map.end()) {
            return false;
        }
        symbol = found_locate->second;
        return true;
    }
    
    bool get_locate_by_symbol(const std::string& symbol, uint16_t& locate) {
        auto found_symbol = symbol_to_locate_map.find(symbol);
        if (found_symbol == symbol_to_locate_map.end()) {
            return false;
        }
        locate = found_symbol->second;
        return true;
    }

    void add_order(const Order& order) {
        auto [it, emplaced] = order_map.emplace(order.order_reference_number, order);
        assert (emplaced && "More than one Order with same order reference number");
    }
    
    void add_order(Order&& order) {
        auto [it, emplaced] = order_map.emplace(order.order_reference_number, std::move(order));
        assert (emplaced && "More than one Order with same order reference number");
    }

    bool get_order_by_reference_number(const uint64_t reference_number, Order& order) {
        auto found_reference = order_map.find(reference_number);
        if (found_reference == order_map.end()) {
            return false;
        }
        order = found_reference->second;
        return true;
    }

    void replace_order(
        const uint64_t original_order_reference_number, 
        const uint64_t new_order_reference_number, 
        const uint32_t shares, const float price
    ) {
        auto found_order = order_map.find(original_order_reference_number);
        assert (found_order != order_map.end() && "Original order reference not found");
        const Order& old_order = found_order->second;
        Order new_order{
            .stock_locate = old_order.stock_locate,
            .side = old_order.side,
            .shares = shares,
            .price = price,
            .order_reference_number = new_order_reference_number
        };
        order_map.erase(found_order);
        auto [it, emplaced] = order_map.emplace(new_order_reference_number, std::move(new_order));
        assert(emplaced && "New order reference exists");
    }

    void add_trade(const Trade& trade) {
        auto [it, emplaced] = trade_map.emplace(trade.match_number, trade);
        assert (emplaced && "More than one Trade with same match number");
        handle_trade_(trade);
    }

    void add_trade(Trade&& trade) {
        auto [it, emplaced] = trade_map.emplace(trade.match_number, std::move(trade));
        assert(emplaced && "More than one Trade with same match number");
        handle_trade_(it->second);
    }

    // void cancel_trade(const uint16_t match_number) {
    //     // TODO
    // }
};


#endif // SYSTEM_DATA_H