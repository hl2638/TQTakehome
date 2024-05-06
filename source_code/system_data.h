#ifndef SYSTEM_DATA_H
#define SYSTEM_DATA_H
#include <cstdint>
#include <cassert>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <iostream>
#include <iomanip>
#include "trade_types.h"
#include "utils.h"

class SecurityStats {
    uint16_t stock_locate;
    uint64_t traded_shares;
    float total_traded_value;
public:
    SecurityStats(const uint16_t locate):
    stock_locate{locate}, traded_shares{0}, total_traded_value{0} {}
    bool handle_trade(const Trade& trade) {
        traded_shares += trade.shares;
        total_traded_value += trade.price * trade.shares;

        return true;
    }
    bool reverse_trade(const Trade& trade) {
        traded_shares -= trade.shares;
        total_traded_value -= trade.price * trade.shares;
        return true;
    }

    inline float get_vwap() const {
        return traded_shares == 0 ? 0 : total_traded_value / traded_shares;
    }

};

class SystemData {
public:

    enum class PrintFormat {
        csv,
        log
    };

    SystemData(const std::string& output_dir_path, const PrintFormat& format)
    : output_dir_{output_dir_path}, print_format_{format} {
        if (!std::filesystem::exists(output_dir_path)) {
            try {
                std::filesystem::create_directories(output_dir_path);
            } catch (...) {
                std::cerr << "Error creating directory: " << output_dir_path << std::endl;
            }
        }
    }

    void market_open() {
        market_open_ = true;
    }

    void update_timestamp(const uint64_t timestamp) {
        int current_hour = get_hour_by_timestamp(latest_timestamp_);
        int potential_next_hour = get_hour_by_timestamp(timestamp);
        if (market_open_ && current_hour < potential_next_hour) {
            print_vwaps_(potential_next_hour);
        }
        latest_timestamp_ = timestamp; 
    }
    bool add_stock_record(uint16_t locate, const std::string& symbol) {
        auto found_locate = locate_to_symbol_map.find(locate);
        if (found_locate == locate_to_symbol_map.end()) {
            locate_to_symbol_map.emplace(locate, symbol);
        } else {
            if (found_locate->second != symbol) {
                return false;
            }
        }
        
        auto found_symbol = symbol_to_locate_map.find(symbol);
        if (found_symbol == symbol_to_locate_map.end()) {
            symbol_to_locate_map.emplace(symbol, locate);
        } else {
            if (found_symbol->second != locate) {
                return false;
            }
        }

        return true;
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

    bool add_order(const Order& order) {
        auto [it, emplaced] = order_map.emplace(order.order_reference_number, order);
        return emplaced;
    }
    
    bool add_order(Order&& order) {
        auto [it, emplaced] = order_map.emplace(order.order_reference_number, std::move(order));
        return emplaced;
    }

    bool get_order_by_reference_number(const uint64_t reference_number, Order& order) {
        auto found_reference = order_map.find(reference_number);
        if (found_reference == order_map.end()) {
            return false;
        }
        order = found_reference->second;
        return true;
    }

    bool replace_order(
        const uint64_t original_order_reference_number, 
        const uint64_t new_order_reference_number, 
        const uint32_t shares, const float price
    ) {
        auto found_order = order_map.find(original_order_reference_number);
        if (found_order == order_map.end()) {
            return false;
        }
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
        
        return emplaced;
    }

    bool add_trade(const Trade& trade) {
        auto [it, emplaced] = trade_map.emplace(trade.match_number, trade);
        if (!emplaced) {
            return false;
        }
        return handle_trade_(trade);
    }

    bool add_trade(Trade&& trade) {
        auto [it, emplaced] = trade_map.emplace(trade.match_number, std::move(trade));
        if (!emplaced) {
            return false;
        }
        return handle_trade_(it->second);
    }

    bool cancel_trade(const uint16_t match_number) {
        auto found_trade = trade_map.find(match_number);
        if (found_trade == trade_map.end()) {
            return false;
        }
        if (!reverse_trade_(found_trade->second)) {
            return false;
        }
        trade_map.erase(found_trade);
        return true;
    }

private:
    std::unordered_map<uint16_t, std::string> locate_to_symbol_map;
    std::unordered_map<std::string, uint16_t> symbol_to_locate_map;
    std::unordered_map<uint16_t, SecurityStats> locate_to_sec_stats_map;
    std::unordered_map<uint64_t, Order> order_map; // key = order reference number
    std::unordered_map<uint64_t, Trade> trade_map; // key = match number

    uint64_t latest_timestamp_ = 0;
    bool market_open_ = false;
    std::string output_dir_;
    std::ofstream ofs_;

    PrintFormat print_format_;

    bool handle_trade_(const Trade& trade) {
        // Insert entry if stock does not exist 
        auto [it, emplaced] = locate_to_sec_stats_map.emplace(trade.stock_locate, trade.stock_locate);
        return it->second.handle_trade(trade);
    }

    bool reverse_trade_(const Trade& trade) {
        auto it = locate_to_sec_stats_map.find(trade.stock_locate);
        if (it == locate_to_sec_stats_map.end()) {
            return false;
        }
        return it->second.reverse_trade(trade);
    }

    /*
        Only one thread has access to SystemData,
        so no need to lock when printing.
        I don't think it's necessary to print on a separate thread
        since it would lock and parser would block anyways
    */
    void print_vwaps_(const int hour) {
        switch(print_format_) {
            case PrintFormat::csv: {
                print_vwaps_csv_(hour);
                break;
            }
            case PrintFormat::log: {
                print_vwaps_log_(hour);
            }
        }
    }

    void print_vwaps_log_(const int hour) {
        std::string file_name = std::to_string(hour) + ".log";
        std::string output_file =  output_dir_ + "/" + file_name;
        ofs_.open(output_file);
        if (!ofs_.is_open()) {
            std::cerr << "Error opening output file " << output_file << std::endl;
            return;
        }
        
        ofs_ << std::setw(2) << std::setfill('0') << hour << ":00:00" << std::endl;

        for (const auto& [locate, stats]: locate_to_sec_stats_map) {
            ofs_ << std::left << std::setw(8) << locate_to_symbol_map.at(locate) << " "
            << std::fixed << std::setprecision(4) << stats.get_vwap()
            << std::endl;
        }
        ofs_ << "-------------------------------" << std::endl << std::endl;
        ofs_.close();
    }

    void print_vwaps_csv_(const int hour) {
        std::string file_name = std::to_string(hour) + ".csv";
        std::string output_file =  output_dir_ + "/" + file_name;
        ofs_.open(output_file);
        if (!ofs_.is_open()) {
            std::cerr << "Error opening output file " << output_file << std::endl;
            return;
        }
        ofs_ << "hour,symbol,vwap" << std::endl;
        for (const auto& [locate, stats]: locate_to_sec_stats_map) {
            ofs_ << hour << "," 
            << locate_to_symbol_map.at(locate) << "," 
            << std::fixed << std::setprecision(4) << stats.get_vwap() << std::endl;
        }
        ofs_.close();
    }
};


#endif // SYSTEM_DATA_H